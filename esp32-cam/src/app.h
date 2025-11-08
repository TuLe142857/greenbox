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
    const char *server_url;

    void initCommandHandlers();

public:
    App() {};

    void init(
        const char *wifi_ssid,
        const char *wifi_password,
        const char *server_url,
        const char *blynk_auth_token);

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
    this->wifi_ssid = wifi_ssid;
    this->wifi_password = wifi_password;
    this->server_url = server_url;

    this->camera.init();

    this->uart.init();

    this->myBlynk.init(blynk_auth_token);

    this->myBlynk.addTimerFunction(
        // request get data
        [this]()
        {
            Serial.println("GET_GARBAGE_BIN_LEVEL");
        });

        
    this->initCommandHandlers();
    
    connectWifi(this->wifi_ssid, this->wifi_password);
}

void App::initCommandHandlers()
{
    // return "CLASS " + <response>
    this->uart.addCommandHandler(
        "CLASSIFY",
        [this](std::vector<String> tokens){
            bool flash_on = tokens.size() > 1 && tokens[1] == "--flash";
            camera_fb_t * fb = this->camera.capture(flash_on);

            String response = sendImageToServer(fb, this->server_url);
            Serial.println(response);

            esp_camera_fb_return(fb);
        }
    );

    this->uart.addCommandHandler(
        "GARBAGE_BIN_LEVEL",
        [this](std::vector<String> tokens)
        {
            if (tokens.size() == 5)
            {
                Blynk.virtualWrite(V0, tokens[1].toFloat());
                Blynk.virtualWrite(V1, tokens[2].toFloat());
                Blynk.virtualWrite(V2, tokens[3].toFloat());
                Blynk.virtualWrite(V3, tokens[4].toFloat());
            }
            else
            {
                Blynk.virtualWrite(V0, 25);
                Blynk.virtualWrite(V1, 25);
                Blynk.virtualWrite(V2, 50);
                Blynk.virtualWrite(V3, 50);
            }
        });

}

void App::run()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        connectWifi(this->wifi_ssid, this->wifi_password);
    }

    this->uart.run();
    
    if (WiFi.status() == WL_CONNECTED)
        this->myBlynk.run();

    delay(5);
}

#endif