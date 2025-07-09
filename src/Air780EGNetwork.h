#ifndef AIR780EG_NETWORK_H
#define AIR780EG_NETWORK_H

#include <Arduino.h>
#include "Air780EGCore.h"
#include "Air780EGDebug.h"

class Air780EGNetwork {
private:
    static const char* TAG;
    
    Air780EGCore* core;
    
    // 缓存的网络状态
    struct NetworkStatus {
        bool is_registered;
        int signal_strength;        // dBm
        String operator_name;
        String network_type;        // 2G/3G/4G
        String imei;
        String imsi;
        String ccid;               // SIM卡ID
        unsigned long last_update;
        bool data_valid;
    } network_status;
    
    unsigned long network_update_interval = 5000; // 5秒更新一次
    unsigned long last_loop_time = 0;
    bool network_enabled = false;
    
    // 内部方法
    void updateNetworkStatus();
    void updateSignalStrength();
    void updateRegistrationStatus();
    void updateOperatorInfo();
    void updateModuleInfo();
    
public:
    Air780EGNetwork(Air780EGCore* core_instance);
    
    // 网络控制
    bool enableNetwork();
    bool disableNetwork();
    void loop(); // 定期更新网络状态
    
    // 获取缓存的网络信息
    bool isNetworkRegistered();
    int getSignalStrength(); // 获取信号强度
    String getOperatorName();
    String getNetworkType();
    String getIMEI();
    String getIMSI();
    String getCCID();
    
    // 网络配置
    bool setAPN(const String& apn, const String& username = "", const String& password = "");
    bool activatePDP();
    bool deactivatePDP();
    bool isPDPActive();
    
    // 配置更新间隔
    void setUpdateInterval(unsigned long interval_ms);
    unsigned long getUpdateInterval() const;
    
    // 状态查询
    bool isEnabled() const;
    bool isDataValid() const;
    unsigned long getLastUpdateTime() const;
    
    // 调试方法
    void printNetworkInfo();
};

#endif // AIR780EG_NETWORK_H
