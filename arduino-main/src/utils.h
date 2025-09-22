#ifndef UTILITIES_FUNCTION
#define UTILITIES_FUNCTION

#include<Arduino.h>

String * parse_token(String s, int &size){
    s.trim();
    size =0;
    if(s.length() == 0)
        return nullptr;

    int start = 0;
    int end = 0;
    while(start < s.length()){
        if (s[start] == '"'){
            start ++;
            end = start+1;
            while(end < s.length() && s[end] != '"')
                end ++;
            size ++;

            end++;
            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }else{
            end = start+1;
            while(end < s.length() && s[end] != ' ')
                end ++;
            size ++;

            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }
    }
    String *tokens = new String[size];

    start = 0;
    int idx=0;
    while(start < s.length()){
        if (s[start] == '"'){
            start ++;
            end = start+1;
            while(end < s.length() && s[end] != '"')
                end ++;
            tokens[idx++] = s.substring(start, end);

            end++;
            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }else{
            end = start+1;
            while(end < s.length() && s[end] != ' ')
                end ++;
            tokens[idx++] = s.substring(start, end);

            while(end < s.length() && s[end] == ' ')
                end ++;
            start = end;
        }
    }
    return tokens;
}
#endif