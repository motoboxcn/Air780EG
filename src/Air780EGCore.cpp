#include "Air780EGCore.h"
#include "Air780EGURC.h"

const char* Air780EGCore::TAG = "Air780EGCore";

Air780EGCore::Air780EGCore() : serial(nullptr), last_at_time(0) {
}

Air780EGCore::~Air780EGCore() {
    // 析构函数
}

bool Air780EGCore::begin(HardwareSerial* ser, int baudrate) {
    return begin(ser, baudrate, -1, -1);
}

bool Air780EGCore::begin(HardwareSerial* ser, int baudrate, int rst_pin, int pwr_pin) {
    if (!ser) {
        AIR780EG_LOGE(TAG, "Serial pointer is null");
        return false;
    }
    
    serial = ser;
    reset_pin = rst_pin;
    power_pin = pwr_pin;
    
    // 配置控制引脚
    if (reset_pin >= 0) {
        pinMode(reset_pin, OUTPUT);
        digitalWrite(reset_pin, HIGH);
        AIR780EG_LOGD(TAG, "Reset pin configured: %d", reset_pin);
    }
    
    if (power_pin >= 0) {
        pinMode(power_pin, OUTPUT);
        digitalWrite(power_pin, HIGH);
        AIR780EG_LOGD(TAG, "Power pin configured: %d", power_pin);
    }
    
    // 初始化串口
    serial->begin(baudrate);
    delay(1000);
    
    AIR780EG_LOGI(TAG, "Initializing Air780EG module...");
    
    // 清空串口缓冲区
    clearSerialBuffer();
    
    // 测试AT指令响应
    for (int i = 0; i < 5; i++) {
        if (sendATCommandBool("AT")) {
            AIR780EG_LOGI(TAG, "Module responded to AT command");
            initialized = true;
            break;
        }
        AIR780EG_LOGW(TAG, "AT command failed, retry %d/5", i + 1);
        delay(1000);
    }
    
    if (!initialized) {
        AIR780EG_LOGE(TAG, "Failed to initialize module");
        return false;
    }
    
    // 关闭回显
    enableEcho(false);
    
    // 启用网络注册URC
    sendATCommandBool("AT+CREG=1");
    sendATCommandBool("AT+CGREG=1");
    
    AIR780EG_LOGI(TAG, "Module initialized successfully");
    return true;
}

void Air780EGCore::loop() {
    if (!serial || !initialized) {
        return;
    }
    
    // 处理接收到的数据，传递给URC管理器
    while (serial->available()) {
        String line = readLine();
        if (line.length() > 0 && urc_manager) {
            urc_manager->processLine(line);
        }
    }
}

String Air780EGCore::readLine() {
    static String buffer = "";
    
    while (serial->available()) {
        char c = serial->read();
        
        if (c == '\r' || c == '\n') {
            if (buffer.length() > 0) {
                String result = buffer;
                buffer = "";
                result.trim();
                return result;
            }
        } else {
            buffer += c;
            
            // 防止缓冲区过大
            if (buffer.length() > 512) {
                AIR780EG_LOGW(TAG, "Line buffer overflow, clearing");
                buffer = "";
            }
        }
    }
    
    return "";
}

void Air780EGCore::clearSerialBuffer() {
    if (!serial) return;
    
    while (serial->available()) {
        serial->read();
    }
    AIR780EG_LOGV(TAG, "Serial buffer cleared");
}

String Air780EGCore::readResponse(unsigned long timeout) {
    if (!serial) return "";
    
    String response = "";
    unsigned long start_time = millis();
    
    while (millis() - start_time < timeout) {
        if (serial->available()) {
            char c = serial->read();
            response += c;
            
            // 检查是否收到完整响应
            if (response.endsWith("OK\r\n") || 
                response.endsWith("ERROR\r\n") || 
                response.endsWith("FAIL\r\n")) {
                break;
            }
        }
        delay(1);
    }
    
    response.trim();
    response_cache = response;
    
    AIR780EG_LOGV(TAG, "Response: %s", response.c_str());
    return response;
}

String Air780EGCore::sendATCommand(const String& cmd, unsigned long timeout) {
    if (!serial || !initialized) {
        AIR780EG_LOGE(TAG, "Module not initialized");
        return "";
    }
    
    // 确保AT指令间有足够间隔
    unsigned long current_time = millis();
    if (current_time - last_at_time < at_command_delay) {
        delay(at_command_delay - (current_time - last_at_time));
    }
    
    AIR780EG_LOGD(TAG, "Sending AT command: %s", cmd.c_str());
    
    // 清空接收缓冲区
    clearSerialBuffer();
    
    // 发送AT指令
    serial->println(cmd);
    last_at_time = millis();
    
    // 读取响应
    String response = readResponse(timeout);
    
    if (response.length() == 0) {
        AIR780EG_LOGW(TAG, "No response for command: %s", cmd.c_str());
    }
    
    return response;
}

bool Air780EGCore::sendATCommandBool(const String& cmd, unsigned long timeout) {
    String response = sendATCommand(cmd, timeout);
    bool success = response.indexOf("OK") >= 0;
    
    if (!success) {
        AIR780EG_LOGW(TAG, "Command failed: %s, Response: %s", cmd.c_str(), response.c_str());
    }
    
    return success;
}

String Air780EGCore::sendATCommandWithResponse(const String& cmd, const String& expected_response, unsigned long timeout) {
    String response = sendATCommand(cmd, timeout);
    
    if (response.indexOf(expected_response) >= 0) {
        AIR780EG_LOGD(TAG, "Expected response found: %s", expected_response.c_str());
    } else {
        AIR780EG_LOGW(TAG, "Expected response not found. Expected: %s, Got: %s", 
                     expected_response.c_str(), response.c_str());
    }
    
    return response;
}

bool Air780EGCore::isReady() {
    if (!initialized) return false;
    return sendATCommandBool("AT");
}

void Air780EGCore::reset() {
    AIR780EG_LOGI(TAG, "Resetting module...");
    
    if (reset_pin >= 0) {
        // 硬件复位
        digitalWrite(reset_pin, LOW);
        delay(100);
        digitalWrite(reset_pin, HIGH);
        delay(2000);
        AIR780EG_LOGI(TAG, "Hardware reset completed");
    } else {
        // 软件复位
        sendATCommand("AT+CFUN=1,1");
        delay(5000);
        AIR780EG_LOGI(TAG, "Software reset completed");
    }
    
    initialized = false;
    
    // 重新初始化
    if (serial) {
        delay(1000);
        for (int i = 0; i < 10; i++) {
            if (sendATCommandBool("AT")) {
                initialized = true;
                AIR780EG_LOGI(TAG, "Module ready after reset");
                break;
            }
            delay(1000);
        }
    }
}

void Air780EGCore::powerOn() {
    if (power_pin >= 0) {
        AIR780EG_LOGI(TAG, "Powering on module...");
        digitalWrite(power_pin, HIGH);
        delay(2000);
    }
}

void Air780EGCore::powerOff() {
    if (power_pin >= 0) {
        AIR780EG_LOGI(TAG, "Powering off module...");
        digitalWrite(power_pin, LOW);
        delay(1000);
    }
}

void Air780EGCore::setURCManager(Air780EGURC* manager) {
    urc_manager = manager;
    AIR780EG_LOGD(TAG, "URC manager set: %p", manager);
}

Air780EGURC* Air780EGCore::getURCManager() const {
    return urc_manager;
}

void Air780EGCore::setATCommandDelay(unsigned long delay_ms) {
    at_command_delay = delay_ms;
    AIR780EG_LOGD(TAG, "AT command delay set to %lu ms", delay_ms);
}

unsigned long Air780EGCore::getATCommandDelay() const {
    return at_command_delay;
}

bool Air780EGCore::isInitialized() const {
    return initialized;
}

HardwareSerial* Air780EGCore::getSerial() const {
    return serial;
}

void Air780EGCore::enableEcho(bool enable) {
    String cmd = enable ? "ATE1" : "ATE0";
    if (sendATCommandBool(cmd)) {
        AIR780EG_LOGD(TAG, "Echo %s", enable ? "enabled" : "disabled");
    }
}

String Air780EGCore::getLastResponse() const {
    return response_cache;
}
