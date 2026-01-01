#pragma once

#include<Arduino.h>
#include<Adafruit_VL53L0X.h>
#include"distance_sensor.h"

class LazerDistanceSensor:public DistanceSensor{
private:
    Adafruit_VL53L0X sensor;
    VL53L0X_RangingMeasurementData_t m;
    float __measureDistanceCM__(){
        sensor.rangingTest(&m, false);
        return m.RangeMilliMeter/10;
    }
public:
    boolean init(){
        return sensor.begin();    
    }

    float measureDistanceCM(int num_samples = 1){
        if(num_samples <= 0)
        num_samples = 1;
        double total = 0;
        for(int i = 0; i < num_samples; i++){
            total += this->__measureDistanceCM__();
            delay(30);
        }

        return float(total/num_samples);
    }
};