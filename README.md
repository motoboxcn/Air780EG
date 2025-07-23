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
- **定时任务**：支持定时消息发布
- **缓存机制**：离线消息缓存和重发机制

### 定位服务 (Air780EGGNSS)
- **GNSS定位**：GPS/北斗/GLONASS多星座支持
- **LBS基站定位**：基于蜂窝网络的快速定位（手动调用）
- **WiFi定位**：基于WiFi热点的室内定位辅助（手动调用）
- **手动定位策略**：用户完全控制定位方式选择
- 实时位置、速度、航向信息
- 卫星数量和精度信息
- UTC时间和日期信息
- 定位精度评估和质量指标

### 调试系统 (Air780EGDebug)
- 多级别日志输出 (ERROR, WARN, INFO, DEBUG, VERBOSE)
- 可配置输出流
- 时间戳支持
- 模块标签识别

## 安装

Air780EG库支持两种引入方式：

### 方式一：PlatformIO依赖管理（推荐）

在你的 `platformio.ini` 文件中添加库依赖：

```ini
[env:your_environment]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    https://github.com/zhoushoujianwork/Air780EG.git#v1.2.1
```

### 方式二：手动放置到lib目录

将整个`Air780EG`文件夹复制到你的Arduino项目的`lib`目录下：

```
your_project/
├── lib/
│   └── Air780EG/
├── src/
│   └── main.cpp
└── platformio.ini
```

### 详细安装指南

更多安装选项、版本控制、常见问题解决等，请参考：[安装指南文档](docs/Installation.md)

### 系统要求

- **平台**：ESP32/ESP32-S3
- **框架**：Arduino Framework
- **编译器**：支持C++11标准
- **内存**：建议至少512KB RAM

## 快速开始

详细的使用说明和配置指南，请参考：[快速开始文档](docs/QuickStart.md)

## 文档

- [安装指南](docs/Installation.md) - 详细的安装和配置说明
- [快速开始](docs/QuickStart.md) - 基本使用流程和配置
- [定位策略](docs/LocationStrategy.md) - v1.2.1定位功能变更说明
- [异步定位](docs/AsyncLocation.md) - 异步定位功能说明（已废弃）

## 示例程序

库提供了多个示例程序：

- `BasicModem` - 基本功能演示
- `MqttClient` - MQTT客户端示例
- `GNSSTest` - GNSS定位测试
- `ManualLocationControl` - 手动定位控制演示 (v1.2.1新增)

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
bool enableLBS(bool enable);            // 启用/禁用LBS定位
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

// 主循环间隔建议
delay(50);  // 50ms
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


## 设备购买

如需购买Air780EG开发板和模块，可通过以下渠道：

![淘宝购买链接](docs/shop_taobao.png)

**推荐配置**：
- Air780EG开发板
- 4G全网通SIM卡
- GNSS天线
- 4G天线
- AT 版本
- 稳定电源适配器


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

### v1.2.2 (最新版本)
- **重大修复**：解决串口并发冲突导致的MQTT发布失败问题
- **修复**：解决GNSS响应被错误识别为MQTT消息的URC冲突
- **新增**：AT命令队列系统，统一管理串口访问
- **新增**：阻塞命令管理，WiFi定位期间暂停其他AT命令
- **优化**：智能URC识别，区分AT响应和真正的主动上报
- **架构**：职责分离，队列机制与URC机制各司其职

### v1.2.1
- **重大修改**：移除自动WiFi/LBS定位切换功能，避免串口并发冲突
- **修复**：解决MQTT定时任务延迟问题，确保按时执行
- **优化**：将定位策略控制权交给用户，提高系统稳定性
- **新增**：定位状态查询方法 `isGNSSSignalLost()`, `getLocationSource()`, `getLastLocationTime()`
- **文档**：新增定位策略说明和最佳实践指南
- **示例**：提供手动定位控制示例代码

### v1.2.0
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
