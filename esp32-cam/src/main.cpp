#include "config.h"
#include "app.h"

App app;
void setup()
{
    Serial.begin(115200);
    app.init(
        WIFI_SSID,
        WIFI_PASSWORD,
        SERVER_URL,
        BLYNK_AUTH_TOKEN,
        BLYNK_UPDATE_INTERVAL_MS
    );
}

void loop()
{
    app.run();
}