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

// GPS时间信息结构
typedef struct
{
    int year;        // 年份 (如: 2025)
    int month;       // 月份 (1-12)
    int day;         // 日期 (1-31)
    int hour;        // 小时 (0-23)
    int minute;      // 分钟 (0-59)
    int second;      // 秒 (0-59)
    int millisecond; // 毫秒 (0-999)
    bool valid;      // 时间是否有效
    unsigned long last_update; // 最后更新时间
} gps_time_t;

// 缓存的GNSS数据
typedef struct
{
    double latitude;
    double longitude;
    double altitude;
    float speed;  // km/h
    float course; // 度
    int satellites; // 卫星数量
    float hdop;       // 水平精度因子
    unsigned long last_update;
    bool is_lbs_valid; // LBS数据有效性
    bool is_wifi_valid; // WiFi数据有效性
    bool is_gnss_valid; // GNSS数据有效性
    gps_time_t gps_time; // GPS时间信息
} gnss_data_t;



class Air780EGGNSS
{
private:
    static const char *TAG;

    Air780EGCore *core;

    
    unsigned long gnss_update_interval = 3000; // 默认3秒，动态调整
    unsigned long last_loop_time = 0;
    
    // 兜底定位配置
    struct FallbackConfig {
        bool enabled = false;
        unsigned long gnss_timeout = 15000;      // GNSS信号丢失超时时间(ms)
        unsigned long wifi_interval = 120000;    // WiFi定位间隔(ms)
        unsigned long lbs_interval = 60000;      // LBS定位间隔(ms)
        bool prefer_wifi_over_lbs = true;        // 是否优先使用WiFi定位
        unsigned long last_wifi_time = 0;        // 上次WiFi定位时间
        unsigned long last_lbs_time = 0;         // 上次LBS定位时间
    } fallbackConfig;
    bool gnss_enabled = false;
    bool lbs_location_enabled = false;

    // 内部方法
    void updateGNSSData();
    bool parseGNSSResponse(const String &response);
    String normalizeDate(const String &date);
    String normalizeTime(const String &time);
    void parseGPSTime(const String &date_part, const String &time_part);

public:
    Air780EGGNSS(Air780EGCore *core_instance);

    gnss_data_t gnss_data;

    // GNSS控制
    bool enableGNSS();
    bool disableGNSS();
    
    // 辅助定位方法 - 这些方法会阻塞30秒，请在单独线程或按需调用
    bool updateLBS();           // 基站定位，会阻塞最多30秒
    bool updateWIFILocation();  // WiFi定位，会阻塞最多30秒
    
    // 兜底定位配置
    void configureFallbackLocation(bool enable, unsigned long gnss_timeout,
                                  unsigned long lbs_interval, unsigned long wifi_interval,
                                  bool prefer_wifi);
    void handleFallbackLocation();
    
    unsigned long getLastLocationTime(); // 获取最后定位时间

    void loop(); // 定期更新GNSS数据

    // 获取缓存的GNSS信息
    bool isValid();
    double getLatitude();
    double getLongitude();
    double getAltitude();
    float getSpeed();
    float getCourse();
    int getSatelliteCount();
    float getHDOP();
    String getLocationType();
    
    // GPS时间获取
    gps_time_t getGPSTime(); // 获取GPS时间信息
    String getGPSTimeString(); // 获取格式化的GPS时间字符串
    bool isGPSTimeValid(); // 检查GPS时间是否有效

    // 状态查询
    bool isEnabled() const;
    unsigned long getLastUpdateTime() const;
    bool isBlockingCommandActive() const;  // 检查是否有阻塞命令正在执行

    // 调试方法
    void printGNSSInfo();
    String getRawGNSSData();
    String getLocationJSON();
};

#endif // AIR780EG_GNSS_H
