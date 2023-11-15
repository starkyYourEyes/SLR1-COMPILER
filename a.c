// #include <stdio.h>
// struct{
    
// }WORDS[];
// const char* const KEYWORDS[] = {"if", "then", "begin", "end", "not", "end", "or"};
// int main(){
//     // printf("%d\n", sizeof(KEYWORDS));
//     for(int i = 0; i < sizeof(KEYWORDS) / sizeof(char*); i ++){
//         printf("%s\n", KEYWORDS[i]);
//     }
//     return 0;
// }
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void demo(char s[], int x){
    // ++ *x;   // equals to (*x) ++;
    // *x ++;
    // while (x)
}
int main(void){
    int x = 14;
    // printf("%d, ", x);
    char tmp[10];
    tmp[0] = 'S';
    itoa(x,tmp + 1,10);
    printf("%d, %s", strlen(tmp), tmp);
}



