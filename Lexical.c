#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define ID_MAX_LEN 32
#define ID_MAX_NUM 20
#define LINE_MAX 1024
FILE* fp;           // 读取文件
FILE* fp_res;       // 结果写入到另一个文件中
char ch;            // 当前读入的字符
char KEYWORD[][8] = {"begin", "end", "if", "then", "or", "and", "not", "rop", "true", "false" };	// keyword
char buffer[ID_MAX_LEN];    // 临时存储词法分析器读入的串
int idx;            // 全局变量默认值即为0，idx指针保存当前buffer的长度
int line_num;       // 当前分析的行号
struct identify{    // 标识符
    int cnt;
    char identify[ID_MAX_NUM][ID_MAX_LEN];
} ID;
struct outputs{
    int cnt;
    char output[64][128];
} OUT;
bool is_digit(char ch){
    return ch >= '0' && ch <= '9';
}
bool is_alpha(char ch){
    return (ch >='a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}
bool is_keyword(char *s){
    for (int i = 0; i < sizeof(KEYWORD) / sizeof(KEYWORD[0]); ++ i)
        if (strcmp(KEYWORD[i], s) == 0)
            return true;
    return false;
}
bool is_identify_repeated(char *s){
    for (int i = 0; i < ID.cnt; ++ i)
        if (strcmp(ID.identify[i], s) == 0)
            return true;
    return false;
}
void scanner(char *s, int *loc){
    if (is_alpha(s[*loc]) || s[*loc] == '_'){     // 如果读取的字符是字母
        // 如果以字母开头，不是关键字就是标识符
        buffer[idx ++] = s[*loc];	        //将该字母添加到临时字符串
        ++ *loc;	                        //读取下一位字符
        while (is_alpha(s[*loc]) || is_digit(s[*loc]) || s[*loc] == '_')	
            buffer[idx ++] = s[*loc], ++ *loc;;    //如果下一位字符是字母或数字, 继续读取下一位字符
        buffer[idx] = '\0';                 // 当前读取截止
        // printf("buffer:%s\n", buffer);
        if (is_keyword(buffer)){
            printf("(%s, %s)\n", buffer, buffer);	        //将结果以二元组的形式输出到屏幕
            fprintf(fp_res, "(%s, %s)\n", buffer, buffer);	//将字符保存到输出文件中
        } else {    // 这里把扫描到的东西全部都添加到输出，不考虑重复！词法分析本该如此
            if(!is_identify_repeated(buffer))
                strcpy(ID.identify[ID.cnt ++], buffer);
            printf("(%s, id)\n", buffer);
            fprintf(fp_res, "(%s, id)\n", buffer);
        }
        // memset(buffer, 0, sizeof(buffer));   // 清空buffer
        idx = 0;                                // idx指针清零
        -- *loc;	                            // 回退一个字符
    } else if (is_digit(s[*loc])){              // 如果读取的字符是数字
        buffer[idx ++] = s[*loc];	            // 将该字母添加到临时字符串
        ++ *loc;
        while (is_digit(s[*loc]))	            // 如果下一位还是数字
            buffer[idx ++] = s[*loc], ++ *loc;
        buffer[idx] = '\0';                     // 当前读取截止
        printf("(%s, id)\n", buffer);
        fprintf(fp_res, "(%s, id)\n", buffer);
        idx = 0;                                // idx指针清零
        -- *loc;	                            // 回退一个字符
    } else {   // 其他字符
        switch (s[*loc]){
        // 算术运算符
        case'+':
            printf("(%c, +)\n", s[*loc]);
            fprintf(fp_res, "(%c, +)\n", s[*loc]);
            break;
        case'-':
            printf("(%c, -)\n", s[*loc]);
            fprintf(fp_res, "(%c, -)\n", s[*loc]);
            break;
        case'*':
            printf("(%c, *)\n", s[*loc]);
            fprintf(fp_res, "(%c, *)\n", s[*loc]);
            break;
        case'/':
            printf("(%c, /)\n", s[*loc]);
            fprintf(fp_res, "(%c, /)\n", s[*loc]);
            break;
        //关系运算符
        case'<':
        case'>':
        case'!':
        case'=':    // 即==，比较运算符
            ++ *loc;
            if (s[*loc] == '='){	
                //单引号：字符；双引号：字符串
                printf("(%c=, rop)\n", s[*loc]);
                fprintf(fp_res, "(%c=, rop)\n", s[*loc]);
            } else {
                -- *loc;	// 回退一个字符
                printf("(%c, rop)\n", s[*loc]);
                fprintf(fp_res, "(%c, rop)\n", s[*loc]);
            }
            break;
        //分界符
        case':':
            ++ *loc;
            if (s[*loc] == '=') {	 //单引号：字符；双引号：字符串
                printf("(:=, :=)\n");
                fprintf(fp_res, "(:=, :=)\n");
            } else {
                printf("line %d:%d  unexpected character '%c'", line_num, *loc, s[*loc]);
                exit(-1);
            }
            break;
        case';':
            printf("(%c, ;)\n", s[*loc]);
            fprintf(fp_res, "(%c, ;)\n", s[*loc]);
            break;
        case'(':
            printf("(%c, ()\n", s[*loc]);
            fprintf(fp_res, "(%c, ()\n", s[*loc]);
            break;
        case')':
            printf("(%c, ))\n", s[*loc]);
            fprintf(fp_res, "(%c, ))\n", s[*loc]);
            break;
        default:
            printf("line %d:%d  unexpected character '%c'", line_num, *loc, s[*loc]);
            exit(-1);
        }
    }
}
int main(int argc, char *argv[]){
    fp = fopen(argv[1], "r");
    fp_res = fopen("lex_res.txt", "w");
    char buf[LINE_MAX] = {0};
	if (NULL == fp || NULL == fp_res){
		printf("open %s failed.\n", argv[1]);
		return -1;
	}
    while (fgets(buf, LINE_MAX, fp)){   // 逐行
        int len = strlen(buf), loc = 0;
        line_num ++;
        for(; buf[loc]; loc ++){
            char ch = buf[loc];
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '#') continue;
            scanner(buf, &loc);
        }
    }
    if (0 == feof){
        printf("fgets error\n"); // 未读到文件末尾
        return -1;
	}
    // printf("ok\n");
	fclose(fp);
    fclose(fp_res);
    // for (int i = 0; i < sizeof(KEYWORD) / sizeof(KEYWORD[0]); ++ i){
    //     printf("%d:%s, ", strlen(KEYWORD[i]), KEYWORD[i]);
    // }
    return 0;
}