#include "app.h"

App app;
void setup() {
    Serial.begin(115200);
    app.init();
}

void loop() {
    app.run();
}
