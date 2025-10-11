#ifndef AIR780EG_CORE_H
#define AIR780EG_CORE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <queue>
#include "Air780EGDebug.h"

class Air780EGURC; // 前向声明

// AT命令结构体
struct ATCommand {
    String command;
    String type;
    String expected_response;
    unsigned long timeout;
    unsigned long timestamp;
    bool is_blocking;
    bool completed;
    String response;
    
    ATCommand(const String& cmd, const String& cmd_type, const String& expected, 
              unsigned long to = 1000, bool blocking = false) 
        : command(cmd), type(cmd_type), expected_response(expected), 
          timeout(to), timestamp(millis()), is_blocking(blocking), 
          completed(false), response("") {}
};

class Air780EGCore {
private:
    static const char* TAG;
    
    HardwareSerial* serial;
    String response_cache;
    unsigned long last_at_time;
    unsigned long at_command_delay = 100; // AT指令间最小间隔
    
    bool initialized = false;
    bool boot_rom = false;
    int power_pin = -1;
    
    // URC管理器
    Air780EGURC* urc_manager = nullptr;
    
    // 命令队列管理
    std::queue<ATCommand> command_queue;
    ATCommand* current_command = nullptr;
    bool echo_enabled = false;
    String accumulated_response = "";
    unsigned long command_start_time = 0;
    
    // 阻塞命令状态管理
    bool is_blocking_command_active = false;
    String blocking_command_type = "";
    unsigned long blocking_command_start = 0;
    static const unsigned long BLOCKING_COMMAND_TIMEOUT = 30000;  // 30秒超时
    void checkBlockingCommandTimeout();
    
    // 内部方法
    void clearSerialBuffer();
    bool isAtReady();
    String readResponseUntilExpected(const String& expected_response,unsigned long timeout);
    String readLine(); // 读取一行数据
    
    // 队列管理方法
    String getCommandType(const String& cmd);
    bool isCompleteResponse(const String& response, const String& cmd_type);
    void addToQueue(const String& cmd, const String& expected, unsigned long timeout = 1000, bool blocking = false);
    void processCommandQueue();
    bool executeCurrentCommand();
    void checkAndDispatchURC(const String& response);
    bool isRealURC(const String& line);
    void dispatchURC(const String& urc);
    
public:
    Air780EGCore();
    ~Air780EGCore();
    
    // 初始化和配置
    bool begin(HardwareSerial* ser, int baudrate, int rx_pin, int tx_pin, int power_pin);
    
    // AT指令交互
    String sendATCommand(const String& cmd, unsigned long timeout = 1000);
    String sendATCommandUntilExpected(const String& cmd, const String& expected_response, unsigned long timeout = 1000);
    bool sendATCommandBool(const String& cmd, unsigned long timeout = 1000);
    String sendATCommandWithResponse(const String& cmd, const String& expected_response, unsigned long timeout = 3000);
    String readResponse(unsigned long timeout);
    
    // 非阻塞AT指令方法
    bool sendATCommandAsync(const String& cmd, const String& expected_response = "OK", unsigned long timeout = 1000);
    bool isCommandCompleted(const String& cmd_type);
    String getCommandResponse(const String& cmd_type);
    void processCommands(); // 在主循环中调用
    
    // 模块控制
    bool isReady();
    void powerOn();
    void powerOff();
    
    // 初始化模块
    bool initModem();

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

    // 检查设备是否重启过 boot.rom
    bool isBootRom() const { return boot_rom; }
    
    // 阻塞命令管理
    bool isBlockingCommandActive() const;
    void setBlockingCommandActive(const String& cmd_type);
    void clearBlockingCommand();
};

#endif // AIR780EG_CORE_H
