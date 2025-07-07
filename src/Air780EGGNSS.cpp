#include "Air780EGGNSS.h"

const char* Air780EGGNSS::TAG = "Air780EGGNSS";

Air780EGGNSS::Air780EGGNSS(Air780EGCore* core_instance) : core(core_instance) {
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

bool Air780EGGNSS::enableGNSS() {
    if (!core || !core->isInitialized()) {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "Enabling GNSS...");
    
    // 启用GNSS功能
    if (!core->sendATCommandBool("AT+CGNSPWR=1")) {
        AIR780EG_LOGE(TAG, "Failed to power on GNSS");
        return false;
    }
    
    delay(1000);
    
    // 设置GNSS模式 (GPS + 北斗)
    if (!setGNSSMode(3)) {
        AIR780EG_LOGW(TAG, "Failed to set GNSS mode, using default");
    }
    
    // 设置更新频率
    if (!setUpdateRate(gnss_update_interval)) {
        AIR780EG_LOGW(TAG, "Failed to set update rate, using default");
    }
    
    gnss_enabled = true;
    AIR780EG_LOGI(TAG, "GNSS enabled successfully");
    
    return true;
}

bool Air780EGGNSS::disableGNSS() {
    if (!core || !core->isInitialized()) {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "Disabling GNSS...");
    
    // 关闭GNSS功能
    if (!core->sendATCommandBool("AT+CGNSPWR=0")) {
        AIR780EG_LOGE(TAG, "Failed to power off GNSS");
        return false;
    }
    
    gnss_enabled = false;
    gnss_data.data_valid = false;
    
    AIR780EG_LOGI(TAG, "GNSS disabled");
    return true;
}

void Air780EGGNSS::loop() {
    if (!gnss_enabled || !core || !core->isInitialized()) {
        return;
    }
    
    unsigned long current_time = millis();
    
    // 检查是否需要更新GNSS数据
    if (current_time - last_loop_time >= gnss_update_interval) {
        updateGNSSData();
        last_loop_time = current_time;
    }
}

void Air780EGGNSS::updateGNSSData() {
    AIR780EG_LOGV(TAG, "Updating GNSS data...");
    
    String response = core->sendATCommand("AT+CGNSINF", 3000);
    
    if (response.indexOf("OK") >= 0) {
        if (parseGNSSResponse(response)) {
            gnss_data.last_update = millis();
            gnss_data.data_valid = true;
            
            AIR780EG_LOGD(TAG, "GNSS data updated - Fixed: %s, Sats: %d, Lat: %.6f, Lng: %.6f", 
                         gnss_data.is_fixed ? "Yes" : "No", 
                         gnss_data.satellites,
                         gnss_data.latitude, 
                         gnss_data.longitude);
        } else {
            AIR780EG_LOGW(TAG, "Failed to parse GNSS response");
        }
    } else {
        AIR780EG_LOGW(TAG, "No response from GNSS command");
    }
}

bool Air780EGGNSS::parseGNSSResponse(const String& response) {
    // 查找+CGNSINF:行
    int info_start = response.indexOf("+CGNSINF: ");
    if (info_start < 0) {
        return false;
    }
    
    info_start += 10; // "+CGNSINF: "的长度
    int info_end = response.indexOf('\r', info_start);
    if (info_end < 0) {
        info_end = response.length();
    }
    
    String info_line = response.substring(info_start, info_end);
    AIR780EG_LOGV(TAG, "GNSS info line: %s", info_line.c_str());
    
    // 解析逗号分隔的字段
    int field_count = 0;
    int start = 0;
    
    for (int i = 0; i <= info_line.length(); i++) {
        if (i == info_line.length() || info_line.charAt(i) == ',') {
            String field = info_line.substring(start, i);
            field.trim();
            
            switch (field_count) {
                case 0: // GNSS运行状态
                    gnss_data.is_fixed = (field == "1");
                    break;
                case 1: // 定位状态
                    // 1表示已定位
                    break;
                case 2: // UTC日期时间
                    if (field.length() >= 14) {
                        gnss_data.date = field.substring(0, 8);      // YYYYMMDD
                        gnss_data.timestamp = field.substring(8);    // HHMMSS.sss
                    }
                    break;
                case 3: // 纬度
                    if (field.length() > 0) {
                        gnss_data.latitude = field.toDouble();
                    }
                    break;
                case 4: // 经度
                    if (field.length() > 0) {
                        gnss_data.longitude = field.toDouble();
                    }
                    break;
                case 5: // 海拔高度
                    if (field.length() > 0) {
                        gnss_data.altitude = field.toDouble();
                    }
                    break;
                case 6: // 速度 (km/h)
                    if (field.length() > 0) {
                        gnss_data.speed = field.toFloat();
                    }
                    break;
                case 7: // 航向角
                    if (field.length() > 0) {
                        gnss_data.course = field.toFloat();
                    }
                    break;
                case 8: // HDOP
                    if (field.length() > 0) {
                        gnss_data.hdop = field.toFloat();
                    }
                    break;
                case 9: // PDOP
                    // 暂不使用
                    break;
                case 10: // VDOP
                    // 暂不使用
                    break;
                case 11: // 卫星数量
                    if (field.length() > 0) {
                        gnss_data.satellites = field.toInt();
                    }
                    break;
            }
            
            field_count++;
            start = i + 1;
        }
    }
    
    return field_count >= 12; // 至少解析到卫星数量字段
}

bool Air780EGGNSS::setGNSSMode(int mode) {
    String cmd = "AT+CGNSCFG=\"MODE\"," + String(mode);
    bool result = core->sendATCommandBool(cmd);
    
    if (result) {
        AIR780EG_LOGD(TAG, "GNSS mode set to %d", mode);
    } else {
        AIR780EG_LOGE(TAG, "Failed to set GNSS mode to %d", mode);
    }
    
    return result;
}

bool Air780EGGNSS::setUpdateRate(int rate_ms) {
    String cmd = "AT+CGNSCFG=\"RATE\"," + String(rate_ms);
    bool result = core->sendATCommandBool(cmd);
    
    if (result) {
        AIR780EG_LOGD(TAG, "GNSS update rate set to %d ms", rate_ms);
    } else {
        AIR780EG_LOGE(TAG, "Failed to set GNSS update rate to %d ms", rate_ms);
    }
    
    return result;
}

void Air780EGGNSS::setUpdateFrequency(float hz) {
    if (hz < 0.1) hz = 0.1;
    if (hz > 10.0) hz = 10.0;
    
    update_frequency = hz;
    gnss_update_interval = (unsigned long)(1000.0 / hz);
    
    AIR780EG_LOGD(TAG, "Update frequency set to %.1f Hz (%lu ms interval)", 
                 hz, gnss_update_interval);
    
    // 如果GNSS已启用，更新硬件设置
    if (gnss_enabled) {
        setUpdateRate(gnss_update_interval);
    }
}

float Air780EGGNSS::getUpdateFrequency() const {
    return update_frequency;
}

bool Air780EGGNSS::isFixed() {
    return gnss_data.data_valid && gnss_data.is_fixed;
}

double Air780EGGNSS::getLatitude() {
    return gnss_data.latitude;
}

double Air780EGGNSS::getLongitude() {
    return gnss_data.longitude;
}

double Air780EGGNSS::getAltitude() {
    return gnss_data.altitude;
}

float Air780EGGNSS::getSpeed() {
    return gnss_data.speed;
}

float Air780EGGNSS::getCourse() {
    return gnss_data.course;
}

int Air780EGGNSS::getSatelliteCount() {
    return gnss_data.satellites;
}

float Air780EGGNSS::getHDOP() {
    return gnss_data.hdop;
}

String Air780EGGNSS::getTimestamp() {
    return gnss_data.timestamp;
}

String Air780EGGNSS::getDate() {
    return gnss_data.date;
}

bool Air780EGGNSS::isEnabled() const {
    return gnss_enabled;
}

bool Air780EGGNSS::isDataValid() const {
    return gnss_data.data_valid;
}

unsigned long Air780EGGNSS::getLastUpdateTime() const {
    return gnss_data.last_update;
}

void Air780EGGNSS::printGNSSInfo() {
    AIR780EG_LOGI(TAG, "=== GNSS Information ===");
    AIR780EG_LOGI(TAG, "Fixed: %s", gnss_data.is_fixed ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Satellites: %d", gnss_data.satellites);
    AIR780EG_LOGI(TAG, "Latitude: %.6f", gnss_data.latitude);
    AIR780EG_LOGI(TAG, "Longitude: %.6f", gnss_data.longitude);
    AIR780EG_LOGI(TAG, "Altitude: %.2f m", gnss_data.altitude);
    AIR780EG_LOGI(TAG, "Speed: %.2f km/h", gnss_data.speed);
    AIR780EG_LOGI(TAG, "Course: %.2f°", gnss_data.course);
    AIR780EG_LOGI(TAG, "HDOP: %.2f", gnss_data.hdop);
    AIR780EG_LOGI(TAG, "Date: %s", gnss_data.date.c_str());
    AIR780EG_LOGI(TAG, "Time: %s", gnss_data.timestamp.c_str());
    AIR780EG_LOGI(TAG, "Update Freq: %.1f Hz", update_frequency);
    AIR780EG_LOGI(TAG, "Last Update: %lu ms ago", millis() - gnss_data.last_update);
    AIR780EG_LOGI(TAG, "======================");
}

String Air780EGGNSS::getRawGNSSData() {
    if (!core || !core->isInitialized()) {
        return "";
    }
    
    return core->sendATCommand("AT+CGNSINF", 3000);
}
