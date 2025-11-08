#ifndef APP_UTILS_FUNCTION_H
#define APP_UTILS_FUNCTION_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <vector>
#include <HTTPClient.h>

/*
-----------------------------------------------------
                FUNCTION PROTOTYPES
-----------------------------------------------------
*/

void connectWifi(const char *ssid, const char *password, int timeout_second = 10);

String sendImageToServer(camera_fb_t *fb, const char *server_url);

std::vector<String> parse_tokens(String s);

/*
-----------------------------------------------------
                FUNCTION DEFINITIONS
-----------------------------------------------------
*/

String sendImageToServer(camera_fb_t *fb, const char *server_url)
{   
    Serial.printf("Send image to server %s\n", server_url);
    String response = "";
    bool check_ok = true;

    if (WiFi.status() != WL_CONNECTED)
    {
        response = "ERROR WIFI_NOT_CONNECTED";
        check_ok = false;
    }

    if(fb == nullptr){
        response = "ERROR NULL_IMG";
        check_ok = false;
    }

    if (check_ok)
    {
        HTTPClient http;
        http.begin(server_url);
        http.setTimeout(15000);
        http.addHeader("Content-Type", "image/jpeg");

        int http_code = http.POST(fb->buf, fb->len);

        if (http_code > 0)
        {
            response = "CLASS " + http.getString();
        }
        else
        {
            response = "ERROR HTTP" + String(http_code);
        }

        http.end();
    }

    return response;
}

std::vector<String> parse_tokens(String s)
{
    std::vector<String> tokens;
    int start = 0;
    int end = 0;
    s.trim();
    while (start < s.length())
    {
        if (s[start] == '"')
        {
            start++;
            end = start + 1;
            while (end < s.length() && s[end] != '"')
                end++;
            tokens.push_back(s.substring(start, end));

            end++;
            while (end < s.length() && s[end] == ' ')
                end++;
            start = end;
        }
        else
        {
            end = start + 1;
            while (end < s.length() && s[end] != ' ')
                end++;
            tokens.push_back(s.substring(start, end));

            while (end < s.length() && s[end] == ' ')
                end++;
            start = end;
        }
    }
    return tokens;
}

void connectWifi(const char *ssid, const char *password, int timeout_second)
{

    if (password == nullptr || strlen(password) == 0)
    {
        WiFi.begin(ssid);
    }
    else
    {
        WiFi.begin(ssid, password);
    }

    int count = 0;
    while (WiFi.status() != WL_CONNECTED && count != timeout_second)
    {
        count++;
        digitalWrite(LED_PIN, count%2);
        delay(500);
    }
}

#endif