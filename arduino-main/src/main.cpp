#include <Arduino.h>
#include"ultrasonic_sensor.h"
#include"triple_servo.h"
#include"utils.h"
TripleServo ts;
UltraSonicSensor us;
void setup() {
    Serial.begin(115200);
    ts.attach(2, 3, 4);
    us.attach(5, 6);
}


void loop() {
    if(Serial.available() > 0){
        String command = Serial.readStringUntil('\n');
        int token_count = 0;
        String *tokens = parse_token(command, token_count);
        // Serial.print("token count = "); Serial.println(token_count);
        // for(int i = 0; i <  token_count; i++){
        //     Serial.print("token ");
        //     Serial.print(i+1);
        //     Serial.println(tokens[i].c_str());
        // }


        if (token_count == 0){
            return;
        }
        if(tokens[0] == "distance"){
            Serial.print("Distance: ");
            Serial.println(us.measureDistance());
        }
        if(tokens[0] == "drop"){
            ts.dropAt(tokens[1].toInt());
        }


        delete []tokens;
    }   
    delay(100);
}