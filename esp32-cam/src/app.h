#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include <HTTPClient.h>

#include "uart.h"
#include "camera.h"
#include "my_blynk.h"
#include "config/app_config.h"
#include "config/wifi_config_portal.h"
#include "utils.h"

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/
enum AppState{
    NORMAL,
    CONFIG_MODE,
    WIFI_CONNECTING,
    WIFI_DISCONNECTED
};

class App
{
private:
    int resetButtonPin;

    AppState current_state;
    AppState previous_state;

    Camera camera;
    UART uart;
    MyBlynk myBlynk;

    String wifi_ssid;
    String wifi_password;
    String server_url;

    unsigned long wifi_connecting_at=0;
    unsigned long wifi_disconnected_at=0;

    void initCommandHandlers();
    void setState(AppState state);
    void sendStateToArduino();
    void fetchGarbageBinLevel();
    bool sendImageToServer(camera_fb_t *fb, String &response, String &error);
public:
    App();
    void init(
        const char *blynk_auth_token,
        unsigned long blynk_update_interval_ms = 15UL * 60UL * 1000UL // 15 minutes
    );
    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

// Blynk on update data request from app
BLYNK_WRITE(V5){
    int x = param.asInt();
    if(x){
        Serial.println("GET_GARBAGE_BIN_LEVEL");
        Blynk.virtualWrite(V5, 0);
    };
}

App::App():
    current_state(AppState::NORMAL),
    previous_state(AppState::NORMAL),
    resetButtonPin(2)
{}

void App::init(
    const char *blynk_auth_token,
    unsigned long blynk_update_interval_ms)
{
    pinMode(this->resetButtonPin, INPUT_PULLUP);
    
    if(! AppConfig::readConfig(this->wifi_ssid, this->wifi_password, this->server_url)){
        this->setState(AppState::CONFIG_MODE);
    }else{
        this->setState(AppState::WIFI_CONNECTING);
    }
    

    this->camera.init();

    this->uart.init();

    this->myBlynk.init(blynk_auth_token);
    this->myBlynk.addTimerFunction(
        [this]()
        {
            this->fetchGarbageBinLevel();
        },
        blynk_update_interval_ms
    );

    this->initCommandHandlers();

    digitalWrite(LED_PIN, LED_ON);
}

void App::run()
{
    switch (this->current_state){
        case AppState::CONFIG_MODE:
        {
            WiFiConfigPortal wcp;
            wcp.init("GreenBox Setup");
            wcp.run([this](){
                this->uart.run();
            });
            AppConfig::readConfig(this->wifi_ssid, this->wifi_password, this->server_url);
            this->setState(AppState::WIFI_CONNECTING);
            break;
        }

        case AppState::NORMAL:
        {
            if(WiFi.status() != WL_CONNECTED){
                this->setState(AppState::WIFI_CONNECTING);
                break;
            }
            break;
        }

        case AppState::WIFI_CONNECTING:{
            if(WiFi.status() == WL_CONNECTED){
                this->setState(AppState::NORMAL);
                break;
            }

            static const unsigned long connecting_timeout_ms = 15000;
            unsigned long now = millis();
            if(this->wifi_connecting_at == 0 || (now-this->wifi_connecting_at) > connecting_timeout_ms){
                this->setState(AppState::WIFI_DISCONNECTED);
            }
            break;
        }

        case AppState::WIFI_DISCONNECTED:
        {   
            static const unsigned long retry_connectting_after = 3000;
            unsigned long now = millis();
            if((now - this->wifi_disconnected_at) > retry_connectting_after){
                this->setState(AppState::WIFI_CONNECTING);
            }
            break;
        }
    }

    // check reset button
    if(digitalRead(this->resetButtonPin)==LOW){
        AppConfig::clearConfig();
        this->setState(AppState::CONFIG_MODE);
    }

    this->uart.run();

    if (WiFi.status() == WL_CONNECTED)
        this->myBlynk.run();

    delay(1);
}

void App::initCommandHandlers()
{
    this->uart.addCommandHandler(
        "PING",
        [this](std::vector<String> tokens){
            this->sendStateToArduino();
        }
    );

    // return "CLASS " + <response> or "ERROR " + error
    this->uart.addCommandHandler(
        "CLASSIFY",
        [this](std::vector<String> tokens)
        {
            if(this->current_state != AppState::NORMAL){
                return;
            }
            
            bool flash_on = tokens.size() > 1 && tokens[1] == "--flash";
            camera_fb_t *fb = this->camera.capture(flash_on);
            Serial.println("CLASSIFYING");
            
            String response, error;
            if (this->sendImageToServer(fb, response, error)){
                Serial.println("CLASS " + response);
                
                // request get bin level after classify
                this->fetchGarbageBinLevel();
            }else{
                Serial.println("ERROR " + error);
            }
            esp_camera_fb_return(fb);
        });

    this->uart.addCommandHandler(
        "GARBAGE_BIN_LEVEL",
        [this](std::vector<String> tokens)
        {
            if(this->current_state != AppState::NORMAL)
                return;

            if (tokens.size() >= 5)
            {
                this->myBlynk.updateBinLevel(
                    tokens[1].toFloat(),
                    tokens[2].toFloat(),
                    tokens[3].toFloat(),
                    tokens[4].toFloat());
            }
        });
}

void App::sendStateToArduino(){
    Serial.print("ESP_STATUS ");
    Serial.println(this->current_state);
}

void App::setState(AppState state){
    if (state == this->current_state)
        return;
    
    this->previous_state = this->current_state;
    this->current_state = state;

    // send app state to arduino
    this->sendStateToArduino();

    switch (this->current_state){
        case AppState::NORMAL:
        {   
            this->fetchGarbageBinLevel();
            break;
        }
        case AppState::CONFIG_MODE:{
            break;
        }
        case AppState::WIFI_CONNECTING:{
            if(this->wifi_password.length() == 0 || this->wifi_password == nullptr){
                WiFi.begin(this->wifi_ssid);
            }else{
                WiFi.begin(this->wifi_ssid, this->wifi_password);
            }
            this->wifi_connecting_at = millis();
            break;
        }
        case AppState::WIFI_DISCONNECTED:{
            this->wifi_disconnected_at = millis();
            break;
        }
    }
}

void App::fetchGarbageBinLevel(){
    Serial.println("GET_GARBAGE_BIN_LEVEL");
}

bool App::sendImageToServer(camera_fb_t *fb, String &response, String &error){
    if(fb == nullptr){
        error = "NULL_IMG";
        return false;
    }

    HTTPClient http;
    http.begin(this->server_url);
    http.setTimeout(15000);
    http.addHeader("Content-Type", "image/jpeg");
    int http_code = http.POST(fb->buf, fb->len);

    if(http_code > 0){
        response = http.getString();
    }else{
        error = "HTTP_ERROR_" + String(http_code);
    }

    http.end();
    return http_code > 0;
}

#endif