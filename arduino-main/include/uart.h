#ifndef UART_H
#define UART_H

#include "utils.h"
#include <Arduino.h>

/*
------------------------------------------------
            PROTOTYPES
------------------------------------------------
*/


struct CommandHandler{
    String command;
    void (*func)(String *tokens, int n);
    CommandHandler(){
        command = "";
    }
    CommandHandler(String command, void (*func)(String *tokens, int n)){
        this->command = command;
        this->func = func;
    }
};

#define MAX_COMMAND_HANDLER 20


class UART {
private:
    CommandHandler commandHandlers[MAX_COMMAND_HANDLER];
    int commandHandlerCount = 0;
public:
    void init();
    void addCommandHandler(String command, void (*handler)(String *tokens, int n));
    void run();

};

/*
------------------------------------------------
            DEFINITIONS
------------------------------------------------
*/

void UART::init(){

}

void UART::addCommandHandler(String command, void (*handler)(String *tokens, int n)){
    if (this->commandHandlerCount == MAX_COMMAND_HANDLER)
        return;
    this->commandHandlers[this->commandHandlerCount++] = CommandHandler(command, handler);
}


void UART::run(){
    if (!Serial.available())
        return;

    int tokens_count;
    String s = Serial.readStringUntil('\n');
    // Serial.print("ARDUINO GET "); Serial.println(s);
    String *tokens = parse_token(s, tokens_count);
    if (tokens_count == 0){
        delete [] tokens;
        return;
    }

    for (int i = 0; i < this->commandHandlerCount; i++){
        if (tokens[0] == this->commandHandlers[i].command){
            this->commandHandlers[i].func(tokens, tokens_count);
        }
    }

    delete [] tokens;
}

#endif
