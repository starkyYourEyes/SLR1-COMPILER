// 每行最大长度
#ifndef _PRODUCTION
#define _PRODUCTION
#include <stdio.h>
#include <string.h>
#define maxsize 100 	// in KMP
#define LINE_MAX 1024

#define MAX_LEN_PRODUCTION 20
char lines[32][MAX_LEN_PRODUCTION]; // 产生式的个数
int line_num = 0;

int get_production_left(char* line){
	// 产生式的开头不可以有空格
	// 找到产生式的左边的结束的位置
	int loc = 0;
	for(; line[loc] && line[loc] != '-'; ++ loc){};	// 找到 - 的位置
	loc --;
	while (loc >= 0 && line[loc] == ' '){ loc --; }; // 跳过空格
	if (loc < 0) return -1; // 错误
	return loc;
}
int get_production_right(char* line){
	// 找到产生式的右边的开始的位置
	int loc = 0;
	for(; line[loc] && line[loc] != '>'; ++ loc){};	// 找到 > 的位置
	loc ++;
	while (line[loc] && line[loc] == ' '){ loc ++; }; // 跳过空格
	if (loc >= strlen(line)) return -1; // 错误
	return loc;
}
void read_lines(char *path){
	FILE *fp;
	int line_len = 0;		  // 文件每行的长度
	char buf[LINE_MAX] = {0}; // 行数据缓存
	fp = fopen(path, "r");
	if (NULL == fp){
		printf("open %s failed.\n", path);
		return;
	}
	while (fgets(buf, LINE_MAX, fp)){
		line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r', 空格' '
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1] || ' ' == buf[line_len - 1]){
			buf[line_len - 1] = '\0';
			line_len--;
			// if (0 == line_len)
			// 	continue; //空行
		}
		if (0 == line_len) continue; //空行
		strcpy(lines[line_num ++], buf);
	}
	if (0 == feof){
		printf("fgets error\n"); // 未读到文件末尾
		return;
	}
	fclose(fp);
}

#endif