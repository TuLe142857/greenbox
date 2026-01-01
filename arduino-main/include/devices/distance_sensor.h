#pragma once

class DistanceSensor{
public:
    virtual float measureDistanceCM(int num_samples=0) = 0;
};