// #define TEST 
#include "app.h"

#ifndef TEST
    void setup()
    {
        Serial.begin(115200);
        app.init();
    }

    void loop()
    {
        app.run();
    }
#else
    #include<LiquidCrystal_I2C.h>
    #define LCD_I2C_ADDRESS 0x27
    #define LCD_ROWS 16
    #define LCD_COLUMNS 2
    LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
    void setup()
    {

        Serial.begin(115200);
        app.init();
        lcd.init();
        lcd.backlight();
    }

    void loop()
    {
        if(Serial.available()){
            lcd.clear();
            lcd.print(Serial.readStringUntil('\n'));
        }   
        delay(1);
    }

#endif