#ifndef UART_H
#define UART_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <functional>

#include "camera.h"
#include "utils.h"
#include "uart.h"

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/

class UART
{
private:
    std::function<camera_fb_t *(bool flash_on)> capture;
    const char *server_url;

public:
    void init(const char *server_url, std::function<camera_fb_t *(bool flash_on)> captureCallback);
    void classifyGarbage(bool flash_on = false);
    String runCommand(String command);
    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

void UART::init(const char *server_url, std::function<camera_fb_t *(bool flash_on)> captureCallback)
{
    this->server_url = server_url;
    this->capture = captureCallback;
}

String UART::runCommand(String command)
{
    Serial.println(command);
    return Serial.readStringUntil('\n');
}

void UART::classifyGarbage(bool flash_on)
{
    camera_fb_t *fb = this->capture(flash_on);

    // SEND IMAGE TO SERVER
    Serial.printf("Send image to server %s\n", this->server_url);

    bool run_http = true;
    String response;
    if (WiFi.status() != WL_CONNECTED)
    {
        response = "ERROR WIFI_NOT_CONNECTED";
        run_http = false;
    }

    if (run_http)
    {
        HTTPClient http;
        http.begin(this->server_url);
        http.setTimeout(15000);
        http.addHeader("Content-Type", "image/jpeg");
        int http_code = http.POST(fb->buf, fb->len);
        if (http_code > 0)
            response = http.getString();
        else
        {
            response = "ERROR HTTP" + String(http_code);
            http.end();
        }
    }

    esp_camera_fb_return(fb);
    Serial.println(response);
}

void UART::run()
{
    if (!Serial.available())
        return;

    std::vector<String> tokens = parse_tokens(Serial.readStringUntil('\n'));

    Serial.printf("Get %d tokens\n", tokens.size());
    for (String tk : tokens)
    {
        Serial.println(tk);
    }
    if (tokens[0] == "CLASSIFY")
    {
        bool flash_on = tokens.size() > 1 && tokens[1] == "--flash";
        this->classifyGarbage(flash_on);
    }
}

#endif