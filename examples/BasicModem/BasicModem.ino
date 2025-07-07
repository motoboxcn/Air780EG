/*
 * Air780EG Basic Modem Example
 * 
 * 这个示例展示了如何使用Air780EG库的基本功能：
 * - 初始化模块
 * - 启用网络功能
 * - 启用GNSS定位
 * - 获取网络和定位信息
 */

#include <Air780EG.h>

// 创建Air780EG实例
Air780EG air780;

// 串口配置 (根据你的硬件连接调整)
#define AIR780_SERIAL Serial2
#define AIR780_BAUDRATE 115200
#define AIR780_RESET_PIN -1  // 如果没有连接复位引脚，设为-1
#define AIR780_POWER_PIN -1  // 如果没有连接电源控制引脚，设为-1

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("Air780EG Basic Example Starting...");
    
    // 设置日志级别 (可选)
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
    Air780EG::setLogOutput(&Serial);
    Air780EG::enableTimestamp(true);
    
    // 显示库版本信息
    Air780EG::printVersion();
    
    // 初始化Air780EG模块
    Serial.println("Initializing Air780EG...");
    if (!air780.begin(&AIR780_SERIAL, AIR780_BAUDRATE, AIR780_RESET_PIN, AIR780_POWER_PIN)) {
        Serial.println("Failed to initialize Air780EG!");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("Air780EG initialized successfully!");
    
    // 启用网络功能
    Serial.println("Enabling network...");
    if (air780.getNetwork().enableNetwork()) {
        Serial.println("Network enabled successfully!");
        
        // 设置网络状态更新间隔为10秒
        air780.getNetwork().setUpdateInterval(10000);
    } else {
        Serial.println("Failed to enable network!");
    }
    
    // 启用GNSS定位
    Serial.println("Enabling GNSS...");
    if (air780.getGNSS().enableGNSS()) {
        Serial.println("GNSS enabled successfully!");
        
        // 设置GNSS更新频率为1Hz
        air780.getGNSS().setUpdateFrequency(1.0);
    } else {
        Serial.println("Failed to enable GNSS!");
    }
    
    Serial.println("Setup completed!");
    Serial.println("Waiting for network registration and GNSS fix...");
}

void loop() {
    // 必须调用这个方法来维护所有功能模块
    air780.loop();
    
    // 每10秒显示一次状态信息
    static unsigned long last_status_time = 0;
    if (millis() - last_status_time >= 10000) {
        last_status_time = millis();
        
        Serial.println("\n=== Status Update ===");
        
        // 显示网络状态
        if (air780.getNetwork().isDataValid()) {
            Serial.printf("Network Registered: %s\n", 
                         air780.getNetwork().isNetworkRegistered() ? "Yes" : "No");
            Serial.printf("Signal Strength: %d dBm\n", 
                         air780.getNetwork().getSignalStrength());
            Serial.printf("Operator: %s\n", 
                         air780.getNetwork().getOperatorName().c_str());
            Serial.printf("Network Type: %s\n", 
                         air780.getNetwork().getNetworkType().c_str());
        } else {
            Serial.println("Network data not available yet");
        }
        
        // 显示GNSS状态
        if (air780.getGNSS().isDataValid()) {
            Serial.printf("GNSS Fixed: %s\n", 
                         air780.getGNSS().isFixed() ? "Yes" : "No");
            Serial.printf("Satellites: %d\n", 
                         air780.getGNSS().getSatelliteCount());
            
            if (air780.getGNSS().isFixed()) {
                Serial.printf("Location: %.6f, %.6f\n", 
                             air780.getGNSS().getLatitude(), 
                             air780.getGNSS().getLongitude());
                Serial.printf("Altitude: %.2f m\n", 
                             air780.getGNSS().getAltitude());
                Serial.printf("Speed: %.2f km/h\n", 
                             air780.getGNSS().getSpeed());
            }
        } else {
            Serial.println("GNSS data not available yet");
        }
        
        Serial.println("==================");
    }
    
    // 每60秒显示一次详细状态
    static unsigned long last_detail_time = 0;
    if (millis() - last_detail_time >= 60000) {
        last_detail_time = millis();
        
        Serial.println("\n=== Detailed Status ===");
        air780.printStatus();
    }
    
    delay(100);
}
