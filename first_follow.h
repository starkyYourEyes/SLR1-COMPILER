#ifndef _VN_VT
#define _VN_VT
#include <string.h>
#include <stdio.h>
#include "production.h"
#include "utils.h"
#define MAX_LEN_VN 3
#define MAX_LEN_VT 10
struct CHARS{
	int len_vn;				 // 非终结符的个数
	char vn[20][MAX_LEN_VN]; // 非终结符, 最长为3, S' + '\0'
	int len_vt;				 // 终结符的个数
	char vt[40][MAX_LEN_VT]; // 终结符
} *V;

bool is_vn(char ch){ 
	// 判断字符是不是非终结符, 这里假设非终结符只有一个字母，因为S'只出现在产生式左侧，这里忽略他不计
	for (int i = 0; i < V->len_vn; ++i)
		if (ch == V->vn[i][0])
			return true;
	return false;
}
char *is_prefix(char s[]){
	// 这里的s对应的是产生式的右边
	/*is_prefix计算的是，某一个非终结符s，其对应的产生式 右边的直接的终结符，直接加入到其first集中
	也可以用来判断：s这个字符串的开头是不是一个非终结符。*/
	// 如果是一个非终结符，返回这个非终结符串，否则返回NULL
	for (int i = 0; i < V->len_vt; ++i){
		int len = strlen(V->vt[i]);
		// printf("vt:%s, len=%d\n", V->vt[i], len);
		int j = 0;
		for (; j < len; ++j)
			if (s[j] != V->vt[i][j])
				break;
		if (j >= len)
			return V->vt[i];
	}
	return NULL;
}
int get_vn_no(char* vn){
	// 找到这个非终结符的编号，
	// 因为非终结符的 顺序都是按照V中的顺序来的，所以非终结符的顺序唯一，只需要确定其编号
	for (int i = 0; i < V->len_vn; ++ i)
		if (strcmp(vn, V->vn[i]) == 0)
			return i;
	return -1;
}
int get_vt_no(char* vt){
	// 找到这个终结符的编号，
	// 因为终结符的 顺序都是按照V中的顺序来的，所以终结符的顺序唯一，只需要确定其编号
	for (int i = 0; i < V->len_vt; ++ i){
		if (strcmp(vt, V->vt[i]) == 0)
			return i;
	}
	return -1;
}
int get_vs(char *path){
	// 获取所有的终结符和非终结符
	FILE *fp;
	int line_num = 0;		  // 文件行数
	int line_len = 0;		  // 文件每行的长度
	char buf[LINE_MAX]; // 行数据缓存
	fp = fopen(path, "r");
	if (NULL == fp){
		printf("open %s failed.\n", path);
		return -1;
	}
	// 第一次读取，读取所有的非终结符
	while (fgets(buf, LINE_MAX, fp)){
		line_num++;
		line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r'
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1]){
			buf[line_len - 1] = '\0';
			line_len--;
		}
		if (0 == line_len) continue; //空行
		// printf("%s\n", buf);
		// 扫描获取非终结符
		int start = 0;
		while (buf[start] == ' '){
			start++;
		}; // 去除开头的空格
		strcpy(buf, buf + start);
		int loc = 0;
		for (; buf[loc] != '-' && buf[loc] != ' '; loc++){};
		buf[loc] = '\0';
		bool flag = false;
		for (int i = 0; i < V->len_vn; ++i)
			if (strcmp(V->vn[i], buf) == 0){
				flag = true;
				break;
			}
		if (!flag){
			strcpy(V->vn[V->len_vn], buf);
			V->len_vn++;
		}
	}
	rewind(fp); // 文件指针回到开头
	// 读取所有的终结符
	while (fgets(buf, LINE_MAX, fp)){
		line_num++;
		line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r'
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1]){
			buf[line_len - 1] = '\0';
			line_len--;
			// if (0 == line_len)
			// 	continue; //空行
		}
		if (0 == line_len) continue; //空行
		int loc = 0;
		for (; buf[loc] != '>' || buf[loc] == ' '; loc++){}; // 清除'>'后面的空格,短路或
		// printf("raw: %s\n", buf + loc + 1);
		strcpy(buf, buf + loc + 1);

		int i = 0;
		char word[10];
		for (; buf[i]; i++){ // 双指针
			memset(word, '\0', strlen(word));
			// int start = i;   // 右侧只会出现，空格，\0, 终结符，小写字母，符号
			if (is_vn(buf[i]) || buf[i] == ' ')
				continue; // 非终结符或空格，直接跳过
			int j = i;
			for (; is_alpha(buf[j]); ++j){};
			// printf("%s\n", buf);
			// printf("loc:%d\n", j);
			if (j != i){
				strncpy(word, buf + i, j - i);
				word[j - i] = '\0';
				// printf("word1:%s\n", word);
				bool flag = false;
				for (int k = 0; k < V->len_vt; ++k)
					if (strcmp(V->vt[k], word) == 0){
						flag = true;
						break;
					}
				if (!flag){
					strcpy(V->vt[V->len_vt], word);
					V->len_vt++;
				}
				i = j - 1;
			} else{
				// 需要判断是否达到字符串末尾
				for (; !is_alpha(buf[j]) && !is_vn(buf[j]) && buf[j] != ' ' && buf[j]; ++j){
					// printf("buf[j]:%c\n", buf[j]);
				};
				if (j != i){
					strncpy(word, buf + i, j - i);
					word[j - i] = '\0';
					// printf("word2:%s\n", word);
					bool flag = false;
					for (int k = 0; k < V->len_vt; ++k)
						if (strcmp(V->vt[k], word) == 0){
							flag = true;
							break;
						}
					if (!flag){
						strcpy(V->vt[V->len_vt], word);
						V->len_vt++;
					}
					i = j - 1;
				}
			}
		}
	}
	if (0 == feof){
		printf("fgets error\n"); // 未读到文件末尾
		return -1;
	}
	fclose(fp);
	return line_num;
}

#endif