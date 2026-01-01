
#ifndef ULTRASONIC_SENSOR
#define ULTRASONIC_SENSOR

#include<Arduino.h>
#include"distance_sensor.h"
class UltraSonicSensor:public DistanceSensor{
private:
    int trigger_pin;
    int echo_pin;
    float __measureDistanceCM__();
public:
    UltraSonicSensor(){}

    void attach(int trigger_pin, int echo_pin);
    

    // Measures distance in centimeters by averaging multiple readings.
    // If num_samples <= 0, the value is automatically set to 1.
    float measureDistanceCM(int num_samples = 1);

};

void UltraSonicSensor::attach(int trigger_pin, int echo_pin){
    this->trigger_pin = trigger_pin;
    this->echo_pin = echo_pin;

    pinMode(trigger_pin, OUTPUT);
    pinMode(echo_pin, INPUT);
}

float UltraSonicSensor::__measureDistanceCM__(){
    digitalWrite(this->trigger_pin, LOW);
    delayMicroseconds(2);
    digitalWrite(this->trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(this->trigger_pin, LOW);


    unsigned long t = pulseIn(this->echo_pin, HIGH);
    return t*0.01715;
}

float UltraSonicSensor::measureDistanceCM(int num_samples){
    if(num_samples <= 0)
        num_samples = 1;
    double total = 0;
    for(int i = 0; i < num_samples; i++){
        total += this->__measureDistanceCM__();
        delay(1);
    }

    return float(total/num_samples);
}

#endif
