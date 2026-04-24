#include "WiFiSpammer.hpp"
#include <esp_wifi.h>
#include <esp_random.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#include <esp_log.h>

static const char* TAG = "WiFiSpammer";

static const char* rickroll_lyrics[] = {
    "Never gonna give you up",
    "Never gonna let you down",
    "Never gonna run around and desert you",
    "Never gonna make you cry",
    "Never gonna say goodbye",
    "Never gonna tell a lie and hurt you",
    "We've known each other for so long",
    "Your heart's been aching",
    "but you're too shy to say it",
    "Inside we both know what's been going on",
    "We know the game and we're gonna play it",
    "And if you ask me how I'm feeling",
    "Don't tell me you're too blind to see"
};
static const int RICKROLL_LINES = sizeof(rickroll_lyrics) / sizeof(rickroll_lyrics[0]);

WiFiSpammer::WiFiSpammer() {
}

WiFiSpammer::~WiFiSpammer() {
    stopSpam();
    if (initialized) {
        esp_wifi_stop();
        esp_wifi_deinit();
        
        // Remove the netif if we created it, or leave it alone if it's managed globally.
        // It's safest to just stop and deinit the driver to free the large memory buffers.
        initialized = false;
    }
}

#include <esp_netif.h>
#include <esp_event.h>

void WiFiSpammer::init() {
    if (initialized) return;

    // Ensure networking core is initialized
    esp_netif_init();
    esp_event_loop_create_default();
    
    // Create a default STA netif if it doesn't exist
    // It's safe to call if already created elsewhere, though it might leak a small struct or warn
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!sta_netif) {
        esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_wifi_init(&cfg);
    if (err != ESP_OK && err != ESP_ERR_WIFI_STATE) {
        ESP_LOGE(TAG, "Failed to init wifi: %d", err);
    }
    
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    
    // Use WIFI_MODE_STA and transmit over WIFI_IF_STA
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_err_t start_err = esp_wifi_start();
    if (start_err != ESP_OK && start_err != ESP_ERR_WIFI_STATE) {
        ESP_LOGE(TAG, "Failed to start wifi: %d", start_err);
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Wi-Fi Spammer Initialized");
}

void WiFiSpammer::spamTask(void *arg) {
    WiFiSpammer* spammer = static_cast<WiFiSpammer*>(arg);

    const uint8_t beacon_template[] = {
        0x80, 0x00, 0x00, 0x00, // Frame Control, Duration
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination address (Broadcast)
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source address - overwritten
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // BSSID - overwritten
        0xc0, 0x6c, // Seq number
        0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
        0x64, 0x00, // Beacon interval
        0x01, 0x04, // Capabilities
        0x00 // SSID Tag Number
    };

    const uint8_t post_base[] = {
        0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Supported Rates
        0x03, 0x01, 0x04, // DS Parameter Set (Channel 4)
        0x30, 0x18, 0x01, 0x00, 0x00, 0x0f, 0xac, // RSN Info
        0x02, 0x02, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 
        0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00
    };

    uint8_t frame[128];
    int line_index = 0;

    while (spammer->running) {
        memset(frame, 0, sizeof(frame));
        int offset = 0;

        // Copy template
        memcpy(frame, beacon_template, sizeof(beacon_template));
        offset += sizeof(beacon_template);

        // Generate Random MAC
        uint32_t r1 = esp_random();
        uint32_t r2 = esp_random();
        uint8_t mac[6] = {
            (uint8_t)((r1 & 0xFF) | 0x02), // Local bit set
            (uint8_t)((r1 >> 8) & 0xFF),
            (uint8_t)((r1 >> 16) & 0xFF),
            (uint8_t)((r1 >> 24) & 0xFF),
            (uint8_t)(r2 & 0xFF),
            (uint8_t)((r2 >> 8) & 0xFF)
        };

        // Insert MAC into Source and BSSID
        memcpy(&frame[10], mac, 6);
        memcpy(&frame[16], mac, 6);

        // Determine SSID
        char ssid[33] = {0};
        if (spammer->currentType == WiFiSpamType::RICKROLL) {
            strncpy(ssid, rickroll_lyrics[line_index], 32);
            line_index = (line_index + 1) % RICKROLL_LINES;
        } else {
            // Random SSID
            const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
            int len = (esp_random() % 16) + 6; // Length between 6 and 22
            for(int i = 0; i < len; i++) {
                ssid[i] = charset[esp_random() % (sizeof(charset) - 1)];
            }
        }

        uint8_t ssid_len = strlen(ssid);
        
        // Add SSID length
        frame[offset++] = ssid_len;

        // Add SSID string
        memcpy(&frame[offset], ssid, ssid_len);
        offset += ssid_len;

        // Add Post Base
        memcpy(&frame[offset], post_base, sizeof(post_base));
        offset += sizeof(post_base);

        // Transmit frame
        // WIFI_IF_STA = 0
        esp_wifi_80211_tx((wifi_interface_t)0, frame, offset, false);

        // Delay to prevent watchdog panic and allow stack processing
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    vTaskDelete(NULL);
}

void WiFiSpammer::startSpam(WiFiSpamType type) {
    if (running) return;
    currentType = type;
    running = true;
    xTaskCreate(spamTask, "wifi_spam", 4096, this, 5, (TaskHandle_t*)&taskHandle);
    ESP_LOGI(TAG, "Wi-Fi Spam Started");
}

void WiFiSpammer::stopSpam() {
    if (!running) return;
    running = false;
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for task to exit
    ESP_LOGI(TAG, "Wi-Fi Spam Stopped");
}
