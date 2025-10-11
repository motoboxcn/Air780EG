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
    gnss_data.last_update = 0;
    gnss_data.data_valid = false;
    
    // 初始化GPS时间
    gnss_data.gps_time.year = 0;
    gnss_data.gps_time.month = 0;
    gnss_data.gps_time.day = 0;
    gnss_data.gps_time.hour = 0;
    gnss_data.gps_time.minute = 0;
    gnss_data.gps_time.second = 0;
    gnss_data.gps_time.millisecond = 0;
    gnss_data.gps_time.valid = false;
    gnss_data.gps_time.last_update = 0;
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

// 解析GPS时间信息
void Air780EGGNSS::parseGPSTime(const String &date_part, const String &time_part)
{
    // 解析日期部分 YYYYMMDD
    if (date_part.length() >= 8) {
        gnss_data.gps_time.year = date_part.substring(0, 4).toInt();
        gnss_data.gps_time.month = date_part.substring(4, 6).toInt();
        gnss_data.gps_time.day = date_part.substring(6, 8).toInt();
    } else {
        gnss_data.gps_time.year = 0;
        gnss_data.gps_time.month = 0;
        gnss_data.gps_time.day = 0;
    }
    
    // 解析时间部分 HHMMSS.sss
    if (time_part.length() >= 6) {
        gnss_data.gps_time.hour = time_part.substring(0, 2).toInt();
        gnss_data.gps_time.minute = time_part.substring(2, 4).toInt();
        gnss_data.gps_time.second = time_part.substring(4, 6).toInt();
        
        // 解析毫秒部分（如果存在）
        if (time_part.length() > 7 && time_part.charAt(6) == '.') {
            String ms_str = time_part.substring(7);
            // 取前3位作为毫秒
            if (ms_str.length() >= 3) {
                gnss_data.gps_time.millisecond = ms_str.substring(0, 3).toInt();
            } else {
                gnss_data.gps_time.millisecond = ms_str.toInt() * (1000 / pow(10, ms_str.length()));
            }
        } else {
            gnss_data.gps_time.millisecond = 0;
        }
    } else {
        gnss_data.gps_time.hour = 0;
        gnss_data.gps_time.minute = 0;
        gnss_data.gps_time.second = 0;
        gnss_data.gps_time.millisecond = 0;
    }
    
    // 验证时间有效性
    gnss_data.gps_time.valid = (gnss_data.gps_time.year >= 2020 && gnss_data.gps_time.year <= 2030 &&
                               gnss_data.gps_time.month >= 1 && gnss_data.gps_time.month <= 12 &&
                               gnss_data.gps_time.day >= 1 && gnss_data.gps_time.day <= 31 &&
                               gnss_data.gps_time.hour >= 0 && gnss_data.gps_time.hour <= 23 &&
                               gnss_data.gps_time.minute >= 0 && gnss_data.gps_time.minute <= 59 &&
                               gnss_data.gps_time.second >= 0 && gnss_data.gps_time.second <= 59);
    
    gnss_data.gps_time.last_update = millis();
    
    AIR780EG_LOGD(TAG, "GPS时间解析: %04d-%02d-%02d %02d:%02d:%02d.%03d (有效: %s)", 
                  gnss_data.gps_time.year, gnss_data.gps_time.month, gnss_data.gps_time.day,
                  gnss_data.gps_time.hour, gnss_data.gps_time.minute, gnss_data.gps_time.second, gnss_data.gps_time.millisecond,
                  gnss_data.gps_time.valid ? "是" : "否");
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
            gnss_data.data_valid = true;
            gnss_data.is_fixed = true;
            gnss_data.satellites = 0; // WIFI没有卫星信息
            gnss_data.hdop = 0.0;     // WIFI没有HDOP信息
            gnss_data.altitude = 0.0; // WIFI没有海拔信息
            gnss_data.speed = 0.0;    // WIFI没有速度信息
            gnss_data.course = 0.0;   // WIFI没有航向信息
            gnss_data.last_update = millis();
            gnss_data.location_type = "WIFI";
            AIR780EG_LOGD(TAG, "WIFI data parsed - Lat: %.6f, Lng: %.6f",
                          gnss_data.latitude, gnss_data.longitude);
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
                gnss_data.data_valid = true;
                gnss_data.is_fixed = true;
                gnss_data.satellites = 0; // LBS没有卫星信息
                gnss_data.hdop = 0.0;     // LBS没有HDOP信息
                gnss_data.altitude = 0.0; // LBS没有海拔信息
                gnss_data.speed = 0.0;    // LBS没有速度信息
                gnss_data.course = 0.0;   // LBS没有航向信息
                gnss_data.last_update = millis();
                gnss_data.location_type = "LBS";
                AIR780EG_LOGD(TAG, "LBS data parsed - Lat: %.6f, Lng: %.6f",
                              gnss_data.latitude, gnss_data.longitude);
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

        // 兜底定位逻辑，会导致串口通讯不可用，阻塞 AT 指令；
        if (fallbackConfig.enabled && gnss_data.data_valid == false)
        {
            handleFallbackLocation();
        }
        else if (gnss_data.data_valid == false)
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
            // 只有在GNSS真正定位成功时才更新location_type
            if (gnss_data.is_fixed && gnss_data.data_valid) {
                gnss_data.location_type = "GNSS";
            }
            AIR780EG_LOGD(TAG, "GNSS data updated - Fixed: %s, DataValid: %s, Sats: %d, Lat: %.6f, Lng: %.6f, Type: %s",
                          gnss_data.is_fixed ? "Yes" : "No",
                          gnss_data.data_valid ? "Yes" : "No",
                          gnss_data.satellites,
                          gnss_data.latitude,
                          gnss_data.longitude,
                          gnss_data.location_type.c_str());
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
                    
                    // 解析GPS时间信息
                    parseGPSTime(date_part, time_part);
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
    if (gnss_data.gps_time.valid) {
        AIR780EG_LOGI(TAG, "GPS Time: %s", getGPSTimeString().c_str());
    } else {
        AIR780EG_LOGI(TAG, "GPS Time: Invalid");
    }
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
    doc["location_type"] = gnss_data.location_type;
    doc["satellites"] = gnss_data.satellites;
    doc["is_fixed"] = gnss_data.is_fixed;
    doc["data_valid"] = gnss_data.data_valid;
    
    // 添加GPS时间信息
    if (gnss_data.gps_time.valid) {
        doc["gps_time"] = getGPSTimeString();
        doc["gps_time_valid"] = true;
    } else {
        doc["gps_time"] = "Invalid";
        doc["gps_time_valid"] = false;
    }
    
    return doc.as<String>();
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

// 配置兜底定位
void Air780EGGNSS::configureFallbackLocation(bool enable, unsigned long gnss_timeout,
                                            unsigned long lbs_interval, unsigned long wifi_interval,
                                            bool prefer_wifi) {
    fallbackConfig.enabled = enable;
    fallbackConfig.gnss_timeout = gnss_timeout;
    fallbackConfig.lbs_interval = lbs_interval;
    fallbackConfig.wifi_interval = wifi_interval;
    fallbackConfig.prefer_wifi_over_lbs = prefer_wifi;
    
    // 设置初始时间为较早的时间，确保启动后能立即尝试定位
    unsigned long currentTime = millis();
    fallbackConfig.last_lbs_time = currentTime - lbs_interval;  // 设置为"已经过了间隔时间"
    fallbackConfig.last_wifi_time = currentTime - wifi_interval; // 设置为"已经过了间隔时间"
    
    AIR780EG_LOGD(TAG, "时间戳初始化: 当前时间=%lu, WiFi上次时间=%lu, LBS上次时间=%lu", 
                  currentTime, fallbackConfig.last_wifi_time, fallbackConfig.last_lbs_time);
    
    AIR780EG_LOGI(TAG, "兜底定位配置: %s, GNSS超时:%lus, LBS间隔:%lus, WiFi间隔:%lus, 优先WiFi:%s, 启动时立即尝试定位",
                  enable ? "启用" : "禁用",
                  gnss_timeout/1000,
                  lbs_interval/1000,
                  wifi_interval/1000,
                  prefer_wifi ? "是" : "否");
}

// 处理兜底定位逻辑
void Air780EGGNSS::handleFallbackLocation() {
    if (!fallbackConfig.enabled) {
        return;
    }
    
    unsigned long currentTime = millis();
    
    AIR780EG_LOGD(TAG, "=== 兜底定位检查开始 ===");
    AIR780EG_LOGD(TAG, "当前时间: %lu, WiFi上次: %lu, LBS上次: %lu", 
                  currentTime, fallbackConfig.last_wifi_time, fallbackConfig.last_lbs_time);
    
    // 检查GNSS信号是否丢失
    if (!isGNSSSignalLost()) {
        // GNSS信号正常，不需要兜底
        AIR780EG_LOGD(TAG, "GNSS信号正常，跳过兜底定位");
        return;
    }
    
    // 检查是否有阻塞命令正在执行
    if (core->isBlockingCommandActive()) {
        AIR780EG_LOGD(TAG, "有阻塞命令正在执行，跳过兜底定位");
        return;
    }
    
    AIR780EG_LOGD(TAG, "兜底定位配置: 优先%s, WiFi间隔: %lu秒, LBS间隔: %lu秒", 
                  fallbackConfig.prefer_wifi_over_lbs ? "WiFi" : "LBS",
                  fallbackConfig.wifi_interval/1000,
                  fallbackConfig.lbs_interval/1000);
    
    if (fallbackConfig.prefer_wifi_over_lbs) {
        // 检查是否达到WiFi定位间隔
        unsigned long wifi_elapsed = currentTime - fallbackConfig.last_wifi_time;
        AIR780EG_LOGD(TAG, "距离上次WiFi定位: %lu秒 (间隔: %lu秒)", 
                      wifi_elapsed/1000, fallbackConfig.wifi_interval/1000);
        
        if (wifi_elapsed >= fallbackConfig.wifi_interval) {
            AIR780EG_LOGI(TAG, "尝试WiFi定位...");
            bool wifi_success = updateWIFILocation();
            AIR780EG_LOGD(TAG, "WiFi定位结果: %s", wifi_success ? "成功" : "失败");
            
            // 无论成功失败都要更新时间戳，避免重复执行
            fallbackConfig.last_wifi_time = currentTime;
            AIR780EG_LOGD(TAG, "WiFi时间戳已更新: %lu", fallbackConfig.last_wifi_time);
            
            // 只有在WiFi定位失败且达到LBS间隔时才尝试LBS
            if (!wifi_success) {
                unsigned long lbs_elapsed = currentTime - fallbackConfig.last_lbs_time;
                AIR780EG_LOGD(TAG, "WiFi定位失败，距离上次LBS定位: %lu秒 (间隔: %lu秒)", 
                             lbs_elapsed/1000, fallbackConfig.lbs_interval/1000);
                
                if (lbs_elapsed >= fallbackConfig.lbs_interval) {
                    AIR780EG_LOGI(TAG, "WiFi定位失败，尝试LBS定位");
                    bool lbs_success = updateLBS();
                    AIR780EG_LOGD(TAG, "LBS定位结果: %s", lbs_success ? "成功" : "失败");
                    
                    // 无论成功失败都要更新时间戳
                    fallbackConfig.last_lbs_time = currentTime;
                }
            }
        }
    } else {
        // 检查是否达到LBS定位间隔
        unsigned long lbs_elapsed = currentTime - fallbackConfig.last_lbs_time;
        AIR780EG_LOGD(TAG, "距离上次LBS定位: %lu秒 (间隔: %lu秒)", 
                      lbs_elapsed/1000, fallbackConfig.lbs_interval/1000);
        
        if (lbs_elapsed >= fallbackConfig.lbs_interval) {
            AIR780EG_LOGI(TAG, "尝试LBS定位...");
            bool lbs_success = updateLBS();
            AIR780EG_LOGD(TAG, "LBS定位结果: %s", lbs_success ? "成功" : "失败");
            
            // 无论成功失败都要更新时间戳，避免重复执行
            fallbackConfig.last_lbs_time = currentTime;
            AIR780EG_LOGD(TAG, "LBS时间戳已更新: %lu", fallbackConfig.last_lbs_time);
            
            // 只有在LBS定位失败且达到WiFi间隔时才尝试WiFi
            if (!lbs_success) {
                unsigned long wifi_elapsed = currentTime - fallbackConfig.last_wifi_time;
                AIR780EG_LOGD(TAG, "LBS定位失败，距离上次WiFi定位: %lu秒 (间隔: %lu秒)", 
                             wifi_elapsed/1000, fallbackConfig.wifi_interval/1000);
                
                if (wifi_elapsed >= fallbackConfig.wifi_interval) {
                    AIR780EG_LOGI(TAG, "LBS定位失败，尝试WiFi定位");
                    bool wifi_success = updateWIFILocation();
                    AIR780EG_LOGD(TAG, "WiFi定位结果: %s", wifi_success ? "成功" : "失败");
                    
                    // 无论成功失败都要更新时间戳
                    fallbackConfig.last_wifi_time = currentTime;
                    AIR780EG_LOGD(TAG, "WiFi时间戳已更新(备用): %lu", fallbackConfig.last_wifi_time);
                }
            }
        }
    }
    
    AIR780EG_LOGD(TAG, "=== 兜底定位检查结束 ===");
}

// 检查GNSS信号是否丢失
bool Air780EGGNSS::isGNSSSignalLost() {
    if (!gnss_enabled) {
        AIR780EG_LOGD(TAG, "GNSS信号丢失: GNSS功能未启用");
        return true;
    }
    
    // 检查GNSS数据的时效性
    unsigned long currentTime = millis();
    
    // 如果未定位，认为信号丢失
    if (!gnss_data.is_fixed) {
        AIR780EG_LOGD(TAG, "GNSS信号丢失: 未定位 (is_fixed: %d)", gnss_data.is_fixed);
        return true;
    }
    
    // 如果数据无效但已定位，检查数据时效性
    if (!gnss_data.data_valid) {
        unsigned long elapsed = currentTime - gnss_data.last_update;
        if (elapsed > fallbackConfig.gnss_timeout) {
            AIR780EG_LOGD(TAG, "GNSS信号丢失: 数据无效且过期 (data_valid: %d, 已过 %lu秒)", 
                          gnss_data.data_valid, elapsed/1000);
            return true;
        } else {
            AIR780EG_LOGD(TAG, "GNSS信号正常: 已定位但数据无效，仍在超时范围内 (data_valid: %d, 已过 %lu秒)", 
                          gnss_data.data_valid, elapsed/1000);
            return false; // 已定位且在超时范围内，认为信号正常
        }
    }
    
    // 检查数据是否过期（只有在数据有效时才检查）
    unsigned long elapsed = currentTime - gnss_data.last_update;
    if (elapsed > fallbackConfig.gnss_timeout) {
        AIR780EG_LOGD(TAG, "GNSS信号丢失: 数据过期 (已过 %lu秒, 超时: %lu秒)", 
                      elapsed/1000, fallbackConfig.gnss_timeout/1000);
        return true;
    }
    
    // 检查定位类型是否为GNSS
    if (gnss_data.location_type != "GNSS") {
        AIR780EG_LOGD(TAG, "GNSS信号丢失: 定位类型不是GNSS (当前: %s)", 
                      gnss_data.location_type.c_str());
        return true;
    }
    
    AIR780EG_LOGD(TAG, "GNSS信号正常: 类型=%s, 卫星数=%d, 更新时间=%lu秒前", 
                  gnss_data.location_type.c_str(), gnss_data.satellites, elapsed/1000);
    return false;
}

// GPS时间获取方法
gps_time_t Air780EGGNSS::getGPSTime()
{
    return gnss_data.gps_time;
}

String Air780EGGNSS::getGPSTimeString()
{
    if (!gnss_data.gps_time.valid) {
        return "Invalid GPS Time";
    }
    
    char time_str[32];
    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d.%03d",
             gnss_data.gps_time.year, gnss_data.gps_time.month, gnss_data.gps_time.day,
             gnss_data.gps_time.hour, gnss_data.gps_time.minute, gnss_data.gps_time.second, gnss_data.gps_time.millisecond);
    
    return String(time_str);
}

bool Air780EGGNSS::isGPSTimeValid()
{
    return gnss_data.gps_time.valid;
}

