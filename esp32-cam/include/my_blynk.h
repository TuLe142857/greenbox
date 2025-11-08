#ifndef MY_BLYNK_H
#define MY_BLYNK_H

#include <functional>
#include <BlynkSimpleEsp32.h>

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/

class MyBlynk
{
private:
    BlynkTimer timer;

public:
    void init(const char *auth_token);
    void addTimerFunction(std::function<void ()> callback, unsigned long interval=2000L);
    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

void MyBlynk::init(const char *auth_token)
{
    Blynk.config(auth_token);
}

void MyBlynk::addTimerFunction(std::function<void ()> callback, unsigned long interval){
    this->timer.setInterval(interval, callback);
}

void MyBlynk::run()
{
    Blynk.run();
    this->timer.run();
}

#endif