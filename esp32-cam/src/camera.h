#ifndef ESP32_DEFAULT_CAMERA_H
#define ESP32_DEFAULT_CAMERA_H

#include<Arduino.h>
#include<esp_camera.h>

/**
 * These default pin configs is from:
 * https://github.com/easytarget/esp32-cam-webserver/blob/master/camera_pins.h
 */
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_PIN           33 // Status led
#define LED_ON           LOW // - Pin is inverted.
#define LED_OFF         HIGH //
#define LAMP_PIN           4 // LED FloodLamp.

class Camera{
private:
    camera_fb_t *fb;
public:
    Camera(){};

    /*
    Default config:
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 12;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
     */
    bool init();

    
    void changeFrameSize(framesize_t fs);

    //Quality of JPEG output. 0-63 lower means higher quality
    void changeQuality(int quality);

    camera_fb_t * capture(bool flash_on = false);
};

bool Camera::init(){
    camera_config_t config;

    // can not modify
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;

    // camera frame option, can modify
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 12;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.fb_count = 1;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    return esp_camera_init(&config) == ESP_OK;
}

void Camera::changeFrameSize(framesize_t fs){
    sensor_t * camera_sensor = esp_camera_sensor_get();
    camera_sensor->set_framesize(camera_sensor, fs);
}

void Camera::changeQuality(int quality){
    if (!(quality >= 0 && quality < 64))
        return;
    sensor_t * camera_sensor = esp_camera_sensor_get();
    camera_sensor->set_quality(camera_sensor, quality);
}

camera_fb_t * Camera::capture(bool flash_on){
    int current_flash_status;
    if (flash_on){
        current_flash_status = digitalRead(LAMP_PIN);
        digitalWrite(LAMP_PIN, HIGH);
    }

    //remove old frame
    this->fb = esp_camera_fb_get();
    esp_camera_fb_return(this->fb);

    // get new frame
    this->fb = esp_camera_fb_get();
    
    if(flash_on){
        digitalWrite(LAMP_PIN, current_flash_status);
    }

    return this->fb;
}

#endif