#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#define ID_MAX_LEN 32
#define ID_MAX_NUM 20

FILE* fp;           // 读取文件
FILE* fp_res;       // 结果写入到另一个文件中
char ch;            // 当前读入的字符
char KEYWORD[][8] = {"begin", "end", "if", "then", "or", "and", "not", "rop", "true", "false" };	// keyword
char buffer[ID_MAX_LEN];    // 临时存储词法分析器读入的串
int idx;            // 全局变量默认值即为0，idx指针保存当前buffer的长度
struct identify{ // 标识符
    int cnt;
    char identify[ID_MAX_NUM][ID_MAX_LEN];
} ID;
struct outputs
{
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
void scanner(){
    if (is_alpha(ch) || ch == '_'){     // 如果读取的字符是字母
        buffer[idx ++] = ch;	        //将该字母添加到临时字符串
        ch = fgetc(fp);	                //读取下一位字符
        while (is_alpha(ch) || is_digit(ch) || ch == '_')	
            buffer[idx ++] = ch, ch = fgetc(fp);    //如果下一位字符是字母或数字, 继续读取下一位字符
        buffer[idx] = '\0';             // 当前读取截止
        /*to be optimized, 关键字和数字以及运算符的二元组会在输出文件中重复*/
        if (is_keyword(buffer)){
            printf("(%s, keyword)\n", buffer);	        //将结果以二元组的形式输出到屏幕
            fprintf(fp_res, "(%s, keyword)\n", buffer);	//将字符保存到输出文件中
        } else {    // to be optimized???, 这里把扫描到的东西全部都添加到输出，不考虑重复！
            strcpy(ID.identify[ID.cnt ++], buffer);
            printf("(%s, identify)\n", buffer);
            fprintf(fp_res, "(%s, identify)\n", buffer);
        }
        // memset(buffer, 0, sizeof(buffer));   // 清空buffer
        idx = 0;                                // idx指针清零
        fseek(fp, -1, 1);	                    //回退一个字符
    } else if (is_digit(ch)){                   //如果读取的字符是数字
        buffer[idx ++] = ch;	                //将该字母添加到临时字符串
        ch = fgetc(fp);
        while (is_digit(ch))	                    //如果下一位还是数字
            buffer[idx ++] = ch, ch = fgetc(fp);
        buffer[idx] = '\0';                     // 当前读取截止
        printf("(%s, unsigned integer)\n", buffer);
        fprintf(fp_res, "(%s, unsigned integer)\n", buffer);
        idx = 0;                                // idx指针清零
        fseek(fp, -1, 1);	                    //回退一个字符
    } else {   //其他字符
        switch (ch){
        //算术运算符
        case'+':
            printf("(%c,  Aoperator)\n", ch);
            fprintf(fp_res, "(%c, operator)\n", ch);
            break;
        case'-':
            printf("(%c, operator)\n", ch);
            fprintf(fp_res, "(%c, Aoperator)\n", ch);
            break;
        case'*':
            printf("(%c, operator)\n", ch);
            fprintf(fp_res, "(%c, Aoperator)\n", ch);
            break;
        case'/':
            printf("(%c, operator)\n", ch);
            fprintf(fp_res, "(%c, Aoperator)\n", ch);
            // printf("输入有误！\n");
            // fseek(fp, -1, 1);	//回退一个字符
            break;
        //关系运算符
        case'=':    // 即==，比较运算符
            printf("(%c, Roperator)\n", ch);
            fprintf(fp_res, "(%c, Roperator)\n", ch);
            break;
        case'<':
            ch = fgetc(fp);
            if (ch == '='){	  //单引号：字符；双引号：字符串
                printf("(<=, Roperator)\n");
                fprintf(fp_res, "(<=, Roperator)\n");
            } else {
                printf("(<, Roperator)\n");
                fprintf(fp_res, "(<, Roperator)\n");
                fseek(fp, -1, 1);	//回退一个字符
            }
            break;
        case'>':
            ch = fgetc(fp);
            if (ch == '='){	
                //单引号：字符；双引号：字符串
                printf("(>=, Roperator)\n");
                fprintf(fp_res, "(>=, Roperator)\n");
            } else {
                printf("(>, Roperator)\n");
                fprintf(fp_res, "(>, Roperator)\n");
                fseek(fp, -1, 1);	//回退一个字符
            }
            break;
        case'!':
            ch = fgetc(fp);
            if (ch == '=') {	
                //单引号：字符；双引号：字符串
                printf("(!=, Roperator)\n");
                fprintf(fp_res, "(!=, Roperator)\n");
            } else {
                printf("Roperator'!'\n");
                fprintf(fp_res, "Roperator'!'\n");
                fseek(fp, -1, 1);	//回退一个字符
            }
            break;
        //分界符
        case':':
            ch = fgetc(fp);
            if (ch == '=') {	 //单引号：字符；双引号：字符串
                printf("(:=, equal)\n");
                fprintf(fp_res, "(:=, equal)\n");
            } else {
                printf("错误的输入':'\n");
                fprintf(fp_res, "错误的输入':'\n");
                fseek(fp, -1, 1);	//回退一个字符
            }
            break;
        case';':
            printf("(%c, separator)\n", ch);
            fprintf(fp_res, "(%c, separator)\n", ch);
            break;
        case'(':
            printf("(%c, bracket)\n", ch);
            fprintf(fp_res, "(%c, bracket)\n", ch);
            break;
        case')':
            printf("(%c, bracket)\n", ch);
            fprintf(fp_res, "(%c, bracket)\n", ch);
            break;
            //case'.':
            //	/*printf("(%c,分界符)\n", ch);
            //	result_cifa[count++] = ch;
            //	fprintf(fp1, "(%c,分界符)\n", ch);*/
            //	printf("输入有误！\n");
            //	fseek(fp, -1, 1);	//回退一个字符
            //	break;
        }
    }
}

int main(int argc, char *argv[]){
    fp = fopen(argv[1], "r");
    fp_res = fopen("lex_res.txt", "w");
	if (NULL == fp || NULL == fp_res){
		printf("open %s failed.\n", argv[1]);
		return -1;
	}
    while(ch != EOF) {
        ch = fgetc(fp);
        if (ch == ' ' || ch == '\t' || ch == '\n') continue; // 遇到空格、换行符等直接跳过    
        else scanner();
    }
    if (0 == feof){
        printf("fgets error\n"); // 未读到文件末尾
        return -1;
	}
    // printf("ok\n");
	fclose(fp);
    for (int i = 0; i < sizeof(KEYWORD) / sizeof(KEYWORD[0]); ++ i){
        printf("%d:%s, ", strlen(KEYWORD[i]), KEYWORD[i]);
    }
    return 0;
}