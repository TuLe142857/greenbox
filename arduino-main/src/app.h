#ifndef APP_H
#define APP_H


#include <Arduino.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include "uart.h"
#include "utils.h"
#include "devices/triple_servo.h"
#include "devices/ultrasonic_sensor.h"

#define GARBAGE_BIN_COUNT 4
#define GARBAGE_BIN_LEVEL_SENSOR_OFF_SET_CM 5
#define GARBAGE_BIN_DEPTH_CM 10

#define HUMAN_DETECT_THRESHOLD_CM 30
#define GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM 15

#define CLASSIFY_TIMEOUT_MS 15000UL
#define DELAY_TIME_CONFIRM_GARBAGE_MS 2000UL

#define LCD_I2C_ADDRESS 0x27
#define LCD_ROWS 16
#define LCD_COLUMNS 2
#define LCD_RENDER_INTEVAL_MS 1000UL

/*
------------------------------------------------
            PROTOTYPES
------------------------------------------------
*/

enum AppState
{
    WAITING_FOR_ESP, // wait for config/connect wifi/..
    NORMAL,
    GARBAGE_FULL,
    WAITING_FOR_GARBAGE,
    CONFIRM_GARBAGE,
    CLASSIFYING,
    DROPPING_GARBAGE,
    ERROR
};

static String AppStateName[] = {
    "WAITING_FOR_ESP",
    "NORMAL",
    "GARBAGE_FULL",
    "WAITING_FOR_GARBAGE",
    "CONFIRM_GARBAGE",
    "CLASSIFYING",
    "DROPPING_GARBAGE",
    "ERROR"
};

class App
{
private:
    AppState current_state;
    AppState previous_state;

    // timestamp millisecond
    unsigned long start_classify_at;
    unsigned long start_confirm_garbage_at;
    unsigned long last_ping_at;

    String error_message;

    int classify_result; // bin_id

    UART uart;

    // devices & sensors
    LiquidCrystal_I2C lcd;
    TripleServo tripleServo;
    Servo lidServo;
    UltraSonicSensor garbageDetectSensor, humanDetectSensor;
    UltraSonicSensor garbageBinLevelSensor[GARBAGE_BIN_COUNT];

public:
    App();
    void init();
    void run();

    void addCommandHandler(String command, void (*handler)(String tokens[], int n));

    bool isGarbageOnTray();
    bool isHumanNearby();
    float getGarbageBinLevel(int bin_id); // fill percentage %

    void setState(AppState state);
    void setErrorMessage(String message);

    void closeLid();
    void openLid();

    //lcd
    void render(const String line0="", const String line1="");

    void setClassifyResult(int bin_id);
    void dropGarbage();

    // send classify request to ESP32-CAM
    static void requestClassify();

    // send garbage bin level to ESP32-CAM
    static void reportGarbageBinLevel(float b1, float b2, float b3, float b4);
};

// GLOBAL VARIABLE FOR APP
// use like wrapper for CommandHandler
App app;

/*
------------------------------------------------
            DEFINTIONS
------------------------------------------------
*/

App::App() : current_state(WAITING_FOR_ESP),
             previous_state(WAITING_FOR_ESP),
             start_classify_at(0),
             start_confirm_garbage_at(0),
             last_ping_at(0),
             error_message(""),
             classify_result(-1),
             lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS)
{
}

void App::init(){
    // devices & sensors
    this->garbageDetectSensor.attach(2, 3);
    this->humanDetectSensor.attach(4, 5);
    this->garbageBinLevelSensor[0].attach(6, 7);
    this->garbageBinLevelSensor[1].attach(8, 9);
    this->garbageBinLevelSensor[2].attach(10, 11);
    this->garbageBinLevelSensor[3].attach(12, 13);

    this->tripleServo.attach(A0, A1, A2);
    this->lidServo.attach(A3);
    this->lidServo.write(0);

    this->lcd.init();
    this->lcd.backlight();

    // uart & command handlers
    this->uart.init();

    this->uart.addCommandHandler(
        "ESP_STATUS",
        [](String tokens[], int n)
        {
            if(n == 2){
                int status_code = tokens[1].toInt();
                Serial.println(status_code);
                switch (status_code){
                    //ready
                    case 0:{
                        app.setState(NORMAL);
                        break;
                    }

                    // on config mode
                    case 1:{
                        app.setState(WAITING_FOR_ESP);
                        app.render("PLEASE", "CONFIG WIFI");
                        break;
                    }

                    // try connecting wifi
                    case 2:{
                        app.setState(WAITING_FOR_ESP);
                        app.render("CONNECTING WIFI");
                        break;
                    }

                    //wifi disconnected
                    case 3:{
                        app.setState(WAITING_FOR_ESP);
                        app.render("WIFI", "DISCONNECTED");
                        break;
                    }
                }
            }
        }
    );

    this->uart.addCommandHandler(
        "GET_GARBAGE_BIN_LEVEL",
        [](String tokens[], int n)
        {
            App::reportGarbageBinLevel(
                app.getGarbageBinLevel(0),
                app.getGarbageBinLevel(1),
                app.getGarbageBinLevel(2),
                app.getGarbageBinLevel(3)
            );
        });

    this->uart.addCommandHandler(
        "CLASS",
        [](String tokens[], int n)
        {
            if ((app.current_state != CLASSIFYING) || (n == 1))
                return;
            app.setClassifyResult(tokens[1].toInt());
        });

    this->uart.addCommandHandler(
        "ERROR",
        [](String tokens[], int n)
        {
            String message = "";
            for (int i = 1; i < n; i++)
            {
                message += tokens[i] + " ";
            }
            app.setErrorMessage(message);
        });

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hello Word");
}

void App::run()
{
    this->uart.run();

    switch (this->current_state)
    {
        case WAITING_FOR_ESP:
        {
            unsigned long now = millis();
            if((now - app.last_ping_at) > 1000){
                Serial.println("PING");
                app.last_ping_at = now;
            }
            break;
        }
        case NORMAL:
        {
            // lcd render garbage bin level
            static float binLevel[GARBAGE_BIN_COUNT] = {0, 0, 0, 0};
            static unsigned long last_render = 0;
            static int next_bin_id = 0;
            unsigned long now = millis();

            if ((now - last_render) >= LCD_RENDER_INTEVAL_MS){
                binLevel[next_bin_id] = this->getGarbageBinLevel(next_bin_id);
                if(binLevel[next_bin_id] >= 90){

                    this->lcd.clear();
                    this->lcd.setCursor(0, 0);
                    this->lcd.print("GARBAGE FULL");

                    this->setState(GARBAGE_FULL);

                    last_render = now;
                    next_bin_id = (next_bin_id+1)%GARBAGE_BIN_COUNT;
                    break;
                }

                lcd.clear();
                this->render(
                    String(binLevel[0]) + " " + String(binLevel[1]),
                    String(binLevel[2]) + " " + String(binLevel[3])
                );

                last_render = now;
                next_bin_id = (next_bin_id+1)%GARBAGE_BIN_COUNT;
            }

            // check if human near -> change state to waiting for garbage
            if (this->isHumanNearby())
            {
                this->openLid();
                this->setState(WAITING_FOR_GARBAGE);

                this->lcd.clear();
                this->lcd.setCursor(0, 0);
                this->lcd.print("Vui long bo rac ");
                this->lcd.setCursor(0, 1);
                this->lcd.print("vao khay");
            }

            break;
        }
        
        case GARBAGE_FULL:
        {   
            static unsigned long last_recheck = 0;
            unsigned long now = millis();

            // recheck after 2s
            if ((now - last_recheck) < 2000UL)
                break;
            
            if ((this->getGarbageBinLevel(0) <= 90) ||
                (this->getGarbageBinLevel(1) <= 90) ||
                (this->getGarbageBinLevel(2) <= 90) ||
                (this->getGarbageBinLevel(3) <= 90)
            ){
                this->setState(NORMAL);
            }

            last_recheck = now;
            break;
        }

        case WAITING_FOR_GARBAGE:
        {
            static bool is_human_get_out = false;
            static unsigned long get_out_at = millis();
            unsigned long now = millis();
            if (this->isGarbageOnTray())
            {
                this->setState(CONFIRM_GARBAGE);
            }
            else if (!this->isHumanNearby())
            {
                if(is_human_get_out && get_out_at - now > 1000){
                    this->setState(NORMAL);
                    this->closeLid();
                    is_human_get_out = false;
                }else if (!is_human_get_out){
                    is_human_get_out = true;
                    get_out_at = now;
                }
                
            }
            break;
        }

        case CONFIRM_GARBAGE:
        {
            if (!this->isGarbageOnTray())
            {
                this->setState(WAITING_FOR_GARBAGE);
                break;
            }
            unsigned long now = millis();
            if ((now - this->start_confirm_garbage_at) >= (DELAY_TIME_CONFIRM_GARBAGE_MS))
            {
                this->closeLid();
                this->setState(CLASSIFYING);
                App::requestClassify();

                this->lcd.clear();
                this->lcd.setCursor(0, 0);
                this->lcd.print("Dang phan loai");
                this->lcd.setCursor(0, 1);
                this->lcd.print("rac...");

            }
            break;
        }

        case CLASSIFYING:
        {
            unsigned long now = millis();
            if ((now - this->start_classify_at) >= (CLASSIFY_TIMEOUT_MS))
            {
                // retry classify
                this->start_classify_at = millis();
                App::requestClassify();
            }
            break;
        }

        case DROPPING_GARBAGE:
        {
            this->lcd.clear();
            this->lcd.setCursor(0, 0);
            this->lcd.print("Phan loai: ");
            this->lcd.print(this->classify_result);
            this->dropGarbage();
            break;
        }

        case ERROR:
        {
            Serial.println("Something wrong: " + this->error_message);
            if (this->previous_state != ERROR)
            {
                this->setState(this->previous_state);
                if (this->previous_state == CLASSIFYING)
                {
                    App::requestClassify();
                }
            }
            else
            {
                this->setState(NORMAL);
                this->closeLid();
            }
            break;
        }
    } // end switch(this->current_state)

    delay(10);
}

void App::render(const String line0, const String line1){
    this->lcd.clear();
    this->lcd.setCursor(0, 0);
    this->lcd.print(line0);
    this->lcd.setCursor(0, 1);
    this->lcd.print(line1);
}

void App::addCommandHandler(String command, void (*handler)(String tokens[], int n))
{
    this->uart.addCommandHandler(command, handler);
}

bool App::isGarbageOnTray()
{
    float d = this->garbageDetectSensor.measureDistanceCM();
    #ifdef DEBUG
        Serial.print("Garbage sensor(cm): ");
        Serial.println(d);
    #endif
    return d < GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM;
}

bool App::isHumanNearby()
{   
    float d = this->humanDetectSensor.measureDistanceCM();
    #ifdef DEBUG
        Serial.print("Human detect sensor(cm): ");
        Serial.println(d);
    #endif
    return d < HUMAN_DETECT_THRESHOLD_CM;
}

float App::getGarbageBinLevel(int binId)
{
    if (!(binId >= 0 && binId < GARBAGE_BIN_COUNT))
    {
        return 0;
    }

    float d = this->garbageBinLevelSensor[binId].measureDistanceCM();

    float garbage_fill_height = (GARBAGE_BIN_DEPTH_CM + GARBAGE_BIN_LEVEL_SENSOR_OFF_SET_CM) - d;

    if (garbage_fill_height < 0)
        garbage_fill_height = 0;

    if (garbage_fill_height > GARBAGE_BIN_DEPTH_CM)
        garbage_fill_height = GARBAGE_BIN_DEPTH_CM;

    float fill_percentage = (garbage_fill_height / GARBAGE_BIN_DEPTH_CM) * 100;

    #ifdef DEBUG
        Serial.print("Garbage bin level-bin");
        Serial.println(binId);
        Serial.print(": ");
        Serial.println(d);
        Serial.print("(cm)~");
        Serial.print(fill_percentage);
        Serial.println("%");
    #endif

    
    return fill_percentage;
}

void App::setState(AppState state)
{
    if (state == this->current_state)
    {
        return;
    }
    else if (state == CONFIRM_GARBAGE)
    {
        this->start_confirm_garbage_at = millis();
    }
    else if (state == CLASSIFYING)
    {
        this->start_classify_at = millis();
    }

    this->previous_state = current_state;
    this->current_state = state;

    // DEBUG
    Serial.println("===== DEBUG =====");
    Serial.println("App state change:");
    Serial.println("Current State: " + AppStateName[this->current_state]);
    Serial.println("Previous State: " + AppStateName[this->previous_state]);
}

void App::setErrorMessage(String message)
{
    this->error_message = message;
    this->setState(ERROR);
}

void App::closeLid()
{
    int current_angle = this->lidServo.read();
    int new_angle = 0;
    if (current_angle == new_angle)
        return;
    int step = current_angle > new_angle ? -1 : 1;
    for (int i = current_angle; i != new_angle; i += step)
    {
        this->lidServo.write(i);
        delay(10);
    }
}

void App::openLid()
{
    int current_angle = this->lidServo.read();
    int new_angle = 90;
    if (current_angle == new_angle)
        return;
    int step = current_angle > new_angle ? -1 : 1;
    for (int i = current_angle; i != new_angle; i += step)
    {
        this->lidServo.write(i);
        delay(10);
    }
}

void App::setClassifyResult(int bin_id)
{
    if (!(bin_id >= 0 && bin_id < GARBAGE_BIN_COUNT))
    {
        return;
    }
    this->classify_result = bin_id;
    this->setState(DROPPING_GARBAGE);
}

void App::dropGarbage()
{
    if (!(this->classify_result >= 0 && this->classify_result < GARBAGE_BIN_COUNT))
        return;
    int angle = (this->classify_result * 90) + 45;
    this->tripleServo.dropAt(angle);
    this->setState(NORMAL);
    this->classify_result = -1;
}

void App::requestClassify(){
    Serial.println("CLASSIFY --flash");
}

void App::reportGarbageBinLevel(float b1, float b2, float b3, float b4){
    Serial.println(
        "GARBAGE_BIN_LEVEL " + String(b1)
        + " " + String(b2)
        + " " + String(b3)
        + " " + String(b4) 
    );
}

#endif