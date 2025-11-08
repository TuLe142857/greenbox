#ifndef APP_UTILS_FUNCTION_H
#define APP_UTILS_FUNCTION_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <vector>

/*
-----------------------------------------------------
                FUNCTION PROTOTYPES
-----------------------------------------------------
*/

void connectWifi(const char *ssid, const char *password, int timeout_second = 10);
std::vector<String> parse_tokens(String s);

/*
-----------------------------------------------------
                FUNCTION DEFINITIONS
-----------------------------------------------------
*/

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
        delay(500);
    }
}

#endif