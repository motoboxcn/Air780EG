#include <Air780EG.h>

Air780EG air780;

// 定位策略配置
unsigned long lastLocationCheck = 0;
const unsigned long LOCATION_CHECK_INTERVAL = 30000; // 30秒检查一次
bool manualLocationInProgress = false;

void setup() {
    Serial.begin(115200);
    
    if (air780.begin(&Serial2, 115200)) {
        Serial.println("Air780EG initialized");
        
        // 启用网络和GNSS
        air780.getNetwork().enableNetwork();
        air780.getGNSS().enableGNSS();
        air780.getGNSS().setUpdateFrequency(1.0);
        
        // 启用LBS辅助定位
        air780.getGNSS().enableLBS(true);
        
        Serial.println("=== 手动定位控制演示 ===");
        Serial.println("GNSS信号丢失时，用户可选择性调用WiFi/LBS定位");
        Serial.println("避免自动定位造成的串口冲突和MQTT延迟");
    }
}

void loop() {
    // 必须调用，维护所有功能模块
    air780.loop();
    
    // 手动定位策略
    handleManualLocationStrategy();
    
    // 显示定位信息
    displayLocationStatus();
    
    // 模拟MQTT等其他任务
    simulateOtherTasks();
    
    delay(1000);
}

void handleManualLocationStrategy() {
    unsigned long currentTime = millis();
    
    // 每30秒检查一次定位状态
    if (currentTime - lastLocationCheck >= LOCATION_CHECK_INTERVAL) {
        lastLocationCheck = currentTime;
        
        if (air780.getGNSS().isFixed()) {
            // GNSS定位正常
            Serial.println("✓ GNSS location available");
            manualLocationInProgress = false;
        } 
        else if (air780.getGNSS().isGNSSSignalLost() && !manualLocationInProgress) {
            // GNSS信号丢失，用户决定是否启用辅助定位
            Serial.println("⚠ GNSS signal lost!");
            Serial.println("Options:");
            Serial.println("1. Wait for GNSS recovery (recommended for moving vehicles)");
            Serial.println("2. Use WiFi location (good for stationary, urban areas)");
            Serial.println("3. Use LBS location (backup option)");
            
            // 根据业务逻辑决定是否启用辅助定位
            if (shouldUseBackupLocation()) {
                requestBackupLocation();
            }
        }
    }
}

bool shouldUseBackupLocation() {
    // 用户可以根据业务需求自定义策略
    // 例如：
    // - 车辆静止时使用WiFi定位
    // - 紧急情况下使用LBS定位
    // - 室内环境优先WiFi定位
    
    // 示例策略：GNSS丢失超过5分钟才使用备用定位
    static unsigned long gnssLostTime = 0;
    
    if (air780.getGNSS().isGNSSSignalLost()) {
        if (gnssLostTime == 0) {
            gnssLostTime = millis();
        }
        
        // 5分钟后启用备用定位
        if (millis() - gnssLostTime > 300000) {
            gnssLostTime = 0; // 重置
            return true;
        }
    } else {
        gnssLostTime = 0; // GNSS恢复，重置计时
    }
    
    return false;
}

void requestBackupLocation() {
    if (manualLocationInProgress) {
        Serial.println("Backup location request already in progress...");
        return;
    }
    
    manualLocationInProgress = true;
    Serial.println("🔍 Starting backup location request...");
    Serial.println("⚠ This will block for up to 30 seconds!");
    
    // 策略1：优先WiFi定位（更精确）
    bool success = air780.getGNSS().updateWIFILocation();
    
    if (success) {
        Serial.println("✓ WiFi location obtained");
    } else {
        Serial.println("✗ WiFi location failed, trying LBS...");
        
        // 策略2：WiFi失败时使用LBS
        success = air780.getGNSS().updateLBS();
        
        if (success) {
            Serial.println("✓ LBS location obtained");
        } else {
            Serial.println("✗ All backup location methods failed");
        }
    }
    
    manualLocationInProgress = false;
}

void displayLocationStatus() {
    static unsigned long lastDisplay = 0;
    
    if (millis() - lastDisplay >= 10000) { // 每10秒显示一次
        lastDisplay = millis();
        
        Serial.println("\n=== 定位状态 ===");
        
        if (air780.getGNSS().isFixed()) {
            Serial.printf("✓ 位置: %.6f, %.6f\n",
                air780.getGNSS().getLatitude(),
                air780.getGNSS().getLongitude());
            Serial.printf("  来源: %s\n", air780.getGNSS().getLocationSource().c_str());
            Serial.printf("  卫星: %d颗\n", air780.getGNSS().getSatelliteCount());
            Serial.printf("  更新: %lu秒前\n", 
                (millis() - air780.getGNSS().getLastLocationTime()) / 1000);
        } else {
            Serial.println("✗ 无定位信息");
            
            if (manualLocationInProgress) {
                Serial.println("  🔄 备用定位请求中...");
            }
        }
        
        Serial.println("================");
    }
}

void simulateOtherTasks() {
    static unsigned long lastTask = 0;
    
    // 模拟每2秒执行的任务（如MQTT发布）
    if (millis() - lastTask >= 2000) {
        lastTask = millis();
        
        // 只有在非定位请求期间才执行其他任务
        if (!manualLocationInProgress) {
            Serial.printf("[%lu] MQTT/其他任务正常执行 ✓\n", millis());
        } else {
            Serial.printf("[%lu] 等待定位完成... ⏳\n", millis());
        }
    }
}

// 可选：通过串口命令手动触发定位
void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        
        if (cmd == "wifi") {
            Serial.println("手动触发WiFi定位...");
            bool success = air780.getGNSS().updateWIFILocation();
            Serial.println(success ? "WiFi定位成功" : "WiFi定位失败");
        }
        else if (cmd == "lbs") {
            Serial.println("手动触发LBS定位...");
            bool success = air780.getGNSS().updateLBS();
            Serial.println(success ? "LBS定位成功" : "LBS定位失败");
        }
        else if (cmd == "status") {
            Serial.printf("GNSS固定: %s\n", air780.getGNSS().isFixed() ? "是" : "否");
            Serial.printf("信号丢失: %s\n", air780.getGNSS().isGNSSSignalLost() ? "是" : "否");
            Serial.printf("位置来源: %s\n", air780.getGNSS().getLocationSource().c_str());
        }
    }
}
