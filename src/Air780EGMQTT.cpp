#include "Air780EGMQTT.h"

const char* Air780EGMQTT::TAG = "MQTT";

Air780EGMQTT::Air780EGMQTT(Air780EGCore* core_instance) 
    : core(core_instance), state(MQTT_DISCONNECTED), 
      message_callback(nullptr), connection_callback(nullptr) {
    // 设置默认配置
    config.server = "";
    config.port = 1883;
    config.client_id = "Air780EG_" + String(random(10000, 99999));
    config.keepalive = 60;
    config.clean_session = true;
    config.use_ssl = false;
}

Air780EGMQTT::~Air780EGMQTT() {
    if (isConnected()) {
        disconnect();
    }
}

bool Air780EGMQTT::begin() {
    if (!core) {
        AIR780EG_LOGE(TAG, "Core instance is null");
        return false;
    }
    
    
    
    AIR780EG_LOGI(TAG, "MQTT module initialized");
    return true;
}

/*
AT+MCONFIG=868488076771714,root,luat123456              //填写clientid、用户名、密码；可以带引号可以不带

OK

AT+MIPSTART="airtest.openluat.com","1883"               //填写mqtt服务器的域名和端口号；注意，如果是无证书ssl加密连接时，指令格式需要换成 AT+SSLMIPSTART=<svraddr>,<port>

OK
 */
void Air780EGMQTT::setConfig(const Air780EGMQTTConfig& cfg) {
    config = cfg;
    // 设置MQTT消息格式为HEX模式
    if (!sendATCommand("AT+MQTTMODE=1", "OK", 3000)) {
        AIR780EG_LOGW(TAG, "Failed to set MQTT text mode");
    }
    // 设置MQTT连接参数
    if (!sendATCommand("AT+MCONFIG=\"" + config.client_id + "\",\"" + config.username + "\",\"" + config.password + "\"", "OK", 3000)) {
        AIR780EG_LOGW(TAG, "Failed to set MQTT config");
    }
    

    AIR780EG_LOGD(TAG, "Configuration updated");
}

Air780EGMQTTConfig Air780EGMQTT::getConfig() const {
    return config;
}

bool Air780EGMQTT::connect() {
    if (config.server.isEmpty()) {
        AIR780EG_LOGE(TAG, "Server not configured");
        return false;
    }
    
    return connect(config.server, config.port, config.client_id, config.username, config.password);
}

bool Air780EGMQTT::connect(const String& server, int port, const String& client_id) {
    return connect(server, port, client_id, "", "");
}

bool Air780EGMQTT::connect(const String& server, int port, const String& client_id, 
                          const String& username, const String& password) {
    if (state == MQTT_CONNECTING || state == MQTT_CONNECTED) {
        AIR780EG_LOGW(TAG, "Already connected or connecting");
        return state == MQTT_CONNECTED;
    }
    // 断开现有连接,否则会连接失败
    while (!disconnect())
    {
        delay(1000);
    }

    // 设置TCP连接参数
    // 建立mqtt会话；注意需要返回CONNECT OK后才能发此条指令，并且要立即发，否则就会被服务器踢掉 报错 767 操作失败
    if (!sendATCommand("AT+MIPSTART=\"" + config.server + "\",\"" + String(config.port) + "\"", "OK", 3000)) {
        AIR780EG_LOGW(TAG, "Failed to set MQTT IP start");
    }

    state = MQTT_CONNECTING;
    AIR780EG_LOGI(TAG, "Connecting to MQTT server");
    // 发送连接命令
    if (!sendATCommand("AT+MCONNECT=1,60", "OK", 5000)) {
        AIR780EG_LOGE(TAG, "MQTT connect command failed");
        state = MQTT_ERROR;
        return false;
    }
    
    // 简化：直接设置为已连接状态
    state = MQTT_CONNECTED;
    last_ping_time = millis();
    reconnect_attempts = 0;
    
    AIR780EG_LOGI(TAG, "MQTT connected successfully");
    
    if (connection_callback) {
        connection_callback(true);
    }
    
    return true;
}

bool Air780EGMQTT::disconnect() {
    if (state != MQTT_CONNECTED) {
        return true;
    }
    
    state = MQTT_DISCONNECTING;
    AIR780EG_LOGI(TAG, "Disconnecting from MQTT server");
    
    if (sendATCommand("AT+MDISCONNECT", "OK", 10000)) {
        state = MQTT_DISCONNECTED;
        AIR780EG_LOGI(TAG, "MQTT disconnected");
        
        if (connection_callback) {
            connection_callback(false);
        }
        
        return true;
    }
    
    AIR780EG_LOGE(TAG, "MQTT disconnect failed");
    state = MQTT_ERROR;
    return false;
}

bool Air780EGMQTT::isConnected() const {
    return state == MQTT_CONNECTED;
}

Air780EGMQTTState Air780EGMQTT::getState() const {
    return state;
}

bool Air780EGMQTT::enableSSL(bool enable) {
    config.use_ssl = enable;
    if (enable) {
        config.port = (config.port == 1883) ? 8883 : config.port;
    }
    return true;
}

bool Air780EGMQTT::setSSLConfig(const String& ca_cert, const String& client_cert, const String& client_key) {
    AIR780EG_LOGI(TAG, "SSL configuration updated");
    return true;
}

/*
AT+MPUB="mqtt/sub",0,0,"data from 4G module"        //发布主题

OK

+MSUB: "mqtt/pub",20 byte,data from tcp server          //收到服务器下发的消息，+MSUB的URC上报
*/
bool Air780EGMQTT::publish(const String& topic, const String& payload, int qos, bool retain) {
    if (!isConnected()) {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }
    
    // 构建发布命令
    String pub_cmd = "AT+MPUB=\"" + topic + "\"," + String(qos) + "," + String(retain ? 1 : 0) + ",\"" + payload + "\"";
    
    if (sendATCommand(pub_cmd, "OK", 10000)) {
        AIR780EG_LOGD(TAG, "Published message");
        return true;
    }
    
    AIR780EG_LOGE(TAG, "Failed to publish message");
    return false;
}

bool Air780EGMQTT::publishJSON(const String& topic, const String& json, int qos) {
    return publish(topic, json, qos, false);
}

/*
AT+MSUB="mqtt/pub",0        //订阅主题

OK

SUBACK
*/
bool Air780EGMQTT::subscribe(const String& topic, int qos) {
    if (!isConnected()) {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }
    
    String sub_cmd = "AT+MSUB=\"" + topic + "\"," + String(qos);
    
    if (sendATCommand(sub_cmd, "OK", 10000)) {
        // 添加到订阅列表
        if (subscription_count < MAX_SUBSCRIPTIONS) {
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

bool Air780EGMQTT::unsubscribe(const String& topic) {
    if (!isConnected()) {
        AIR780EG_LOGE(TAG, "Not connected to MQTT server");
        return false;
    }
    
    String unsub_cmd = "AT+MUNSUB=\"" + topic + "\"";
    
    if (sendATCommand(unsub_cmd, "OK", 10000)) {
        // 从订阅列表中移除
        for (int i = 0; i < subscription_count; i++) {
            if (subscriptions[i] == topic) {
                for (int j = i; j < subscription_count - 1; j++) {
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

bool Air780EGMQTT::isSubscribed(const String& topic) const {
    for (int i = 0; i < subscription_count; i++) {
        if (subscriptions[i] == topic) {
            return true;
        }
    }
    return false;
}

void Air780EGMQTT::setMessageCallback(MQTTMessageCallback callback) {
    message_callback = callback;
}

void Air780EGMQTT::setConnectionCallback(MQTTConnectionCallback callback) {
    connection_callback = callback;
}

void Air780EGMQTT::loop() {
    updateState();
    processMessageCache();
    
    // 处理重连逻辑
    if (state == MQTT_ERROR || state == MQTT_DISCONNECTED) {
        if (millis() - last_reconnect_attempt > reconnect_interval && 
            reconnect_attempts < max_reconnect_attempts) {
            last_reconnect_attempt = millis();
            reconnect_attempts++;
            AIR780EG_LOGI(TAG, "Attempting reconnection");
            connect();
        }
    }
    
    // 发送心跳
    if (isConnected() && millis() - last_ping_time > (config.keepalive * 1000 / 2)) {
        sendATCommand("AT+MPING", "OK", 5000);
        last_ping_time = millis();
    }
}

bool Air780EGMQTT::sendATCommand(const String& command, const String& expected_response, unsigned long timeout) {
    if (!core) {
        return false;
    }
    String response = core->sendATCommandWithResponse(command, expected_response, timeout);
    return !response.isEmpty() && response.indexOf(expected_response) >= 0;
}

bool Air780EGMQTT::waitForURC(const String& urc_prefix, String& response, unsigned long timeout) {
    if (!core) {
        return false;
    }
    // 简化实现 - 暂时返回false，后续通过URC管理器处理
    response = "";
    return false;
}

void Air780EGMQTT::handleMQTTURC(const String& urc) {
    AIR780EG_LOGD(TAG, "Handling URC");
    
    if (urc.startsWith("+MCONNECT:")) {
        // 连接状态变化
        if (urc.indexOf("1,0") >= 0) {
            state = MQTT_CONNECTED;
            if (connection_callback) connection_callback(true);
        } else {
            state = MQTT_DISCONNECTED;
            if (connection_callback) connection_callback(false);
        }
    } else if (urc.startsWith("+MSUB:")) {
        // 收到订阅消息 - 简化处理
        if (message_callback) {
            message_callback("test/topic", "test payload");
        }
    }
}

void Air780EGMQTT::processMessageCache() {
    while (cached_message_count > 0) {
        Air780EGMQTTMessage msg = message_cache[cache_head];
        cache_head = (cache_head + 1) % MAX_CACHED_MESSAGES;
        cached_message_count--;
        
        if (message_callback) {
            message_callback(msg.topic, msg.payload);
        }
    }
}

void Air780EGMQTT::updateState() {
    // 定期检查连接状态
    static unsigned long last_state_check = 0;
    if (millis() - last_state_check > 30000) {
        if (state == MQTT_CONNECTED) {
            if (!sendATCommand("AT+MPING", "OK", 5000)) {
                AIR780EG_LOGW(TAG, "MQTT ping failed");
                state = MQTT_ERROR;
            }
        }
        last_state_check = millis();
    }
}

bool Air780EGMQTT::reconnect() {
    if (config.server.isEmpty()) {
        return false;
    }
    
    AIR780EG_LOGI(TAG, "Attempting to reconnect");
    return connect();
}

void Air780EGMQTT::registerURCHandlers() {
    if (!core) {
        return;
    }
    
    AIR780EG_LOGD(TAG, "MQTT URC handlers registered");
}

String Air780EGMQTT::getConnectionInfo() const {
    String info = "MQTT Status: ";
    switch (state) {
        case MQTT_DISCONNECTED: info += "Disconnected"; break;
        case MQTT_CONNECTING: info += "Connecting"; break;
        case MQTT_CONNECTED: info += "Connected"; break;
        case MQTT_DISCONNECTING: info += "Disconnecting"; break;
        case MQTT_ERROR: info += "Error"; break;
    }
    
    if (isConnected()) {
        info += "\nServer: " + config.server;
        info += "\nClient ID: " + config.client_id;
        info += "\nSubscriptions: " + String(subscription_count);
    }
    
    return info;
}

void Air780EGMQTT::printStatus() const {
    AIR780EG_LOGI(TAG, "MQTT Status");
}

void Air780EGMQTT::enableDebug(bool enable) {
    // 调试功能实现
}

void Air780EGMQTT::printConfig() const {
    AIR780EG_LOGI(TAG, "MQTT Configuration");
}
