#ifndef UART_H
#define UART_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <functional>
#include <map>

#include "camera.h"
#include "utils.h"
#include "uart.h"

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/

class UART
{
private:
    std::map<String, std::function<void(std::vector<String> tokens)>> command_handlers;
public:
    void init();
    void addCommandHandler(String command, std::function<void(std::vector<String> tokens)> handler);
    void run();
};

/*
-------------------------------------------------
                DEFINITIONS
-------------------------------------------------
*/

void UART::init(){}

void UART::addCommandHandler(String command, std::function<void(std::vector<String> tokens)> handler){
    this->command_handlers[command] = handler;
}


void UART::run()
{
    if (!Serial.available())
        return;
    String s = Serial.readStringUntil('\n');
    // Serial.println("ESP Recieved "+ s);
    std::vector<String> tokens = parse_tokens(s);
    
    if (tokens.size() == 0)
        return;
    if (this->command_handlers.count(tokens[0])){
        this->command_handlers[tokens[0]](tokens);
    }
}

#endif