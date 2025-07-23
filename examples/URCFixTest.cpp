/*
 * URC修复验证测试
 * 
 * 此测试用于验证URC机制与队列机制冲突修复的效果
 * 
 * 验证要点：
 * 1. GNSS响应（+CGNSINF:）不再触发MQTT回调
 * 2. 真正的MQTT URC（+MSUB:）能正确处理
 * 3. 队列机制正常工作
 * 4. 无JSON解析错误
 */

#include <Arduino.h>
#include "Air780EG.h"

// 串口配置
#define SERIAL_BAUDRATE 115200
#define AIR780EG_RX_PIN 16
#define AIR780EG_TX_PIN 17
#define AIR780EG_POWER_PIN 18

// 测试计数器
int gnss_updates = 0;
int mqtt_callbacks = 0;
int json_errors = 0;

// MQTT消息回调
void onMQTTMessage(const String& topic, const String& payload) {
    mqtt_callbacks++;
    
    Serial.println("=== MQTT消息回调触发 ===");
    Serial.printf("主题: %s\n", topic.c_str());
    Serial.printf("负载: %s\n", payload.c_str());
    
    // 检查是否是错误的GNSS响应
    if (topic == "GNSS" && payload.indexOf("+CGNSINF:") >= 0) {
        Serial.println("❌ 错误：GNSS响应被当作MQTT消息处理！");
        json_errors++;
    } else if (topic.startsWith("+")) {
        Serial.println("✅ 正确：真正的MQTT消息");
    }
    
    Serial.println("=== MQTT消息回调结束 ===");
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== URC修复验证测试 ===");
    
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
    
    // 启用GNSS
    if (!air780eg.getGNSS().enableGNSS()) {
        Serial.println("Failed to enable GNSS");
    } else {
        Serial.println("GNSS enabled successfully");
    }
    
    // 设置MQTT消息回调
    air780eg.getMQTT().setMessageCallback(onMQTTMessage);
    
    Serial.println("=== 开始URC修复验证测试 ===");
    Serial.println("观察要点：");
    Serial.println("1. GNSS更新不应触发MQTT回调");
    Serial.println("2. 无JSON解析错误");
    Serial.println("3. 队列机制正常工作");
}

void loop() {
    // 必须调用主循环处理AT命令队列
    air780eg.loop();
    
    // 统计GNSS更新次数
    static int last_gnss_updates = 0;
    if (air780eg.getGNSS().isDataValid()) {
        gnss_updates++;
    }
    
    // 每10秒显示一次统计信息
    static unsigned long last_stats = 0;
    unsigned long current_time = millis();
    
    if (current_time - last_stats >= 10000) {
        Serial.println("=== 测试统计信息 ===");
        Serial.printf("运行时间: %lu 秒\n", current_time / 1000);
        Serial.printf("GNSS更新次数: %d\n", gnss_updates);
        Serial.printf("MQTT回调次数: %d\n", mqtt_callbacks);
        Serial.printf("JSON错误次数: %d\n", json_errors);
        
        // 验证结果
        if (json_errors == 0) {
            Serial.println("✅ 测试通过：无JSON解析错误");
        } else {
            Serial.println("❌ 测试失败：存在JSON解析错误");
        }
        
        if (gnss_updates > last_gnss_updates && mqtt_callbacks == 0) {
            Serial.println("✅ 测试通过：GNSS更新未触发MQTT回调");
        }
        
        Serial.println("==================");
        last_stats = current_time;
        last_gnss_updates = gnss_updates;
    }
    
    delay(100); // 主循环延迟
}
