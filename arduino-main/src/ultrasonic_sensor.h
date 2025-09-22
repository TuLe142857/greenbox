#ifndef ULTRASONIC_SENSOR
#define ULTRASONIC_SENSOR

#include<Arduino.h>

class UltraSonicSensor{
private:
    int trigger_pin;
    int echo_pin;
public:
    UltraSonicSensor(){}

    void attach(int trigger_pin, int echo_pin);

    float measureDistance();

};

void UltraSonicSensor::attach(int trigger_pin, int echo_pin){
    this->trigger_pin = trigger_pin;
    this->echo_pin = echo_pin;

    pinMode(trigger_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
}

float UltraSonicSensor::measureDistance(){
    digitalWrite(this->trigger_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(this->trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->trigger_pin, LOW);


    unsigned long t = pulseIn(this->echo_pin, HIGH);
    return t/58.0;
}



#endif