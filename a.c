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
void demo(int *x){
    // ++ *x;   // equals to (*x) ++;
    // *x ++;
    
}
int main(void){
    int x = 1;
    printf("%d, ", x);
    demo(&x);
    printf("%d", x);
}



