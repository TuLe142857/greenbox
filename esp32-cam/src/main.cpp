#define PRODUCTION
// #define DEBUG

#ifdef PRODUCTION
    #include "blynk_config.h"
    #include "app.h"
    App app;
    void setup()
    {
        Serial.begin(115200);
        // AppConfig::clearConfig();
        // AppConfig::writeConfig(
        //     "TuLe456",
        //     "tule1933",
        //     "server_url";
        // );
        app.init(
            BLYNK_AUTH_TOKEN,
            BLYNK_UPDATE_INTERVAL_MS
        );
    }

    void loop()
    {
        app.run();
    }
#elif defined(DEBUG)
    #include <Arduino.h>
    #include <Preferences.h>
    #include <nvs_flash.h>

    Preferences preferences;

    void initNVS() {
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            Serial.println("NVS bị lỗi hoặc hỏng. Đang xóa Flash...");
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
            Serial.println("Đã xóa và khởi tạo lại NVS!");
        }
        ESP_ERROR_CHECK(err);
    }

    void setup() {
        Serial.begin(115200);
        delay(1000);
        initNVS();
        Preferences pref;
        pref.begin("greenbox", false);
        pref.putString("check-memory", "ok");
        pref.end();

        pref.begin("greenbox", false);
        Serial.println("fix nvs: "+ pref.getString("check-memory"));
        pref.end();
    }

    void loop() {}
#endif


