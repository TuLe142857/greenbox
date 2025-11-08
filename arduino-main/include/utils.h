#ifndef APP_UTILS_FUNTIONS_H
#define APP_UTILS_FUNTIONS_H

#include<Arduino.h>

/*
------------------------------------------------
            FUNTION PROTOTYPES
------------------------------------------------
*/

void parse_tokens(String s, String tokens[], int max_tokens, int &token_count);


/*
------------------------------------------------
            FUNTION DEFINITIONS
------------------------------------------------
*/

void parse_tokens(String s, String tokens[], int max_tokens, int &token_count) {
    token_count = 0;
    s.trim();
    int start = 0;

    if (s.length() == 0 || max_tokens == 0) {
        return;
    }

    while (start < s.length() && token_count < max_tokens) {
        while (start < s.length() && s[start] == ' ') {
            start++;
        }
        
        if (start >= s.length()) {
            break; 
        }

        int end;
        if (s[start] == '"') {
            start++;
            end = s.indexOf('"', start);
            
            if (end == -1) { 
                end = s.length();
            }
            
            tokens[token_count] = s.substring(start, end);
            start = end + 1;
            
        } else {
            end = s.indexOf(' ', start);
            
            if (end == -1) {
                end = s.length();
            }
            
            tokens[token_count] = s.substring(start, end);
            start = end;
        }
        token_count++; 
    }
    
}


#endif