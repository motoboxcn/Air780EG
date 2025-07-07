#ifndef AIR780EG_GNSS_H
#define AIR780EG_GNSS_H

#include <Arduino.h>
#include "Air780EGCore.h"
#include "Air780EGDebug.h"

class Air780EGGNSS {
private:
    static const char* TAG;
    
    Air780EGCore* core;
    
    // 缓存的GNSS数据
    struct GNSSData {
        bool is_fixed;
        double latitude;
        double longitude;
        double altitude;
        float speed;            // km/h
        float course;           // 度
        int satellites;
        float hdop;             // 水平精度因子
        String timestamp;       // UTC时间
        String date;            // UTC日期
        unsigned long last_update;
        bool data_valid;
    } gnss_data;
    
    unsigned long gnss_update_interval = 1000; // 1Hz默认
    unsigned long last_loop_time = 0;
    bool gnss_enabled = false;
    float update_frequency = 1.0; // Hz
    
    // 内部方法
    void updateGNSSData();
    bool parseGNSSResponse(const String& response);
    double convertCoordinate(const String& coord, const String& direction);
    
public:
    Air780EGGNSS(Air780EGCore* core_instance);
    
    // GNSS控制
    bool enableGNSS();
    bool disableGNSS();
    void loop(); // 定期更新GNSS数据
    
    // 获取缓存的GNSS信息
    bool isFixed();
    double getLatitude();
    double getLongitude();
    double getAltitude();
    float getSpeed();
    float getCourse();
    int getSatelliteCount();
    float getHDOP();
    String getTimestamp();
    String getDate();
    
    // GNSS配置
    bool setGNSSMode(int mode = 1); // 0=关闭, 1=GPS, 2=北斗, 3=GPS+北斗
    bool setUpdateRate(int rate_ms = 1000);
    
    // 配置更新频率
    void setUpdateFrequency(float hz); // 支持0.1Hz到10Hz
    float getUpdateFrequency() const;
    
    // 状态查询
    bool isEnabled() const;
    bool isDataValid() const;
    unsigned long getLastUpdateTime() const;
    
    // 调试方法
    void printGNSSInfo();
    String getRawGNSSData();
};

#endif // AIR780EG_GNSS_H
