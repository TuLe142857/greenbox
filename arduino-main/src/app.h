#ifndef APP_H
#define APP_H

#include <Arduino.h>
#include <Servo.h>
#include "uart.h"
#include "utils.h"
#include "devices/triple_servo.h"
#include "devices/ultrasonic_sensor.h"

/*
------------------------------------------------
            PROTOTYPES
------------------------------------------------
*/

#define GARBAGE_BIN_COUNT 4
#define HUMAN_DETECT_THRESHOLD_CM 30
#define GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM 15
#define GARBAGE_CAPACITY_SENSOR_OFF_SET_CM 5
#define BIN_TOTAL_DEPTH_CM 10

enum AppState {
        NORMAL,
        WAITING_FOR_GARBAGE, // lid opened
        CLASSIFYING,        // sending image to server, wait for response
        DROPPING_GARBAGE    // received response from server, doing classify
    };

class App{
private:
    AppState current_state = NORMAL;
    int binIdToDrop = -1;

    UART uart;

    // device
    TripleServo tripleServo;
    Servo lidServo;
    UltraSonicSensor garbageDetectSensor, humanDetectSensor;
    UltraSonicSensor garbageBinCapacitySensor[GARBAGE_BIN_COUNT];

    void initCommandHandlers();
public:
    void init();

    bool isGarbageOnTray();
    bool isHumanNearBy();
    float getBinLevel(int binId);

    void setDropTarget(int binId);
    void dropGarbageAt(int binId);

    void run();

};

// GLOBAL VARIABLE FOR APP
extern App app;

/*
------------------------------------------------
            DEFINTIONS
------------------------------------------------
*/

void App::init(){
    Serial.println("CLASSIFY --flash");
    this->uart.init();
    this->initCommandHandlers();

    // device pin

    this->garbageBinCapacitySensor[0].attach(2, 3);
    this->garbageBinCapacitySensor[1].attach(4, 5);
    this->garbageBinCapacitySensor[2].attach(6, 7);
    this->garbageBinCapacitySensor[3].attach(8, 9);

    this->lidServo.attach(10);
    this->humanDetectSensor.attach(11, 12);

    this->tripleServo.attach(A0, A1, A2);
    this->garbageDetectSensor.attach(A3, A4);

    // reset servo
    this->lidServo.write(0);
}

bool App::isGarbageOnTray(){
    return false;
    // return (this->garbageDetectSensor.measureDistanceCM() < GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM);
}

bool App::isHumanNearBy(){
    return (this->humanDetectSensor.measureDistanceCM() < HUMAN_DETECT_THRESHOLD_CM);
}

float App::getBinLevel(int binId){
    if (! (binId >= 0 && binId < GARBAGE_BIN_COUNT)){
        return 0;
    }

    float d = this->garbageBinCapacitySensor[binId].measureDistanceCM();
    float garbage_fill_height = (BIN_TOTAL_DEPTH_CM + GARBAGE_CAPACITY_SENSOR_OFF_SET_CM) - d;

    if (garbage_fill_height < 0)
        garbage_fill_height = 0;
    if (garbage_fill_height > BIN_TOTAL_DEPTH_CM)
        garbage_fill_height = BIN_TOTAL_DEPTH_CM;
    // return d;
    return (garbage_fill_height/BIN_TOTAL_DEPTH_CM) * 100;
}

void App::setDropTarget(int binId){
    if (! (binId >= 0 && binId < GARBAGE_BIN_COUNT)){
        return;
    }
    this->binIdToDrop = binId;
    this->current_state = DROPPING_GARBAGE;
}

void App::dropGarbageAt(int binId){
    if (! (binId >= 0 && binId < GARBAGE_BIN_COUNT)){
        return;
    }
    int angle = (binId * 90) + 45;
    this->tripleServo.dropAt(angle);
    this->current_state = NORMAL;
}

void App::initCommandHandlers(){

    /*
    ----------------------------------------------
            COMMAND: GET_GARBAGE_BIN_LEVEL
    ----------------------------------------------
     */
    this->uart.addCommandHandler(
        "GET_GARBAGE_BIN_LEVEL",
        [](String *tokens, int n){
            float bin_levels[GARBAGE_BIN_COUNT];
            for (int i = 0; i < GARBAGE_BIN_COUNT; i++){
                bin_levels[i] = app.getBinLevel(i);
            }
            Serial.println(
                "GARBAGE_BIN_LEVEL " + String(bin_levels[0])
                + " " + String(bin_levels[1])
                + " " + String(bin_levels[2])
                + " " + String(bin_levels[3])
            );
        }
    );


    /*
    ----------------------------------------------
            COMMAND: CLASS
    ----------------------------------------------
     */
    this->uart.addCommandHandler(
        "CLASS",
        [](String *tokens, int n){
            if (n == 1)
                return;
            int classify_result = tokens[1].toInt();
            app.setDropTarget(classify_result);
        }
    );
}


void App::run(){
    this->uart.run();
    // Serial.print("human detect ");
    // Serial.println(this->isHumanNearBy());
    // Serial.print("state "); Serial.println(this->current_state);

    switch (this->current_state)
    {
    case NORMAL:
        if (this->isHumanNearBy()){
            this->lidServo.write(90);
            this->current_state = WAITING_FOR_GARBAGE;
        }
        break;

    case WAITING_FOR_GARBAGE:
        if (this->isGarbageOnTray()){
            // call esp32-cam to capture image and send to server 
            this->current_state = CLASSIFYING;
            this->lidServo.write(0);
            Serial.println("CLASSIFY");
        }
        else if(!this->isHumanNearBy()){
            this->current_state = NORMAL;
            this->lidServo.write(0);
        }
        break;

    case CLASSIFYING:
        // process on uart, nothing to do here
        break;

    case DROPPING_GARBAGE:
        this->dropGarbageAt(this->binIdToDrop);
        break;
    }

    

    delay(10);
}


#endif