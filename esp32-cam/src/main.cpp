#include <Arduino.h>
#include<WiFi.h>
#include<HTTPClient.h>
#include"utils.h"
#include"camera.h"

#define SERIAL_SPEED 115200

String ssid="H 08";
String pass = "000000000";
String server_url = "http://192.168.1.106:5000/classify";


Camera camera;
void setup() {
    Serial.begin(SERIAL_SPEED);
    pinMode(LAMP_PIN, OUTPUT);  digitalWrite(LAMP_PIN, LOW);
    pinMode(LED_PIN, OUTPUT);   digitalWrite(LED_PIN, LED_OFF);

    // Attempt to connect to WiFi. If the first attempt fails, retry one more time.
    // Timeout = default 10s
    if (!connectWiFi(ssid, pass))
        connectWiFi(ssid, pass);
    camera.init();
    

    // setup ok, turn on led
    digitalWrite(LED_PIN, LED_ON);
}



void loop() {
    if (WiFi.status() != WL_CONNECTED)
        connectWiFi(ssid, pass);

    if (Serial.available() > 0){
        std::vector<String> tokens = parse_tokens(Serial.readStringUntil('\n'));
        if(tokens.size() == 0)
            return;

        /**
         * Command: classify [--flash]
         * Return(send via uart): 
         *      - class <type of waste>
         *      - error <http error code | wifi-not-connected>
         */ 
        if (tokens[0] == "classify"){

            if (WiFi.status() != WL_CONNECTED){
                Serial.println("error wifi-not-connected");
                return;
            }

            camera_fb_t *fb = camera.capture((tokens.size() == 2 && tokens[1]=="--flash"));
            String response;
            int http_code = sendImageToServer(server_url, fb, response);

            if (http_code > 0)
                Serial.println("class " + response + " http-code " + String(http_code));
            else
                Serial.println("error " + String(http_code) + " message " + response);
            
            esp_camera_fb_return(fb);
        }
        
        /**
         * Command: server_url <url>
         * Change server url
         */
        else if(tokens[0] == "server_url" && tokens.size() == 2){
            server_url = tokens[1];
        }

        /**
         * Command: wifi <ssid> <password>
         * Change and reconnect wifi
         */
        else if(tokens[0] == "wifi" && tokens.size() == 3){
            ssid = tokens[1];
            pass = tokens[2];

            // Attempt to connect to WiFi. If the first attempt fails, retry one more time.
            // Timeout = default 10s
            if (!connectWiFi(ssid, pass))
                connectWiFi(ssid, pass);
        }
    }

    delay(50);
}
