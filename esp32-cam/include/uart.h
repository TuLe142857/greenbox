#ifndef UART_H
#define UART_H

#include <Arduino.h>
#include <functional>
#include <map>

#include "utils.h"

/*
-------------------------------------------------
                PROTOTYPES
-------------------------------------------------
*/

/**
 * @brief This class implement UART communication
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
    std::vector<String> tokens = parse_tokens(s);
    if (tokens.size() == 0)
        return;

    String command = tokens[0];
    if (this->command_handlers.count(command)){
        this->command_handlers[command](tokens);
    }
}


#endif