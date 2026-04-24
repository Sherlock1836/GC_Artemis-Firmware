#pragma once

#include <stdint.h>
#include <esp_err.h>

enum class BLEPayloadType {
    APPLE_ACTION,
    APPLE_DEVICE,
    SAMSUNG,
    MICROSOFT,
    GOOGLE,
    FLIPPER_ZERO,
    AIRTAG,
    ALL
};

class BLESpammer {
public:
    BLESpammer();
    ~BLESpammer();

    void init();
    void startSpam(BLEPayloadType type);
    void stopSpam();

    bool isRunning() const { return running; }

private:
    bool initialized = false;
    bool running = false;
    BLEPayloadType currentType;

    static void spamTask(void *arg);
    void generateRandomMac(uint8_t* mac);
    void buildPayload(BLEPayloadType type, uint8_t* payload, uint8_t& out_len);
    
    // Task handle for FreeRTOS
    void* taskHandle = nullptr;
};
