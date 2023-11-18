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

int NUM_VN = 0;		// 非终结符的个数

struct SET{
	char vn[MAX_LEN_VN];	  // 非终结符的名称
	int cnt;				  // first集中终结符的个数
	// set存储的是first集和follow集中的元素，且假设每个非终结符的first、follow集中的元素个数不超过20个
	// 在NODE_中表示计算这个非终结符的FOLLOW集所依赖的其他的非终结符
	char set[20][MAX_LEN_VT]; 
} FIRST_[20], FOLLOW_[20], NODE_[20];

bool is_repeated(struct SET* F, char *s){
	// 判断是不是有重复，
	// 比如s是一个字符串，是不是在F的first集合中出现过。
	for (int i = 0; i < F->cnt; ++ i)
		if (strcmp(F->set[i], s) == 0)
			return true;
	return false;
}
void cal_first(struct SET *FIRST){
	// 不递归求解First集
	// 先扫描一遍所有的产生式，把能得到的first集都算出来
	for (int i = 0; i < line_num; ++i){
		// 找到当前行的非终结符，即tmp
		int loc = 0;
		for (; lines[i][loc] != '-'; loc++){};
		while (lines[i][loc] == ' '){loc--;};
		char tmp[3];
		strncpy(tmp, lines[i], loc);
		tmp[loc] = '\0';
		// 找到当前传入的非终结符的对应行
		if (strcmp(FIRST->vn, tmp) == 0){
			// printf("FIRST->vn:%s, tmp%s\n", FIRST->vn, tmp);
			// 确定产生式右边开始的位置
			int loc = get_production_right(lines[i]);
			char *tmp = lines[i] + loc;
			char ct[10];
			strcpy(ct, tmp); // ct为产生式右侧的字符串，长度最大为1，根据给定文法
			ct[1] = '\0';	 // 取ct，ct为产生式右边的第一个字符，判断他是不是非终结符。

			// 这里其实只要执行一遍就可以了，扫描一遍，将右侧直接的终结符，如S->aS，这种的直接添加到first集
			/*To be optimized*/
			char *s = is_prefix(tmp);
			if (s == NULL){ // 右边没有直接的非终结符，跳过
			} else{		// 右边有直接的非终结符，加入到first集中
				bool flag = true; // 判断该非终结符是否已经在first集中
				for (int k = 0; k < FIRST->cnt; ++k)
					if (strcmp(s, FIRST->set[k]) == 0){
						flag = false;
						break;
					}
				if (flag){ // 没出现过，直接加入，且对应的cnt++
					strcpy(FIRST->set[FIRST->cnt], s);
					FIRST->cnt++;
				}
			}

			for (int i = 0; i < V->len_vn; ++i){
				if (strcmp(ct, V->vn[i]) == 0)
				{ // 如果是非终结符,且是第i个非终结符,把第i个非终结符的first拼接
					// printf("vn:%s, ct:%s, %s, %s\n", FIRST->vn, ct, V->vn[i], FIRST_[i].vn);
					for (int j = 0; j < FIRST_[i].cnt; ++j){
						bool flag = true;
						for (int k = 0; k < FIRST->cnt; ++k)
							if (strcmp(FIRST_[i].set[j], FIRST->set[k]) == 0){
								flag = false;
								break;
							}
						if (flag){
							// printf("========%s, %s, %s\n", FIRST->vn, FIRST_[i].vn, FIRST_[i].set[j]);
							strcpy(FIRST->set[FIRST->cnt], FIRST_[i].set[j]);
							FIRST->cnt++;
						}
					}
				}
			}
		}
	}
}
void cal_follow(){
	// 计算follow集，read_lines函数已经把产生式的个数以及产生式读入到了lines数组中
	// 采用状态图的计算方法，即使图中会出现环，但是判断条件为循环计算直到不再变化为止

	// for (int i = 0; i < line_num; ++ i){
	// 	printf("loc:%d\n", get_production_right(lines[i]));
	// }	

	// for (int i = 0; i < line_num; ++ i) printf("%d, %s\n", strlen(lines[i]), lines[i]);
	// 1. 扫描所有的产生式建立相应的FOLLOW集的关系图，并计算出能计算出的FOLLOW集 
	for (int i = 0; i < line_num; ++ i){
		char vn_left[3];	// 记录产生式的左部
		int loc_left = get_production_left(lines[i]);
		if (loc_left == -1){
			printf("error1!\n");
			return;
		}
		strncpy(vn_left, lines[i], loc_left + 1);	//
		vn_left[loc_left + 1] = '\0';
		int loc = get_production_right(lines[i]);
		if (loc == -1) {
			printf("error2!\n");
			return;
		}
		// printf("-----------------%s------------------\n", lines[i]);
		for (; lines[i][loc]; ++ loc){
			if (is_vn(lines[i][loc])){
				// 对于产生式右侧的非终结符，计算其FOLLOW集
				int j = loc + 1;
				// to be optimized, 假设非终结符都是一个字符的长度
				char current_vn[2];	// 当前的这个非终结符,改造为字符串的形式
				current_vn[0] =  lines[i][loc], current_vn[1] = '\0';
				int n = get_vn_no(current_vn);
				// printf("%c, %s, %s\n", lines[i][j], lines[i], current_vn);
				// for (int i = 0 ; i < NODE_[n].cnt; ++ i)
				// 	printf("%s, ", NODE_[n].set[i]);
				// printf("==%s\n", vn_left);
				if (!lines[i][j]){
					// 如果当前这个非终结符是产生式最后一个字符，FOLLOW(当前) += FOLLOW(产生式左边)
					if (!is_repeated(&NODE_[n], vn_left)){
						printf("%s depends on %s\n", current_vn, vn_left);
						strcpy(NODE_[n].set[NODE_[n].cnt], vn_left);
						NODE_[n].cnt += 1;
					}
				} else{
					// 如果他不是最后一个字符，因为我们的文法中的first集中不含有空串~，简化了操作
					// 直接把后面的字符的first集加入到follow集中即可
					while (lines[i][j] && lines[i][j] == ' ') j ++;	// 跳过空格
					if (!lines[i][j]){
						printf("error3!"); return;
					} 
					// printf("lines[i] + j:%s\n", lines[i] + j);
					char* s = is_prefix(lines[i] + j);	
					if (s == NULL){	// 如果是非终结符，把他的first集添加进去
						char tmp[2];
						tmp[0] = lines[i][j], tmp[1] = 0;
						// printf("ct is vn:%s\n", tmp);
						int x = get_vn_no(tmp);
						for (int i = 0; i < FIRST_[x].cnt; ++ i)
							if (!is_repeated(&FOLLOW_[n], FIRST_[x].set[i])){
								// printf("%s adds not repeated elements:%s\n",FOLLOW_[n].vn, FIRST_[x].set[i]);
								strcpy(FOLLOW_[n].set[FOLLOW_[n].cnt], FIRST_[x].set[i]);
								FOLLOW_[n].cnt ++;
							}
					} else{	//如果是终结符, 直接添加
						// printf("ct is vs:%s\n", s);
						// printf("%s adds not repeated elements:%s\n",FOLLOW_[n].vn, s);
						if (!is_repeated(&FOLLOW_[n], s)){
							strcpy(FOLLOW_[n].set[FOLLOW_[n].cnt], s);
							FOLLOW_[n].cnt ++;
						}
					}
				}

			}
		}
	}
	// 2.根据NODE_中的依赖关系，完成计算。
	// 2.1，首先去除对于自己的依赖（自闭环）
	for (int i = 0; i < V->len_vn; ++ i){
		if (NODE_[i].cnt == 0) continue;
		for (int j = 0; j < NODE_[i].cnt; ++ j){
			if (strcmp(NODE_[i].vn, NODE_[i].set[j]) == 0){
				// printf("%s == %s\n", NODE_[i].vn, NODE_[i].set[j]);
				// printf("%s replaces %s\n", NODE_[i].set[NODE_[i].cnt - 1], NODE_[i].set[j]);
				// 采用最后一个元素替换他来删除这个元素，避免移动后面元素
				strcpy(NODE_[i].set[j], NODE_[i].set[NODE_[i].cnt - 1]);
				NODE_[i].cnt --;
			}
		}
	}
	// printf("\n===============================================\n");
	// for (int i = 0; i < V->len_vn; ++ i){
	// 	printf("vn:%-2s--", V->vn[i]);
	// 	for (int j = 0; j < NODE_[i].cnt; ++ j)
	// 		printf("%s, ", NODE_[i].set[j]);
	// 	printf("\n");
	// }
	// printf("\n===============================================\n");
	
	// 2.2 处理无自闭环的关系图。
	for (int i = 0; i < V->len_vn; ++ i){
		if (NODE_[i].cnt == 0) continue;
		// 如何处理环形依赖，如A依赖B，B依赖C，C依赖A，to be optimized
		// 2.2.1 先处理所依赖的没有依赖的。。。如S依赖S',但是S'的FOLLOW不再依赖其他的
		// 对于给定的这个文法。。到此就可以结束了...
		for (int j = 0; j < NODE_[i].cnt; ++ j){
			// 每一个依赖的序号
			int no_depend = get_vn_no(NODE_[i].set[j]);
			if (NODE_[no_depend].cnt == 0){
				for (int k = 0; k < FOLLOW_[no_depend].cnt; ++ k){
					if (!is_repeated(&FOLLOW_[i], FOLLOW_[no_depend].set[k])){
						strcpy(FOLLOW_[i].set[FOLLOW_[i].cnt], FOLLOW_[no_depend].set[k]);
						FOLLOW_[i].cnt++;
					}
				}
				// 把这个依赖删去
				strcpy(NODE_[i].set[j], NODE_[i].set[NODE_[i].cnt - 1]);
				NODE_[i].cnt --;
				j --;	// 最后一个被换到了当前的位置，所以j--
			}
		}
	}
	// printf("\n===============================================\n");
	// for (int i = 0; i < V->len_vn; ++ i){
	// 	printf("vn:%-2s--", V->vn[i]);
	// 	for (int j = 0; j < NODE_[i].cnt; ++ j)
	// 		printf("%s, ", NODE_[i].set[j]);
	// 	printf("\n");
	// }
	// printf("\n===============================================\n");
	
}

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

	// 将FIRST集中的非终结符按照V中的非终结符的顺序填进去
	for (int i = 0; i < V->len_vn; ++i) strcpy(FIRST_[i].vn, V->vn[i]);
	int preFirst[20] = {0};
	int afterFirst[20] = {0};
	for (;;){ // 重复直到不再发生变化
		for (int i = 0; i < V->len_vn; ++i)
			preFirst[i] = FIRST_[i].cnt;
		for (int i = 0; i < V->len_vn; ++i) // 对每一个非终结符，逐个计算其First集
			cal_first(&FIRST_[i]);
		bool flag = false;
		for (int i = 0; i < V->len_vn; ++i)
			if (preFirst[i] != FIRST_[i].cnt){
				preFirst[i] = FIRST_[i].cnt;
				flag = true;
			}
		if (!flag)
			break;
	}
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

	// 将FOLLOW集中的非终结符按照V中的非终结符的顺序填进去
	for (int i = 0; i < V->len_vn; ++i) strcpy(NODE_[i].vn, V->vn[i]);
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
