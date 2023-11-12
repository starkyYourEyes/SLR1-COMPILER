#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
    int n = 0;
    char c = ' ';
    FILE* fp = fopen("grammar.txt", "r");
    if (fp == NULL){
        printf("error occured while opening file.!");
        return 0;
    }

    while (c != EOF){
      c = fgetc(fp); // 获取一个字符
      if (c == 'S') n++; // 统计美元符号 '$' 在文件中出现的次数
    }
    printf("%d times", n);
    fclose(fp);
    fp=NULL;

    return 0;
}