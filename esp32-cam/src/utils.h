#ifndef UTILITIES_FUNCTIONS_H
#define UTILITIES_FUNCTIONS_H
#include<Arduino.h>
#include<vector>
#include<WiFi.h>
#include<HTTPClient.h>
#include<esp_camera.h>


bool connectWiFi(String &ssid, String &password, unsigned int timeout_second = 10){
    WiFi.begin(ssid, password);

    int current_led_status = digitalRead(33);
    int count = 0;
    while(WiFi.status() != WL_CONNECTED && count != timeout_second*2){
        count ++;
        digitalWrite(33, count%2);
        delay(500);
    }

    digitalWrite(33, current_led_status);
    if (WiFi.status() != WL_CONNECTED){
        WiFi.disconnect();
        return false;
    }
    return true;
}

int sendImageToServer(String server_url, camera_fb_t *fb, String &response_message, unsigned int timeout_ms_second=20000){
    HTTPClient http;
    http.begin(server_url);
    http.setTimeout(timeout_ms_second);

    http.addHeader("Content-Type", "image/jpeg");
    int http_code = http.POST(fb->buf, fb->len);
    if (http_code > 0)
        response_message = http.getString();
    http.end();
    return http_code;
}


std::vector<String> parse_tokens(String s){
    std::vector<String> tokens;
    int start = 0;
    int end = 0;
    s.trim();
    while(start < s.length()){
        if (s[start] == '"'){
            start ++;
            end = start+1;
            while(end < s.length() && s[end] != '"')
                end ++;
            tokens.push_back(s.substring(start, end));

            end++;
            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }else{
            end = start+1;
            while(end < s.length() && s[end] != ' ')
                end ++;
            tokens.push_back(s.substring(start, end));

            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }
    }
    return tokens;
}

#endif