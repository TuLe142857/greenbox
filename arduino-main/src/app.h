#ifndef APP_H
#define APP_H

#include <LiquidCrystal_I2C.h>
#include "uart.h"
#include "utils.h"
#include "devices/triple_servo.h"
#include "devices/ultrasonic_sensor.h"

#define LID_OPEN_ANGLE 90
#define LID_CLOSE_ANGLE 0
#define LID_IMAGE_CAPTURE_ANGLE 60


#define GARBAGE_BIN_COUNT 4
#define GARBAGE_BIN_LEVEL_SENSOR_OFF_SET_CM 18
#define GARBAGE_BIN_DEPTH_CM 18

#define HUMAN_DETECT_THRESHOLD_CM 30
#define HUMAN_DETECT_THRESHOLD_MS 300
#define GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM 15

#define CLASSIFY_TIMEOUT_MS 15000UL
#define DELAY_TIME_CONFIRM_GARBAGE_MS 2000UL

#define LCD_I2C_ADDRESS 0x27
#define LCD_ROWS 16
#define LCD_COLUMNS 2

const String CLASSIFY_RESULT_MAPPING[GARBAGE_BIN_COUNT] = {
    "ORGANIC", "PAPER", "METAL", "PLASTIC"
}; 
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

    int classify_result; // bin_id 0-3

    UART uart;

    // devices & sensors
    LiquidCrystal_I2C lcd;
    TripleServo tripleServo;
    Servo lidServo;
    UltraSonicSensor garbageDetectSensor, humanDetectSensor;
    UltraSonicSensor garbageBinLevelSensor[GARBAGE_BIN_COUNT];

    /**
     * @brief Inititalize UART command handlers
     */
    void initCommandHandler();
public:
    App();

    /**
     * @brief Inittialize Application
     */
    void init();

    /**
     * @brief Primary Loop
     */
    void run();


    bool isGarbageOnTray();
    bool isHumanNearby();
    float getGarbageBinLevel(int bin_id); // fill percentage 0 - 100

    void setState(AppState state);
    void setErrorMessage(String message);

    void rotateLid(int angle);
    void closeLid();
    void openLid();

    /**
     * @brief Display text to LCD.
     * @param line0 text display on first line, default ""
     * @param line0 text display on second line, default ""
     */
    void display(const String line0="", const String line1="");

    /**
     * @brief Measure bin level + display to LCD. Auto set app state to GARBAGE_FULL and notify to ESP32-CAM when any bin level >= 90.
     * @return true if garbage is full (any bin level >= 90), else return false.
     */
    boolean displayGarbageBinLevel();


    void setClassifyResult(int bin_id);
    void dropGarbage();

    /**
     * @brief Send classify request to ESP32-CAM
     */
    void requestClassify();

    /**
     * @brief Send garbage bin level to ESP32-CAM. Auto set app state to GARBAGE_FULL when any bin level >= 90
     */
    void reportGarbageBinLevel(float b1, float b2, float b3, float b4);
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
    this->garbageBinLevelSensor[0].attach(8, 9);
    this->garbageBinLevelSensor[1].attach(6, 7);
    this->garbageBinLevelSensor[2].attach( 12, 13);
    this->garbageBinLevelSensor[3].attach(10, 11);

    this->tripleServo.attach(A0, A1, A2);
    this->lidServo.attach(A3);
    this->lidServo.write(0);

    this->lcd.init();
    this->lcd.backlight();

    this->display("WAITING FOR", "ESP32-CAM ...");

    // uart & command handlers
    this->uart.init();

    this->initCommandHandler();
}

void App::initCommandHandler(){
    // on received status from ESP32-CAM
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
                        app.display("PLEASE", "CONFIG WIFI");
                        break;
                    }

                    // try connecting wifi
                    case 2:{
                        app.setState(WAITING_FOR_ESP);
                        app.display("CONNECTING WIFI");
                        break;
                    }

                    //wifi disconnected
                    case 3:{
                        app.setState(WAITING_FOR_ESP);
                        app.display("WIFI", "DISCONNECTED");
                        break;
                    }
                }
            }
        }
    );

    // on received garbage bin level request from ESP32-CAM
    this->uart.addCommandHandler(
        "GET_GARBAGE_BIN_LEVEL",
        [](String tokens[], int n)
        {
            app.reportGarbageBinLevel(
                app.getGarbageBinLevel(0),
                app.getGarbageBinLevel(1),
                app.getGarbageBinLevel(2),
                app.getGarbageBinLevel(3)
            );
        });

    // on received notification "image is sending to server for classifying" from ESP32-CAM
    this->uart.addCommandHandler(
        "CLASSIFYING",
        [](String tokens[], int n)
        {
            if(app.current_state == CLASSIFYING){
                delay(1000);
                app.closeLid();
            }       
        });

    // on  received classify result from ESP32-CAM
    this->uart.addCommandHandler(
        "CLASS",
        [](String tokens[], int n)
        {
            if ((app.current_state != CLASSIFYING) || (n == 1))
                return;
            app.setClassifyResult(tokens[1].toInt());
        });

    // on received ERROR MESSAGE from ESP32-CAM
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
}

void App::run()
{
    this->uart.run();

    switch (this->current_state)
    {
        case WAITING_FOR_ESP:
        {
            // ping every 1s
            unsigned long now = millis();
            if((now - app.last_ping_at) > 1000){
                Serial.println("PING");
                this->last_ping_at = now;
            }
            break;
        }

        case NORMAL:
        {

            static unsigned long last_render = 0;
            static unsigned long last_seen_human = 0;
            unsigned long now = millis();


            // display garbage bin level on LCD
            // if garbage full => change state to GARBAGE_FULL
            if ((now - last_render) >= 1000){
                // this method display bin level
                // when garbage full(any bin level >= 90), auto: 
                //      - set AppState to GARBAGE_FULL
                //      - send notification to ESP32-CAM
                boolean isFull = this->displayGarbageBinLevel();
                last_render = now;
                if(isFull)
                    break;
            }
                
            // check if human near -> change state to waiting for garbage
            if (this->isHumanNearby()){
                if(last_seen_human == 0){
                    last_seen_human = now;
                }
                if(now - last_seen_human > HUMAN_DETECT_THRESHOLD_MS){
                    this->setState(WAITING_FOR_GARBAGE);
                    this->display("Vui long bo rac", "vao khay");
                    // reset
                    last_seen_human = 0;
                    last_render = 0;
                }
            }
            break;
        }
        
        case GARBAGE_FULL:
        {   
            static unsigned long last_recheck = 0;
            unsigned long now = millis();

            // recheck after 1s
            if ((now - last_recheck) > 1000UL){
                float binLevel[GARBAGE_BIN_COUNT];
                boolean isFull = false;
                for(int i = 0; i < GARBAGE_BIN_COUNT; i++){
                    binLevel[i] = this->getGarbageBinLevel(i);
                    if(binLevel[i] >= 90)
                        isFull = true;
                }
                if (! isFull){
                    this->setState(NORMAL);
                    this->reportGarbageBinLevel(binLevel[0], binLevel[1], binLevel[2], binLevel[3]);
                }
                last_recheck = now;
            }
                
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
            else if(!this->isHumanNearby())
            {
                if(is_human_get_out && get_out_at - now > 300){
                    this->setState(NORMAL);
                    is_human_get_out = false;   // reset
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
                this->setState(CLASSIFYING);
                App::requestClassify();

                this->display("Dang phan loai", "rac...");
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
            this->lcd.print(this->classify_result);
            this->display("Phan loai rac:", CLASSIFY_RESULT_MAPPING[this->classify_result]);
            this->dropGarbage();
            break;
        }

        case ERROR:
        {
            this->display("ERROR: ", this->error_message);
            delay(1000);
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
            }
            break;
        }
    } // end switch(this->current_state)

    delay(1);
}

void App::display(const String line0, const String line1){
    this->lcd.clear();
    this->lcd.setCursor(0, 0);
    this->lcd.print(line0);
    this->lcd.setCursor(0, 1);
    this->lcd.print(line1);
}

boolean App::displayGarbageBinLevel(){
    float binLevel[GARBAGE_BIN_COUNT];
    for(int i = 0;  i < GARBAGE_BIN_COUNT; i++){
        binLevel[i] = this->getGarbageBinLevel(i);
    }

    // display result to LCD screen
    lcd.clear();
    lcd.setCursor(8, 1);    lcd.print(binLevel[0]);
    lcd.setCursor(8, 0);    lcd.print(binLevel[1]);
    lcd.setCursor(0, 0);    lcd.print(binLevel[2]);
    lcd.setCursor(0, 1);    lcd.print(binLevel[3]);

    boolean isFull =  (binLevel[0] >= 90) ||
                      (binLevel[1] >= 90) ||
                      (binLevel[2] >= 90) ||
                      (binLevel[3] >= 90);
    if (isFull){
        // This method auto set app state to GARBAGE_FULL when any bin level >= 90
        this->reportGarbageBinLevel(binLevel[0], binLevel[1], binLevel[2], binLevel[3]);
    }
    return isFull;
}

bool App::isGarbageOnTray()
{
    float d = this->garbageDetectSensor.measureDistanceCM(2);
    #ifdef DEBUG
        Serial.print("Garbage detect sensor(cm): ");
        Serial.println(d);
    #endif
    return d < GARBAGE_ON_TRAY_DETECT_THRESHOLD_CM;
}

bool App::isHumanNearby()
{   
    float d = this->humanDetectSensor.measureDistanceCM(2);
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
    if (state == this->current_state){
        return;
    }

    this->previous_state = current_state;
    this->current_state = state;
    switch (this->current_state){
        case WAITING_FOR_ESP:{
            this->closeLid();
            break;
        }
        case NORMAL:{
            this->closeLid();
            break;
        }
        case GARBAGE_FULL:{
            this->closeLid();
            break;
        }
        case WAITING_FOR_GARBAGE:{
            this->openLid();
            break;
        }
        case CONFIRM_GARBAGE:{
            this->start_confirm_garbage_at = millis();
            break;
        }
        case CLASSIFYING:{
            this->start_classify_at = millis();
            break;
        }
        case DROPPING_GARBAGE:{
            this->closeLid();
            break;
        }
        case ERROR:{
            this->closeLid();
            break;
        }
    }

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

void App::rotateLid(int angle){
    if(!(angle >= 0 && angle <= 180))
        return;

    int current_angle = this->lidServo.read();
    int new_angle = angle;
    if (current_angle == new_angle)
        return;
    int step = current_angle > new_angle ? -1 : 1;
    for (int i = current_angle; i != new_angle; i += step)
    {
        this->lidServo.write(i);
        delay(10);
    }
}

void App::closeLid()
{   
    this->rotateLid(0);
}

void App::openLid()
{   
    this->rotateLid(90);
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
    this->rotateLid(LID_IMAGE_CAPTURE_ANGLE);
    Serial.println("CLASSIFY");
}

void App::reportGarbageBinLevel(float b0, float b1, float b2, float b3){
    if((b0 >= 90) || (b1 >= 90) || (b2 >= 90) || (b3 >= 90)){
        this->setState(GARBAGE_FULL);
    }
    Serial.println(
        "GARBAGE_BIN_LEVEL " + String(b0)
        + " " + String(b1)
        + " " + String(b2)
        + " " + String(b3) 
    );
}

#endif