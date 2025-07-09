/*
 * Air780EG MQTT 示例
 * 
 * 本示例演示如何使用Air780EG库的MQTT功能：
 * 1. 连接到MQTT服务器
 * 2. 订阅主题
 * 3. 发布消息
 * 4. 处理接收到的消息
 * 5. 发布GPS定位数据
 */

#include <Air780EG.h>

// MQTT配置
const char* MQTT_SERVER = "your-mqtt-server.com";
const int MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "Air780EG_Test";
const char* MQTT_USERNAME = "your_username";
const char* MQTT_PASSWORD = "your_password";

// 主题定义
const char* TOPIC_STATUS = "device/status";
const char* TOPIC_GPS = "device/gps";
const char* TOPIC_CONTROL = "device/control";

// 全局变量
unsigned long lastStatusPublish = 0;
unsigned long lastGPSPublish = 0;
const unsigned long STATUS_INTERVAL = 30000;  // 30秒发布一次状态
const unsigned long GPS_INTERVAL = 10000;     // 10秒发布一次GPS

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Air780EG MQTT Example Starting...");
    
    // 设置调试级别
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    Air780EG::setLogOutput(&Serial);
    Air780EG::enableTimestamp(true);
    
    // 初始化Air780EG模块
    // 根据你的硬件连接修改引脚
    if (!air780eg.begin(&Serial2, 115200, 16, 17, 18)) {
        Serial.println("Failed to initialize Air780EG module!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("Air780EG module initialized successfully");
    
    // 等待网络注册
    Serial.println("Waiting for network registration...");
    while (!air780eg.getNetwork().isNetworkRegistered()) {
        air780eg.loop();
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nNetwork registered!");
    
    // 显示网络信息
    air780eg.getNetwork().printNetworkInfo();
    
    // 启用GNSS
    if (air780eg.getGNSS().enableGNSS()) {
        Serial.println("GNSS enabled");
        air780eg.getGNSS().setUpdateFrequency(1.0); // 1Hz更新
    }
    
    // 初始化MQTT
    setupMQTT();
}

void setupMQTT() {
    Serial.println("Setting up MQTT...");
    
    // 初始化MQTT模块
    if (!air780eg.getMQTT().begin()) {
        Serial.println("Failed to initialize MQTT module!");
        return;
    }
    
    // 配置MQTT连接参数
    Air780EGMQTTConfig config;
    config.server = MQTT_SERVER;
    config.port = MQTT_PORT;
    config.client_id = MQTT_CLIENT_ID;
    config.username = MQTT_USERNAME;
    config.password = MQTT_PASSWORD;
    config.keepalive = 60;
    config.clean_session = true;
    
    air780eg.getMQTT().setConfig(config);
    
    // 设置消息回调函数
    air780eg.getMQTT().setMessageCallback([](const String& topic, const String& payload) {
        Serial.println("Received MQTT message:");
        Serial.println("  Topic: " + topic);
        Serial.println("  Payload: " + payload);
        
        // 处理控制命令
        if (topic == TOPIC_CONTROL) {
            handleControlMessage(payload);
        }
    });
    
    // 设置连接状态回调
    air780eg.getMQTT().setConnectionCallback([](bool connected) {
        if (connected) {
            Serial.println("MQTT connected successfully!");
            
            // 订阅控制主题
            air780eg.getMQTT().subscribe(TOPIC_CONTROL, 1);
            
            // 发布上线消息
            String onlineMsg = "{\"status\":\"online\",\"timestamp\":" + String(millis()) + "}";
            air780eg.getMQTT().publishJSON(TOPIC_STATUS, onlineMsg, 1);
            
        } else {
            Serial.println("MQTT disconnected!");
        }
    });
    
    // 连接到MQTT服务器
    if (air780eg.getMQTT().connect()) {
        Serial.println("Connecting to MQTT server...");
    } else {
        Serial.println("Failed to start MQTT connection!");
    }
}

void loop() {
    // 必须调用主循环
    air780eg.loop();
    
    // 定期发布状态信息
    if (millis() - lastStatusPublish > STATUS_INTERVAL) {
        publishStatus();
        lastStatusPublish = millis();
    }
    
    // 定期发布GPS信息
    if (millis() - lastGPSPublish > GPS_INTERVAL) {
        publishGPS();
        lastGPSPublish = millis();
    }
    
    delay(100);
}

void publishStatus() {
    if (!air780eg.getMQTT().isConnected()) {
        return;
    }
    
    // 构建状态JSON
    String status = "{";
    status += "\"timestamp\":" + String(millis()) + ",";
    status += "\"uptime\":" + String(millis() / 1000) + ",";
    status += "\"network\":{";
    status += "\"registered\":" + String(air780eg.getNetwork().isNetworkRegistered() ? "true" : "false") + ",";
    status += "\"signal\":" + String(air780eg.getNetwork().getSignalStrength()) + ",";
    status += "\"operator\":\"" + air780eg.getNetwork().getOperatorName() + "\"";
    status += "},";
    status += "\"gnss\":{";
    status += "\"enabled\":" + String(air780eg.getGNSS().isEnabled() ? "true" : "false") + ",";
    status += "\"fixed\":" + String(air780eg.getGNSS().isFixed() ? "true" : "false");
    if (air780eg.getGNSS().isFixed()) {
        status += ",\"satellites\":" + String(air780eg.getGNSS().getSatelliteCount());
    }
    status += "}";
    status += "}";
    
    if (air780eg.getMQTT().publishJSON(TOPIC_STATUS, status, 0)) {
        Serial.println("Status published");
    } else {
        Serial.println("Failed to publish status");
    }
}

void publishGPS() {
    if (!air780eg.getMQTT().isConnected() || !air780eg.getGNSS().isFixed()) {
        return;
    }
    
    // 构建GPS JSON
    String gps = "{";
    gps += "\"timestamp\":" + String(millis()) + ",";
    gps += "\"latitude\":" + String(air780eg.getGNSS().getLatitude(), 6) + ",";
    gps += "\"longitude\":" + String(air780eg.getGNSS().getLongitude(), 6) + ",";
    gps += "\"altitude\":" + String(air780eg.getGNSS().getAltitude(), 2) + ",";
    gps += "\"speed\":" + String(air780eg.getGNSS().getSpeed(), 2) + ",";
    gps += "\"course\":" + String(air780eg.getGNSS().getCourse(), 2) + ",";
    gps += "\"satellites\":" + String(air780eg.getGNSS().getSatelliteCount()) + ",";
    gps += "\"hdop\":" + String(air780eg.getGNSS().getHDOP(), 2) + ",";
    gps += "\"utc_time\":\"" + air780eg.getGNSS().getTimestamp() + "\",";
    gps += "\"utc_date\":\"" + air780eg.getGNSS().getDate() + "\"";
    gps += "}";
    
    if (air780eg.getMQTT().publishJSON(TOPIC_GPS, gps, 0)) {
        Serial.println("GPS data published: " + 
                      String(air780eg.getGNSS().getLatitude(), 6) + ", " + 
                      String(air780eg.getGNSS().getLongitude(), 6));
    } else {
        Serial.println("Failed to publish GPS data");
    }
}

void handleControlMessage(const String& payload) {
    Serial.println("Processing control message: " + payload);
    
    // 简单的JSON解析（实际项目中建议使用ArduinoJson库）
    if (payload.indexOf("\"gnss\":\"enable\"") >= 0) {
        if (air780eg.getGNSS().enableGNSS()) {
            Serial.println("GNSS enabled via MQTT command");
        }
    } else if (payload.indexOf("\"gnss\":\"disable\"") >= 0) {
        if (air780eg.getGNSS().disableGNSS()) {
            Serial.println("GNSS disabled via MQTT command");
        }
    } else if (payload.indexOf("\"status\":\"request\"") >= 0) {
        // 立即发布状态
        publishStatus();
    } else if (payload.indexOf("\"reboot\":true") >= 0) {
        Serial.println("Reboot command received");
        // 发布离线消息
        String offlineMsg = "{\"status\":\"offline\",\"reason\":\"reboot\",\"timestamp\":" + String(millis()) + "}";
        air780eg.getMQTT().publishJSON(TOPIC_STATUS, offlineMsg, 1);
        delay(1000);
        ESP.restart();
    }
}
