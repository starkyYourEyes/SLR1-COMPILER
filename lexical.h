#ifndef _LEXICAL
#define _LEXICAL
#include "first_follow.h"
#include <stdlib.h>

char KEYWORD[][8] = {"begin", "end", "if", "then", "or", "and", "not", "rop", "true", "false" };	// keyword
int line_ct_num;       // 当前分析的行号

struct identify{    // 标识符
    int cnt;
    char identify[ID_MAX_NUM][ID_MAX_LEN];
} ID;

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

void scanner(char *s, int *loc, int line, FILE* fp_res){
    int idx = 0;                            // idx指针保存当前buffer的长度
    char buffer[ID_MAX_LEN];    // 临时存储词法分析器读入的串
    if (is_alpha(s[*loc]) || s[*loc] == '_'){     // 如果读取的字符是字母
        // 如果以字母开头，不是关键字就是标识符
        buffer[idx ++] = s[*loc];	        //将该字母添加到临时字符串
        
        ++ *loc;	                        //读取下一位字符
        while (is_alpha(s[*loc]) || is_digit(s[*loc]) || s[*loc] == '_')	
            buffer[idx ++] = s[*loc], ++ *loc;;    //如果下一位字符是字母或数字, 继续读取下一位字符
        buffer[idx] = '\0';                 // 当前读取截止
        // printf("buffer:%s\n", buffer);
        if (is_keyword(buffer)){
            printf("(%s, %s, %d)\n", buffer, buffer, line);	        //将结果以二元组的形式输出到屏幕
            fprintf(fp_res, "(%s, %s, %d)\n", buffer, buffer, line);	//将字符保存到输出文件中
        } else {    // 这里把扫描到的东西全部都添加到输出，不考虑重复！词法分析本该如此
            if(!is_identify_repeated(buffer))
                strcpy(ID.identify[ID.cnt ++], buffer);
            printf("(%s, id, %d)\n", buffer, line);
            fprintf(fp_res, "(%s, id, %d)\n", buffer, line);
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
        printf("(%s, id, %d)\n", buffer, line);
        fprintf(fp_res, "(%s, id, %d)\n", buffer, line);
        idx = 0;                                // idx指针清零
        -- *loc;	                            // 回退一个字符
    } else {   // 其他字符
        switch (s[*loc]){
        // 算术运算符
        case'+':
            printf("(%c, +, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, +, %d)\n", s[*loc], line);
            break;
        case'-':
            printf("(%c, -, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, -, %d)\n", s[*loc], line);
            break;
        case'*':
            printf("(%c, *, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, *, %d)\n", s[*loc], line);
            break;
        case'/':
            printf("(%c, /, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, /, %d)\n", s[*loc], line);
            break;
        //关系运算符
        
        case'<':
        case'>':
        case'!':
        case'=':    // 即==，比较运算符
            ++ *loc;
            if (s[*loc] == '='){	
                //单引号：字符；双引号：字符串
                printf("(%c=, rop, %d)\n", s[*loc], line);
                fprintf(fp_res, "(%c=, rop, %d)\n", s[*loc], line);
            } else {
                -- *loc;	// 回退一个字符
                printf("(%c, rop, %d)\n", s[*loc], line);
                fprintf(fp_res, "(%c, rop, %d)\n", s[*loc], line);
            }
            break;
        //分界符
        case':':
            ++ *loc;
            if (s[*loc] == '=') {	 //单引号：字符；双引号：字符串
                printf("(:=, :=, %d)\n", line);
                fprintf(fp_res, "(:=, :=, %d)\n", line);
            } else {
                printf("line %d:%d  unexpected character '%c'", line_ct_num, *loc, s[*loc]);
                exit(-1);
            }
            break;
        case';':
            printf("(%c, ;, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, ;, %d)\n", s[*loc], line);
            break;
        case'(':
            printf("(%c, (, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, (, %d)\n", s[*loc], line);
            break;
        case')':
            printf("(%c, ), %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, ), %d)\n", s[*loc], line);
            break;
        case'#':
            printf("(%c, #, %d)\n", s[*loc], line);
            fprintf(fp_res, "(%c, #, %d)\n", s[*loc], line);
            break;
        default:
            printf("line %d:%d  unexpected character '%c'", line_ct_num, *loc, s[*loc]);
            exit(-1);
        }
    }
}

#endif