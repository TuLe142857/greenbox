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
    void (*func)(String tokens[], int n);

    CommandHandler(){
        command = "";
        func = [](String tokens[], int n){
            // do nothing ...
        };
    }

    CommandHandler(String command, void (*func)(String *tokens, int n)){
        this->command = command;
        this->func = func;
    }
};

#define MAX_COMMAND_HANDLER 20
#define MAX_TOKENS 20

class UART {
private:
    String tokens[MAX_TOKENS];
    CommandHandler commandHandlers[MAX_COMMAND_HANDLER];
    int commandHandlerCount = 0;
public:
    void init();
    void addCommandHandler(String command, void (*handler)(String tokens[], int n));
    void run();

};

/*
------------------------------------------------
            DEFINITIONS
------------------------------------------------
*/

void UART::init(){
    this->commandHandlerCount = 0;
}

void UART::addCommandHandler(String command, void (*handler)(String tokens[], int n)){
    if (this->commandHandlerCount == MAX_COMMAND_HANDLER)
        return;
    this->commandHandlers[this->commandHandlerCount++] = CommandHandler(command, handler);
}


void UART::run(){
    if (!Serial.available())
        return;
    String s = Serial.readStringUntil('\n');

    int tokens_count;
    parse_tokens(s, this->tokens, MAX_TOKENS, tokens_count);

    // Serial.print("Arduino received: "); Serial.println(s);
    // Serial.println("Parse into " + String(tokens_count) + " tokens");
    // for (int i = 0; i < tokens_count; i++){
    //     Serial.println("Tokens " + String(i) + ": " + this->tokens[i]);
    // }

    if (tokens_count == 0){
        return;
    }

    for (int i = 0; i < this->commandHandlerCount; i++){
        if (this->tokens[0] == this->commandHandlers[i].command){
            this->commandHandlers[i].func(this->tokens, tokens_count);
        }
    }
}

#endif
