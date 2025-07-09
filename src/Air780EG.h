#ifndef AIR780EG_H
#define AIR780EG_H

#include <Arduino.h>
#include <HardwareSerial.h>

// 包含所有子模块
#include "Air780EGDebug.h"
#include "Air780EGCore.h"
#include "Air780EGNetwork.h"
#include "Air780EGGNSS.h"
#include "Air780EGURC.h"
#include "Air780EGMQTT.h"

// 版本信息
#define AIR780EG_VERSION_MAJOR 1
#define AIR780EG_VERSION_MINOR 0
#define AIR780EG_VERSION_PATCH 0
#define AIR780EG_VERSION_STRING "1.0.0"

class Air780EG {
private:
    static const char* TAG;
    
    Air780EGCore core;
    Air780EGNetwork network;
    Air780EGGNSS gnss;
    Air780EGURC urc_manager;
    Air780EGMQTT mqtt;
    
    bool initialized = false;
    unsigned long last_loop_time = 0;
    unsigned long loop_interval = 100; // 主循环间隔
    
    // 初始化URC处理器
    void setupURCHandlers();
    
public:
    Air780EG();
    ~Air780EG();
    
    // 初始化方法
    bool begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin);
    
    // 主循环 - 必须在loop()中调用
    void loop();
    
    // 获取各功能模块的引用
    Air780EGCore& getCore() { return core; }
    Air780EGNetwork& getNetwork() { return network; }
    Air780EGGNSS& getGNSS() { return gnss; }
    Air780EGURC& getURCManager() { return urc_manager; }
    Air780EGMQTT& getMQTT() { return mqtt; }
    
    // 便捷方法
    bool isReady();
    // void reset();
    void powerOn();
    void powerOff();
    
    // 配置方法
    void setLoopInterval(unsigned long interval_ms);
    unsigned long getLoopInterval() const;
    
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
