# 定位策略说明

## 重要变更

**v1.2.1**: 移除了自动WiFi/LBS定位切换功能，改为用户手动控制，避免串口并发冲突。

## 问题背景

### 串口并发冲突
Air780EG模块使用单一串口进行AT指令通信。当WiFi定位(`AT+WIFILOC=1,1`)或LBS定位(`AT+CIPGSMLOC=1,1`)执行时：

- **阻塞时间**: 最长30秒
- **串口占用**: 独占串口资源
- **并发冲突**: 与MQTT、GNSS查询等其他AT指令冲突
- **响应混乱**: 导致串口响应解析错误

### 实际日志示例
```
[31122] [DEBUG] [Air780EGCore] > AT+WIFILOC=1,1
[32144] [DEBUG] [Air780EGCore] > AT+MPUB="topic",0,0,"data"
[37206] [ERROR] [MQTT] Failed to publish message, response: 
[56284] [VERB ] [Air780EGCore] < +IIO:48
K
K
+QTTT 1
K
```

## 解决方案

### 1. 移除自动切换
```cpp
// 原有实现（已移除）
if (gnss_data.data_valid == false) {
    startAsyncWiFiLocation(); // 自动触发，导致冲突
}

// 新实现
if (gnss_data.data_valid == false) {
    AIR780EG_LOGW(TAG, "GNSS signal lost. Manual WiFi/LBS location available if needed.");
}
```

### 2. 用户手动控制
用户根据业务需求决定何时使用辅助定位：

```cpp
// 检查GNSS状态
if (air780.getGNSS().isGNSSSignalLost()) {
    // 用户决定是否需要辅助定位
    if (needBackupLocation()) {
        // 注意：这会阻塞30秒
        bool success = air780.getGNSS().updateWIFILocation();
        if (!success) {
            air780.getGNSS().updateLBS(); // 备选方案
        }
    }
}
```

## 推荐策略

### 1. 基于业务场景
```cpp
bool shouldUseBackupLocation() {
    // 车辆静止 + 城市环境 = WiFi定位
    if (isVehicleStationary() && isUrbanArea()) {
        return true;
    }
    
    // 紧急情况 = LBS定位
    if (isEmergencyMode()) {
        return true;
    }
    
    // 移动中 = 等待GNSS恢复
    return false;
}
```

### 2. 基于时间策略
```cpp
// GNSS丢失超过5分钟才使用备用定位
static unsigned long gnssLostTime = 0;

if (air780.getGNSS().isGNSSSignalLost()) {
    if (gnssLostTime == 0) {
        gnssLostTime = millis();
    }
    
    if (millis() - gnssLostTime > 300000) { // 5分钟
        requestBackupLocation();
        gnssLostTime = 0;
    }
}
```

### 3. 基于环境检测
```cpp
// 检测到室内环境时使用WiFi定位
if (isIndoorEnvironment()) {
    air780.getGNSS().updateWIFILocation();
}

// 偏远地区使用LBS定位
if (isRemoteArea()) {
    air780.getGNSS().updateLBS();
}
```

## 最佳实践

### ✅ 推荐做法
1. **定期检查**: 每30秒检查一次GNSS状态
2. **智能策略**: 根据业务场景决定是否使用辅助定位
3. **用户提示**: 告知用户辅助定位会暂停其他功能
4. **错误处理**: 辅助定位失败时的降级策略

### ❌ 避免做法
1. **频繁调用**: 避免短时间内多次调用辅助定位
2. **盲目自动**: 不要无条件自动触发辅助定位
3. **忽略阻塞**: 不要在关键任务期间调用辅助定位
4. **缺少反馈**: 不要忽略辅助定位的执行状态

## API参考

### 状态查询
```cpp
bool isGNSSSignalLost();        // 检查GNSS信号是否丢失
String getLocationSource();     // 获取位置来源
unsigned long getLastLocationTime(); // 获取最后定位时间
```

### 手动定位
```cpp
bool updateWIFILocation();      // WiFi定位（阻塞30秒）
bool updateLBS();              // LBS定位（阻塞30秒）
```

### 使用示例
```cpp
void handleLocationStrategy() {
    if (air780.getGNSS().isGNSSSignalLost()) {
        Serial.println("⚠ GNSS signal lost");
        
        if (shouldUseBackupLocation()) {
            Serial.println("🔍 Starting backup location (30s)...");
            
            bool success = air780.getGNSS().updateWIFILocation();
            if (!success) {
                success = air780.getGNSS().updateLBS();
            }
            
            Serial.println(success ? "✓ Backup location obtained" : "✗ Backup location failed");
        }
    }
}
```

## 性能对比

| 场景 | 自动切换 | 手动控制 |
|------|----------|----------|
| MQTT延迟 | 30秒阻塞 | 无影响 |
| 串口冲突 | 经常发生 | 完全避免 |
| 用户控制 | 无法控制 | 完全控制 |
| 资源消耗 | 不可预测 | 可控制 |
| 调试难度 | 困难 | 简单 |

## 总结

通过移除自动定位切换，将控制权交给用户，可以：

1. **完全避免串口并发冲突**
2. **确保MQTT等关键功能正常运行**
3. **提供更灵活的定位策略**
4. **提高系统稳定性和可预测性**

用户可以根据具体业务需求，在合适的时机调用辅助定位功能。
