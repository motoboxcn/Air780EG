#include "Air780EGHTTP.h"

Air780EGHTTP::Air780EGHTTP(Air780EGCore* core_instance) : core(core_instance) {
}

bool Air780EGHTTP::init() {
    String response = core->sendATCommand("AT+HTTPINIT", 5000);
    http_initialized = response.indexOf("OK") >= 0;
    return http_initialized;
}

bool Air780EGHTTP::setURL(const String& url) {
    if (!http_initialized) return false;
    
    String cmd = "AT+HTTPPARA=\"URL\",\"" + url + "\"";
    String response = core->sendATCommand(cmd, 5000);
    return response.indexOf("OK") >= 0;
}

bool Air780EGHTTP::setUserAgent(const String& userAgent) {
    if (!http_initialized) return false;
    
    String cmd = "AT+HTTPPARA=\"USERDATA\",\"" + userAgent + "\"";
    String response = core->sendATCommand(cmd, 5000);
    return response.indexOf("OK") >= 0;
}

bool Air780EGHTTP::get() {
    if (!http_initialized) return false;
    
    String response = core->sendATCommand("AT+HTTPACTION=0", 30000);
    return response.indexOf("+HTTPACTION: 0,200") >= 0;
}

int Air780EGHTTP::getContentLength() {
    if (!http_initialized) return -1;
    
    String response = core->sendATCommand("AT+HTTPHEAD", 10000);
    
    int start = response.indexOf("Content-Length: ");
    if (start >= 0) {
        start += 16;
        int end = response.indexOf("\r", start);
        if (end > start) {
            return response.substring(start, end).toInt();
        }
    }
    return -1;
}

bool Air780EGHTTP::readData(uint8_t* buffer, size_t maxSize, size_t& actualSize) {
    if (!http_initialized) return false;
    
    String cmd = "AT+HTTPREAD=0," + String(maxSize);
    String response = core->sendATCommand(cmd, 10000);
    
    int dataStart = response.indexOf("+HTTPREAD: ");
    if (dataStart >= 0) {
        int lenEnd = response.indexOf("\r\n", dataStart);
        int dataLen = response.substring(dataStart + 11, lenEnd).toInt();
        
        if (dataLen > 0) {
            int contentStart = lenEnd + 2;
            actualSize = min((size_t)dataLen, maxSize);
            
            // 复制二进制数据
            for (size_t i = 0; i < actualSize && (contentStart + i) < response.length(); i++) {
                buffer[i] = response.charAt(contentStart + i);
            }
            return true;
        }
    }
    
    actualSize = 0;
    return false;
}

void Air780EGHTTP::close() {
    if (http_initialized) {
        core->sendATCommand("AT+HTTPTERM", 5000);
        http_initialized = false;
    }
}

bool Air780EGHTTP::downloadFile(const String& url, std::function<bool(uint8_t*, size_t)> writeCallback, 
                               std::function<void(int)> progressCallback) {
    if (!init()) return false;
    if (!setURL(url)) return false;
    if (!get()) return false;
    
    int totalSize = getContentLength();
    if (totalSize <= 0) return false;
    
    size_t downloaded = 0;
    uint8_t buffer[1024];
    
    while (downloaded < totalSize) {
        size_t chunkSize;
        if (!readData(buffer, sizeof(buffer), chunkSize)) {
            break;
        }
        
        if (chunkSize > 0) {
            if (!writeCallback(buffer, chunkSize)) {
                close();
                return false;
            }
            
            downloaded += chunkSize;
            
            if (progressCallback) {
                int progress = (downloaded * 100) / totalSize;
                progressCallback(progress);
            }
        }
        
        yield();
    }
    
    close();
    return downloaded == totalSize;
}
