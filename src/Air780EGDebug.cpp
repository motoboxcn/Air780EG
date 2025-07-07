#include "Air780EGDebug.h"
#include <stdarg.h>

// 静态成员初始化
Air780EGLogLevel Air780EGDebug::log_level = AIR780EG_LOG_INFO;
Print* Air780EGDebug::output_stream = &Serial;
bool Air780EGDebug::timestamp_enabled = true;

void Air780EGDebug::setLogLevel(Air780EGLogLevel level) {
    log_level = level;
}

Air780EGLogLevel Air780EGDebug::getLogLevel() {
    return log_level;
}

void Air780EGDebug::setOutputStream(Print* stream) {
    if (stream) {
        output_stream = stream;
    }
}

void Air780EGDebug::enableTimestamp(bool enable) {
    timestamp_enabled = enable;
}

const char* Air780EGDebug::getLevelString(Air780EGLogLevel level) {
    switch (level) {
        case AIR780EG_LOG_ERROR:   return "ERROR";
        case AIR780EG_LOG_WARN:    return "WARN ";
        case AIR780EG_LOG_INFO:    return "INFO ";
        case AIR780EG_LOG_DEBUG:   return "DEBUG";
        case AIR780EG_LOG_VERBOSE: return "VERB ";
        default:                   return "UNKN ";
    }
}

void Air780EGDebug::printPrefix(Air780EGLogLevel level, const char* tag) {
    if (!output_stream) return;
    
    if (timestamp_enabled) {
        output_stream->printf("[%8lu] ", millis());
    }
    
    output_stream->printf("[%s] [%s] ", getLevelString(level), tag);
}

void Air780EGDebug::error(const char* tag, const char* format, ...) {
    if (log_level < AIR780EG_LOG_ERROR || !output_stream) return;
    
    printPrefix(AIR780EG_LOG_ERROR, tag);
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output_stream->println(buffer);
}

void Air780EGDebug::warn(const char* tag, const char* format, ...) {
    if (log_level < AIR780EG_LOG_WARN || !output_stream) return;
    
    printPrefix(AIR780EG_LOG_WARN, tag);
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output_stream->println(buffer);
}

void Air780EGDebug::info(const char* tag, const char* format, ...) {
    if (log_level < AIR780EG_LOG_INFO || !output_stream) return;
    
    printPrefix(AIR780EG_LOG_INFO, tag);
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output_stream->println(buffer);
}

void Air780EGDebug::debug(const char* tag, const char* format, ...) {
    if (log_level < AIR780EG_LOG_DEBUG || !output_stream) return;
    
    printPrefix(AIR780EG_LOG_DEBUG, tag);
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output_stream->println(buffer);
}

void Air780EGDebug::verbose(const char* tag, const char* format, ...) {
    if (log_level < AIR780EG_LOG_VERBOSE || !output_stream) return;
    
    printPrefix(AIR780EG_LOG_VERBOSE, tag);
    
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    output_stream->println(buffer);
}
