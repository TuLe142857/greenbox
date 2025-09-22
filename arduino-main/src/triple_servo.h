#ifndef TRIPLE_SERVO_MODULE
#define TRIPLE_SERVO_MODULE

#include<Arduino.h>

#if defined(ESP32)
    #include<ESP32Servo.h>
#elif defined(ARDUINO_AVR_UNO)
    #include<Servo.h>
#endif

class TripleServo{
private:
    Servo servos[3];

    void rotate(int servo_id, int angle, int delay_t=10){
        if (!(servo_id >= 0 && servo_id < 3))
            return;
        if (angle < 0) angle = 0;
        if (angle > 180) angle = 180;
        if (delay_t < 0) delay_t = 0;


        int current_angle = this->servos[servo_id].read();
        if(current_angle == angle)
            return;

        if (delay_t == 0){
            this->servos[servo_id].write(angle);
            return;
        }

        int step = (angle > current_angle) ? (1) : (-1);
        for (int i = current_angle + step; i != angle; i+= step){
            this->servos[servo_id].write(i);
            delay(delay_t);
        }
        this->servos[servo_id].write(angle);
    }

public:
    TripleServo(){}

    void attach(int pin0, int pin1, int pin2);

    void dropAt(int angle, int delay_t=10);
};


void TripleServo::attach(int pin0, int pin1, int pin2){
    this->servos[0].attach(pin0);
    this->servos[1].attach(pin1);
    this->servos[2].attach(pin2);


    this->servos[0].write(0);
    this->servos[1].write(0);
    this->servos[2].write(0);
}


void TripleServo::dropAt(int angle, int delay_t){
    if (angle < 0)
        angle = 0;
    if (angle > 360)
        angle = 360;
    if (delay_t < 0)
        delay_t = 0;

    int sv0_angle = (angle <= 180) ? (angle) : (180);
    int sv1_angle = (angle <= 180) ? (0) : (angle-180);
    int sv2_angle = 135;

    // drop
    this->rotate(0, sv0_angle, delay_t);
    this->rotate(1, sv1_angle, delay_t);
    this->rotate(2, sv2_angle, delay_t);

    // return
    this->rotate(2, 0, delay_t);
    this->rotate(1, 0, delay_t);
    this->rotate(0, 0, delay_t);

}

#endif