#include "Air780EGCore.h"
#include "Air780EGURC.h"

const char *Air780EGCore::TAG = "Air780EGCore";

Air780EGCore::Air780EGCore() : serial(nullptr), last_at_time(0)
{
}

Air780EGCore::~Air780EGCore()
{
    // 析构函数
}

bool Air780EGCore::begin(HardwareSerial *ser, int baudrate, int rx_pin, int tx_pin, int pwr_pin)
{
    if (!ser)
    {
        AIR780EG_LOGE(TAG, "Serial pointer is null");
        return false;
    }

    serial = ser;
    power_pin = pwr_pin;
    AIR780EG_LOGD(TAG, "Power pin: %d", power_pin);

    if (power_pin >= 0)
    {
        pinMode(power_pin, OUTPUT);

        digitalWrite(power_pin, LOW);
        delay(100);
        digitalWrite(power_pin, HIGH);
        delay(2000); // 等待模块启动
        
        AIR780EG_LOGD(TAG, "Power pin configured: %d", power_pin);
        // 只有当power_pin有效时才初始化串口（表示由库管理）
        serial->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
    }else{
        AIR780EG_LOGD(TAG, "Power pin not configured, serial should be initialized externally");
        // power_pin为-1表示外部已经管理EN引脚和串口初始化，不需要返回false
    }
    delay(1000); // 等待模块稳定
    // 清空缓冲区
    while (serial->available())
    {
        serial->read();
    }

    // 多次尝试AT测试
    while (!isAtReady())
    {
        AIR780EG_LOGI(TAG, "Module not ready, retry...");
        delay(1000);
    }
    AIR780EG_LOGI(TAG, "Module AT ready");
    initialized = true;

    // 清空串口缓冲区
    clearSerialBuffer();

    while (!initModem())
    {
        AIR780EG_LOGI(TAG, "Module initModem failed, retry...");
        delay(1000);
    }
    AIR780EG_LOGI(TAG, "Module initModem successfully");
    return true;

}

bool Air780EGCore::isAtReady()
{
    // 使用原始的命令确认是否有返回
    serial->println("AT");
    String response = readResponse(1000);
    // 处理response boot.rom 还在初始化时候等待
    if (response.indexOf("boot.rom") >= 0)
    {
        AIR780EG_LOGI(TAG, "boot.rom还在初始化,等待...");
        delay(1000);
        return false;
    }
    return response.indexOf("OK") >= 0;
}


bool Air780EGCore::initModem()
{
    delay(2000);

    // 关闭回显
    if (!sendATCommandBool("ATE0"))
    {
        AIR780EG_LOGE(TAG, "关闭回显失败");
        return false;
    }

    // 启用网络注册状态主动上报
    if (!sendATCommandBool("AT+CEREG=1"))
    {
        AIR780EG_LOGE(TAG, "设置CEREG失败");
        return false;
    }

    // 检查 PIN 码状态
    int pinRetry = 0;
    const int maxPinRetry = 5;
    String pinStatus;
    do
    {
        pinStatus = sendATCommandWithResponse("AT+CPIN?", "READY");
        if (pinStatus.indexOf("READY") >= 0)
            break;
        delay(500);
        pinRetry++;
    } while (pinRetry < maxPinRetry);
    if (pinStatus.indexOf("READY") < 0)
    {
        AIR780EG_LOGE(TAG, "SIM卡 PIN 码未就绪");
        return false;
    }

    // 检查信号强度
    // int csqRetry = 0;
    // int csq = -1;
    // do
    // {
    //     csq = getCSQ();
    //     if (csq >= 5)
    //         break;
    //     delay(1000);
    //     csqRetry++;
    // } while (csqRetry < 5);
    // if (csq < 5)
    // {
    //     AIR780EG_LOGE(TAG, "信号太弱，无法注册网络");
    //     return false;
    // }

    // 激活PDP上下文
    if (!sendATCommandWithResponse("AT+MIPCALL?", "OK", 5000))
    {
        AIR780EG_LOGE(TAG, "激活PDP上下文失败");
        return false;
    }

    return true;
}


void Air780EGCore::loop()
{
    if (!serial || !initialized)
    {
        return;
    }

    // 处理接收到的数据，仅将URC特征的行传递给URC管理器
    while (serial->available())
    {
        String line = readLine();
        if (line.length() > 0 && urc_manager)
        {
            // 只处理具有URC特征的行
            if (line.startsWith("+") ||
                line.startsWith("RING") ||
                line.startsWith("NO CARRIER") ||
                line.startsWith("CMTI") ||
                line.startsWith("CLIP") ||
                line.startsWith("CGEV") ||
                line.startsWith("CMS ERROR") ||
                line.startsWith("CME ERROR"))
            {
                urc_manager->processLine(line);
            }
            // 其他普通AT响应行不处理
        }
    }
}

String Air780EGCore::readLine()
{
    static String buffer = "";

    while (serial->available())
    {
        char c = serial->read();

        if (c == '\r' || c == '\n')
        {
            if (buffer.length() > 0)
            {
                String result = buffer;
                buffer = "";
                result.trim();
                return result;
            }
        }
        else
        {
            buffer += c;

            // 防止缓冲区过大
            if (buffer.length() > 512)
            {
                AIR780EG_LOGW(TAG, "Line buffer overflow, clearing");
                buffer = "";
            }
        }
    }

    return "";
}

void Air780EGCore::clearSerialBuffer()
{
    if (!serial)
        return;

    while (serial->available())
    {
        serial->read();
    }
    AIR780EG_LOGV(TAG, "Serial buffer cleared");
}

String Air780EGCore::readResponse(unsigned long timeout)
{
    if (!serial)
        return "";

    String response = "";
    unsigned long start_time = millis();

    while (millis() - start_time < timeout)
    {
        if (serial->available())
        {
            char c = serial->read();
            response += c;

            // 检查是否收到完整响应
            if (response.endsWith("OK\r\n") ||
                response.endsWith("ERROR\r\n") ||
                response.endsWith("FAIL\r\n"))
            {
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

String Air780EGCore::sendATCommand(const String &cmd, unsigned long timeout)
{
    if (!serial || !initialized)
    {
        AIR780EG_LOGE(TAG, "Module not initialized");
        return "";
    }

    // 确保AT指令间有足够间隔
    unsigned long current_time = millis();
    if (current_time - last_at_time < at_command_delay)
    {
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

    if (response.length() == 0)
    {
        AIR780EG_LOGW(TAG, "No response for command: %s", cmd.c_str());
    }

    return response;
}

bool Air780EGCore::sendATCommandBool(const String &cmd, unsigned long timeout)
{
    String response = sendATCommand(cmd, timeout);
    bool success = response.indexOf("OK") >= 0;

    if (!success)
    {
        AIR780EG_LOGW(TAG, "Command failed: %s, Response: %s", cmd.c_str(), response.c_str());
    }

    return success;
}

String Air780EGCore::sendATCommandWithResponse(const String &cmd, const String &expected_response, unsigned long timeout)
{
    String response = sendATCommand(cmd, timeout);

    if (response.indexOf(expected_response) >= 0)
    {
        AIR780EG_LOGD(TAG, "Expected response found: %s", expected_response.c_str());
    }
    else
    {
        AIR780EG_LOGW(TAG, "Expected response not found. Expected: %s, Got: %s",
                      expected_response.c_str(), response.c_str());
    }

    return response;
}

bool Air780EGCore::isReady()
{
    if (!initialized)
        return false;
    return sendATCommandBool("AT");
}

void Air780EGCore::powerOn()
{
    if (power_pin >= 0)
    {
        AIR780EG_LOGI(TAG, "Powering on module...");
        digitalWrite(power_pin, HIGH);
        delay(2000);
    }
}

void Air780EGCore::powerOff()
{
    if (power_pin >= 0)
    {
        AIR780EG_LOGI(TAG, "Powering off module...");
        digitalWrite(power_pin, LOW);
        delay(1000);
    }
}

void Air780EGCore::setURCManager(Air780EGURC *manager)
{
    urc_manager = manager;
    AIR780EG_LOGD(TAG, "URC manager set: %p", manager);
}

Air780EGURC *Air780EGCore::getURCManager() const
{
    return urc_manager;
}

void Air780EGCore::setATCommandDelay(unsigned long delay_ms)
{
    at_command_delay = delay_ms;
    AIR780EG_LOGD(TAG, "AT command delay set to %lu ms", delay_ms);
}

unsigned long Air780EGCore::getATCommandDelay() const
{
    return at_command_delay;
}

bool Air780EGCore::isInitialized() const
{
    return initialized;
}

HardwareSerial *Air780EGCore::getSerial() const
{
    return serial;
}

void Air780EGCore::enableEcho(bool enable)
{
    String cmd = enable ? "ATE1" : "ATE0";
    if (sendATCommandBool(cmd))
    {
        AIR780EG_LOGD(TAG, "Echo %s", enable ? "enabled" : "disabled");
    }
}

String Air780EGCore::getLastResponse() const
{
    return response_cache;
}

int Air780EGCore::getCSQ()
{
    String response = sendATCommandWithResponse("AT+CSQ", "OK");
    if (response.indexOf("+CSQ:") >= 0)
    {
        return response.substring(5, 7).toInt();
    }
    return -1;
}