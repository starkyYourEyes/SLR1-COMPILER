#ifndef _UTILS
#define _UTILS
#include <stdio.h>
#define ID_MAX_LEN 32
#define ID_MAX_NUM 20
#define LINE_MAX 1024
bool is_digit(char ch){
    return ch >= '0' && ch <= '9';
}
bool is_alpha(char ch){
    return (ch >='a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

#endif