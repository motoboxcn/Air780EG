#ifndef AIR780EG_URC_H
#define AIR780EG_URC_H

#include <Arduino.h>
#include <functional>
#include <vector>
#include "Air780EGDebug.h"

// URC消息类型枚举
enum class URCType {
    NETWORK_REGISTRATION,   // +CREG, +CGREG, +CEREG
    SIGNAL_QUALITY,        // +CSQ
    GNSS_INFO,            // +UGNSINF, +CGNSINF
    MQTT_MESSAGE,         // +MSUB
    SMS_RECEIVED,         // +CMTI
    CALL_INCOMING,        // +RING, +CLIP
    PDP_DEACTIVATED,      // +CGEV
    ERROR_REPORT,         // +CME ERROR, +CMS ERROR
    UNKNOWN
};

// URC数据结构
struct URCData {
    URCType type;
    String raw_data;
    String prefix;
    std::vector<String> parameters;
    unsigned long timestamp;
    
    URCData() : type(URCType::UNKNOWN), timestamp(0) {}
    URCData(URCType t, const String& raw, const String& pre) 
        : type(t), raw_data(raw), prefix(pre), timestamp(millis()) {}
};

// URC处理器回调函数类型
typedef std::function<void(const URCData&)> URCHandler;

class Air780EGURC {
private:
    static const char* TAG;
    
    // URC处理器注册表
    struct HandlerEntry {
        String prefix;
        URCType type;
        URCHandler handler;
        bool enabled;
        String description;
        
        HandlerEntry(const String& p, URCType t, URCHandler h, const String& desc = "") 
            : prefix(p), type(t), handler(h), enabled(true), description(desc) {}
    };
    
    std::vector<HandlerEntry> handlers;
    bool processing_enabled = true;
    
    // 统计信息
    unsigned long total_processed = 0;
    unsigned long total_matched = 0;
    unsigned long total_unmatched = 0;
    
    // 内部方法
    URCType identifyURCType(const String& line);
    URCData parseURCLine(const String& line);
    std::vector<String> parseParameters(const String& data, const String& prefix);
    
public:
    Air780EGURC();
    ~Air780EGURC();
    
    // URC处理控制
    void enableProcessing(bool enable = true);
    bool isProcessingEnabled() const;
    
    // 处理器注册和管理
    void registerHandler(const String& prefix, URCType type, URCHandler handler, const String& description = "");
    void unregisterHandler(const String& prefix);
    void enableHandler(const String& prefix, bool enable = true);
    bool hasHandler(const String& prefix) const;
    
    // 便捷的处理器注册方法
    void onNetworkRegistration(URCHandler handler);
    void onSignalQuality(URCHandler handler);
    void onGNSSInfo(URCHandler handler);
    void onMQTTMessage(URCHandler handler);
    void onSMSReceived(URCHandler handler);
    void onIncomingCall(URCHandler handler);
    void onPDPDeactivated(URCHandler handler);
    void onErrorReport(URCHandler handler);
    
    // 主要处理方法
    void processLine(const String& line);
    
    // 状态查询
    size_t getHandlerCount() const;
    unsigned long getTotalProcessed() const;
    unsigned long getTotalMatched() const;
    unsigned long getTotalUnmatched() const;
    
    // 调试和管理
    void printHandlers() const;
    void printStatistics() const;
    void clearHandlers();
    void resetStatistics();
};

// URC数据解析辅助函数
namespace URCHelper {
    // 网络注册状态解析
    struct NetworkRegistration {
        int n;          // 网络注册URC报告设置
        int stat;       // 注册状态 (0=未搜索,1=已注册本地,2=搜索中,3=注册被拒,5=已注册漫游)
        String lac;     // 位置区域码
        String ci;      // 小区ID
        int act;        // 接入技术 (0=GSM,2=UTRAN,7=E-UTRAN)
        
        bool isRegistered() const { return stat == 1 || stat == 5; }
        String getStatusString() const;
        String getAccessTechnologyString() const;
    };
    
    // GNSS信息解析
    struct GNSSInfo {
        bool run_status;        // GNSS运行状态
        bool fix_status;        // 定位状态
        String utc_datetime;    // UTC日期时间
        double latitude;        // 纬度
        double longitude;       // 经度
        double altitude;        // 海拔
        float speed;           // 速度 (km/h)
        float course;          // 航向 (度)
        float hdop;            // 水平精度因子
        float pdop;            // 位置精度因子
        float vdop;            // 垂直精度因子
        int satellites_view;    // 可见卫星数
        int satellites_used;    // 使用卫星数
        
        bool isValid() const { return run_status && fix_status; }
    };
    
    // MQTT消息解析
    struct MQTTMessage {
        String topic;
        String payload;
        int qos;
        bool retained;
        int message_id;
        
        bool isValid() const { return topic.length() > 0; }
    };
    
    // 信号质量解析
    struct SignalQuality {
        int rssi;           // 接收信号强度指示 (0-31, 99=未知)
        int ber;            // 误码率 (0-7, 99=未知)
        
        int getRSSIdBm() const;  // 转换为dBm值
        bool isValid() const { return rssi != 99; }
    };
    
    // 解析函数
    NetworkRegistration parseNetworkRegistration(const URCData& urc);
    GNSSInfo parseGNSSInfo(const URCData& urc);
    MQTTMessage parseMQTTMessage(const URCData& urc);
    SignalQuality parseSignalQuality(const URCData& urc);
    
    // 通用参数解析
    std::vector<String> splitParameters(const String& params, char delimiter = ',');
    String unquoteString(const String& str);
    double parseCoordinate(const String& coord_str);
}

#endif // AIR780EG_URC_H
