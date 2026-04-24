#pragma once

#include <stdint.h>

enum class WiFiSpamType {
    RICKROLL,
    RANDOM_SSID
};

class WiFiSpammer {
public:
    WiFiSpammer();
    ~WiFiSpammer();

    void init();
    void startSpam(WiFiSpamType type);
    void stopSpam();

    bool isRunning() const { return running; }

private:
    bool initialized = false;
    bool running = false;
    WiFiSpamType currentType;

    static void spamTask(void *arg);
    
    // Task handle for FreeRTOS
    void* taskHandle = nullptr;
};
