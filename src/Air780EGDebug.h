#ifndef AIR780EG_DEBUG_H
#define AIR780EG_DEBUG_H

#include <Arduino.h>

// 日志级别定义
enum Air780EGLogLevel {
    AIR780EG_LOG_NONE = 0,
    AIR780EG_LOG_ERROR = 1,
    AIR780EG_LOG_WARN = 2,
    AIR780EG_LOG_INFO = 3,
    AIR780EG_LOG_DEBUG = 4,
    AIR780EG_LOG_VERBOSE = 5
};

class Air780EGDebug {
private:
    static Air780EGLogLevel log_level;
    static Print* output_stream;
    static bool timestamp_enabled;
    
    static const char* getLevelString(Air780EGLogLevel level);
    static void printPrefix(Air780EGLogLevel level, const char* tag);
    
public:
    // 设置日志级别
    static void setLogLevel(Air780EGLogLevel level);
    static Air780EGLogLevel getLogLevel();
    
    // 设置输出流（默认Serial）
    static void setOutputStream(Print* stream);
    
    // 启用/禁用时间戳
    static void enableTimestamp(bool enable = true);
    
    // 日志输出方法
    static void error(const char* tag, const char* format, ...);
    static void warn(const char* tag, const char* format, ...);
    static void info(const char* tag, const char* format, ...);
    static void debug(const char* tag, const char* format, ...);
    static void verbose(const char* tag, const char* format, ...);
    
    // 便捷宏定义
    #define AIR780EG_LOGE(tag, format, ...) Air780EGDebug::error(tag, format, ##__VA_ARGS__)
    #define AIR780EG_LOGW(tag, format, ...) Air780EGDebug::warn(tag, format, ##__VA_ARGS__)
    #define AIR780EG_LOGI(tag, format, ...) Air780EGDebug::info(tag, format, ##__VA_ARGS__)
    #define AIR780EG_LOGD(tag, format, ...) Air780EGDebug::debug(tag, format, ##__VA_ARGS__)
    #define AIR780EG_LOGV(tag, format, ...) Air780EGDebug::verbose(tag, format, ##__VA_ARGS__)
};

#endif // AIR780EG_DEBUG_H
