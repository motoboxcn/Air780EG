#ifndef AIR780EG_HTTP_H
#define AIR780EG_HTTP_H

#include <Arduino.h>
#include "Air780EGCore.h"

class Air780EGHTTP {
private:
    Air780EGCore* core;
    bool http_initialized = false;
    
public:
    Air780EGHTTP(Air780EGCore* core_instance);
    
    // HTTP基础操作
    bool init();
    bool setURL(const String& url);
    bool setUserAgent(const String& userAgent);
    bool get();
    int getContentLength();
    bool readData(uint8_t* buffer, size_t maxSize, size_t& actualSize);
    void close();
    
    // 便捷方法
    bool downloadFile(const String& url, std::function<bool(uint8_t*, size_t)> writeCallback, 
                     std::function<void(int)> progressCallback = nullptr);
};

#endif
