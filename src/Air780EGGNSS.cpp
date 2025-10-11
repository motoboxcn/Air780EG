#include "Air780EGGNSS.h"

const char *Air780EGGNSS::TAG = "Air780EGGNSS";

Air780EGGNSS::Air780EGGNSS(Air780EGCore *core_instance) : core(core_instance)
{
    // 初始化GNSS数据结构
    gnss_data.is_fixed = false;
    gnss_data.latitude = 0.0;
    gnss_data.longitude = 0.0;
    gnss_data.altitude = 0.0;
    gnss_data.speed = 0.0;
    gnss_data.course = 0.0;
    gnss_data.satellites = 0;
    gnss_data.hdop = 0.0;
    gnss_data.timestamp = "";
    gnss_data.date = "";
    gnss_data.last_update = 0;
    gnss_data.data_valid = false;
}

bool Air780EGGNSS::enableGNSS()
{
    if (!core || !core->isInitialized())
    {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }

    AIR780EG_LOGI(TAG, "Enabling GNSS...");

    // 启用GNSS功能
    if (!core->sendATCommandBool("AT+CGNSPWR=1"))
    {
        AIR780EG_LOGE(TAG, "Failed to power on GNSS");
        return false;
    }

    // AT +CGNSAID=31,1,1,1  使能辅助定位
    if (!core->sendATCommandBool("AT+CGNSAID=31,1,1,1"))
    {
        AIR780EG_LOGE(TAG, "Failed to enable GNSS auxiliary positioning");
        return false;
    }

    // AT+CGNSURC=1  设置自动上报
    // 0 表示不自动上报
    if (!core->sendATCommandBool("AT+CGNSURC=0"))
    {
        AIR780EG_LOGE(TAG, "Failed to disable GNSS automatic reporting");
        return false;
    }

    delay(1000);

    gnss_enabled = true;
    AIR780EG_LOGI(TAG, "GNSS enabled successfully");

    return true;
}

// bool Air780EGGNSS::enableLBSAndWIFI()
// {
//     lbs_location_enabled = true;
//     if (lbs_location_enabled)
//         if (!core->sendATCommandBool("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"")) {
//             AIR780EG_LOGE(TAG, "Failed to set GPRS bearer");
//             return false;
//         }

//         // AT+SAPBR=3,1,"APN","" 设置APN
//         if (!core->sendATCommandBool("AT+SAPBR=3,1,\"APN\",\"\"")) {
//             AIR780EG_LOGE(TAG, "Failed to set APN");
//             return false;
//         }

//         // AT+SAPBR=1,1 激活GPRS承载
//         if (!core->sendATCommandBool("AT+SAPBR=1,1")) {
//             AIR780EG_LOGE(TAG, "Failed to activate GPRS bearer");
//             return false;
//         }

//         // 查询是否激活 AT+SAPBR=2,1 +SAPBR: 1,1,010.169.179.213 OK
//         String response = core->sendATCommand("AT+SAPBR=2,1", 3000);
//         if (response.indexOf("+SAPBR: 1,1") >= 0) {
//             AIR780EG_LOGI(TAG, "GPRS bearer activated");
//         } else {
//             AIR780EG_LOGE(TAG, "Failed to activate GPRS bearer");
//             return false;
//         }

//         // AT+CIPGSMLOC=1,1 +CIPGSMLOC: 0,034.7983328,114.3214505,2023/06/05,14:38:50 OK
//         response = core->sendATCommandUntilExpected("AT+CIPGSMLOC=1,1", "OK", 30000);
//         if (response.indexOf("+CIPGSMLOC:") >= 0) {
//             AIR780EG_LOGI(TAG, "GSM location retrieved: %s", response.c_str());
//         } else {
//             AIR780EG_LOGE(TAG, "Failed to get GSM location");
//             return false;
//         }

//         // 解析基站位置
//         String location = response.substring(response.indexOf(",") + 1);
//         String latitude = location.substring(0, location.indexOf(","));
//     return true;
// }

// 添加时间格式标准化函数
String Air780EGGNSS::normalizeDate(const String &date)
{
    String normalized = date;
    
    // 移除可能的额外字符（如\r\n\r\nOK）
    int end_pos = normalized.indexOf('\r');
    if (end_pos >= 0) {
        normalized = normalized.substring(0, end_pos);
    }
    
    // 处理 YYYYMMDD 格式
    if (normalized.length() == 8 && normalized.indexOf('/') < 0) {
        return normalized; // 已经是标准格式
    }
    
    // 处理 YYYY/MM/DD 格式，转换为 YYYYMMDD
    if (normalized.indexOf('/') >= 0) {
        // 移除所有斜杠
        normalized.replace("/", "");
        return normalized;
    }
    
    return normalized;
}

String Air780EGGNSS::normalizeTime(const String &time)
{
    String normalized = time;
    
    // 移除可能的额外字符（如\r\n\r\nOK）
    int end_pos = normalized.indexOf('\r');
    if (end_pos >= 0) {
        normalized = normalized.substring(0, end_pos);
    }
    
    // 处理 HHMMSS 格式
    if (normalized.length() == 6 && normalized.indexOf(':') < 0) {
        return normalized; // 已经是标准格式
    }
    
    // 处理 HH:MM:SS 格式，转换为 HHMMSS
    if (normalized.indexOf(':') >= 0) {
        // 移除所有冒号
        normalized.replace(":", "");
        return normalized;
    }
    
    return normalized;
}

bool Air780EGGNSS::updateWIFILocation()
{
    // 检查是否有阻塞命令正在执行
    if (core->isBlockingCommandActive()) {
        AIR780EG_LOGW(TAG, "Another blocking command is active, skipping WIFI location");
        return false;
    }

    AIR780EG_LOGD(TAG, "开始WiFi定位...");
    // 使用同步方式发送WiFi定位命令（恢复原有行为）
    String response = core->sendATCommandUntilExpected("AT+WIFILOC=1,1", "OK", 30000);
    
    if (response.indexOf("+WIFILOC:") >= 0)
    {
        AIR780EG_LOGD(TAG, "WIFI location retrieved: %s", response.c_str());

        // 找到+WIFILOC:的位置
        int start_pos = response.indexOf("+WIFILOC:") + 9; // "+WIFILOC:"的长度
        String data_part = response.substring(start_pos);

        // 解析逗号分隔的数据
        int comma_count = 0;
        int start = 0;
        String latitude, longitude, date, time;

        for (int i = 0; i <= data_part.length(); i++)
        {
            if (i == data_part.length() || data_part.charAt(i) == ',')
            {
                String field = data_part.substring(start, i);
                field.trim();

                switch (comma_count)
                {
                case 0: // 错误码 (0表示成功)
                    break;
                case 1: // 纬度
                    latitude = field;
                    break;
                case 2: // 经度
                    longitude = field;
                    break;
                case 3: // 日期
                    date = field;
                    break;
                case 4: // 时间
                    time = field;
                    break;
                }

                comma_count++;
                start = i + 1;
            }
        }

        // 转换并设置数据
        if (latitude.length() > 0 && longitude.length() > 0)
        {
            gnss_data.latitude = latitude.toDouble();
            gnss_data.longitude = longitude.toDouble();
            gnss_data.date = normalizeDate(date);
            gnss_data.timestamp = normalizeTime(time);
            gnss_data.data_valid = true;
            gnss_data.is_fixed = true;
            gnss_data.satellites = 0; // WIFI没有卫星信息
            gnss_data.hdop = 0.0;     // WIFI没有HDOP信息
            gnss_data.altitude = 0.0; // WIFI没有海拔信息
            gnss_data.speed = 0.0;    // WIFI没有速度信息
            gnss_data.course = 0.0;   // WIFI没有航向信息
            gnss_data.last_update = millis();
            gnss_data.location_type = "WIFI";
            AIR780EG_LOGD(TAG, "WIFI data parsed - Lat: %.6f, Lng: %.6f, Date: %s, Time: %s",
                          gnss_data.latitude, gnss_data.longitude,
                          gnss_data.date.c_str(), gnss_data.timestamp.c_str());
            AIR780EG_LOGD(TAG, "WiFi定位完成，结果: 成功");
            return true;
        }
        else
        {
            AIR780EG_LOGE(TAG, "Failed to parse WIFI data - invalid coordinates");
            AIR780EG_LOGD(TAG, "WiFi定位完成，结果: 失败 - 数据解析错误");
            return false;
        }
    }
    else if (response.indexOf("ERROR") >= 0)
    {
        AIR780EG_LOGE(TAG, "WIFI location request failed: %s", response.c_str());
        AIR780EG_LOGD(TAG, "WiFi定位完成，结果: 失败 - 命令错误");
        return false;
    }
    else
    {
        AIR780EG_LOGE(TAG, "Failed to get WIFI location, unexpected response: %s", response.c_str());
        AIR780EG_LOGD(TAG, "WiFi定位完成，结果: 失败 - 超时或无效响应");
        return false;
    }
}

bool Air780EGGNSS::updateLBS()
{
    if (!lbs_location_enabled) {
        AIR780EG_LOGD(TAG, "LBS定位未启用");
        return false;
    }

    // 检查是否有阻塞命令正在执行
    if (core->isBlockingCommandActive()) {
        AIR780EG_LOGW(TAG, "Another blocking command is active, skipping LBS location");
        return false;
    }

    AIR780EG_LOGD(TAG, "开始LBS定位...");
    // AT+CIPGSMLOC=1,1 +CIPGSMLOC: 0,31.1826152,120.6673126,2025/07/11,23:38:22 OK
    String response = core->sendATCommandUntilExpected("AT+CIPGSMLOC=1,1", "OK", 30000);
        if (response.indexOf("+CIPGSMLOC:") >= 0)
        {
            AIR780EG_LOGI(TAG, "GSM location retrieved: %s", response.c_str());

            // 找到+CIPGSMLOC:的位置
            int start_pos = response.indexOf("+CIPGSMLOC:") + 11; // "+CIPGSMLOC:"的长度
            String data_part = response.substring(start_pos);

            // 解析逗号分隔的数据
            int comma_count = 0;
            int start = 0;
            String latitude, longitude, date, time;

            for (int i = 0; i <= data_part.length(); i++)
            {
                if (i == data_part.length() || data_part.charAt(i) == ',')
                {
                    String field = data_part.substring(start, i);
                    field.trim();

                    switch (comma_count)
                    {
                    case 0: // 错误码 (0表示成功)
                        break;
                    case 1: // 纬度
                        latitude = field;
                        break;
                    case 2: // 经度
                        longitude = field;
                        break;
                    case 3: // 日期
                        date = field;
                        break;
                    case 4: // 时间
                        time = field;
                        break;
                    }

                    comma_count++;
                    start = i + 1;
                }
            }

            // 转换并设置数据
            if (latitude.length() > 0 && longitude.length() > 0)
            {
                gnss_data.latitude = latitude.toDouble();
                gnss_data.longitude = longitude.toDouble();
                gnss_data.date = normalizeDate(date);
                gnss_data.timestamp = normalizeTime(time);
                gnss_data.data_valid = true;
                gnss_data.is_fixed = true;
                gnss_data.satellites = 0; // LBS没有卫星信息
                gnss_data.hdop = 0.0;     // LBS没有HDOP信息
                gnss_data.altitude = 0.0; // LBS没有海拔信息
                gnss_data.speed = 0.0;    // LBS没有速度信息
                gnss_data.course = 0.0;   // LBS没有航向信息
                gnss_data.last_update = millis();
                gnss_data.location_type = "LBS";
                AIR780EG_LOGD(TAG, "LBS data parsed - Lat: %.6f, Lng: %.6f, Date: %s, Time: %s",
                              gnss_data.latitude, gnss_data.longitude,
                              gnss_data.date.c_str(), gnss_data.timestamp.c_str());
                AIR780EG_LOGD(TAG, "LBS定位完成，结果: 成功");
                return true;
            }
            else
            {
                AIR780EG_LOGE(TAG, "Failed to parse LBS data - invalid coordinates");
                AIR780EG_LOGD(TAG, "LBS定位完成，结果: 失败 - 数据解析错误");
                return false;
            }
        }
        else
        {
            AIR780EG_LOGE(TAG, "Failed to get GSM location");
            AIR780EG_LOGD(TAG, "LBS定位完成，结果: 失败 - 命令错误");
            return false;
        }
}

bool Air780EGGNSS::disableGNSS()
{
    if (!core || !core->isInitialized())
    {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }

    AIR780EG_LOGI(TAG, "Disabling GNSS...");

    // 关闭GNSS功能
    if (!core->sendATCommandBool("AT+CGNSPWR=0"))
    {
        AIR780EG_LOGE(TAG, "Failed to power off GNSS");
        return false;
    }

    gnss_enabled = false;
    gnss_data.data_valid = false;

    AIR780EG_LOGI(TAG, "GNSS disabled");
    return true;
}

void Air780EGGNSS::loop()
{
    if (!core || !core->isInitialized())
    {
        return;
    }

    // 检查是否有阻塞命令正在执行（如WiFi定位）
    if (core->isBlockingCommandActive()) {
        // 阻塞命令执行期间，暂停GNSS更新避免串口冲突
        return;
    }

    unsigned long current_time = millis();

    // 根据GNSS状态动态调整查询间隔
    unsigned long interval = gnss_data.data_valid ? 3000 : 10000; // 有效时3秒，无效时10秒
    
    // 检查是否需要更新GNSS数据
    if (current_time - last_loop_time >= interval)
    {
        // 当gnss 信号丢失的时候，通过LBS辅助定位
        if (gnss_enabled)
        {
            AIR780EG_LOGD(TAG, "GNSS查询间隔: %lu ms (状态: %s)", 
                         interval, gnss_data.data_valid ? "有效" : "无效");
            updateGNSSData();
        }

        // 补充定位
        // GNSS信号丢失时不自动调用WiFi/LBS定位，避免串口冲突
        // 用户需要根据业务需求手动调用 updateWIFILocation() 或 updateLBS()
        if (gnss_data.data_valid == false)
        {
            AIR780EG_LOGD(TAG, "GNSS signal lost. Manual WiFi/LBS location available if needed.");
        }

        last_loop_time = current_time;
    }
}

void Air780EGGNSS::updateGNSSData()
{
    AIR780EG_LOGV(TAG, "Updating GNSS data...");

    String response = core->sendATCommand("AT+CGNSINF", 3000);

    if (response.indexOf("OK") >= 0)
    {
        if (parseGNSSResponse(response))
        {
            gnss_data.last_update = millis();
            gnss_data.location_type = "GNSS";
            AIR780EG_LOGD(TAG, "GNSS data updated - Fixed: %s, DataValid: %s, Sats: %d, Lat: %.6f, Lng: %.6f",
                          gnss_data.is_fixed ? "Yes" : "No",
                          gnss_data.data_valid ? "Yes" : "No",
                          gnss_data.satellites,
                          gnss_data.latitude,
                          gnss_data.longitude);
        }
        else
        {
            AIR780EG_LOGW(TAG, "Failed to parse GNSS response");
        }
    }
    else
    {
        AIR780EG_LOGW(TAG, "No response from GNSS command");
    }
}

// 定位失败的时候会保留之前的位置信息，所以需要判断是否定位成功来确认是否是最新位置信息
bool Air780EGGNSS::parseGNSSResponse(const String &response)
{
    // 查找+CGNSINF:行
    int info_start = response.indexOf("+CGNSINF: ");
    if (info_start < 0)
    {
        return false;
    }

    info_start += 10; // "+CGNSINF: "的长度
    int info_end = response.indexOf('\r', info_start);
    if (info_end < 0)
    {
        info_end = response.length();
    }

    String info_line = response.substring(info_start, info_end);
    AIR780EG_LOGV(TAG, "GNSS info line: %s", info_line.c_str());

    // 临时变量存储解析的数据
    bool temp_is_fixed = false;
    bool temp_data_valid = false;
    double temp_latitude = 0.0;
    double temp_longitude = 0.0;
    double temp_altitude = 0.0;
    float temp_speed = 0.0;
    float temp_course = 0.0;
    float temp_hdop = 0.0;
    int temp_satellites = 0;
    String temp_date = "";
    String temp_timestamp = "";

    // 解析逗号分隔的字段
    int field_count = 0;
    int start = 0;

    for (int i = 0; i <= info_line.length(); i++)
    {
        if (i == info_line.length() || info_line.charAt(i) == ',')
        {
            String field = info_line.substring(start, i);
            field.trim();

            switch (field_count)
            {
            case 0: // GNSS运行状态
                temp_is_fixed = (field == "1");
                break;
            case 1: // 定位状态
                // 1表示已定位
                temp_data_valid = (field == "1");
                if (!temp_data_valid)
                {
                    AIR780EG_LOGD(TAG, "还在定位中... data_valid: %d", temp_data_valid);
                }
                break;
            case 2: // UTC日期时间
                if (field.length() >= 14)
                {
                    String date_part = field.substring(0, 8);   // YYYYMMDD
                    String time_part = field.substring(8); // HHMMSS.sss
                    
                    // 标准化日期和时间格式
                    temp_date = normalizeDate(date_part);
                    temp_timestamp = normalizeTime(time_part);
                }
                break;
            case 3: // 纬度
                if (field.length() > 0)
                {
                    temp_latitude = field.toDouble();
                }
                break;
            case 4: // 经度
                if (field.length() > 0)
                {
                    temp_longitude = field.toDouble();
                }
                break;
            case 5: // 海拔高度
                if (field.length() > 0)
                {
                    temp_altitude = field.toDouble();
                }
                break;
            case 6: // 速度 (km/h)
                if (field.length() > 0)
                {
                    temp_speed = field.toFloat();
                }
                break;
            case 7: // 航向角
                if (field.length() > 0)
                {
                    temp_course = field.toFloat();
                }
                break;
            case 8: // HDOP
                if (field.length() > 0)
                {
                    temp_hdop = field.toFloat();
                }
                break;
            case 9: // PDOP
                // 暂不使用
                break;
            case 10: // VDOP
                // 暂不使用
                break;
            case 11: // 卫星数量
                if (field.length() > 0)
                {
                    temp_satellites = field.toInt();
                }
                break;
            }

            field_count++;
            start = i + 1;
        }
    }

    // 只有在定位成功时才更新位置相关字段
    if (temp_data_valid && temp_is_fixed)
    {
        // 更新所有字段
        gnss_data.is_fixed = temp_is_fixed;
        gnss_data.data_valid = temp_data_valid;
        gnss_data.latitude = temp_latitude;
        gnss_data.longitude = temp_longitude;
        gnss_data.altitude = temp_altitude;
        gnss_data.speed = temp_speed;
        gnss_data.course = temp_course;
        gnss_data.hdop = temp_hdop;
        gnss_data.satellites = temp_satellites;
        gnss_data.date = temp_date;
        gnss_data.timestamp = temp_timestamp;
        
        AIR780EG_LOGD(TAG, "定位成功，更新位置信息 - Lat: %.6f, Lng: %.6f", 
                      gnss_data.latitude, gnss_data.longitude);
    }
    else
    {
        // 定位未成功，只更新状态字段，保留位置信息
        gnss_data.is_fixed = temp_is_fixed;
        gnss_data.data_valid = temp_data_valid;
        gnss_data.satellites = temp_satellites;
        gnss_data.hdop = temp_hdop;
        
        // 如果之前有有效定位，保留位置信息
        if (gnss_data.data_valid)
        {
            AIR780EG_LOGD(TAG, "定位未成功，保留上次位置信息 - Lat: %.6f, Lng: %.6f", 
                          gnss_data.latitude, gnss_data.longitude);
        }
        else
        {
            AIR780EG_LOGD(TAG, "定位未成功，无历史位置信息");
        }
    }

    return field_count >= 12; // 至少解析到卫星数量字段
}

bool Air780EGGNSS::isFixed()
{
    return gnss_data.data_valid && gnss_data.is_fixed;
}

double Air780EGGNSS::getLatitude()
{
    return gnss_data.latitude;
}

double Air780EGGNSS::getLongitude()
{
    return gnss_data.longitude;
}

double Air780EGGNSS::getAltitude()
{
    return gnss_data.altitude;
}

float Air780EGGNSS::getSpeed()
{
    return gnss_data.speed;
}

float Air780EGGNSS::getCourse()
{
    return gnss_data.course;
}

int Air780EGGNSS::getSatelliteCount()
{
    return gnss_data.satellites;
}

float Air780EGGNSS::getHDOP()
{
    return gnss_data.hdop;
}

String Air780EGGNSS::getTimestamp()
{
    return gnss_data.timestamp;
}

String Air780EGGNSS::getDate()
{
    return gnss_data.date;
}

bool Air780EGGNSS::isEnabled() const
{
    return gnss_enabled;
}

bool Air780EGGNSS::isBlockingCommandActive() const
{
    return core ? core->isBlockingCommandActive() : false;
}

bool Air780EGGNSS::isDataValid() const
{
    return gnss_data.data_valid;
}

unsigned long Air780EGGNSS::getLastUpdateTime() const
{
    return gnss_data.last_update;
}

void Air780EGGNSS::printGNSSInfo()
{
    AIR780EG_LOGI(TAG, "=== GNSS Information ===");
    AIR780EG_LOGI(TAG, "Fixed: %s", gnss_data.is_fixed ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Location Type: %s", gnss_data.location_type.c_str());
    AIR780EG_LOGI(TAG, "Satellites: %d", gnss_data.satellites);
    AIR780EG_LOGI(TAG, "Latitude: %.6f", gnss_data.latitude);
    AIR780EG_LOGI(TAG, "Longitude: %.6f", gnss_data.longitude);
    AIR780EG_LOGI(TAG, "Altitude: %.2f m", gnss_data.altitude);
    AIR780EG_LOGI(TAG, "Speed: %.2f km/h", gnss_data.speed);
    AIR780EG_LOGI(TAG, "Course: %.2f°", gnss_data.course);
    AIR780EG_LOGI(TAG, "HDOP: %.2f", gnss_data.hdop);
    AIR780EG_LOGI(TAG, "Date: %s", gnss_data.date.c_str());
    AIR780EG_LOGI(TAG, "Time: %s", gnss_data.timestamp.c_str());
    AIR780EG_LOGI(TAG, "Last Update: %lu ms ago", millis() - gnss_data.last_update);
    AIR780EG_LOGI(TAG, "======================");
}

String Air780EGGNSS::getRawGNSSData()
{
    if (!core || !core->isInitialized())
    {
        return "";
    }

    return core->sendATCommand("AT+CGNSINF", 3000);
}

String Air780EGGNSS::getLocationJSON()
{
    DynamicJsonDocument doc(1024);
    doc["latitude"] = gnss_data.latitude;
    doc["longitude"] = gnss_data.longitude;
    doc["altitude"] = gnss_data.altitude;
    doc["speed"] = gnss_data.speed;
    doc["course"] = gnss_data.course;
    doc["hdop"] = gnss_data.hdop;
    doc["date"] = gnss_data.date;
    doc["timestamp"] = gnss_data.timestamp;
    doc["location_type"] = gnss_data.location_type;
    doc["satellites"] = gnss_data.satellites;
    doc["is_fixed"] = gnss_data.is_fixed;
    doc["data_valid"] = gnss_data.data_valid;
    doc["location_type"] = gnss_data.location_type;
    doc["satellites"] = gnss_data.satellites;
    return doc.as<String>();
}
// 检查GNSS信号是否丢失
bool Air780EGGNSS::isGNSSSignalLost()
{
    return !gnss_data.data_valid || !gnss_data.is_fixed;
}

// 获取当前位置来源
String Air780EGGNSS::getLocationSource()
{
    return gnss_data.location_type;
}

// 获取最后定位时间
unsigned long Air780EGGNSS::getLastLocationTime()
{
    return gnss_data.last_update;
}
