#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "lexical.h"

int main(int argc, char *argv[]){
    FILE* fp;           // 读取文件
    fp = fopen(argv[1], "r");
    FILE* fp_res = fopen("files/lex_res.txt", "w");

    char buf[LINE_MAX] = {0};
	if (NULL == fp || NULL == fp_res){
		printf("open %s failed.\n", argv[1]);
		return -1;
	}
    int line_no = 0;
    while (fgets(buf, LINE_MAX, fp)){   // 逐行
        int len = strlen(buf), loc = 0;
        line_ct_num ++;
        for(; buf[loc]; loc ++){
            char ch = buf[loc];
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' ) continue;
            scanner(buf, &loc, line_no, fp_res);
        }
        line_no ++;
    }
    if (0 == feof){
        printf("fgets error\n"); // 未读到文件末尾
        return -1;
	}
	fclose(fp);
    fclose(fp_res);
    return 0;
}