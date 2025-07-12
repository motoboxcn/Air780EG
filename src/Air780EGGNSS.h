#ifndef AIR780EG_GNSS_H
#define AIR780EG_GNSS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Air780EGCore.h"
#include "Air780EGDebug.h"

/*
https://docs.openluat.com/air780eg/at/app/at_command/#gps
只有这几个功能
GPS相关命令
AT+CGNSPWRGPS 开关
AT+CGNSINF读取 GNSS 信息
AT+CGNSURC打开 GNSS URC 上报
AT+CGNSTST将读取到的 GNSS 数据发送到 AT 口
AT+CGNSCMD给 GNSS 发送控制命令
AT+CGNSAID设置辅助定位
AT+CGNSDEL删除 EPO 文件
AT+CGNSSEQ定义 NMEA 解析
*/

class Air780EGGNSS
{
private:
    static const char *TAG;

    Air780EGCore *core;

    // 缓存的GNSS数据
    struct GNSSData
    {
        bool is_fixed; // 运行状态
        double latitude;
        double longitude;
        double altitude;
        float speed;  // km/h
        float course; // 度
        int satellites;
        float hdop;       // 水平精度因子
        String timestamp; // UTC时间
        String date;      // UTC日期
        unsigned long last_update;
        String location_type; // 定位类型：LBS、WIFI、GNSS
        bool data_valid; // 数据有效性，定位是否获取到
    } gnss_data;
    unsigned long gnss_update_interval = 100; // 500ms
    unsigned long last_loop_time = 0;
    bool gnss_enabled = false;
    bool lbs_location_enabled = false;

    // 内部方法
    void updateGNSSData();
    bool parseGNSSResponse(const String &response);
    String normalizeDate(const String &date);
    String normalizeTime(const String &time);

public:
    Air780EGGNSS(Air780EGCore *core_instance);

    // GNSS控制
    bool enableGNSS();
    bool disableGNSS();
    
    // 辅助定位方法 - 这些方法会阻塞30秒，请在单独线程或按需调用
    bool updateLBS();           // 基站定位，会阻塞最多30秒
    bool updateWIFILocation();  // WiFi定位，会阻塞最多30秒
    
    // 定位状态查询
    bool isGNSSSignalLost();    // 检查GNSS信号是否丢失
    String getLocationSource(); // 获取当前位置来源 ("GNSS", "WiFi", "LBS")
    unsigned long getLastLocationTime(); // 获取最后定位时间

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

    // 状态查询
    bool isEnabled() const;
    bool isDataValid() const;
    unsigned long getLastUpdateTime() const;

    // 调试方法
    void printGNSSInfo();
    String getRawGNSSData();
    String getLocationJSON();
};

#endif // AIR780EG_GNSS_H
