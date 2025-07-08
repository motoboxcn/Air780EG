#include "Air780EG.h"

const char* Air780EG::TAG = "Air780EG";

Air780EG air780eg;

Air780EG::Air780EG() : network(&core), gnss(&core) {
    AIR780EG_LOGI(TAG, "Air780EG library v%s initialized", AIR780EG_VERSION_STRING);
}

Air780EG::~Air780EG() {
    // 析构函数
}

bool Air780EG::begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin) {
    AIR780EG_LOGI(TAG, "Initializing Air780EG module...");
    
    // 初始化核心模块
    if (!core.begin(serial, baudrate, rx_pin, tx_pin, power_pin)) {
        AIR780EG_LOGE(TAG, "Failed to initialize core module");
        return false;
    }
    
    // 设置URC管理器
    core.setURCManager(&urc_manager);
    
    // 设置URC处理器
    setupURCHandlers();
    
    // 等待模块稳定
    delay(2000);
    
    // 检查模块是否就绪
    if (!core.isReady()) {
        AIR780EG_LOGE(TAG, "Module not ready after initialization");
        return false;
    }
    
    initialized = true;
    AIR780EG_LOGI(TAG, "Air780EG module initialized successfully");
    
    return true;
}

void Air780EG::setupURCHandlers() {
    AIR780EG_LOGD(TAG, "Setting up URC handlers...");
    
    // 网络注册URC处理
    urc_manager.onNetworkRegistration([this](const URCData& urc) {
        AIR780EG_LOGV(TAG, "Network registration URC: %s", urc.raw_data.c_str());
        // 网络模块会通过定时查询获取状态，这里只记录日志
        auto reg = URCHelper::parseNetworkRegistration(urc);
        AIR780EG_LOGD(TAG, "Network status: %s", reg.getStatusString().c_str());
    });
    
    // GNSS信息URC处理
    urc_manager.onGNSSInfo([this](const URCData& urc) {
        AIR780EG_LOGV(TAG, "GNSS info URC: %s", urc.raw_data.c_str());
        // GNSS模块会通过定时查询获取数据，这里只记录日志
        auto gnss_info = URCHelper::parseGNSSInfo(urc);
        if (gnss_info.isValid()) {
            AIR780EG_LOGD(TAG, "GNSS fix: %.6f, %.6f", gnss_info.latitude, gnss_info.longitude);
        }
    });
    
    // MQTT消息URC处理
    urc_manager.onMQTTMessage([this](const URCData& urc) {
        AIR780EG_LOGI(TAG, "MQTT message URC: %s", urc.raw_data.c_str());
        auto mqtt_msg = URCHelper::parseMQTTMessage(urc);
        if (mqtt_msg.isValid()) {
            AIR780EG_LOGI(TAG, "MQTT: Topic=%s, Payload=%s", 
                         mqtt_msg.topic.c_str(), mqtt_msg.payload.c_str());
        }
    });
    
    // 错误报告URC处理
    urc_manager.onErrorReport([this](const URCData& urc) {
        AIR780EG_LOGW(TAG, "Error report URC: %s", urc.raw_data.c_str());
    });
    
    AIR780EG_LOGD(TAG, "URC handlers setup completed");
}

void Air780EG::loop() {
    if (!initialized) {
        AIR780EG_LOGI(TAG, "Air780EG module not initialized");
        return;
    }
    
    unsigned long current_time = millis();
    
    // 控制主循环频率
    if (current_time - last_loop_time < loop_interval) {
        return;
    }
    
    last_loop_time = current_time;
    
    // 调用核心模块的loop方法（处理URC）
    core.loop();
    
    // 调用各子模块的loop方法
    network.loop();
    gnss.loop();
}

bool Air780EG::isReady() {
    return initialized && core.isReady();
}

void Air780EG::reset() {
    AIR780EG_LOGI(TAG, "Resetting Air780EG module...");
    
    // 发送重启命令
    core.sendATCommand("AT+CFUN=1,1", 5000);
    
    // 等待模块重启
    delay(3000);
    
    // 重置状态
    initialized = core.isInitialized();
    
    if (initialized) {
        AIR780EG_LOGI(TAG, "Module reset successfully");
    } else {
        AIR780EG_LOGE(TAG, "Module reset failed");
    }
}

void Air780EG::powerOn() {
    AIR780EG_LOGI(TAG, "Powering on Air780EG module...");
    core.powerOn();
}

void Air780EG::powerOff() {
    AIR780EG_LOGI(TAG, "Powering off Air780EG module...");
    
    // 先禁用各功能模块
    if (gnss.isEnabled()) {
        gnss.disableGNSS();
    }
    
    if (network.isEnabled()) {
        network.disableNetwork();
    }
    
    // 然后关闭电源
    core.powerOff();
    initialized = false;
}

void Air780EG::setLoopInterval(unsigned long interval_ms) {
    loop_interval = interval_ms;
    AIR780EG_LOGD(TAG, "Loop interval set to %lu ms", interval_ms);
}

unsigned long Air780EG::getLoopInterval() const {
    return loop_interval;
}

void Air780EG::setLogLevel(Air780EGLogLevel level) {
    Air780EGDebug::setLogLevel(level);
    AIR780EG_LOGI(TAG, "Log level set to %d", level);
}

void Air780EG::setLogOutput(Print* stream) {
    Air780EGDebug::setOutputStream(stream);
    AIR780EG_LOGI(TAG, "Log output stream configured");
}

void Air780EG::enableTimestamp(bool enable) {
    Air780EGDebug::enableTimestamp(enable);
    AIR780EG_LOGI(TAG, "Timestamp %s", enable ? "enabled" : "disabled");
}

bool Air780EG::isInitialized() const {
    return initialized;
}

String Air780EG::getVersion() {
    return String(AIR780EG_VERSION_STRING);
}

void Air780EG::printVersion() {
    AIR780EG_LOGI(TAG, "Air780EG Library Version: %s", AIR780EG_VERSION_STRING);
    AIR780EG_LOGI(TAG, "Build Date: %s %s", __DATE__, __TIME__);
}

void Air780EG::printStatus() {
    AIR780EG_LOGI(TAG, "=== Air780EG Status ===");
    AIR780EG_LOGI(TAG, "Library Version: %s", AIR780EG_VERSION_STRING);
    AIR780EG_LOGI(TAG, "Initialized: %s", initialized ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Core Ready: %s", core.isReady() ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Network Enabled: %s", network.isEnabled() ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "GNSS Enabled: %s", gnss.isEnabled() ? "Yes" : "No");
    AIR780EG_LOGI(TAG, "Loop Interval: %lu ms", loop_interval);
    AIR780EG_LOGI(TAG, "Uptime: %lu ms", millis());
    AIR780EG_LOGI(TAG, "=====================");
    
    // 显示网络状态
    if (network.isEnabled() && network.isDataValid()) {
        network.printNetworkInfo();
    }
    
    // 显示GNSS状态
    if (gnss.isEnabled() && gnss.isDataValid()) {
        gnss.printGNSSInfo();
    }
}
