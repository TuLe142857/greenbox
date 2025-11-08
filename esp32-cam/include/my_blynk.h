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
    void init(const char *auth_token, std::function<void()> sendDataFunction);
    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

void MyBlynk::init(const char *auth_token, std::function<void()> sendDataFunction)
{
    Blynk.config(auth_token);

    // call back function for timer
    this->timer.setInterval(
        2000L,
        sendDataFunction);
}

void MyBlynk::run()
{
    Blynk.run();
    this->timer.run();
}

#endif