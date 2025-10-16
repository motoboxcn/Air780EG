#include "Air780EG.h"

const char* Air780EG::TAG = "Air780EG";

Air780EG air780eg;

Air780EG::Air780EG() : network(&core), gnss(&core), mqtt(&core, &gnss) {
    AIR780EG_LOGI(TAG, "Air780EG library v%s initialized", AIR780EG_VERSION_STRING);
}

Air780EG::~Air780EG() {
    // 析构函数
}

bool Air780EG::begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin) {
    // 使用默认配置
    Air780EGConfig defaultConfig;
    return begin(serial, baudrate, rx_pin, tx_pin, power_pin, defaultConfig);
}

bool Air780EG::begin(HardwareSerial* serial, int baudrate, int rx_pin, int tx_pin, int power_pin, const Air780EGConfig& config) {
    AIR780EG_LOGI(TAG, "Initializing Air780EG module...");
    
    // 保存配置
    this->config = config;
    
    // 初始化核心模块
    if (!core.begin(serial, baudrate, rx_pin, tx_pin, power_pin)) {
        AIR780EG_LOGE(TAG, "Failed to initialize core module");
        return false;
    }
    
    // 等待模块稳定
    delay(2000);
    
    // 检查模块是否就绪
    if (!core.isReady()) {
        AIR780EG_LOGE(TAG, "Module not ready after initialization");
        return false;
    }
    
    // 根据配置启用功能模块
    if (config.enableGNSS) {
        AIR780EG_LOGI(TAG, "Enabling GNSS module...");
        gnss.enableGNSS();
        
        // 配置兜底定位
        if (config.enableFallbackLocation) {
            AIR780EG_LOGI(TAG, "Configuring fallback location...");
            gnss.configureFallbackLocation(
                config.enableFallbackLocation,
                config.gnss_timeout,
                config.lbs_interval,
                config.wifi_interval,
                config.prefer_wifi_over_lbs
            );
        }
    }
    
    initialized = true;
    AIR780EG_LOGI(TAG, "Air780EG module initialized successfully");
    AIR780EG_LOGI(TAG, "Features enabled - GSM:%s, MQTT:%s, GNSS:%s, Fallback:%s", 
                  config.enableGSM ? "Yes" : "No",
                  config.enableMQTT ? "Yes" : "No", 
                  config.enableGNSS ? "Yes" : "No",
                  config.enableFallbackLocation ? "Yes" : "No");
    return true;
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

    // 检查到设备是否重启过 boot.rom
    if (core.isBootRom()) {
        AIR780EG_LOGI(TAG, "Device has been restarted");
        core.initModem();
    }
    
    // 处理AT命令队列 - 这是关键的新增部分
    core.processCommands();
    
    // 调用各子模块的loop方法
    network.loop();
    gnss.loop();
    mqtt.loop();
}

bool Air780EG::isReady() {
    return initialized && core.isReady();
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

void Air780EG::setConfig(const Air780EGConfig& config) {
    this->config = config;
    AIR780EG_LOGI(TAG, "Configuration updated");
}

const Air780EGConfig& Air780EG::getConfig() const {
    return config;
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
    if (gnss.isEnabled() && gnss.isValid()) {
        gnss.printGNSSInfo();
    }
}
