# MQTT 定时任务功能

## 概述

Air780EG MQTT库现在支持在setup阶段注册定时任务，这些任务会在MQTT loop中自动执行，支持动态数据结构生成和发布。

## 主要特性

- **灵活的任务注册**: 在setup阶段注册多个定时任务
- **动态数据生成**: 通过回调函数动态生成发布数据
- **任务管理**: 支持启用/禁用、添加/移除任务
- **自动执行**: 在MQTT loop中自动处理所有定时任务
- **连接状态感知**: 只有在MQTT连接时才执行任务

## 核心数据结构

### ScheduledTask 结构体
```cpp
struct ScheduledTask {
    String topic;                    // 发布主题
    ScheduledTaskCallback callback;  // 数据生成回调函数
    unsigned long interval_ms;       // 执行间隔（毫秒）
    unsigned long last_execution;    // 上次执行时间
    int qos;                        // QoS等级
    bool retain;                    // 保留消息标志
    bool enabled;                   // 任务是否启用
    String task_name;               // 任务名称（用于调试）
};
```

### 回调函数类型
```cpp
typedef String (*ScheduledTaskCallback)(void);
```

## API 接口

### 添加定时任务
```cpp
bool addScheduledTask(const String& task_name, const String& topic, 
                     ScheduledTaskCallback callback, unsigned long interval_ms, 
                     int qos = 0, bool retain = false);
```

**参数说明:**
- `task_name`: 任务唯一名称
- `topic`: MQTT发布主题
- `callback`: 数据生成回调函数
- `interval_ms`: 执行间隔（毫秒，最小1000ms）
- `qos`: MQTT QoS等级（0, 1, 2）
- `retain`: 是否为保留消息

### 任务管理
```cpp
bool removeScheduledTask(const String& task_name);           // 移除任务
bool enableScheduledTask(const String& task_name, bool enabled = true);  // 启用任务
bool disableScheduledTask(const String& task_name);         // 禁用任务
int getScheduledTaskCount() const;                          // 获取任务数量
String getScheduledTaskInfo(int index) const;              // 获取任务信息
void clearAllScheduledTasks();                             // 清除所有任务
```

## 使用示例

### 1. 基本使用

```cpp
#include "Air780EG.h"

Air780EGCore core(Serial2, 4, 5);
Air780EGMQTT mqtt(&core);

// 数据生成回调函数
String generateDeviceStatus() {
    String json = "{";
    json += "\"device_id\":\"motobox_001\",";
    json += "\"temperature\":" + String(25.5) + ",";
    json += "\"battery\":" + String(85) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    return json;
}

void setup() {
    // 初始化MQTT...
    
    // 注册定时任务 - 每30秒发布设备状态
    mqtt.addScheduledTask(
        "device_status",                    // 任务名称
        "device/motobox_001/status",        // 发布主题
        generateDeviceStatus,               // 回调函数
        30000,                             // 30秒间隔
        1,                                 // QoS 1
        false                              // 不保留
    );
}

void loop() {
    mqtt.loop();  // 自动处理定时任务
}
```

### 2. 多任务管理

```cpp
void setup() {
    // 初始化...
    
    // 心跳任务 - 每10秒
    mqtt.addScheduledTask("heartbeat", "device/heartbeat", 
                         generateHeartbeat, 10000);
    
    // 传感器数据 - 每30秒
    mqtt.addScheduledTask("sensors", "device/sensors", 
                         generateSensorData, 30000, 1);
    
    // GPS位置 - 每60秒
    mqtt.addScheduledTask("location", "device/location", 
                         generateLocationData, 60000, 1);
    
    // 系统统计 - 每5分钟
    mqtt.addScheduledTask("stats", "device/stats", 
                         generateSystemStats, 300000, 0, true);
}
```

### 3. 动态任务管理

```cpp
void onMQTTMessage(const String& topic, const String& payload) {
    if (topic.endsWith("/task_control")) {
        if (payload.indexOf("add_sensor") >= 0) {
            mqtt.addScheduledTask("new_sensor", "data/sensor", 
                                generateSensorData, 15000);
        }
        else if (payload.indexOf("remove_sensor") >= 0) {
            mqtt.removeScheduledTask("new_sensor");
        }
        else if (payload.indexOf("disable_heartbeat") >= 0) {
            mqtt.disableScheduledTask("heartbeat");
        }
    }
}
```

## 数据生成回调函数示例

### 设备状态信息
```cpp
String generateDeviceStatus() {
    String json = "{";
    json += "\"device_id\":\"" + device_id + "\",";
    json += "\"firmware\":\"v2.3.0\",";
    json += "\"uptime\":" + String(millis()) + ",";
    json += "\"free_memory\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"wifi_rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    return json;
}
```

### 传感器数据
```cpp
String generateSensorData() {
    // 读取真实传感器数据
    float temp = readTemperature();
    float humidity = readHumidity();
    float pressure = readPressure();
    
    String json = "{";
    json += "\"temperature\":" + String(temp, 2) + ",";
    json += "\"humidity\":" + String(humidity, 2) + ",";
    json += "\"pressure\":" + String(pressure, 2) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    return json;
}
```

### GPS位置数据
```cpp
String generateLocationData() {
    GPSData gps = readGPS();
    
    String json = "{";
    json += "\"latitude\":" + String(gps.lat, 6) + ",";
    json += "\"longitude\":" + String(gps.lng, 6) + ",";
    json += "\"altitude\":" + String(gps.alt, 2) + ",";
    json += "\"speed\":" + String(gps.speed, 2) + ",";
    json += "\"satellites\":" + String(gps.satellites) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";
    return json;
}
```

## 最佳实践

### 1. 任务间隔设置
- 最小间隔: 1000ms (1秒)
- 心跳任务: 10-30秒
- 传感器数据: 30-60秒
- 位置数据: 60-300秒
- 统计信息: 300-600秒

### 2. QoS选择
- QoS 0: 心跳、统计信息等不重要数据
- QoS 1: 传感器数据、位置信息等重要数据
- QoS 2: 关键配置、告警信息

### 3. 主题命名规范
```
device/{device_id}/status      # 设备状态
device/{device_id}/sensors     # 传感器数据
device/{device_id}/location    # 位置信息
device/{device_id}/stats       # 统计信息
device/{device_id}/alerts      # 告警信息
```

### 4. 错误处理
```cpp
String generateSafeData() {
    try {
        // 数据生成逻辑
        return generateRealData();
    } catch (...) {
        // 返回错误状态
        return "{\"error\":\"data_generation_failed\",\"timestamp\":" + String(millis()) + "}";
    }
}
```

## 性能考虑

- 最大任务数量: 10个
- 回调函数应尽快返回，避免阻塞
- JSON数据大小建议控制在1KB以内
- 避免在回调函数中执行耗时操作

## 调试功能

```cpp
// 打印所有任务信息
for (int i = 0; i < mqtt.getScheduledTaskCount(); i++) {
    Serial.println("Task " + String(i) + ": " + mqtt.getScheduledTaskInfo(i));
}

// 启用MQTT调试
mqtt.enableDebug(true);
```

## 注意事项

1. 只有在MQTT连接状态下才会执行定时任务
2. 任务名称必须唯一
3. 回调函数不能为空
4. 建议在setup阶段完成所有任务注册
5. 可以在运行时动态管理任务，但要注意线程安全
