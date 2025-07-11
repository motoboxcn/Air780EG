#include "Air780EGURC.h"

const char* Air780EGURC::TAG = "Air780EGURC";

Air780EGURC::Air780EGURC() {
    AIR780EG_LOGD(TAG, "URC Manager initialized");
}

Air780EGURC::~Air780EGURC() {
    clearHandlers();
}

void Air780EGURC::enableProcessing(bool enable) {
    processing_enabled = enable;
    AIR780EG_LOGD(TAG, "URC processing %s", enable ? "enabled" : "disabled");
}

bool Air780EGURC::isProcessingEnabled() const {
    return processing_enabled;
}

void Air780EGURC::registerHandler(const String& prefix, URCType type, URCHandler handler, const String& description) {
    // 检查是否已存在
    for (auto& entry : handlers) {
        if (entry.prefix == prefix) {
            entry.handler = handler;
            entry.type = type;
            entry.description = description;
            AIR780EG_LOGD(TAG, "Updated handler for prefix: %s", prefix.c_str());
            return;
        }
    }
    
    // 添加新处理器
    handlers.emplace_back(prefix, type, handler, description);
    AIR780EG_LOGD(TAG, "Registered handler for prefix: %s (%s)", prefix.c_str(), description.c_str());
}

void Air780EGURC::unregisterHandler(const String& prefix) {
    for (auto it = handlers.begin(); it != handlers.end(); ++it) {
        if (it->prefix == prefix) {
            AIR780EG_LOGD(TAG, "Unregistered handler for prefix: %s", prefix.c_str());
            handlers.erase(it);
            return;
        }
    }
}

void Air780EGURC::processLine(const String& line) {
    if (!processing_enabled || line.length() == 0) {
        return;
    }
    
    total_processed++;
    AIR780EG_LOGV(TAG, "Processing line: %s", line.c_str());
    
    bool matched = false;
    
    // 查找匹配的处理器
    for (const auto& entry : handlers) {
        if (entry.enabled && line.startsWith(entry.prefix)) {
            matched = true;
            total_matched++;
            
            AIR780EG_LOGV(TAG, "Matched prefix: %s", entry.prefix.c_str());
            
            // 解析URC数据
            URCData urc_data = parseURCLine(line);
            urc_data.type = entry.type;
            urc_data.prefix = entry.prefix;
            
            // 调用处理器
            try {
                entry.handler(urc_data);
            } catch (...) {
                AIR780EG_LOGE(TAG, "Exception in URC handler for: %s", entry.prefix.c_str());
            }
            
            break; // 只处理第一个匹配的
        }
    }
    
    if (!matched) {
        total_unmatched++;
        AIR780EG_LOGV(TAG, "No handler for line: %s", line.c_str());
    }
}
URCData Air780EGURC::parseURCLine(const String& line) {
    URCData urc;
    urc.raw_data = line;
    urc.timestamp = millis();
    
    // 查找冒号位置
    int colon_pos = line.indexOf(':');
    if (colon_pos > 0) {
        urc.prefix = line.substring(0, colon_pos + 1);
        
        // 解析参数
        if (colon_pos + 1 < line.length()) {
            String params = line.substring(colon_pos + 1);
            params.trim();
            urc.parameters = parseParameters(params, urc.prefix);
        }
    } else {
        // 没有冒号的URC (如 RING)
        urc.prefix = line;
    }
    
    return urc;
}

std::vector<String> Air780EGURC::parseParameters(const String& data, const String& prefix) {
    std::vector<String> params;
    
    if (data.length() == 0) {
        return params;
    }
    
    // 简单的逗号分割
    int start = 0;
    for (int i = 0; i <= data.length(); i++) {
        if (i == data.length() || data.charAt(i) == ',') {
            if (i > start) {
                String param = data.substring(start, i);
                param.trim();
                params.push_back(param);
            }
            start = i + 1;
        }
    }
    
    return params;
}

// 便捷注册方法
void Air780EGURC::onNetworkRegistration(URCHandler handler) {
    registerHandler("+CREG:", URCType::NETWORK_REGISTRATION, handler, "Network Registration");
    registerHandler("+CGREG:", URCType::NETWORK_REGISTRATION, handler, "GPRS Registration");
    registerHandler("+CEREG:", URCType::NETWORK_REGISTRATION, handler, "Network Registration");
}



void Air780EGURC::onSignalQuality(URCHandler handler) {
    registerHandler("+CSQ:", URCType::SIGNAL_QUALITY, handler, "Signal Quality");
}

void Air780EGURC::onGNSSInfo(URCHandler handler) {
    registerHandler("+UGNSINF:", URCType::GNSS_INFO, handler, "GNSS Info (URC)");
    registerHandler("+CGNSINF:", URCType::GNSS_INFO, handler, "GNSS Info");
}

void Air780EGURC::onMQTTMessage(URCHandler handler) {
    registerHandler("+MSUB:", URCType::MQTT_MESSAGE, handler, "MQTT Message");
}

void Air780EGURC::onSMSReceived(URCHandler handler) {
    registerHandler("+CMTI:", URCType::SMS_RECEIVED, handler, "SMS Received");
}

void Air780EGURC::onIncomingCall(URCHandler handler) {
    registerHandler("RING", URCType::CALL_INCOMING, handler, "Incoming Call");
    registerHandler("+CLIP:", URCType::CALL_INCOMING, handler, "Caller ID");
}

void Air780EGURC::onPDPDeactivated(URCHandler handler) {
    registerHandler("+CGEV:", URCType::PDP_DEACTIVATED, handler, "PDP Context Event");
}

void Air780EGURC::onErrorReport(URCHandler handler) {
    registerHandler("+CME ERROR:", URCType::ERROR_REPORT, handler, "CME Error");
    registerHandler("+CMS ERROR:", URCType::ERROR_REPORT, handler, "CMS Error");
}

// 状态和调试方法
size_t Air780EGURC::getHandlerCount() const {
    return handlers.size();
}

unsigned long Air780EGURC::getTotalProcessed() const {
    return total_processed;
}

unsigned long Air780EGURC::getTotalMatched() const {
    return total_matched;
}

unsigned long Air780EGURC::getTotalUnmatched() const {
    return total_unmatched;
}

void Air780EGURC::printHandlers() const {
    AIR780EG_LOGI(TAG, "=== URC Handlers (%d) ===", handlers.size());
    for (size_t i = 0; i < handlers.size(); i++) {
        const auto& entry = handlers[i];
        AIR780EG_LOGI(TAG, "%d. %s - %s (%s)", 
                     i + 1, 
                     entry.prefix.c_str(), 
                     entry.description.c_str(),
                     entry.enabled ? "enabled" : "disabled");
    }
    AIR780EG_LOGI(TAG, "==================");
}

void Air780EGURC::printStatistics() const {
    AIR780EG_LOGI(TAG, "=== URC Statistics ===");
    AIR780EG_LOGI(TAG, "Total Processed: %lu", total_processed);
    AIR780EG_LOGI(TAG, "Total Matched: %lu", total_matched);
    AIR780EG_LOGI(TAG, "Total Unmatched: %lu", total_unmatched);
    if (total_processed > 0) {
        AIR780EG_LOGI(TAG, "Match Rate: %.1f%%", (float)total_matched / total_processed * 100);
    }
    AIR780EG_LOGI(TAG, "==================");
}

void Air780EGURC::clearHandlers() {
    handlers.clear();
    AIR780EG_LOGD(TAG, "All handlers cleared");
}

void Air780EGURC::resetStatistics() {
    total_processed = 0;
    total_matched = 0;
    total_unmatched = 0;
    AIR780EG_LOGD(TAG, "Statistics reset");
}
