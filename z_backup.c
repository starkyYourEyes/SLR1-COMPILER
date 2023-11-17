// SLR1只针对有LR0分析表有冲突的进行分析，
// 1. 移进-规约冲突
// 2. 规约-规约冲突
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#define COUNT 20
#define ITEM_LEN 5
#define MAX_LEN_PRODUCTION 20
#define LINE_MAX 128
#define MAX_LEN_VN 3
#define MAX_LEN_VT 10
#define MAX_STATUS_NEXT 20
#define NUM_PER_SET 20
typedef char production[MAX_LEN_PRODUCTION];
int line_num;
production lines[32];
struct next_status{
    int status;             // 指向的下一个项目集的UID
    char edge[MAX_LEN_VT];  // 通过那条边指向下一个项目集，即通过什么字符到达的
};


// int **r_action(){
//     int **Action=(int **)malloc(UID * sizeof(int*));
//     for (int i = 0; i < UID; i++) 
//         Action[i] = (int *)malloc(V->len_vt * sizeof(int));
//     for(int i = 0; i < UID; i ++) {
//         printf("action: %d \n",i);
//         for (int j = 0; j < V->len_vt; ++ j) { 
//             if (TABLE_ITEM[i].ACTION[j][0] == 'S')   
//                 Action[i][j] = atoi(TABLE_ITEM[i].ACTION[j] + 1);
//             else if (TABLE_ITEM[i].ACTION[j][0] == 'a')    
//                 Action[i][j] = UID; //获取数字部分， acc
//             else if(TABLE_ITEM[i].ACTION[j][0]=='r')    
//                 Action[i][j] = 100 * (atoi(TABLE_ITEM[i].ACTION[j] + 1));
//             else if(strlen(TABLE_ITEM[i].ACTION[j]) == 0)
//                 Action[i][j] = -1;
//             else {
//                 printf("error in r_action");
//                 exit(-1);
//             }
//             printf("%d ",Action[i][j]);
//         }
//         printf("\n");
//     }
//     return Action;
// }
// int **r_goto() {
//     //分配空间
//     int **Goto=(int **)malloc(UID * sizeof(int*));
//     for (int i = 0; i < UID; i++) Goto[i] = (int *)malloc(V->len_vn * sizeof(int));
//     for(int i = 0; i < UID; i ++) {
//         printf("goto: %d \n", i);
//         for (int j = 0; j < V->len_vn; ++j) {
//             if(strlen(TABLE_ITEM[i].GOTO[j]) != 0) {
//                 Goto[i][j] = atoi(TABLE_ITEM[i].GOTO[j]);   //获取数字部分
//                 printf("%d ", Goto[i][j]);
//                 continue;
//             } else {   
//                 Goto[i][j] = -1;
//                 printf("%d ",Goto[i][j]);
//             } 
//         }
//         printf("\n");
//     }
//     return Goto;
// }

struct lr_item{
    int loc;                        // 其中的·的位置
    production item;  // LR分析的项目
    bool operated;                  // 属于某一个项目集的某个项目在移进的时候是不是已经做过了
};
struct lr_item_set {  
    // lr项目集
    int status;                     // 项目集的名字，如I0,I2...,这里用数字表示0，1，2，3
    int cnt;                        // 项目集中项目的个数
    struct lr_item item_set[NUM_PER_SET];    // 这里假设每一个项目集中最多有20个项目
    // char edge[MAX_LEN_VT];       // 状态图中的边
    // to be optimized
    int cnt_next_status;            // 用于下面的那个数组的计数
    // 存储图的指向的序号（status）的集合，这里假设最多指向MAX_STATUS_NEXT个
    struct next_status next[MAX_STATUS_NEXT];    
};
/*。。。。。。。。。。。。。。。。。。。。。。。。。。*/
int UID; // 分配给每一个项目集的唯一的编号，即项目集的status
struct lr_item_set* ALL_LR_ITEM_SET[COUNT];// 一个指针数组, 用来寻找所有的 lr_item_set，记录起来

struct table_item { 
    // SLR分析表的每一行。
    int status;         // 每一行的编号，也即项目集编号
    int action_idx;
    char ACTION[40][ITEM_LEN];
    int goto_idx;
    char GOTO[20][ITEM_LEN];
} TABLE_ITEM[COUNT];        // 分析表有多少行(项目集有多少个), COUNT就取多少，可以malloc???
struct CHARS{
	int len_vn;				// 非终结符的个数
	char vn[20][MAX_LEN_VN];         // 非终结符, 最长为3, S' + '\0'
	int len_vt;				// 终结符的个数
	char vt[40][MAX_LEN_VT];        // 终结符
} *V;

bool is_vn(char ch){ 
	// 判断字符是不是非终结符, 这里假设非终结符只有一个字母，因为S'只出现在产生式左侧，这里忽略他不计
	for (int i = 0; i < V->len_vn; ++i)
		if (ch == V->vn[i][0])
			return true;
	return false;
}
int get_vt_no(char* vt){
	// 找到这个终结符的编号，
	// 因为终结符的 顺序都是按照V中的顺序来的，所以终结符的顺序唯一，只需要确定其编号
	for (int i = 0; i < V->len_vt; ++ i){
		if (strcmp(vt, V->vt[i]))
			return i;
	}
	return -1;
}
bool is_alpha(char ch){
	return ch <= 'z' && ch >= 'a';
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

int get_vs(char *path){
	// 获取所有的终结符和非终结符
	FILE *fp;
	int line_num = 0;		  // 文件行数
	int line_len = 0;		  // 文件每行的长度
	char buf[LINE_MAX] = {0}; // 行数据缓存
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
			// if (0 == line_len)
			// 	continue; //空行
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
int add_item_to_set(struct lr_item_set* set, int i){ // 参数i表示是第几个产生式
    // 每一次往一个项目集中添加一个新的项目的时候，这个项目的·都在产生式右边的最左边。
    struct lr_item it;
    it.operated = false;
    strcpy(it.item, lines[i]);
    it.loc = get_production_right(it.item);
    set->item_set[set->cnt ++] = it;
    if (set->cnt >= 10) return -1;          // 超长了 
    return 1;
}
bool is_front_repeated(struct lr_item_set* S, char str[]){
    for (int i = 0; i < S->cnt_next_status; ++ i)
        if (strcmp(str, S->next[i].edge) == 0)  
            return true;
    return false;
}
bool equal_prefix(char *s, char *t){
    // s是直到他的长度 的，所以s要放在前面
    // printf("---compares:\n");
    for (int i = 0; s[i]; ++ i)
        if (s[i] != t[i]) return false;
    return true;
}
struct lr_item_set* init_lr_item_set(){
    // 初始化一个lr_item_set并返回
    struct lr_item_set *S = (struct lr_item_set*)malloc(sizeof(struct lr_item_set));
    ALL_LR_ITEM_SET[UID] = S;  // 记录在指针数组中
    S->status = UID ++, S->cnt = 0, S->cnt_next_status = 0;        // 记得初始化值！
    // 将opearated置为false
    memset(S->item_set, 0, NUM_PER_SET * sizeof(struct lr_item));
    // for (int i = 0; i < 10; ++ i)
    //     if (!S->item_set[i].operated) printf("\n\nhhhhhhhhhhhhhhhhhh\n\n");
    // 将对应的next_status中的字符串置0
    memset(S->next, 0, MAX_STATUS_NEXT * (sizeof(struct next_status)));
    return S; 
}

bool is_item_left(struct lr_item_set* S){
    for (int i = 0; i < S->cnt; ++ i)
        if (!S->item_set[i].operated)
            return true;
    return false;
}
void expand(struct lr_item_set* S){
    // 项目集的 核 开始扩张
    for (int i = 0; i < S->cnt; ++ i){
        char c = S->item_set[i].item[0];       
    }
}
void shift(struct lr_item_set* S){
    // 移进
    printf("cnt:%d\n", S->cnt);

    // while (is_item_left(S)){
    //             // 之前的那个项目集还有可以移进的项目
    //             for (int i = 0; i < S->cnt; ++ i){
                    
    //             }
    //         }
    for (int i = 0; i < S->cnt; ++ i){
        int loc = S->item_set[i].loc;
        char c = S->item_set[i].item[loc];
        char tmp[MAX_LEN_VT];
        if (is_vn(c)) tmp[0] = c, tmp[1] = '\0';// 非终结符，移进一位
        // 终结符，移进相应长度的位数, 
        // 词法分析的时候没错，is_prefix()这里一定不会出错.
        else {
            char *s = is_prefix(S->item_set[i].item + loc);
            if (s == NULL){
                printf("error!!!!!!\n");
                exit(-1);
            }
            strcpy(tmp, s);
        }

        // for (int j = 0; j < S->cnt && !S->item_set[j].operated; ++ j){
        //     // 如果移进那个的字符没有重复，新建一个项目集
        //     printf("tmp:%s\n", tmp);
        //     struct lr_item_set* new = init_lr_item_set();
        //     printf("comp:%s, %s\n", tmp, S->item_set[j].item + S->item_set[j].loc);
        //     if (equal_prefix(tmp, S->item_set[j].item + S->item_set[j].loc)){
        //         strcpy(new->item_set[new->cnt].item, S->item_set[j].item);
        //         int k;  // 跳过空格
        //         for (k = loc + strlen(tmp); new->item_set[new->cnt].item[k] && new->item_set[new->cnt].item[k] == ' '; ++ k){};
        //         new->item_set[new->cnt].loc = k;    // 更新loc
        //         // to be optimized. 
        //         // 并且如果扫描到了结尾了-----处理一下to do
        //         // int new_loc = new->item_set[new->cnt].loc;
        //         // if (S->item_set[i].item[loc] == '\0') {
        //         //     // · 到结尾了
        //         // }
        //         new->cnt ++;
        //         S->item_set[j].operated = true; // 标记为已经扫描过
        //         int new_loc = new->item_set[new->cnt].loc;
        //         if (S->item_set[i].item[loc] == '\0') {
        //             // · 到结尾了
        //         }
        //     }
        // }
        
        if (!is_front_repeated(S, tmp)){
            // 如果移进那个的字符没有重复，才新建一个项目集（新建项目集的依据！
            // printf("tmp:%s\n", tmp);
            struct lr_item_set* new = init_lr_item_set();
            // 把信息复制过去
            S->next[S->cnt_next_status].status = new->status;   // UID记录
            strcpy(S->next[S->cnt_next_status].edge, tmp);
            S->cnt_next_status ++;
            // 找到了，给这个新项目集添加项目
            // for (int j = 0; j < S->cnt && !S->item_set[j].operated; ++ j){
            for (int j = 0; j < S->cnt; ++ j){
                if (S->item_set[j].operated) continue;
                // printf("comp:%s, %s\n", tmp, S->item_set[j].item + S->item_set[j].loc);
                if (equal_prefix(tmp, S->item_set[j].item + S->item_set[j].loc)){
                    printf("%s equals %s\n", tmp, S->item_set[j].item + S->item_set[j].loc);
                    // printf("")
                    strcpy(new->item_set[new->cnt].item, S->item_set[j].item);
                    int k;  // 跳过空格
                    for (k = loc + strlen(tmp); new->item_set[new->cnt].item[k] && new->item_set[new->cnt].item[k] == ' '; ++ k){};
                    new->item_set[new->cnt].loc = k;    // 更新loc

                    // to be optimized. 
                    // 并且如果扫描到了结尾了-----处理一下to do
                    // int new_loc = new->item_set[new->cnt].loc;
                    // if (S->item_set[i].item[loc] == '\0') {
                    //     // · 到结尾了

                    // }
                    new->cnt ++;
                    S->item_set[j].operated = true; // 标记为已经扫描过
                    // printf("%s is operated!!!!!\n", tmp);
                    int new_loc = new->item_set[new->cnt].loc;
                    if (S->item_set[i].item[loc] == '\0') {
                        // · 到结尾了

                    }
                }
            }
        }


        printf("===%d, %s\n", S->item_set[i].loc, S->item_set[i].item);
    }
}
/*移进-归约冲突（shift-reduce conflict）*/
void reduce(){
    // 规约
}

void init(struct lr_item_set** S){
    // 求初始的第一个 项目集。
    UID = 0;
    // *S = (struct lr_item_set*)malloc(sizeof(struct lr_item_set));
    // (*S)->status = UID ++, (*S)->cnt = 0, (*S)->cnt_next_status = 0;        // 记得初始化值！
    (*S) = init_lr_item_set();
    add_item_to_set(*S, 0);
    printf("%d, %d, %s\n", (*S)->cnt, (*S)->item_set[0].loc, (*S)->item_set[0].item);
    char ct_vn = (*S)->item_set[0].item[(*S)->item_set[0].loc];   
    // 当前的第一个符号，如果是一个非终结符，则要在I0中添加项目，如果不是直接忽略
    if (is_vn(ct_vn)) 
        for (int i = 0; i < line_num; ++ i){
            int left = get_production_left(lines[i]);
            if (left > 1) break;
            if (ct_vn == lines[i][left]) add_item_to_set(*S, i);
        }
    
    for (int i = 0; i < (*S)->cnt; ++ i)
        printf("%d, %s\n", (*S)->item_set[i].loc, (*S)->item_set[i].item);
}


int main(int argc, char *argv[]){
    read_lines(argv[1]);
    for (int i = 0; i < line_num; ++ i)
        printf("%s\n", lines[i]);
    // 获得所有的终结符和非终结符
    V = (struct CHARS *)malloc(sizeof(struct CHARS));
	V->len_vn = V->len_vt = 0;
    get_vs(argv[1]);

    // 初始化求所有的项目集
    struct lr_item_set* S;
    init(&S);
    // ALL_LR_ITEM_SET = (struct lr_item_set*)malloc(COUNT * sizeof(struct lr_item_set));
    // printf("len:%d\n", S->cnt);
    shift(S);
    printf("UID:%d\n", UID);
    printf("next num:%d\n", S->cnt_next_status);
    for (int i = 0; i < S->cnt_next_status; ++ i){
        printf("%d, %s\n", S->next[i].status, S->next[i].edge);
    }
    printf("====================\n");
    for (int i = 0; i < UID; ++ i){
        for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt; ++ j)
            printf("%d, %s\n", ALL_LR_ITEM_SET[i]->item_set[j].loc, ALL_LR_ITEM_SET[i]->item_set[j].item);
        printf("------------------------------\n");
    }
    // {
    //     printf("%d Vns: \n", V->len_vn);
    //     for (int i = 0; i < V->len_vn; ++i){
    //         printf("%s", V->vn[i]);
    //         if (i < V->len_vn - 1) printf(", ");
    //     }
    //     printf("\n");
    //     printf("%d Vts: \n", V->len_vt);
    //     for (int i = 0; i < V->len_vt; ++i){
    //         printf("%s, ", V->vt[i]);
    //         if (i < V->len_vn - 1) printf(", ");
    //     }
    //     printf("\n");
    // }
    

    return 0;
}