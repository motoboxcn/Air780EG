/*
 * Air780EG URC Test Example
 * 
 * 这个示例展示了如何使用Air780EG库的URC（主动上报）功能：
 * - 注册URC处理器
 * - 处理网络状态变化
 * - 处理GNSS定位信息
 * - 处理MQTT消息
 * - 自定义URC处理器
 */

#include <Air780EG.h>

// 创建Air780EG实例
Air780EG air780;

// 串口配置
#define AIR780_SERIAL Serial2
#define AIR780_BAUDRATE 115200

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Air780EG URC Test Starting...");
    
    // 设置日志级别为DEBUG以查看URC处理详情
    Air780EG::setLogLevel(AIR780EG_LOG_DEBUG);
    Air780EG::setLogOutput(&Serial);
    Air780EG::enableTimestamp(true);
    
    // 初始化Air780EG模块
    if (!air780.begin(&AIR780_SERIAL, AIR780_BAUDRATE)) {
        Serial.println("Failed to initialize Air780EG!");
        while (1) delay(1000);
    }
    
    Serial.println("Air780EG initialized successfully!");
    
    // 注册自定义URC处理器
    setupCustomURCHandlers();
    
    // 启用网络功能
    if (air780.getNetwork().enableNetwork()) {
        Serial.println("Network enabled - waiting for registration URC...");
    }
    
    // 启用GNSS功能
    if (air780.getGNSS().enableGNSS()) {
        Serial.println("GNSS enabled - waiting for positioning URC...");
    }
    
    Serial.println("Setup completed! Monitoring URC messages...");
    Serial.println("Expected URC messages:");
    Serial.println("- +CREG: (network registration)");
    Serial.println("- +CGREG: (GPRS registration)");
    Serial.println("- +UGNSINF: (GNSS info)");
    Serial.println("- +MSUB: (MQTT messages)");
    Serial.println();
}

void setupCustomURCHandlers() {
    // 获取URC管理器
    Air780EGURC& urc = air780.getURCManager();
    
    // 注册网络注册状态处理器
    urc.onNetworkRegistration([](const URCData& data) {
        Serial.println("🌐 Network Registration URC received!");
        Serial.printf("   Raw: %s\n", data.raw_data.c_str());
        
        auto reg = URCHelper::parseNetworkRegistration(data);
        Serial.printf("   Status: %s\n", reg.getStatusString().c_str());
        Serial.printf("   Registered: %s\n", reg.isRegistered() ? "Yes" : "No");
        
        if (reg.lac.length() > 0) {
            Serial.printf("   LAC: %s, CI: %s\n", reg.lac.c_str(), reg.ci.c_str());
        }
        if (reg.act >= 0) {
            Serial.printf("   Technology: %s\n", reg.getAccessTechnologyString().c_str());
        }
        Serial.println();
    });
    
    // 注册GNSS信息处理器
    urc.onGNSSInfo([](const URCData& data) {
        Serial.println("🛰️ GNSS Info URC received!");
        Serial.printf("   Raw: %s\n", data.raw_data.c_str());
        
        auto gnss = URCHelper::parseGNSSInfo(data);
        Serial.printf("   Valid: %s\n", gnss.isValid() ? "Yes" : "No");
        Serial.printf("   Satellites: %d\n", gnss.satellites_used);
        
        if (gnss.isValid()) {
            Serial.printf("   Location: %.6f, %.6f\n", gnss.latitude, gnss.longitude);
            Serial.printf("   Altitude: %.2f m\n", gnss.altitude);
            Serial.printf("   Speed: %.2f km/h\n", gnss.speed);
        }
        Serial.println();
    });
    
    // 注册MQTT消息处理器
    urc.onMQTTMessage([](const URCData& data) {
        Serial.println("📨 MQTT Message URC received!");
        Serial.printf("   Raw: %s\n", data.raw_data.c_str());
        
        auto mqtt = URCHelper::parseMQTTMessage(data);
        if (mqtt.isValid()) {
            Serial.printf("   Topic: %s\n", mqtt.topic.c_str());
            Serial.printf("   Payload: %s\n", mqtt.payload.c_str());
            Serial.printf("   QoS: %d\n", mqtt.qos);
        }
        Serial.println();
    });
    
    // 注册错误报告处理器
    urc.onErrorReport([](const URCData& data) {
        Serial.println("❌ Error Report URC received!");
        Serial.printf("   Raw: %s\n", data.raw_data.c_str());
        Serial.println();
    });
    
    // 注册自定义URC处理器示例
    urc.registerHandler("+CSQ:", URCType::SIGNAL_QUALITY, [](const URCData& data) {
        Serial.println("📶 Signal Quality URC received!");
        Serial.printf("   Raw: %s\n", data.raw_data.c_str());
        
        auto sq = URCHelper::parseSignalQuality(data);
        if (sq.isValid()) {
            Serial.printf("   RSSI: %d (%d dBm)\n", sq.rssi, sq.getRSSIdBm());
            Serial.printf("   BER: %d\n", sq.ber);
        }
        Serial.println();
    }, "Custom Signal Quality Handler");
    
    Serial.printf("Registered %d URC handlers\n", urc.getHandlerCount());
}

void loop() {
    // 必须调用这个方法来处理URC消息
    air780.loop();
    
    // 每30秒显示一次URC统计信息
    static unsigned long last_stats_time = 0;
    if (millis() - last_stats_time >= 30000) {
        last_stats_time = millis();
        
        Serial.println("\n=== URC Statistics ===");
        air780.getURCManager().printStatistics();
        Serial.println("=====================\n");
    }
    
    // 每60秒显示一次注册的处理器
    static unsigned long last_handlers_time = 0;
    if (millis() - last_handlers_time >= 60000) {
        last_handlers_time = millis();
        
        Serial.println("\n=== Registered URC Handlers ===");
        air780.getURCManager().printHandlers();
        Serial.println("==============================\n");
    }
    
    delay(100);
}

/*
 * 使用说明：
 * 
 * 1. URC消息会自动被库捕获和处理
 * 2. 你可以注册自己的处理器来响应特定的URC
 * 3. 库提供了便捷的解析函数来提取URC数据
 * 4. 所有URC处理都是异步的，不会阻塞主循环
 * 
 * 常见的Air780EG URC消息：
 * - +CREG: 网络注册状态变化
 * - +CGREG: GPRS注册状态变化  
 * - +UGNSINF: GNSS定位信息更新
 * - +MSUB: MQTT订阅消息接收
 * - +CMTI: 短信接收通知
 * - RING: 来电通知
 * - +CME ERROR: 命令执行错误
 */
