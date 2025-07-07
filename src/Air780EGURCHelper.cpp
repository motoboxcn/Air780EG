#include "Air780EGURC.h"

namespace URCHelper {

// 网络注册状态解析
NetworkRegistration parseNetworkRegistration(const URCData& urc) {
    NetworkRegistration reg = {};
    
    if (urc.parameters.size() >= 2) {
        reg.n = urc.parameters[0].toInt();
        reg.stat = urc.parameters[1].toInt();
        
        if (urc.parameters.size() >= 4) {
            reg.lac = unquoteString(urc.parameters[2]);
            reg.ci = unquoteString(urc.parameters[3]);
        }
        
        if (urc.parameters.size() >= 5) {
            reg.act = urc.parameters[4].toInt();
        }
    }
    
    return reg;
}

String NetworkRegistration::getStatusString() const {
    switch (stat) {
        case 0: return "Not searching";
        case 1: return "Registered (home)";
        case 2: return "Searching";
        case 3: return "Registration denied";
        case 4: return "Unknown";
        case 5: return "Registered (roaming)";
        default: return "Invalid";
    }
}

String NetworkRegistration::getAccessTechnologyString() const {
    switch (act) {
        case 0: return "GSM";
        case 1: return "GSM Compact";
        case 2: return "UTRAN";
        case 3: return "GSM/GPRS with EDGE";
        case 4: return "UTRAN with HSDPA";
        case 5: return "UTRAN with HSUPA";
        case 6: return "UTRAN with HSDPA and HSUPA";
        case 7: return "E-UTRAN";
        default: return "Unknown";
    }
}

// GNSS信息解析
GNSSInfo parseGNSSInfo(const URCData& urc) {
    GNSSInfo gnss = {};
    
    if (urc.parameters.size() >= 15) {
        gnss.run_status = (urc.parameters[0] == "1");
        gnss.fix_status = (urc.parameters[1] == "1");
        gnss.utc_datetime = urc.parameters[2];
        
        if (urc.parameters[3].length() > 0) {
            gnss.latitude = urc.parameters[3].toDouble();
        }
        if (urc.parameters[4].length() > 0) {
            gnss.longitude = urc.parameters[4].toDouble();
        }
        if (urc.parameters[5].length() > 0) {
            gnss.altitude = urc.parameters[5].toDouble();
        }
        if (urc.parameters[6].length() > 0) {
            gnss.speed = urc.parameters[6].toFloat();
        }
        if (urc.parameters[7].length() > 0) {
            gnss.course = urc.parameters[7].toFloat();
        }
        if (urc.parameters[10].length() > 0) {
            gnss.hdop = urc.parameters[10].toFloat();
        }
        if (urc.parameters[11].length() > 0) {
            gnss.pdop = urc.parameters[11].toFloat();
        }
        if (urc.parameters[12].length() > 0) {
            gnss.vdop = urc.parameters[12].toFloat();
        }
        if (urc.parameters[14].length() > 0) {
            gnss.satellites_view = urc.parameters[14].toInt();
        }
        if (urc.parameters[15].length() > 0) {
            gnss.satellites_used = urc.parameters[15].toInt();
        }
    }
    
    return gnss;
}

// MQTT消息解析
MQTTMessage parseMQTTMessage(const URCData& urc) {
    MQTTMessage mqtt = {};
    
    // +MSUB: "topic","payload",qos,retained
    if (urc.parameters.size() >= 2) {
        mqtt.topic = unquoteString(urc.parameters[0]);
        mqtt.payload = unquoteString(urc.parameters[1]);
        
        if (urc.parameters.size() >= 3) {
            mqtt.qos = urc.parameters[2].toInt();
        }
        if (urc.parameters.size() >= 4) {
            mqtt.retained = (urc.parameters[3] == "1");
        }
    }
    
    return mqtt;
}

// 信号质量解析
SignalQuality parseSignalQuality(const URCData& urc) {
    SignalQuality sq = {};
    
    if (urc.parameters.size() >= 2) {
        sq.rssi = urc.parameters[0].toInt();
        sq.ber = urc.parameters[1].toInt();
    }
    
    return sq;
}

int SignalQuality::getRSSIdBm() const {
    if (rssi == 99) return -999; // 未知
    if (rssi >= 0 && rssi <= 31) {
        return -113 + rssi * 2;
    }
    return -999;
}

// 通用辅助函数
std::vector<String> splitParameters(const String& params, char delimiter) {
    std::vector<String> result;
    int start = 0;
    
    for (int i = 0; i <= params.length(); i++) {
        if (i == params.length() || params.charAt(i) == delimiter) {
            if (i > start) {
                String param = params.substring(start, i);
                param.trim();
                result.push_back(param);
            }
            start = i + 1;
        }
    }
    
    return result;
}

String unquoteString(const String& str) {
    String result = str;
    result.trim();
    
    if (result.startsWith("\"") && result.endsWith("\"") && result.length() >= 2) {
        result = result.substring(1, result.length() - 1);
    }
    
    return result;
}

double parseCoordinate(const String& coord_str) {
    return coord_str.toDouble();
}

} // namespace URCHelper
