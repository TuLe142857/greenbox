#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include "uart.h"
#include "camera.h"
#include "my_blynk.h"
#include "utils.h"

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/

class App
{
private:
    Camera camera;
    UART uart;
    MyBlynk myBlynk;

    const char *wifi_ssid;
    const char *wifi_password;

public:
    App() {};

    void init(
        const char *wifi_ssid,
        const char *wifi_password,
        const char *server_url,
        const char *blynk_auth_token);

    /**
     * @brief Set camera JPEG quality
     * @param quality 0-63 lower means higher quality(default 12)
     */
    void setCameraQuality(int quality);

    /**
     * @brief Set the camera frame size. Default FRAMESIZE_240X240
     *
     * @param fs The frame size to set.
     * - FRAMESIZE_96X96      : 96 x 96
     * - FRAMESIZE_QQVGA      : 160 x 120
     * - FRAMESIZE_QCIF       : 176 x 144
     * - FRAMESIZE_HQVGA      : 240 x 176
     * - FRAMESIZE_240X240    : 240 x 240
     * - FRAMESIZE_QVGA       : 320 x 240
     * - FRAMESIZE_CIF        : 400 x 296
     * - FRAMESIZE_HVGA       : 480 x 320
     * - FRAMESIZE_VGA        : 640 x 480
     * - FRAMESIZE_SVGA       : 800 x 600
     * - FRAMESIZE_XGA        : 1024 x 768
     * - FRAMESIZE_HD         : 1280 x 720
     * - FRAMESIZE_SXGA       : 1280 x 1024
     * - FRAMESIZE_UXGA       : 1600 x 1200
     */
    void setCameraFrameSize(framesize_t fs);

    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

void App::init(
    const char *wifi_ssid,
    const char *wifi_password,
    const char *server_url,
    const char *blynk_auth_token)
{
    Serial.println("Init App");

    this->wifi_ssid = wifi_ssid;
    this->wifi_password = wifi_password;

    Serial.println("Init camera");
    this->camera.init();

    Serial.println("Init uart");
    this->uart.init(
        server_url,

        // callback funtion capture camera
        [this](bool flash_on)
        {
            return this->camera.capture(flash_on);
        });

    Serial.println("Init blynk");
    this->myBlynk.init(
        blynk_auth_token,

        // callback funtion send data
        [this]()
        {
            // fake data
            Blynk.virtualWrite(V0, 25.3);
            Blynk.virtualWrite(V1, 22.9);
            Blynk.virtualWrite(V2, 50.5);
            Blynk.virtualWrite(V3, 90.55);
        });

    Serial.println("Connect WiFi...");
    connectWifi(this->wifi_ssid, this->wifi_password);
    digitalWrite(LED_PIN, WiFi.status() == WL_CONNECTED ? LED_ON : LED_OFF);
    Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi connected" : "Can not connect wifi");
}

void App::setCameraFrameSize(framesize_t fs)
{
    this->camera.setFrameSize(fs);
}

void App::setCameraQuality(int quality)
{
    this->camera.setQuality(quality);
}

void App::run()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Reconnect wifi...");
        connectWifi(this->wifi_ssid, this->wifi_password);
        digitalWrite(LED_PIN, WiFi.status() == WL_CONNECTED ? LED_ON : LED_OFF);
        Serial.println(WiFi.status() == WL_CONNECTED ? "WiFi reconnected" : "Can not connect wifi");
    }

    this->uart.run();

    if (WiFi.status() == WL_CONNECTED)
        this->myBlynk.run();

    delay(5);
}

#endif