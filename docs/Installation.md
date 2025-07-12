# Air780EG库安装指南

## 概述

Air780EG库提供两种安装方式，适用于不同的开发环境和项目需求。

## 方式一：PlatformIO依赖管理（推荐）

### 优势
- 自动版本管理
- 依赖解析
- 易于更新
- 团队协作友好

### 安装步骤

1. **在项目根目录找到 `platformio.ini` 文件**

2. **添加库依赖到配置文件**：
   ```ini
   [env:esp32dev]
   platform = espressif32
   board = esp32dev
   framework = arduino
   lib_deps = 
       https://github.com/zhoushoujianwork/Air780EG.git#v1.2.1
   ```

3. **构建项目**：
   ```bash
   pio run
   ```

### 版本控制

#### 使用特定版本（推荐）
```ini
lib_deps = 
    https://github.com/zhoushoujianwork/Air780EG.git#v1.2.1
```

#### 使用最新主分支
```ini
lib_deps = 
    https://github.com/zhoushoujianwork/Air780EG.git#main
```

#### 使用特定提交
```ini
lib_deps = 
    https://github.com/zhoushoujianwork/Air780EG.git#a90669e
```

### 完整配置示例

```ini
[env:esp32-air780eg]
platform = espressif32
board = esp32dev
framework = arduino

# 串口配置
monitor_speed = 115200
upload_speed = 921600

# 库依赖
lib_deps = 
    https://github.com/zhoushoujianwork/Air780EG.git#v1.2.1
    bblanchon/ArduinoJson@^6.21.0

# 编译选项
build_flags = 
    -DCORE_DEBUG_LEVEL=3
    -DAIR780EG_LOG_LEVEL=3

# 分区表（如果需要更大的应用空间）
board_build.partitions = huge_app.csv
```

## 方式二：手动放置到lib目录

### 优势
- 完全离线工作
- 可以修改库源码
- 不依赖网络连接
- 适合定制化开发

### 安装步骤

1. **下载库文件**：
   - 从GitHub下载：https://github.com/zhoushoujianwork/Air780EG/archive/v1.2.1.zip
   - 或者克隆仓库：`git clone https://github.com/zhoushoujianwork/Air780EG.git`

2. **解压并重命名**：
   - 解压下载的文件
   - 将文件夹重命名为 `Air780EG`

3. **放置到项目lib目录**：
   ```
   your_project/
   ├── lib/
   │   └── Air780EG/           # 库文件夹
   │       ├── src/            # 源代码
   │       │   ├── Air780EG.h
   │       │   ├── Air780EG.cpp
   │       │   ├── Air780EGCore.h
   │       │   ├── Air780EGCore.cpp
   │       │   ├── Air780EGGNSS.h
   │       │   ├── Air780EGGNSS.cpp
   │       │   ├── Air780EGMQTT.h
   │       │   ├── Air780EGMQTT.cpp
   │       │   ├── Air780EGNetwork.h
   │       │   ├── Air780EGNetwork.cpp
   │       │   ├── Air780EGDebug.h
   │       │   └── Air780EGDebug.cpp
   │       ├── examples/       # 示例代码
   │       ├── docs/           # 文档
   │       ├── library.properties
   │       └── README.md
   ├── src/
   │   └── main.cpp           # 你的主程序
   ├── include/
   ├── test/
   └── platformio.ini
   ```

4. **在代码中引入**：
   ```cpp
   #include <Air780EG.h>
   ```

### 目录结构说明

- `src/` - 库的源代码文件
- `examples/` - 使用示例
- `docs/` - 详细文档
- `library.properties` - 库的元数据信息
- `README.md` - 库的说明文档

## 验证安装

### 编译测试

创建一个简单的测试文件 `src/main.cpp`：

```cpp
#include <Arduino.h>
#include <Air780EG.h>

Air780EG air780;

void setup() {
    Serial.begin(115200);
    Serial.println("Air780EG Library Test");
    
    // 测试库是否正确引入
    Serial.println("Library imported successfully!");
}

void loop() {
    delay(1000);
}
```

### 编译命令

```bash
# PlatformIO
pio run

# 或者使用特定环境
pio run -e esp32dev
```

### 预期输出

编译成功后应该看到类似输出：
```
Processing esp32dev (platform: espressif32; board: esp32dev; framework: arduino)
...
Building in release mode
Compiling .pio/build/esp32dev/src/main.cpp.o
Compiling .pio/build/esp32dev/lib/Air780EG/Air780EG.cpp.o
...
Building .pio/build/esp32dev/firmware.bin
========================= [SUCCESS] Took X.XX seconds =========================
```

## 常见问题

### 1. 编译错误：找不到头文件

**问题**：`fatal error: Air780EG.h: No such file or directory`

**解决方案**：
- 检查库文件夹是否正确放置在 `lib/` 目录下
- 确认文件夹名称为 `Air780EG`
- 检查 `platformio.ini` 中的 `lib_deps` 配置

### 2. 版本冲突

**问题**：多个版本的库同时存在

**解决方案**：
- 删除 `lib/` 目录下的手动安装版本
- 或者移除 `platformio.ini` 中的 `lib_deps` 配置
- 清理构建缓存：`pio run -t clean`

### 3. 网络问题

**问题**：无法从GitHub下载库

**解决方案**：
- 使用方式二手动安装
- 配置代理或使用镜像源
- 下载zip文件后手动解压

### 4. 内存不足

**问题**：编译时提示内存不足

**解决方案**：
- 使用更大的分区表：`board_build.partitions = huge_app.csv`
- 优化代码，减少内存使用
- 考虑使用ESP32-S3等内存更大的芯片

## 更新库

### PlatformIO方式更新

1. **更新到最新版本**：
   ```ini
   lib_deps = 
       https://github.com/zhoushoujianwork/Air780EG.git#main
   ```

2. **更新到特定版本**：
   ```ini
   lib_deps = 
       https://github.com/zhoushoujianwork/Air780EG.git#v1.2.2
   ```

3. **清理并重新构建**：
   ```bash
   pio run -t clean
   pio run
   ```

### 手动方式更新

1. 删除旧的 `lib/Air780EG/` 文件夹
2. 下载新版本的库文件
3. 按照安装步骤重新放置

## 最佳实践

### 1. 版本管理
- 生产环境使用固定版本标签
- 开发环境可以使用最新版本
- 在项目文档中记录使用的库版本

### 2. 依赖管理
- 优先使用PlatformIO依赖管理
- 避免混合使用两种安装方式
- 定期检查和更新依赖

### 3. 团队协作
- 将 `platformio.ini` 提交到版本控制
- 不要提交 `lib/` 目录下的第三方库
- 使用 `.gitignore` 忽略构建产物

### 4. 测试验证
- 每次更新后进行编译测试
- 验证关键功能是否正常
- 在不同硬件平台上测试

## 支持的平台

- **ESP32** - 完全支持
- **ESP32-S2** - 基本支持
- **ESP32-S3** - 完全支持
- **ESP32-C3** - 基本支持

## 依赖要求

- **Arduino Framework** >= 2.0.0
- **ESP32 Core** >= 2.0.0
- **ArduinoJson** >= 6.0.0 (可选，用于JSON处理)
- **FreeRTOS** (ESP32内置)

## 技术支持

如果在安装过程中遇到问题，可以：

1. 查看项目的 [Issues](https://github.com/zhoushoujianwork/Air780EG/issues)
2. 提交新的Issue描述问题
3. 参考项目文档和示例代码
4. 联系维护者获取帮助
