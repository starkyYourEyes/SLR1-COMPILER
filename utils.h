#ifndef _UTILS
#define _UTILS
#include <stdio.h>
#include <stdbool.h>
bool is_digit(char ch){
    return ch >= '0' && ch <= '9';
}
bool is_alpha(char ch){
    return (ch >='a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

#endif