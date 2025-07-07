#include "Air780EGNetwork.h"

const char* Air780EGNetwork::TAG = "Air780EGNetwork";

Air780EGNetwork::Air780EGNetwork(Air780EGCore* core_instance) : core(core_instance) {
    // 初始化网络状态结构
    network_status.is_registered = false;
    network_status.signal_strength = -999;
    network_status.operator_name = "";
    network_status.network_type = "";
    network_status.imei = "";
    network_status.imsi = "";
    network_status.ccid = "";
    network_status.last_update = 0;
    network_status.data_valid = false;
}

bool Air780EGNetwork::enableNetwork() {
    if (!core || !core->isInitialized()) {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "Enabling network...");
    
    // 设置全功能模式
    if (!core->sendATCommandBool("AT+CFUN=1")) {
        AIR780EG_LOGE(TAG, "Failed to set full functionality mode");
        return false;
    }
    
    delay(2000);
    
    // 获取模块基本信息
    updateModuleInfo();
    
    network_enabled = true;
    AIR780EG_LOGI(TAG, "Network enabled successfully");
    
    // 立即更新一次网络状态
    updateNetworkStatus();
    
    return true;
}

bool Air780EGNetwork::disableNetwork() {
    if (!core || !core->isInitialized()) {
        AIR780EG_LOGE(TAG, "Core not initialized");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "Disabling network...");
    
    // 设置最小功能模式
    if (!core->sendATCommandBool("AT+CFUN=0")) {
        AIR780EG_LOGE(TAG, "Failed to set minimum functionality mode");
        return false;
    }
    
    network_enabled = false;
    network_status.data_valid = false;
    
    AIR780EG_LOGI(TAG, "Network disabled");
    return true;
}

void Air780EGNetwork::loop() {
    if (!network_enabled || !core || !core->isInitialized()) {
        return;
    }
    
    unsigned long current_time = millis();
    
    // 检查是否需要更新网络状态
    if (current_time - last_loop_time >= network_update_interval) {
        updateNetworkStatus();
        last_loop_time = current_time;
    }
}

void Air780EGNetwork::updateNetworkStatus() {
    AIR780EG_LOGV(TAG, "Updating network status...");
    
    updateRegistrationStatus();
    updateSignalStrength();
    updateOperatorInfo();
    
    network_status.last_update = millis();
    network_status.data_valid = true;
    
    AIR780EG_LOGD(TAG, "Network status updated - Registered: %s, Signal: %d dBm", 
                 network_status.is_registered ? "Yes" : "No", 
                 network_status.signal_strength);
}

void Air780EGNetwork::updateSignalStrength() {
    String response = core->sendATCommand("AT+CSQ");
    
    if (response.indexOf("OK") >= 0) {
        int csq_start = response.indexOf("+CSQ: ");
        if (csq_start >= 0) {
            csq_start += 6; // "+CSQ: "的长度
            int comma_pos = response.indexOf(',', csq_start);
            if (comma_pos > csq_start) {
                int rssi = response.substring(csq_start, comma_pos).toInt();
                
                // 转换RSSI值为dBm
                if (rssi >= 0 && rssi <= 31) {
                    network_status.signal_strength = -113 + rssi * 2;
                } else {
                    network_status.signal_strength = -999; // 无效值
                }
                
                AIR780EG_LOGV(TAG, "Signal strength: %d dBm (RSSI: %d)", 
                             network_status.signal_strength, rssi);
            }
        }
    }
}

void Air780EGNetwork::updateRegistrationStatus() {
    String response = core->sendATCommand("AT+CREG?");
    
    if (response.indexOf("OK") >= 0) {
        int creg_start = response.indexOf("+CREG: ");
        if (creg_start >= 0) {
            creg_start += 7; // "+CREG: "的长度
            int comma_pos = response.indexOf(',', creg_start);
            if (comma_pos > creg_start) {
                int status = response.substring(comma_pos + 1, comma_pos + 2).toInt();
                network_status.is_registered = (status == 1 || status == 5);
                
                AIR780EG_LOGV(TAG, "Registration status: %d (%s)", 
                             status, network_status.is_registered ? "Registered" : "Not registered");
            }
        }
    }
}

void Air780EGNetwork::updateOperatorInfo() {
    String response = core->sendATCommand("AT+COPS?");
    
    if (response.indexOf("OK") >= 0) {
        int cops_start = response.indexOf("+COPS: ");
        if (cops_start >= 0) {
            // 解析运营商信息
            int quote_start = response.indexOf('"', cops_start);
            if (quote_start >= 0) {
                int quote_end = response.indexOf('"', quote_start + 1);
                if (quote_end > quote_start) {
                    network_status.operator_name = response.substring(quote_start + 1, quote_end);
                    AIR780EG_LOGV(TAG, "Operator: %s", network_status.operator_name.c_str());
                }
            }
        }
    }
    
    // 获取网络类型
    response = core->sendATCommand("AT+CNSMOD?");
    if (response.indexOf("OK") >= 0) {
        int mode_start = response.indexOf("+CNSMOD: ");
        if (mode_start >= 0) {
            mode_start += 9; // "+CNSMOD: "的长度
            int mode = response.substring(mode_start, mode_start + 1).toInt();
            
            switch (mode) {
                case 1: network_status.network_type = "GSM"; break;
                case 3: network_status.network_type = "EDGE"; break;
                case 4: network_status.network_type = "WCDMA"; break;
                case 5: network_status.network_type = "HSDPA"; break;
                case 6: network_status.network_type = "HSUPA"; break;
                case 7: network_status.network_type = "HSPA"; break;
                case 8: network_status.network_type = "LTE"; break;
                default: network_status.network_type = "Unknown"; break;
            }
            
            AIR780EG_LOGV(TAG, "Network type: %s", network_status.network_type.c_str());
        }
    }
}

void Air780EGNetwork::updateModuleInfo() {
    // 获取IMEI
    String response = core->sendATCommand("AT+CGSN");
    if (response.indexOf("OK") >= 0) {
        int start = response.indexOf('\n');
        int end = response.indexOf('\r', start + 1);
        if (start >= 0 && end > start) {
            network_status.imei = response.substring(start + 1, end);
            network_status.imei.trim();
            AIR780EG_LOGI(TAG, "IMEI: %s", network_status.imei.c_str());
        }
    }
    
    // 获取IMSI
    response = core->sendATCommand("AT+CIMI");
    if (response.indexOf("OK") >= 0) {
        int start = response.indexOf('\n');
        int end = response.indexOf('\r', start + 1);
        if (start >= 0 && end > start) {
            network_status.imsi = response.substring(start + 1, end);
            network_status.imsi.trim();
            AIR780EG_LOGI(TAG, "IMSI: %s", network_status.imsi.c_str());
        }
    }
    
    // 获取CCID
    response = core->sendATCommand("AT+CCID");
    if (response.indexOf("OK") >= 0) {
        int ccid_start = response.indexOf("+CCID: ");
        if (ccid_start >= 0) {
            ccid_start += 7; // "+CCID: "的长度
            int end = response.indexOf('\r', ccid_start);
            if (end > ccid_start) {
                network_status.ccid = response.substring(ccid_start, end);
                network_status.ccid.trim();
                AIR780EG_LOGI(TAG, "CCID: %s", network_status.ccid.c_str());
            }
        }
    }
}

bool Air780EGNetwork::isNetworkRegistered() {
    return network_status.data_valid && network_status.is_registered;
}

int Air780EGNetwork::getSignalStrength() {
    return network_status.signal_strength;
}

String Air780EGNetwork::getOperatorName() {
    return network_status.operator_name;
}

String Air780EGNetwork::getNetworkType() {
    return network_status.network_type;
}

String Air780EGNetwork::getIMEI() {
    return network_status.imei;
}

String Air780EGNetwork::getIMSI() {
    return network_status.imsi;
}

String Air780EGNetwork::getCCID() {
    return network_status.ccid;
}

bool Air780EGNetwork::setAPN(const String& apn, const String& username, const String& password) {
    AIR780EG_LOGI(TAG, "Setting APN: %s", apn.c_str());
    
    String cmd = "AT+CGDCONT=1,\"IP\",\"" + apn + "\"";
    if (!core->sendATCommandBool(cmd)) {
        AIR780EG_LOGE(TAG, "Failed to set APN");
        return false;
    }
    
    if (username.length() > 0 || password.length() > 0) {
        cmd = "AT+CGAUTH=1,1,\"" + password + "\",\"" + username + "\"";
        if (!core->sendATCommandBool(cmd)) {
            AIR780EG_LOGW(TAG, "Failed to set APN authentication");
        }
    }
    
    AIR780EG_LOGI(TAG, "APN configured successfully");
    return true;
}

bool Air780EGNetwork::activatePDP() {
    AIR780EG_LOGI(TAG, "Activating PDP context...");
    
    if (!core->sendATCommandBool("AT+CGACT=1,1", 30000)) {
        AIR780EG_LOGE(TAG, "Failed to activate PDP context");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "PDP context activated");
    return true;
}

bool Air780EGNetwork::deactivatePDP() {
    AIR780EG_LOGI(TAG, "Deactivating PDP context...");
    
    if (!core->sendATCommandBool("AT+CGACT=0,1")) {
        AIR780EG_LOGE(TAG, "Failed to deactivate PDP context");
        return false;
    }
    
    AIR780EG_LOGI(TAG, "PDP context deactivated");
    return true;
}

bool Air780EGNetwork::isPDPActive() {
    String response = core->sendATCommand("AT+CGACT?");
    return response.indexOf("+CGACT: 1,1") >= 0;
}

void Air780EGNetwork::setUpdateInterval(unsigned long interval_ms) {
    network_update_interval = interval_ms;
    AIR780EG_LOGD(TAG, "Update interval set to %lu ms", interval_ms);
}

unsigned long Air780EGNetwork::getUpdateInterval() const {
    return network_update_interval;
}

bool Air780EGNetwork::isEnabled() const {
    return network_enabled;
}

bool Air780EGNetwork::isDataValid() const {
    return network_status.data_valid;
}

unsigned long Air780EGNetwork::getLastUpdateTime() const {
    return network_status.last_update;
}

void Air780EGNetwork::printNetworkInfo() {
    AIR780EG_LOGI(TAG, "=== Network Information ===");
    AIR780EG_LOGI(TAG, "IMEI: %s", network_status.imei.c_str());
    AIR780EG_LOGI(TAG, "IMSI: %s", network_status.imsi.c_str());
    AIR780EG_LOGI(TAG, "CCID: %s", network_status.ccid.c_str());
    AIR780EG_LOGI(TAG, "Registered: %s", network_status.is_registered ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Signal: %d dBm", network_status.signal_strength);
    AIR780EG_LOGI(TAG, "Operator: %s", network_status.operator_name.c_str());
    AIR780EG_LOGI(TAG, "Network Type: %s", network_status.network_type.c_str());
    AIR780EG_LOGI(TAG, "Last Update: %lu ms ago", millis() - network_status.last_update);
    AIR780EG_LOGI(TAG, "========================");
}
