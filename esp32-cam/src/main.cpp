#include "blynk_config.h"
#include "app.h"
App app;
void setup()
{
    Serial.begin(115200);
    app.init(
        BLYNK_AUTH_TOKEN,
        BLYNK_UPDATE_INTERVAL_MS
    );
}

void loop()
{
    app.run();
}


