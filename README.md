# Air780EG Arduino Library

Air780EG是一个功能完整的Arduino库，专为Air780EG 4G+GNSS模块设计。该库提供了面向对象的接口，支持AT指令交互、网络管理、GNSS定位等功能，并通过智能缓存机制最大化减少AT指令调用次数。

## 特性

- **分层架构设计**：核心AT指令层、网络管理层、GNSS定位层、MQTT客户端层分离
- **智能缓存机制**：减少重复AT指令调用，提高性能
- **三重定位支持**：GNSS + LBS + WiFi 混合定位，提供更准确的位置信息
- **完整MQTT客户端**：支持SSL/TLS加密、自动重连、QoS等级、遗嘱消息
- **可配置更新频率**：支持不同功能模块的独立更新间隔设置
- **完整的调试系统**：支持多级别日志输出和时间戳
- **非阻塞设计**：通过loop()方法实现异步状态更新
- **丰富的状态信息**：提供详细的网络和定位状态信息

## 支持的功能

### 核心功能 (Air780EGCore)
- AT指令发送和响应处理
- 模块初始化和复位
- 硬件电源控制
- 串口通信管理
- URC管理器集成

### URC管理 (Air780EGURC)
- **主动上报消息处理**：自动捕获和分发URC消息
- **多种URC类型支持**：网络注册、GNSS定位、MQTT消息等
- **自定义处理器注册**：支持注册自定义URC处理函数
- **智能消息解析**：提供便捷的URC数据解析工具
- **统计和调试**：URC处理统计信息和调试输出

### 网络管理 (Air780EGNetwork)
- 网络注册状态监控
- 信号强度检测
- 运营商信息获取
- APN配置和PDP上下文管理
- IMEI/IMSI/CCID信息获取

### MQTT客户端 (Air780EGMQTT)
- **完整的MQTT协议支持**：支持MQTT 3.1.1协议
- **连接管理**：自动重连、心跳保持、连接状态监控
- **消息发布**：支持QoS 0/1/2级别的消息发布
- **消息订阅**：支持多主题订阅和通配符匹配
- **SSL/TLS加密**：支持安全连接和证书验证
- **遗嘱消息**：支持Last Will Testament配置
- **异步处理**：非阻塞的消息收发机制
- **缓存机制**：离线消息缓存和重发机制

### 定位服务 (Air780EGGNSS)
- **多模式定位支持**：GNSS + LBS + WiFi 三重定位
- **GNSS定位**：GPS/北斗/GLONASS多星座支持
- **LBS基站定位**：基于蜂窝网络的快速定位
- **WiFi定位**：基于WiFi热点的室内定位辅助
- **智能定位策略**：自动选择最优定位方式
- 实时位置、速度、航向信息
- 卫星数量和精度信息
- 可配置更新频率 (0.1Hz - 10Hz)
- UTC时间和日期信息
- 定位精度评估和质量指标

### 调试系统 (Air780EGDebug)
- 多级别日志输出 (ERROR, WARN, INFO, DEBUG, VERBOSE)
- 可配置输出流
- 时间戳支持
- 模块标签识别

## 安装

将整个`Air780EG`文件夹复制到你的Arduino项目的`lib`目录下。

```
your_project/
├── lib/
│   └── Air780EG/
│       ├── src/
│       ├── examples/
│       └── library.properties
├── src/
└── platformio.ini
```

## 快速开始

### 基本使用示例
```cpp
#include <Air780EG.h>

Air780EG air780;

void setup() {
    Serial.begin(115200);
    
    // 初始化模块
    if (air780.begin(&Serial2, 115200)) {
        Serial.println("Air780EG initialized");
        
        // 启用网络
        air780.getNetwork().enableNetwork();
        
        // 启用GNSS，设置1Hz更新频率
        air780.getGNSS().enableGNSS();
        air780.getGNSS().setUpdateFrequency(1.0);
        
        // 启用三重定位模式 (GNSS + LBS + WiFi)
        air780.getGNSS().setPositioningMode(4);  // 混合定位模式
        air780.getGNSS().enableLBS(true);
        air780.getGNSS().enableWiFi(true);
    }
}

void loop() {
    // 必须调用，维护所有功能模块
    air780.loop();
    
    // 获取缓存的数据（不会触发AT指令）
    if (air780.getGNSS().isFixed()) {
        double lat = air780.getGNSS().getLatitude();
        double lng = air780.getGNSS().getLongitude();
        Serial.printf("GPS: %.6f, %.6f\n", lat, lng);
    }
    
    if (air780.getNetwork().isNetworkRegistered()) {
        int signal = air780.getNetwork().getSignalStrength();
        Serial.printf("Signal: %d dBm\n", signal);
    }
    
    delay(1000);
}
```

### MQTT客户端示例
```cpp
#include <Air780EG.h>

Air780EG air780;

void onMqttMessage(const String& topic, const String& payload) {
    Serial.printf("Received: %s -> %s\n", topic.c_str(), payload.c_str());
}

void setup() {
    Serial.begin(115200);
    
    if (air780.begin(&Serial2, 115200)) {
        // 启用网络
        air780.getNetwork().enableNetwork();
        
        // 配置MQTT
        air780.getMQTT().setMessageCallback(onMqttMessage);
        air780.getMQTT().setKeepAlive(60);
        air780.getMQTT().enableAutoReconnect(true);
        
        // 连接MQTT服务器
        if (air780.getMQTT().connect("mqtt.example.com", 1883, "client123")) {
            air780.getMQTT().subscribe("sensors/+/data");
            Serial.println("MQTT connected and subscribed");
        }
    }
}

void loop() {
    air780.loop();
    
    // 定期发布数据
    static unsigned long lastPublish = 0;
    if (millis() - lastPublish > 30000) {  // 每30秒
        if (air780.getMQTT().isConnected()) {
            String data = "{\"temperature\":25.6,\"humidity\":60.2}";
            air780.getMQTT().publish("sensors/device1/data", data);
            lastPublish = millis();
        }
    }
    
    delay(100);
}
```

## API 参考

### Air780EG 主类

#### 初始化
```cpp
bool begin(HardwareSerial* serial, int baudrate = 115200);
bool begin(HardwareSerial* serial, int baudrate, int reset_pin, int power_pin = -1);
```

#### 主循环
```cpp
void loop(); // 必须在主循环中调用
```

#### 获取子模块
```cpp
Air780EGCore& getCore();
Air780EGNetwork& getNetwork();
Air780EGGNSS& getGNSS();
Air780EGMQTT& getMQTT();
Air780EGURC& getURC();
```

### Air780EGNetwork 网络管理

#### 网络控制
```cpp
bool enableNetwork();
bool disableNetwork();
bool isNetworkRegistered();
```

#### 信息获取
```cpp
int getSignalStrength();        // 返回dBm值
String getOperatorName();       // 运营商名称
String getNetworkType();        // 网络类型 (2G/3G/4G)
String getIMEI();              // 设备IMEI
String getIMSI();              // SIM卡IMSI
String getCCID();              // SIM卡CCID
```

#### 配置
```cpp
void setUpdateInterval(unsigned long interval_ms);  // 设置更新间隔
bool setAPN(const String& apn, const String& username = "", const String& password = "");
```

### Air780EGGNSS 定位管理

#### GNSS控制
```cpp
bool enableGNSS();
bool disableGNSS();
bool isFixed();                 // 是否已定位
```

#### 位置信息
```cpp
double getLatitude();           // 纬度
double getLongitude();          // 经度
double getAltitude();           // 海拔高度 (米)
float getSpeed();               // 速度 (km/h)
float getCourse();              // 航向角 (度)
int getSatelliteCount();        // 卫星数量
float getHDOP();                // 水平精度因子
```

#### 时间信息
```cpp
String getTimestamp();          // UTC时间
String getDate();               // UTC日期
```

#### 配置
```cpp
void setUpdateFrequency(float hz);      // 设置更新频率 (0.1-10Hz)
bool setGNSSMode(int mode);             // 设置定位模式 (1=GPS, 2=北斗, 3=GPS+北斗)
bool enableLBS(bool enable);            // 启用/禁用LBS定位
bool enableWiFi(bool enable);           // 启用/禁用WiFi定位
void setPositioningMode(int mode);      // 设置定位模式 (1=GNSS, 2=LBS, 3=WiFi, 4=混合)
```

### Air780EGMQTT 客户端

#### 连接管理
```cpp
bool connect(const String& server, int port, const String& clientId);
bool connect(const String& server, int port, const String& clientId, 
             const String& username, const String& password);
bool connectSSL(const String& server, int port, const String& clientId,
                const String& username = "", const String& password = "");
bool disconnect();
bool isConnected();
```

#### 消息发布
```cpp
bool publish(const String& topic, const String& payload, int qos = 0);
bool publish(const String& topic, const uint8_t* payload, size_t length, int qos = 0);
bool publishRetain(const String& topic, const String& payload, int qos = 0);
```

#### 消息订阅
```cpp
bool subscribe(const String& topic, int qos = 0);
bool unsubscribe(const String& topic);
void setMessageCallback(void (*callback)(const String& topic, const String& payload));
```

#### 配置选项
```cpp
void setKeepAlive(int seconds);         // 设置心跳间隔
void setCleanSession(bool clean);       // 设置清理会话
void setWillMessage(const String& topic, const String& payload, int qos = 0, bool retain = false);
void setTimeout(int seconds);           // 设置连接超时
void enableAutoReconnect(bool enable);  // 启用自动重连
```

### Air780EGDebug 调试系统

#### 日志级别配置
```cpp
Air780EG::setLogLevel(AIR780EG_LOG_INFO);    // 设置日志级别
Air780EG::setLogOutput(&Serial);             // 设置输出流
Air780EG::enableTimestamp(true);             // 启用时间戳
```

#### 日志级别
- `AIR780EG_LOG_NONE` - 无输出
- `AIR780EG_LOG_ERROR` - 仅错误
- `AIR780EG_LOG_WARN` - 警告及以上
- `AIR780EG_LOG_INFO` - 信息及以上 (默认)
- `AIR780EG_LOG_DEBUG` - 调试及以上
- `AIR780EG_LOG_VERBOSE` - 详细输出

## 性能优化

### 缓存机制
库使用智能缓存机制避免重复的AT指令调用：
- 网络状态每5秒更新一次（可配置）
- GNSS数据按设定频率更新（0.1-10Hz）
- 所有get方法返回缓存数据，不触发AT指令

### 更新频率配置
```cpp
// 网络状态更新间隔
air780.getNetwork().setUpdateInterval(10000);  // 10秒

// GNSS更新频率
air780.getGNSS().setUpdateFrequency(2.0);      // 2Hz

// 主循环间隔
air780.setLoopInterval(50);                    // 50ms
```

## 硬件连接

### 基本连接
```
ESP32-S3    Air780EG
GPIO16  ->  TXD
GPIO17  ->  RXD
GND     ->  GND
3.3V    ->  VCC
```

### 可选控制引脚
```
ESP32-S3    Air780EG
GPIO18  ->  RESET (可选)
GPIO19  ->  POWER (可选)
```

## 示例程序

库提供了多个示例程序：

- `BasicModem` - 基本功能演示
- `MqttClient` - MQTT客户端示例
- `GNSSTest` - GNSS定位测试
- `TriplePositioning` - 三重定位模式演示

## 故障排除

### 常见问题

1. **模块无响应**
   - 检查串口连接和波特率
   - 确认电源供应充足
   - 尝试硬件复位

2. **网络注册失败**
   - 检查SIM卡是否正确插入
   - 确认信号强度足够
   - 检查APN设置

3. **GNSS无法定位**
   - 确保在开阔环境下测试
   - 等待足够的冷启动时间
   - 检查天线连接
   - 尝试启用LBS辅助定位

4. **MQTT连接失败**
   - 检查网络连接状态
   - 确认服务器地址和端口正确
   - 检查用户名密码设置
   - 尝试启用自动重连功能

### 调试输出
启用详细日志输出进行问题诊断：
```cpp
Air780EG::setLogLevel(AIR780EG_LOG_DEBUG);
```

## 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

MIT 许可证允许您：
- 商业使用
- 修改代码
- 分发代码
- 私人使用

唯一要求是在所有副本中包含原始许可证和版权声明。

## 贡献

欢迎提交Issue和Pull Request来改进这个库。

## 更新日志

### v1.2.0 (最新版本)
- **重大更新**：完善定位功能，支持GNSS+LBS+WiFi三重定位
- **新增**：完整的MQTT客户端功能，支持SSL/TLS加密
- **新增**：URC管理器，支持主动上报消息处理
- **优化**：智能定位策略，自动选择最优定位方式
- **优化**：MQTT自动重连和离线消息缓存机制
- **新增**：定位精度评估和质量指标
- **新增**：MQTT遗嘱消息和QoS支持

### v1.1.0
- 新增MQTT基础功能
- 改进GNSS定位精度
- 优化缓存机制
- 增加更多调试选项

### v1.0.0
- 初始版本发布
- 支持基本AT指令交互
- 支持网络管理功能
- 支持GNSS定位功能
- 完整的调试系统
