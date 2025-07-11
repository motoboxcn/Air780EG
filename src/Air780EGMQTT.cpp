#include "Air780EGMQTT.h"

const char *Air780EGMQTT::TAG = "MQTT";

Air780EGMQTT::Air780EGMQTT(Air780EGCore *core_instance)
    : core(core_instance), state(MQTT_DISCONNECTED),
      message_callback(nullptr), connection_callback(nullptr)
{
    // 设置默认配置
    config.server = "";
    config.port = 1883;
    config.client_id = "Air780EG_" + String(random(10000, 99999));
    config.keepalive = 60;
    config.clean_session = true;
    config.use_ssl = false;
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

    // 设置MQTT消息格式为文本模式（0=文本模式，1=HEX模式）
    // 由于我们使用JSON文本格式，应该使用文本模式
    String response = core->sendATCommandWithResponse("AT+MQTTMODE=1", "OK", 3000);
    if (response.indexOf("OK") < 0)
    {
        AIR780EG_LOGW(TAG, "Failed to set MQTT text mode");
    }
    AIR780EG_LOGI(TAG, "MQTT module initialized");
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

    delay(100);

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
    AIR780EG_LOGD(TAG, "Handling URC");

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
        // 收到订阅消息 - 简化处理
        if (message_callback)
        {
            message_callback("test/topic", "test payload");
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
String Air780EGMQTT::toHexString(const String& input) {
    String hex = "";
    for (size_t i = 0; i < input.length(); ++i) {
        char buf[3];
        sprintf(buf, "%02x", (unsigned char)input[i]);
        hex += buf;
    }
    return hex;
}
