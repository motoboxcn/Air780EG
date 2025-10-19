#ifndef AIR780EG_H
#define AIR780EG_H

#include <Arduino.h>
#include <HardwareSerial.h>

// 包含所有子模块
#include "Air780EGDebug.h"
#include "Air780EGCore.h"
#include "Air780EGNetwork.h"
#include "Air780EGGNSS.h"
#include "Air780EGMQTT.h"
#include "Air780EGHTTP.h"

// 版本信息
#define AIR780EG_VERSION_MAJOR 1
#define AIR780EG_VERSION_MINOR 0
#define AIR780EG_VERSION_PATCH 0
#define AIR780EG_VERSION_STRING "1.0.0"

// 配置结构
struct Air780EGConfig {
    // 基础功能
    bool enableGSM = true;           // 启用GSM/GPRS通讯
    bool enableMQTT = true;          // 启用MQTT通讯
    
    // 定位功能
    bool enableGNSS = false;         // 启用GNSS定位
    bool enableFallbackLocation = false; // 启用WiFi/LBS兜底定位
    
    // 兜底定位配置
    unsigned long gnss_timeout = 15000;      // GNSS信号丢失超时时间(ms)
    unsigned long wifi_interval = 120000;    // WiFi定位间隔(ms)
    unsigned long lbs_interval = 60000;      // LBS定位间隔(ms)
    bool prefer_wifi_over_lbs = true;        // 是否优先使用WiFi定位
};

class Air780EG {
private:
    static const char* TAG;
    
    Air780EGCore core;
    Air780EGNetwork network;
    Air780EGGNSS gnss;
    Air780EGMQTT mqtt;
    Air780EGHTTP http;
    
    bool initialized = false;
    unsigned long last_loop_time = 0;
    unsigned long loop_interval = 100; // 主循环间隔
    Air780EGConfig config; // 功能配置
    
public:
    Air780EG();
    ~Air780EG();
    
    // 初始化方法
    bool begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin);
    bool begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin, const Air780EGConfig& config);
    
    // 主循环 - 必须在loop()中调用
    void loop();
    
    // 获取各功能模块的引用
    Air780EGCore& getCore() { return core; }
    Air780EGNetwork& getNetwork() { return network; }
    Air780EGGNSS& getGNSS() { return gnss; }
    Air780EGMQTT& getMQTT() { return mqtt; }
    Air780EGHTTP& getHTTP() { return http; }
    
    // 便捷方法
    bool isReady();
    // void reset();
    void powerOn();
    void powerOff();
    
    // 配置方法
    void setLoopInterval(unsigned long interval_ms);
    unsigned long getLoopInterval() const;
    
    // 功能配置
    void setConfig(const Air780EGConfig& config);
    const Air780EGConfig& getConfig() const;
    
    // 调试配置
    static void setLogLevel(Air780EGLogLevel level);
    static void setLogOutput(Print* stream);
    static void enableTimestamp(bool enable = true);
    
    // 状态查询
    bool isInitialized() const;
    
    // 版本信息
    static String getVersion();
    static void printVersion();
    
    // 综合状态显示
    void printStatus();
};

extern Air780EG air780eg;

#endif // AIR780EG_H
