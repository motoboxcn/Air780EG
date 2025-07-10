#ifndef AIR780EG_CORE_H
#define AIR780EG_CORE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "Air780EGDebug.h"

class Air780EGURC; // 前向声明

class Air780EGCore {
private:
    static const char* TAG;
    
    HardwareSerial* serial;
    String response_cache;
    unsigned long last_at_time;
    unsigned long at_command_delay = 100; // AT指令间最小间隔
    
    bool initialized = false;
    int power_pin = -1;
    
    // URC管理器
    Air780EGURC* urc_manager = nullptr;
    
    // 内部方法
    void clearSerialBuffer();
    bool initModem();
    bool isAtReady();
    String readResponse(unsigned long timeout);
    String readLine(); // 读取一行数据
    
public:
    Air780EGCore();
    ~Air780EGCore();
    
    // 初始化和配置
    bool begin(HardwareSerial* ser, int baudrate, int rx_pin, int tx_pin, int power_pin);
    
    // 主循环 - 处理URC消息
    void loop();
    
    // AT指令交互
    String sendATCommand(const String& cmd, unsigned long timeout = 1000);
    bool sendATCommandBool(const String& cmd, unsigned long timeout = 1000);
    String sendATCommandWithResponse(const String& cmd, const String& expected_response, unsigned long timeout = 3000);
    
    // 模块控制
    bool isReady();
    void powerOn();
    void powerOff();
    
    // URC管理器访问
    void setURCManager(Air780EGURC* manager);
    Air780EGURC* getURCManager() const;
    
    // 配置方法
    void setATCommandDelay(unsigned long delay_ms);
    unsigned long getATCommandDelay() const;
    
    // 状态查询
    bool isInitialized() const;
    HardwareSerial* getSerial() const;
    bool isNetworkReadyCheck();
    bool waitExpectedResponse(const String &expected_response, unsigned long timeout = 10000);

    // getCSQ
    int getCSQ();
    
    // 调试方法
    void enableEcho(bool enable = true);
    String getLastResponse() const;
};

#endif // AIR780EG_CORE_H
