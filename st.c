// 文法中的中文’改为英文的'
// 该文法中没有空串
// /*To be optimized*/ --> 待优化的地方,有时间(闲得慌)就回去修改
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "production.h"
#include "first_follow.h"

int main(int argc, char *argv[]){
	FILE* w_res = fopen("files/first-follow-set.txt", "w");
	if (NULL == w_res){
		printf("write %s failed.\n", "files/first-follow-set.txt");
		return -1;
	}
	V = (struct CHARS *)malloc(sizeof(struct CHARS));
	V->len_vn = V->len_vt = 0;

	int line_num = get_vs(argv[1]) / 2;

	printf("line_num = %d\n", line_num);

	printf("%d Vns: \n", V->len_vn);
	for (int i = 0; i < V->len_vn; ++i){
		printf("%s", V->vn[i]);
		if (i < V->len_vn - 1) printf(", ");
	}
	printf("\n");
	printf("%d Vts: \n", V->len_vt);
	for (int i = 0; i < V->len_vt; ++i){
		printf("%s, ", V->vt[i]);
		if (i < V->len_vn - 1) printf(", ");
	}

	printf("\n");

	read_lines(argv[1]);

	get_first_set();
	
	printf("first sets:\n");
	for (int i = 0; i < V->len_vn; ++i){
		printf("%-2s: { ", V->vn[i]);
		fprintf(w_res, "%s:", V->vn[i]);
		for (int j = 0; j < FIRST_[i].cnt; ++j){
			printf("%s", FIRST_[i].set[j]);
			fprintf(w_res, "%s ", FIRST_[i].set[j]);
			if (j < FIRST_[i].cnt - 1) printf(", ");
		}
		printf(" }\n");
		fprintf(w_res, "\n");
	}
	fprintf(w_res, "\n");
	// 将FOLLOW集中的非终结符按照V中的非终结符的顺序填进去
	for (int i = 0; i < V->len_vn; ++i) strcpy(FOLLOW_[i].vn, V->vn[i]);

	if(argc > 2){
		// printf("argc2:%d\n", argc);
		// 如果指定了开始符号
		for(int i = 0; i < V->len_vn; ++ i)
			if (strcmp(V->vn[i], argv[2])){
				FOLLOW_[i].cnt ++;
				strcpy(FOLLOW_[i].set[i], "#");
			}
	} else{
		// 默认开始符号为第一个
		// printf("argc:%d\n", argc);
		FOLLOW_[0].cnt ++;
		strcpy(FOLLOW_[0].set[0], "#");
	}


	cal_follow();
	
	printf("follow sets:\n");
	for (int i = 0; i < V->len_vn; ++ i){
		printf("%-2s:{ ", V->vn[i]);
		fprintf(w_res, "%s:", V->vn[i]);
		for (int j = 0; j < FOLLOW_[i].cnt; ++ j){
			printf("%s", FOLLOW_[i].set[j]);
			fprintf(w_res, "%s ", FOLLOW_[i].set[j]);
			if (j < FOLLOW_[i].cnt - 1)
				printf(", ");
		}
		fprintf(w_res, "\n");
		printf(" }\n");
	}

	fclose(w_res);
	return 0;
}
