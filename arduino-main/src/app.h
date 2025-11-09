/*
==============================================================
    SMART TRASH BIN CONTROLLER (FSM-based)
    ----------------------------------------------------------
    This module implements the core logic for a smart trash bin
    using a Finite State Machine (FSM) design pattern.

    The App class controls sensors, servos, and UART communication
    to manage the full garbage classification and disposal workflow.

==============================================================
    STATE MACHINE OVERVIEW
==============================================================
The system is always in one of the following states. Each state
defines specific actions and the conditions required to transition
to another state.

    NORMAL
        - Default idle state.
        - When a human is detected nearby → open lid → switch to WAITING_FOR_GARBAGE.

    WAITING_FOR_GARBAGE
        - If human leaves → close lid → return to NORMAL.
        - If garbage detected on tray → switch to CONFIRM_GARBAGE.

    CONFIRM_GARBAGE
        - Waits for DELAY_TIME_CONFIRM_GARBAGE_SECOND (non-blocking using millis()).
        - If garbage remains stable → close lid → trigger classification.
        - Transition to CLASSIFYING.
        - If garbage removed early → return to WAITING_FOR_GARBAGE.

    CLASSIFYING
        - Sends command to ESP32-CAM for image classification.
        - If CLASSIFY_TIMEOUT_SECOND expires → re-trigger classification.
        - When classification result received → switch to DROPPING_GARBAGE.

    DROPPING_GARBAGE
        - Opens corresponding servo gate to drop garbage into classified bin.
        - After action completes → return to NORMAL.

    ERROR
        - Entered whenever the ESP32-CAM sends an "ERROR" command.
        - Attempts to recover by restoring the previous state if possible.
        - If recovery fails → resets system to NORMAL and closes lid.

==============================================================
    DESIGN NOTES
==============================================================
    - Finite State Machine (FSM) pattern for clean, event-driven logic.
    - Non-blocking timing control using millis() — no delay() in main logic.
    - Modular device abstraction (TripleServo, UltrasonicSensor, UART).
    - Easy to extend: add new states or transitions safely.
    - All timeouts and thresholds configurable via #define constants.

==============================================================
    USAGE NOTES
==============================================================
    - This class should be instantiated as a single global variable:
          extern App app;

    - The global instance acts as a wrapper for command handlers,
      allowing lambdas in UART command registration to access
      the main App object easily.

    - Example:
        #include "app.h"
        App app;
        void setup() {
            Serial.begin(115200);
            app.init();
        }

        void loop() {
            app.run();
        }

==============================================================
*/
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
#define GARBAGE_LEVEL_SENSOR_OFF_SET_CM 5
#define BIN_TOTAL_DEPTH_CM 10
#define CLASSIFY_TIMEOUT_SECOND 15
#define DELAY_TIME_CONFIRM_GARBAGE_SECOND 2

enum AppState
{
    NORMAL,
    WAITING_FOR_GARBAGE,
    CONFIRM_GARBAGE,
    CLASSIFYING,
    DROPPING_GARBAGE,
    ERROR
};

class App
{
private:
    AppState current_state = NORMAL;
    AppState previous_state = NORMAL;

    // time mili second
    unsigned long start_classify_at = 0;
    unsigned long start_confirm_garbage_at = 0;

    String error_message;

    int binIdToDrop = -1; // after classify, app.current_state = DROPPING_GARBAGE

    UART uart;

    // devices
    TripleServo tripleServo;
    Servo lidServo;
    UltraSonicSensor garbageDetectSensor, humanDetectSensor;
    UltraSonicSensor garbageBinLevelSensor[GARBAGE_BIN_COUNT];

    void initCommandHandlers();

public:
    void init();

    bool isGarbageOnTray();
    bool isHumanNearBy();
    float getBinLevel(int binId); // fill percentage

    void setState(AppState state);
    void setErrorMessage(String s);

    void closeLid();
    void openLid();

    void setDropTarget(int binId);
    void dropGarbageAt(int binId);

    void run();
};

// GLOBAL VARIABLE FOR APP
// use like wrapper for CommandHandler
extern App app;

/*
------------------------------------------------
            DEFINTIONS
------------------------------------------------
*/

void App::init()
{
    this->uart.init();
    this->initCommandHandlers();

    // devices pin
    this->garbageBinLevelSensor[0].attach(2, 3);
    this->garbageBinLevelSensor[1].attach(4, 5);
    this->garbageBinLevelSensor[2].attach(6, 7);
    this->garbageBinLevelSensor[3].attach(8, 9);

    this->lidServo.attach(10);
    this->humanDetectSensor.attach(11, 12);

    this->tripleServo.attach(A0, A1, A2);
    this->garbageDetectSensor.attach(A3, A4);

    // reset servo
    this->lidServo.write(0);
}

bool App::isGarbageOnTray()
{
    float d = this->garbageDetectSensor.measureDistanceCM();

    // DEBUG
    Serial.print("Garbage on tray sensor cm: "); Serial.println(d);

    return (d < GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM);
}

bool App::isHumanNearBy()
{
    float d = this->humanDetectSensor.measureDistanceCM();

    // DEBUG
    Serial.print("Human detect sensor cm: "); Serial.println(d);

    return (d < HUMAN_DETECT_THRESHOLD_CM);
}

float App::getBinLevel(int binId)
{
    if (!(binId >= 0 && binId < GARBAGE_BIN_COUNT))
    {
        return 0;
    }

    float d = this->garbageBinLevelSensor[binId].measureDistanceCM();
    float garbage_fill_height = (BIN_TOTAL_DEPTH_CM + GARBAGE_LEVEL_SENSOR_OFF_SET_CM) - d;

    if (garbage_fill_height < 0)
        garbage_fill_height = 0;
    if (garbage_fill_height > BIN_TOTAL_DEPTH_CM)
        garbage_fill_height = BIN_TOTAL_DEPTH_CM;

    float fill_percentage =  (garbage_fill_height / BIN_TOTAL_DEPTH_CM) * 100;

    // DEBUG
    Serial.println("Bin " + String(binId) + " level: " + String(d) + " cm ~ " + String(fill_percentage) + "%");
    
    return fill_percentage;
}

void App::setState(AppState state)
{
    if (state == CONFIRM_GARBAGE)
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
    String enumName[] = {"NORMAL", "WAITING_FOR_GARBAGE", "CONFIRM_GARBAGE", "CLASSIFYING", "DROPPING_GARBAGE", "ERROR"};
    Serial.println("=============================");
    Serial.println("App state change:");
    Serial.println("Current State: " + enumName[this->current_state]);
    Serial.println("Previous State: " + enumName[this->previous_state]);
}

void App::setErrorMessage(String s)
{
    this->error_message = s;
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

void App::setDropTarget(int binId)
{
    if (!(binId >= 0 && binId < GARBAGE_BIN_COUNT))
    {
        return;
    }
    this->binIdToDrop = binId;
    this->setState(DROPPING_GARBAGE);
}

void App::dropGarbageAt(int binId)
{
    if (!(binId >= 0 && binId < GARBAGE_BIN_COUNT))
    {
        return;
    }
    int angle = (binId * 90) + 45;
    this->tripleServo.dropAt(angle);
    this->setState(NORMAL);
}

void App::initCommandHandlers()
{

    /*
    ----------------------------------------------
            COMMAND: GET_GARBAGE_BIN_LEVEL
    ----------------------------------------------
     */
    this->uart.addCommandHandler(
        "GET_GARBAGE_BIN_LEVEL",
        [](String tokens[], int n)
        {
            float bin_levels[GARBAGE_BIN_COUNT];
            for (int i = 0; i < GARBAGE_BIN_COUNT; i++)
            {
                bin_levels[i] = app.getBinLevel(i);
            }
            Serial.println(
                "GARBAGE_BIN_LEVEL " + String(bin_levels[0]) + " " + String(bin_levels[1]) + " " + String(bin_levels[2]) + " " + String(bin_levels[3]));
        });

    /*
    ----------------------------------------------
            COMMAND: CLASS
    ----------------------------------------------
     */
    this->uart.addCommandHandler(
        "CLASS",
        [](String tokens[], int n)
        {
            if (app.current_state != CLASSIFYING)
                return;
            if (n == 1)
                return;
            int classify_result = tokens[1].toInt();
            app.setDropTarget(classify_result);
        });

    /*
    ----------------------------------------------
            COMMAND: ERROR
    ----------------------------------------------
     */
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
        }

    );
}

void App::run()
{

    this->uart.run();

    switch (this->current_state)
    {
    case NORMAL:
    {
        if (this->isHumanNearBy())
        {
            this->openLid();
            this->setState(WAITING_FOR_GARBAGE);
        }
        break;
    }

    case WAITING_FOR_GARBAGE:
    {
        if (this->isGarbageOnTray())
        {
            this->setState(CONFIRM_GARBAGE);
        }
        else if (!this->isHumanNearBy())
        {
            this->setState(NORMAL);
            this->closeLid();
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
        if ((now - this->start_confirm_garbage_at) >= (DELAY_TIME_CONFIRM_GARBAGE_SECOND * 1000UL))
        {
            this->closeLid();
            this->setState(CLASSIFYING);
            Serial.println("CLASSIFY --flash");
        }
        break;
    }

    case CLASSIFYING:
    {
        unsigned long now = millis();
        if ((now - this->start_classify_at) >= (CLASSIFY_TIMEOUT_SECOND * 1000UL))
        {
            // retry classify
            this->start_classify_at = millis();
            Serial.println("CLASSIFY --flash");
        }
        break;
    }

    case DROPPING_GARBAGE:
    {
        this->dropGarbageAt(this->binIdToDrop);
        break;
    }

    case ERROR:
    {
        // handle error

        // DEBUG
        Serial.println("Something wrong: " + this->error_message);

        // switch to previous state
        if (this->previous_state != ERROR)
        {
            this->setState(this->previous_state);
            // retry classify
            if (this->previous_state == CLASSIFYING)
            {
                Serial.println("CLASSIFY --flash");
            }
        }
        else
        {
            // reset
            this->setState(NORMAL);
            this->closeLid();
        }
        break;
    }

    } // end switch

    delay(10);
}

#endif