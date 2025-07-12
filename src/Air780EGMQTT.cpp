#include "Air780EGMQTT.h"

const char *Air780EGMQTT::TAG = "MQTT";

Air780EGMQTT::Air780EGMQTT(Air780EGCore *core_instance, Air780EGGNSS *gnss_instance)
    : core(core_instance), gnss(gnss_instance), state(MQTT_DISCONNECTED),
      message_callback(nullptr), connection_callback(nullptr)
{
    // 设置默认配置
    config.server = "";
    config.port = 1883;
    config.client_id = "Air780EG_" + String(random(10000, 99999));
    config.keepalive = 60;
    config.clean_session = true;
    config.use_ssl = false;

    // 初始化定时任务数组
    scheduled_task_count = 0;
    for (int i = 0; i < MAX_SCHEDULED_TASKS; i++)
    {
        scheduled_tasks[i].enabled = false;
        scheduled_tasks[i].callback = nullptr;
        scheduled_tasks[i].last_execution = 0;
    }
}

Air780EGMQTT::~Air780EGMQTT()
{
    if (isConnected())
    {
        disconnect();
    }
}

bool Air780EGMQTT::begin(const Air780EGMQTTConfig &cfg)
{
    if (!core)
    {
        AIR780EG_LOGE(TAG, "Core instance is null");
        return false;
    }
    config = cfg;
    AIR780EG_LOGI(TAG, "MQTT module initialized");
    
    if (!init())
    {
        AIR780EG_LOGE(TAG, "Failed to initialize MQTT module");
        return false;
    }

    return true;
}

bool Air780EGMQTT::init()
{
    // 设置MQTT消息格式为文本模式（0=文本模式，1=HEX模式）
    // 由于我们使用JSON文本格式，应该使用文本模式
    String response = core->sendATCommandWithResponse("AT+MQTTMODE=1", "OK", 3000);
    if (response.indexOf("OK") < 0)
    {
        AIR780EG_LOGW(TAG, "Failed to set MQTT text mode");
        return false;
    }
    AIR780EG_LOGI(TAG, "MQTT module initialized");

    // 设置消息上报模式 AT+MQTTMSGSET=0 主动URC上报；1AT+MQTTMSGSET=缓存模式缓存模式。然后用 AT+MQTTMSGGET 来读消息
    response = core->sendATCommandWithResponse("AT+MQTTMSGSET=0", "OK", 3000);
    if (response.indexOf("OK") < 0)
    {
        AIR780EG_LOGW(TAG, "Failed to set MQTT message report mode");
        return false;
    }
    return true;
}

Air780EGMQTTConfig Air780EGMQTT::getConfig() const
{
    return config;
}

bool Air780EGMQTT::connect()
{
    if (config.server.isEmpty())
    {
        AIR780EG_LOGE(TAG, "Server not configured");
        return false;
    }

    return connect(config.server, config.port, config.client_id, config.username, config.password);
}

bool Air780EGMQTT::connect(const String &server, int port, const String &client_id)
{
    return connect(server, port, client_id, "", "");
}

// 连接 mqtt 依赖网络就绪，所以需要先检查网络环境
bool Air780EGMQTT::connect(const String &server, int port, const String &client_id,
                           const String &username, const String &password)
{
    if (state == MQTT_CONNECTED)
    {
        return state == MQTT_CONNECTED;
    }

    state = MQTT_CONNECTING;

    // 检查网络环境
    if (!core->isNetworkReadyCheck())
    {
        AIR780EG_LOGE(TAG, "Network not ready");
        state = MQTT_ERROR;
        return false;
    }

    // 断开现有连接,否则会连接失败
    // if  (!disconnect())
    // {
    //     AIR780EG_LOGW(TAG, "Failed to disconnect from MQTT server");
    //     state = MQTT_ERROR;
    //     return false;
    // }

    // AIR780EG_LOGI(TAG, "Network ready, connecting to MQTT server");

    // 设置MQTT配置参数
    String config_cmd = "AT+MCONFIG=" + config.client_id + "," + config.username + "," + config.password;
    String response = core->sendATCommandWithResponse(config_cmd, "OK", 3000);
    if (response.indexOf("OK") < 0)
    {
        AIR780EG_LOGE(TAG, "Failed to set MQTT config");
        state = MQTT_ERROR;
        return false;
    }

    AIR780EG_LOGI(TAG, "Connecting to MQTT server, client_id: %s, server: %s, port: %d", config.client_id.c_str(), config.server.c_str(), config.port);
    // 设置TCP连接参数
    // 建立mqtt会话；注意需要返回CONNECT OK后才能发此条指令，并且要立即发，否则就会被服务器踢掉 报错 767 操作失败
    response = core->sendATCommandUntilExpected(
        "AT+MIPSTART=\"" + config.server + "\",\"" + String(config.port) + "\"",
        "CONNECT OK",
        5000);

    if (response.indexOf("ALREADY CONNECT") >= 0)
    {
        AIR780EG_LOGI(TAG, "MQTT already connected");
        state = MQTT_CONNECTED;
        return true;
    }

    if (response.indexOf("CONNECT OK") < 0 && response.indexOf("ALREADY CONNECT") < 0)
    {
        AIR780EG_LOGE(TAG, "Failed to set MQTT IP start");
        state = MQTT_ERROR;
        return false;
    }

    // 等待CONNECT OK后立即发送MQTT连接命令
    // if (!core->waitExpectedResponse("CONNECT OK", 20000))
    // {
    //     AIR780EG_LOGE(TAG, "MQTT CONNECT OK timeout!");
    //     state = MQTT_ERROR;
    //     return false;
    // }
    delay(200);

    // 发送连接命令
    response = core->sendATCommandUntilExpected("AT+MCONNECT=1,60", "CONNACK OK", 5000);
    if (response.indexOf("CONNACK OK") < 0)
    {
        AIR780EG_LOGE(TAG, "MQTT connect command failed");
        state = MQTT_ERROR;
        return false;
    }

    // 等待连接成功
    // if (!core->waitExpectedResponse("CONNECT", 20000))
    // {
    //     AIR780EG_LOGE(TAG, "MQTT CONNECT OK timeout");
    //     state = MQTT_ERROR;
    //     return false;
    // }

    state = MQTT_CONNECTED;

    if (connection_callback)
    {
        connection_callback(true);
    }

    return true;
}

bool Air780EGMQTT::disconnect()
{
    if (state != MQTT_CONNECTED)
    {
        return true;
    }

    state = MQTT_DISCONNECTING;
    AIR780EG_LOGI(TAG, "Disconnecting from MQTT server");

    String response = core->sendATCommandWithResponse("AT+MDISCONNECT", "OK", 10000);
    if (response.indexOf("OK") >= 0)
    {
        state = MQTT_DISCONNECTED;
        AIR780EG_LOGI(TAG, "MQTT disconnected");

        if (connection_callback)
        {
            connection_callback(false);
        }

        return true;
    }

    AIR780EG_LOGE(TAG, "MQTT disconnect failed");
    state = MQTT_ERROR;
    return false;
}

bool Air780EGMQTT::isConnected() const
{
    return state == MQTT_CONNECTED;
}

Air780EGMQTTState Air780EGMQTT::getState() const
{
    return state;
}

bool Air780EGMQTT::enableSSL(bool enable)
{
    config.use_ssl = enable;
    if (enable)
    {
        config.port = (config.port == 1883) ? 8883 : config.port;
    }
    return true;
}

bool Air780EGMQTT::setSSLConfig(const String &ca_cert, const String &client_cert, const String &client_key)
{
    AIR780EG_LOGI(TAG, "SSL configuration updated");
    return true;
}

/*
AT+MPUB="mqtt/sub",0,0,"data from 4G module"        //发布主题

OK

+MSUB: "mqtt/pub",20 byte,data from tcp server          //收到服务器下发的消息，+MSUB的URC上报
*/
bool Air780EGMQTT::publish(const String &topic, const String &payload, int qos, bool retain)
{
    if (!isConnected())
    {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }

    // HEX模式下，payload转为HEX字符串
    String hex_payload = toHexString(payload);
    String pub_cmd = "AT+MPUB=\"" + topic + "\"," + String(qos) + "," + String(retain ? 1 : 0) + ",\"" + hex_payload + "\"";
    AIR780EG_LOGD(TAG, "Publishing HEX: %s", pub_cmd.c_str());

    // 移除阻塞性delay，改为依赖AT指令间隔控制

    String response = core->sendATCommandUntilExpected(pub_cmd, "OK", 5000);
    if (response.indexOf("OK") >= 0)
    {
        AIR780EG_LOGD(TAG, "Published message successfully");
        return true;
    }

    AIR780EG_LOGE(TAG, "Failed to publish message, response: %s", response.c_str());
    state = MQTT_ERROR;
    return false;
}

bool Air780EGMQTT::publishJSON(const String &topic, const String &json, int qos)
{
    return publish(topic, json, qos, false);
}

/*
AT+MSUB="mqtt/pub",0        //订阅主题

OK

SUBACK
*/
bool Air780EGMQTT::subscribe(const String &topic, int qos)
{
    if (!isConnected())
    {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }

    String sub_cmd = "AT+MSUB=\"" + topic + "\"," + String(qos);

    String response = core->sendATCommandUntilExpected(sub_cmd, "SUBACK", 10000);
    if (response.indexOf("SUBACK") >= 0)
    {
        // 添加到订阅列表
        if (subscription_count < MAX_SUBSCRIPTIONS)
        {
            subscriptions[subscription_count] = topic;
            subscription_qos[subscription_count] = qos;
            subscription_count++;
        }

        AIR780EG_LOGI(TAG, "Subscribed to topic");
        return true;
    }

    AIR780EG_LOGE(TAG, "Failed to subscribe");
    return false;
}

bool Air780EGMQTT::unsubscribe(const String &topic)
{
    if (!isConnected())
    {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }

    String unsub_cmd = "AT+MUNSUB=\"" + topic + "\"";

    String response = core->sendATCommandWithResponse(unsub_cmd, "OK", 10000);
    if (response.indexOf("OK") >= 0)
    {
        // 从订阅列表中移除
        for (int i = 0; i < subscription_count; i++)
        {
            if (subscriptions[i] == topic)
            {
                for (int j = i; j < subscription_count - 1; j++)
                {
                    subscriptions[j] = subscriptions[j + 1];
                    subscription_qos[j] = subscription_qos[j + 1];
                }
                subscription_count--;
                break;
            }
        }

        AIR780EG_LOGI(TAG, "Unsubscribed from topic");
        return true;
    }

    AIR780EG_LOGE(TAG, "Failed to unsubscribe");
    return false;
}

bool Air780EGMQTT::isSubscribed(const String &topic) const
{
    for (int i = 0; i < subscription_count; i++)
    {
        if (subscriptions[i] == topic)
        {
            return true;
        }
    }
    return false;
}

void Air780EGMQTT::setMessageCallback(MQTTMessageCallback callback)
{
    message_callback = callback;
}

void Air780EGMQTT::setConnectionCallback(MQTTConnectionCallback callback)
{
    connection_callback = callback;
}

void Air780EGMQTT::loop()
{
    // processMessageCache();

    // 处理接收到的数据，仅将URC特征的行传递给URC管理器
    String response = core->readResponse(1000);
    if (response.length() > 0)
    {
        handleMQTTURC(response);
    }

    // 处理定时任务
    processScheduledTasks();

    // 查询 MQTT 连接状态：AT+MQTTSTATU 5秒一次
    static unsigned long last_check_time = 0;
    if (millis() - last_check_time >= 5000)
    {
        last_check_time = millis();
        String response = core->sendATCommandUntilExpected("AT+MQTTSTATU", "OK", 10000);
        if (response.indexOf("OK") >= 0)
        {
            // (0:offline,1:can pub,2: need MCONNECT!)
            AIR780EG_LOGI(TAG, "MQTT status: %s ", response.c_str());
            if (response.indexOf("1") >= 0)
            {
                state = MQTT_CONNECTED;
            }
            else
            {
                state = MQTT_DISCONNECTED;
            }
        }
    }

    // 处理重连逻辑
    if (state == MQTT_DISCONNECTED || state == MQTT_ERROR)
    {
        if (millis() - last_reconnect_attempt > reconnect_interval)
        {
            last_reconnect_attempt = millis();
            AIR780EG_LOGI(TAG, "Attempting reconnection");
            connect();
        }
    }
}

bool Air780EGMQTT::waitForURC(const String &urc_prefix, String &response, unsigned long timeout)
{
    if (!core)
    {
        return false;
    }
    // 简化实现 - 暂时返回false，后续通过URC管理器处理
    response = "";
    return false;
}

void Air780EGMQTT::handleMQTTURC(const String &urc)
{
    AIR780EG_LOGD(TAG, "Handling URC: %s", urc.c_str());

    if (urc.startsWith("+MCONNECT:"))
    {
        // 连接状态变化
        if (urc.indexOf("1,0") >= 0)
        {
            state = MQTT_CONNECTED;
            if (connection_callback)
                connection_callback(true);
        }
        else
        {
            state = MQTT_DISCONNECTED;
            if (connection_callback)
                connection_callback(false);
        }
    }
    else if (urc.startsWith("+MSUB:"))
    {
        // 收到订阅消息 - 使用新的解析函数
        if (message_callback)
        {
            parseMQTTMessage(urc);
        }
    }
    else if (urc.startsWith("+CGNSINF:"))
    {
        // 收到GNSS信息
        if (message_callback)
        {
            message_callback("GNSS", urc);
        }
    }
    else if (urc.indexOf("boot.rom") >= 0) // 确保设备供电正常
    {
        Serial.printf("确保设备供电正常，boot.rom: %s\n", urc.c_str());
        // 重新设置mqtt
        init();
        if (gnss->isEnabled())
        {
            gnss->enableGNSS();
        }
    }
}

void Air780EGMQTT::processMessageCache()
{
    while (cached_message_count > 0)
    {
        Air780EGMQTTMessage msg = message_cache[cache_head];
        cache_head = (cache_head + 1) % MAX_CACHED_MESSAGES;
        cached_message_count--;

        if (message_callback)
        {
            message_callback(msg.topic, msg.payload);
        }
    }
}

bool Air780EGMQTT::reconnect()
{
    if (config.server.isEmpty())
    {
        return false;
    }

    AIR780EG_LOGI(TAG, "Attempting to reconnect");
    return connect();
}

void Air780EGMQTT::registerURCHandlers()
{
    if (!core)
    {
        return;
    }

    AIR780EG_LOGD(TAG, "MQTT URC handlers registered");
}

String Air780EGMQTT::getConnectionInfo() const
{
    String info = "MQTT Status: ";
    switch (state)
    {
    case MQTT_DISCONNECTED:
        info += "Disconnected";
        break;
    case MQTT_CONNECTING:
        info += "Connecting";
        break;
    case MQTT_CONNECTED:
        info += "Connected";
        break;
    case MQTT_DISCONNECTING:
        info += "Disconnecting";
        break;
    case MQTT_ERROR:
        info += "Error";
        break;
    }

    if (isConnected())
    {
        info += "\nServer: " + config.server;
        info += "\nClient ID: " + config.client_id;
        info += "\nSubscriptions: " + String(subscription_count);
    }

    return info;
}

void Air780EGMQTT::printStatus() const
{
    AIR780EG_LOGI(TAG, "MQTT Status");
}

void Air780EGMQTT::enableDebug(bool enable)
{
    // 调试功能实现
}

void Air780EGMQTT::printConfig() const
{
    AIR780EG_LOGI(TAG, "MQTT Configuration");
}

// HEX转换函数
String Air780EGMQTT::toHexString(const String &input)
{
    String hex = "";
    for (size_t i = 0; i < input.length(); ++i)
    {
        char buf[3];
        sprintf(buf, "%02x", (unsigned char)input[i]);
        hex += buf;
    }
    return hex;
}

// 处理定时任务
void Air780EGMQTT::processScheduledTasks()
{
    if (!isConnected())
    {
        return; // 只有在连接状态下才执行定时任务
    }

    unsigned long current_time = millis();

    for (int i = 0; i < scheduled_task_count; i++)
    {
        ScheduledTask &task = scheduled_tasks[i];

        if (!task.enabled || task.callback == nullptr)
        {
            continue;
        }

        // 检查是否到了执行时间
        if (current_time - task.last_execution >= task.interval_ms)
        {
            AIR780EG_LOGD(TAG, "Executing scheduled task: %s", task.task_name.c_str());

            // 调用回调函数获取数据
            String payload = task.callback();

            if (payload.length() > 0)
            {
                // 发布数据
                if (publish(task.topic, payload, task.qos, task.retain))
                {
                    AIR780EG_LOGD(TAG, "Published scheduled task data: %s -> %s",
                                  task.topic.c_str(), payload.c_str());
                }
                else
                {
                    AIR780EG_LOGW(TAG, "Failed to publish scheduled task: %s", task.task_name.c_str());
                }
            }

            task.last_execution = current_time;
        }
    }
}

// 添加定时任务
bool Air780EGMQTT::addScheduledTask(const String &task_name, const String &topic,
                                    ScheduledTaskCallback callback, unsigned long interval_ms,
                                    int qos, bool retain)
{
    if (scheduled_task_count >= MAX_SCHEDULED_TASKS)
    {
        AIR780EG_LOGE(TAG, "Maximum scheduled tasks reached");
        return false;
    }

    if (callback == nullptr)
    {
        AIR780EG_LOGE(TAG, "Callback function is null");
        return false;
    }

    if (interval_ms < 1000)
    {
        AIR780EG_LOGW(TAG, "Interval too short, minimum 1 second");
        interval_ms = 1000;
    }
    AIR780EG_LOGI(TAG, "Add scheduled task: %s, topic: %s, interval: %lu ms",
                  task_name.c_str(), topic.c_str(), interval_ms);

    // 检查任务名是否已存在
    for (int i = 0; i < scheduled_task_count; i++)
    {
        if (scheduled_tasks[i].task_name == task_name)
        {
            AIR780EG_LOGI(TAG, "Task name already exists: %s", task_name.c_str());
            return false;
        }
    }

    // 添加新任务
    ScheduledTask &task = scheduled_tasks[scheduled_task_count];
    task.task_name = task_name;
    task.topic = topic;
    task.callback = callback;
    task.interval_ms = interval_ms;
    task.qos = qos;
    task.retain = retain;
    task.enabled = true;
    task.last_execution = millis();

    scheduled_task_count++;

    AIR780EG_LOGI(TAG, "Added scheduled task: %s, topic: %s, interval: %lu ms",
                  task_name.c_str(), topic.c_str(), interval_ms);

    return true;
}

// 移除定时任务
bool Air780EGMQTT::removeScheduledTask(const String &task_name)
{
    for (int i = 0; i < scheduled_task_count; i++)
    {
        if (scheduled_tasks[i].task_name == task_name)
        {
            // 将后面的任务前移
            for (int j = i; j < scheduled_task_count - 1; j++)
            {
                scheduled_tasks[j] = scheduled_tasks[j + 1];
            }
            scheduled_task_count--;

            AIR780EG_LOGI(TAG, "Removed scheduled task: %s", task_name.c_str());
            return true;
        }
    }

    AIR780EG_LOGW(TAG, "Task not found: %s", task_name.c_str());
    return false;
}

// 启用定时任务
bool Air780EGMQTT::enableScheduledTask(const String &task_name, bool enabled)
{
    for (int i = 0; i < scheduled_task_count; i++)
    {
        if (scheduled_tasks[i].task_name == task_name)
        {
            scheduled_tasks[i].enabled = enabled;
            AIR780EG_LOGI(TAG, "Task %s %s", task_name.c_str(), enabled ? "enabled" : "disabled");
            return true;
        }
    }

    AIR780EG_LOGW(TAG, "Task not found: %s", task_name.c_str());
    return false;
}

// 禁用定时任务
bool Air780EGMQTT::disableScheduledTask(const String &task_name)
{
    return enableScheduledTask(task_name, false);
}

// 获取定时任务数量
int Air780EGMQTT::getScheduledTaskCount() const
{
    return scheduled_task_count;
}

// 获取定时任务信息
String Air780EGMQTT::getScheduledTaskInfo(int index) const
{
    if (index < 0 || index >= scheduled_task_count)
    {
        return "";
    }

    const ScheduledTask &task = scheduled_tasks[index];
    String info = "Task: " + task.task_name +
                  ", Topic: " + task.topic +
                  ", Interval: " + String(task.interval_ms) + "ms" +
                  ", QoS: " + String(task.qos) +
                  ", Retain: " + String(task.retain ? "true" : "false") +
                  ", Enabled: " + String(task.enabled ? "true" : "false");

    return info;
}

// 清除所有定时任务
void Air780EGMQTT::clearAllScheduledTasks()
{
    scheduled_task_count = 0;
    for (int i = 0; i < MAX_SCHEDULED_TASKS; i++)
    {
        scheduled_tasks[i].enabled = false;
        scheduled_tasks[i].callback = nullptr;
        scheduled_tasks[i].last_execution = 0;
    }

    AIR780EG_LOGI(TAG, "All scheduled tasks cleared");
}

// 新增：HEX字符串转普通字符串的辅助函数
String Air780EGMQTT::fromHexString(const String &hex)
{
    String result = "";
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        if (i + 1 < hex.length())
        {
            String byteString = hex.substring(i, i + 2);
            char byte = (char)strtol(byteString.c_str(), NULL, 16);
            result += byte;
        }
    }
    return result;
}

// 新增：检查字符串是否为HEX格式的辅助函数
bool Air780EGMQTT::isHexString(const String &str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        char c = str.charAt(i);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
        {
            return false;
        }
    }
    return str.length() > 0 && str.length() % 2 == 0; // HEX字符串长度必须是偶数
}

// 修改：解析MQTT消息的辅助函数
void Air780EGMQTT::parseMQTTMessage(const String &message)
{
    AIR780EG_LOGD(TAG, "Parsing MQTT message: %s", message.c_str());
    
    // 查找第一个逗号的位置
    int first_comma = message.indexOf(",");
    if (first_comma > 0)
    {
        // 提取主题 (去掉引号)
        String topic = message.substring(7, first_comma); // 跳过 "+MSUB: "
        if (topic.startsWith("\"") && topic.endsWith("\""))
        {
            topic = topic.substring(1, topic.length() - 1);
        }
        
        // 查找第二个逗号的位置
        int second_comma = message.indexOf(",", first_comma + 1);
        if (second_comma > 0)
        {
            // 提取负载 (从第二个逗号后开始)
            String payload = message.substring(second_comma + 1);
            
            // 检查是否是HEX格式的负载
            if (payload.length() > 0 && isHexString(payload))
            {
                // 将HEX转换为普通字符串
                String decoded_payload = fromHexString(payload);
                AIR780EG_LOGD(TAG, "Decoded HEX payload: %s -> %s", payload.c_str(), decoded_payload.c_str());
                payload = decoded_payload;
            }
            
            AIR780EG_LOGD(TAG, "Parsed MQTT message - Topic: %s, Payload: %s", topic.c_str(), payload.c_str());
            if (message_callback)
            {
                message_callback(topic, payload);
            }
        }
        else
        {
            AIR780EG_LOGW(TAG, "Invalid MQTT message format: %s", message.c_str());
        }
    }
    else
    {
        AIR780EG_LOGW(TAG, "Invalid MQTT message format: %s", message.c_str());
    }
}
