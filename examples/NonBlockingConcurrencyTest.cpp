/*
 * 非阻塞并发测试示例
 * 
 * 此示例演示了修复后的非阻塞AT命令队列系统，
 * 可以同时执行WiFi定位和MQTT消息发布而不会产生串口冲突。
 * 
 * 测试场景：
 * 1. 启动WiFi定位（30秒阻塞命令）
 * 2. 在WiFi定位期间定时发布MQTT消息
 * 3. 验证两个功能都能正常工作
 */

#include <Arduino.h>
#include "Air780EG.h"

// 串口配置
#define SERIAL_BAUDRATE 115200
#define AIR780EG_RX_PIN 16
#define AIR780EG_TX_PIN 17
#define AIR780EG_POWER_PIN 18

// MQTT配置
const char* mqtt_server = "your-mqtt-server.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "Air780EG_Test";
const char* mqtt_username = "your_username";
const char* mqtt_password = "your_password";
const char* test_topic = "test/concurrency";

// 测试状态
unsigned long last_mqtt_publish = 0;
unsigned long last_wifi_location = 0;
const unsigned long mqtt_interval = 5000;  // 5秒发布一次MQTT
const unsigned long wifi_interval = 60000; // 60秒请求一次WiFi定位

bool wifi_location_requested = false;
bool mqtt_connected = false;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== Air780EG 非阻塞并发测试 ===");
    
    // 设置调试级别
    Air780EG::setLogLevel(AIR780EG_LOG_DEBUG);
    Air780EG::setLogOutput(&Serial);
    Air780EG::enableTimestamp(true);
    
    // 初始化Air780EG
    if (!air780eg.begin(&Serial2, SERIAL_BAUDRATE, AIR780EG_RX_PIN, AIR780EG_TX_PIN, AIR780EG_POWER_PIN)) {
        Serial.println("Failed to initialize Air780EG");
        while (1) delay(1000);
    }
    
    Serial.println("Air780EG initialized successfully");
    
    // 等待网络注册
    Serial.println("Waiting for network registration...");
    while (!air780eg.getNetwork().isRegistered()) {
        air780eg.loop();
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nNetwork registered!");
    
    // 启用网络
    if (!air780eg.getNetwork().enableNetwork()) {
        Serial.println("Failed to enable network");
        while (1) delay(1000);
    }
    
    // 启用GNSS
    if (!air780eg.getGNSS().enableGNSS()) {
        Serial.println("Failed to enable GNSS");
    }
    
    // 连接MQTT
    Serial.println("Connecting to MQTT server...");
    if (air780eg.getMQTT().connect(mqtt_server, mqtt_port, mqtt_client_id, mqtt_username, mqtt_password)) {
        Serial.println("MQTT connected successfully");
        mqtt_connected = true;
    } else {
        Serial.println("Failed to connect to MQTT server");
    }
    
    Serial.println("=== 开始并发测试 ===");
    Serial.println("测试场景：WiFi定位(30秒) + MQTT发布(5秒间隔)");
}

void loop() {
    // 必须调用主循环处理AT命令队列
    air780eg.loop();
    
    unsigned long current_time = millis();
    
    // 定时发布MQTT消息
    if (mqtt_connected && (current_time - last_mqtt_publish >= mqtt_interval)) {
        String payload = "{\"timestamp\":" + String(current_time) + 
                        ",\"test\":\"concurrency\",\"counter\":" + String(current_time / 1000) + "}";
        
        Serial.println("Publishing MQTT message...");
        if (air780eg.getMQTT().publish(test_topic, payload)) {
            Serial.println("MQTT message published successfully");
        } else {
            Serial.println("MQTT publish in progress or failed");
        }
        
        last_mqtt_publish = current_time;
    }
    
    // 定时请求WiFi定位
    if (current_time - last_wifi_location >= wifi_interval) {
        Serial.println("Requesting WiFi location...");
        wifi_location_requested = true;
        last_wifi_location = current_time;
    }
    
    // 处理WiFi定位请求
    if (wifi_location_requested) {
        if (air780eg.getGNSS().updateWIFILocation()) {
            Serial.println("WiFi location updated successfully:");
            Serial.printf("  Latitude: %.6f\n", air780eg.getGNSS().getLatitude());
            Serial.printf("  Longitude: %.6f\n", air780eg.getGNSS().getLongitude());
            Serial.printf("  Location Type: %s\n", air780eg.getGNSS().getLocationSource().c_str());
            wifi_location_requested = false;
        } else {
            // WiFi定位还在进行中，不需要重复请求
            // Serial.println("WiFi location request in progress...");
        }
    }
    
    // 显示状态信息
    static unsigned long last_status_print = 0;
    if (current_time - last_status_print >= 10000) { // 每10秒显示一次状态
        Serial.println("=== 状态信息 ===");
        Serial.printf("运行时间: %lu 秒\n", current_time / 1000);
        Serial.printf("网络状态: %s\n", air780eg.getNetwork().isRegistered() ? "已注册" : "未注册");
        Serial.printf("MQTT状态: %s\n", air780eg.getMQTT().isConnected() ? "已连接" : "未连接");
        Serial.printf("GNSS状态: %s\n", air780eg.getGNSS().isFixed() ? "已定位" : "未定位");
        Serial.printf("WiFi定位请求: %s\n", wifi_location_requested ? "进行中" : "空闲");
        Serial.println("================");
        last_status_print = current_time;
    }
    
    delay(100); // 主循环延迟
}
