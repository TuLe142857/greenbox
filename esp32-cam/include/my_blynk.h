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

    /**
     * @brief Sends garbage bin levels to Blynk. Automatically triggers a notification if any bin is full (level >= 90%).
     * @param bin1_level Level of bin 1 (0-100%).
     * @param bin2_level Level of bin 2 (0-100%).
     * @param bin3_level Level of bin 3 (0-100%).
     * @param bin4_level Level of bin 4 (0-100%).
     */
    void updateBinLevel(float bin1_level, float bin2_level, float bin3_level, float bin4_level);


    /**
     * @brief Registers a callback function to be called periodically.
     * @param callback The function to execute.
     * @param interval Time interval in milliseconds (default: 2000ms).
     */
    void addTimerFunction(std::function<void()> callback, unsigned long interval = 2000L);

    /**
     * @brief Sends a push notification to the Blynk App.
     * @param message The content of the notification.
     */
    void notification(String message);
    
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

void MyBlynk::updateBinLevel(float bin1_level, float bin2_level, float bin3_level, float bin4_level)
{
    Blynk.virtualWrite(V0, bin1_level);
    Blynk.virtualWrite(V1, bin2_level);
    Blynk.virtualWrite(V2, bin3_level);
    Blynk.virtualWrite(V3, bin4_level);

    if (
        (bin1_level >= 90) ||
        (bin2_level >= 90) ||
        (bin3_level >= 90) ||
        (bin4_level >= 90))
    {
        this->notification("Rác đầy, vui lòng đổ rác");
    }
    else
    {
        Blynk.virtualWrite(V4, "Hello, everything's ok!");
    }
}

void MyBlynk::addTimerFunction(std::function<void()> callback, unsigned long interval)
{
    this->timer.setInterval(interval, callback);
}

void MyBlynk::notification(String message)
{
    Blynk.logEvent("notification", message);
    Blynk.virtualWrite(V4, message);
}

void MyBlynk::run()
{
    Blynk.run();
    this->timer.run();
}

#endif