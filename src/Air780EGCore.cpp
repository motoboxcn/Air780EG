#include "Air780EGCore.h"

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
    }
    else
    {
        AIR780EG_LOGD(TAG, "Power pin not configured, serial should be initialized externally");
    }
    delay(1000); // 等待模块稳定
    // 清空缓冲区
    while (serial->available())
    {
        serial->read();
    }

    while (!initModem())
    {
        AIR780EG_LOGI(TAG, "Module initModem failed, retry...");
        delay(1000);
    }
    AIR780EG_LOGI(TAG, "Module initModem successfully");

    boot_rom = false;
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
    // 多次尝试AT测试
    while (!isAtReady())
    {
        AIR780EG_LOGI(TAG, "Module not ready, retry...");
        delay(1000);
    }
    AIR780EG_LOGI(TAG, "Module AT ready");
    initialized = true;

    /*
+E_UTRAN Service

+CGEV: ME PDN ACT 1

+NITZ: 2025/07/10,15:54:58+0,0
ATE0
*/
    // 等待设备初始化完成信息，并捕捉设备时间，完成首次时间同步
    while (serial->available())
    {
        String line = readLine();
        if (line.indexOf("+E_UTRAN Service") >= 0)
        {
            AIR780EG_LOGI(TAG, "Device initialized");
            break;
        }
    }

    // 清空串口缓冲区
    clearSerialBuffer();

    // 关闭回显
    if (!sendATCommandBool("ATE0"))
    {
        AIR780EG_LOGE(TAG, "关闭回显失败");
        return false;
    }

    // // 启用网络注册状态主动上报
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
    AIR780EG_LOGI(TAG, "SIM卡 PIN 码就绪");

    // // 检查信号强度 - 移到GPRS附着前检查
    // int csqRetry = 0;
    // do
    // {
    //     int csq = getCSQ();
    //     if (csq >= 2 && csq != 99) // 降低阈值，排除无效值
    //         break;
    //     delay(500);
    //     csqRetry++;
    // } while (csqRetry < 5);
    // if (csqRetry >= 5)
    // {
    //     AIR780EG_LOGW(TAG, "信号较弱，但继续尝试连接");
    //     // 不直接返回失败，允许在弱信号下尝试连接
    // }
    // else
    // {
    //     AIR780EG_LOGI(TAG, "信号强度正常");
    // }

    // 检查GPRS附着状态
    // int cgattRetry = 0;
    // do
    // {
    //     String response = sendATCommandWithResponse("AT+CGATT?", "OK", 5000);
    //     if (response.indexOf("+CGATT: 1") >= 0)
    //         break;
    //     delay(1000); // GPRS附着也需要更多时间
    //     cgattRetry++;
    // } while (cgattRetry < 8); // 增加重试次数
    // if (cgattRetry >= 8)
    // {
    //     AIR780EG_LOGE(TAG, "GPRS not attached");
    //     return false;
    // }
    // AIR780EG_LOGI(TAG, "GPRS附着成功");

    // 等待网络注
    // while (!isNetworkReadyCheck())
    // {
    //     AIR780EG_LOGI(TAG, "等待网络注册...");
    //     delay(2000);
    // }

    // 激活PDP上下文 780eg没有这个
    // String response = sendATCommandWithResponse("AT+MIPCALL?", "OK", 5000);
    // if (response.indexOf("OK") < 0)
    // {
    //     AIR780EG_LOGE(TAG, "激活PDP上下文失败");
    //     return false;
    // }
    // AIR780EG_LOGI(TAG, "激活PDP上下文成功");

    return true;
}

bool Air780EGCore::isNetworkReadyCheck()
{

    // CEREG 4G 注册状态
    // CGREG 2G 注册状态
    String response = sendATCommandWithResponse("AT+CEREG?", "OK", 5000);
    // 支持本地网络注册(1)和漫游网络注册(5)
    // 注意：第一个数字是上报模式，第二个数字是注册状态
    if (response.indexOf("+CEREG: 1,1") >= 0 || response.indexOf("+CEREG: 1,5") >= 0 ||
        response.indexOf("+CEREG: 0,1") >= 0 || response.indexOf("+CEREG: 0,5") >= 0)
    {
        AIR780EG_LOGI(TAG, "网络就绪");
    }
    else
    {
        AIR780EG_LOGW(TAG, "网络未就绪");
        return false;
    }

    // 检查GPRS附着状态
    int cgattRetry = 0;
    do
    {
        String response = sendATCommandWithResponse("AT+CGATT?", "OK", 5000);
        if (response.indexOf("+CGATT: 1") >= 0)
            break;
        delay(1000); // GPRS附着也需要更多时间
        cgattRetry++;
    } while (cgattRetry < 8); // 增加重试次数
    if (cgattRetry >= 8)
    {
        AIR780EG_LOGE(TAG, "GPRS not attached");
        return false;
    }
    AIR780EG_LOGI(TAG, "GPRS附着成功");

    return true;
}

// 等待期望的响应，支持超时机制
bool Air780EGCore::waitExpectedResponse(const String &expected_response, unsigned long timeout)
{
    unsigned long start_time = millis();

    while (millis() - start_time < timeout)
    {
        // 检查串口是否有数据
        if (serial->available())
        {
            String line = readLine();
            if (line.length() > 0 && line.indexOf(expected_response) >= 0)
            {
                return true;
            }
        }

        // 添加小延迟避免CPU占用过高
        delay(10);
    }

    return false;
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
                response.endsWith("CONNECT OK\r\n") ||
                response.endsWith("SUBACK\r\n") ||
                response.endsWith("+MSUB:\r\n"))
                break;
        }
        delay(1);
    }

    response.trim();
    response_cache = response;

    // 优化日志输出，显示为输出模式
    AIR780EG_LOGV(TAG, "< %s", response.c_str());

    // if respons has "boot.rom" shoud be reinit.
    if (response.indexOf("boot.rom") >= 0)
    {
        boot_rom = true;
    }
    return response;
}

String Air780EGCore::readResponseUntilExpected(const String &expected_response, unsigned long timeout)
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
            if (response.endsWith(expected_response) ||
                response.endsWith("ERROR\r\n"))
                break;
        }
        delay(1);
    }

    response.trim();
    response_cache = response;

    // 优化日志输出，显示为输出模式
    AIR780EG_LOGV(TAG, "< %s", response.c_str());
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

    // 优化日志输出，显示为输入模式
    AIR780EG_LOGD(TAG, "> %s", cmd.c_str());

    // 清空接收缓冲区
    // clearSerialBuffer();

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

String Air780EGCore::sendATCommandUntilExpected(const String &cmd, const String &expected_response, unsigned long timeout)
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

    // 优化日志输出，显示为输入模式
    AIR780EG_LOGD(TAG, "> %s", cmd.c_str());

    // 清空接收缓冲区
    // clearSerialBuffer();

    serial->println(cmd);
    last_at_time = millis();

    // 读取响应
    String response = readResponseUntilExpected(expected_response, timeout);

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

    /*
    765	Invalid input value	无效输入值
    766	Unsupported mode	不支持的模式
    767	Operation failed	操作失败
    768	Mux already running	多路复用已经在运行
    769	Unable to get control	不能获得控制
    */
    // 增加一个对结果的解析报错 比如 CME ERROR: 767 标识 操作失败
    if (response.indexOf("CME ERROR:") >= 0)
    {
        int error_code = response.substring(10, 13).toInt();
        switch (error_code)
        {
        case 765:
            AIR780EG_LOGE(TAG, "无效输入值");
            break;
        case 766:
            AIR780EG_LOGE(TAG, "不支持的模式");
            break;
        case 767:
            AIR780EG_LOGE(TAG, "操作失败");
            break;
        }
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
    // 更准确的解析方法
    int start_pos = response.indexOf("+CSQ:") + 6; // 跳过 "+CSQ: "
    int end_pos = response.indexOf(",", start_pos);
    if (start_pos > 5 && end_pos > start_pos)
    {
        int csq = response.substring(start_pos, end_pos).toInt();
        return csq;
    }
    return -1;
}
