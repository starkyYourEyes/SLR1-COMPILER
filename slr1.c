// SLR1只针对有LR0分析表有冲突的进行分析，
// 1. 移进-规约冲突
// 2. 规约-规约冲突
// first 集 和 follow集合要事先生成！
// 非终结符集中多了一个 # ！
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#define COUNT 96            // 最多有多少个项目集
#define ITEM_LEN 5          // S12, r4...
#define MAX_LEN_PRODUCTION 20
#define LINE_MAX 128        // 读取的时候读取的每一行的最大长度
#define MAX_NUM_VN 20       // VN的最多个数
#define MAX_NUM_VT 40       // VT的最多个数
#define MAX_LEN_VN 3        // VN的最大长度
#define MAX_LEN_VT 10       // VT的最大长度
#define MAX_STATUS_NEXT 20  // 每一个项目集通过移进而到达的新的项目集的最大个数
#define NUM_PER_SET 20      // 每一个项目集中最多的项目数
#define MAX_STACK_SIZE 256
typedef char production[MAX_LEN_PRODUCTION];
typedef char cpp_string[ITEM_LEN];
int line_num;
production lines[32];
struct SET{                     // first集，follow集
	char vn[MAX_LEN_VN];	    // 非终结符的名称
	int cnt;				    // first集中终结符的个数
	// set存储的是first集和follow集中的元素，且假设每个非终结符的first、follow集中的元素个数不超过20个
	// 在NODE_中表示计算这个非终结符的FOLLOW集所依赖的其他的非终结符
	char set[20][MAX_LEN_VT]; 
} FIRST_[20], FOLLOW_[20];
struct next_status{
    int status;             // 指向的下一个项目集的UID
    char edge[MAX_LEN_VT];  // 通过那条边指向下一个项目集，即通过什么字符到达的
};
struct lr_item{
    int loc;                        // 其中的·的位置
    production item;                // LR分析的项目
    bool operated;                  // 属于某一个项目集的某个项目在移进的时候是不是已经做过了
};
struct lr_item_set {  
    // lr项目集
    int status;                     // 项目集的名字，如I0,I2...,这里用数字表示0，1，2，3
    int core;                       // 该项目集的核 的个数
    int cnt;                        // 项目集中项目的个数
    struct lr_item item_set[NUM_PER_SET];    // 这里假设每一个项目集中最多有20个项目
    // char edge[MAX_LEN_VT];       // 状态图中的边
    // to be optimized
    int cnt_next_status;            // 用于下面的那个数组的计数
    // 存储图的指向的序号（status）的集合，这里假设最多指向MAX_STATUS_NEXT个
    struct next_status next[MAX_STATUS_NEXT];
    bool can_reduce;                // 这个项目集是否可以规约
};
/*。。。。。。。。。。。。。。。。。。。。。。。。。。*/
int UID;            // 分配给每一个项目集的唯一的编号，即项目集的status
int CONTINUE_;      // 标记是否继续扩充项目集（项目集是否还再变化）
struct lr_item_set* ALL_LR_ITEM_SET[COUNT];// 一个指针数组, 用来寻找所有的 lr_item_set，记录起来

struct table_item { 
    // SLR分析表的每一行。
    int status;                 // 每一行的编号，也即项目集编号
    // int action_idx;
    cpp_string ACTION[MAX_NUM_VT];      // 假设终结符最多40个，lazy
    // int goto_idx;
    cpp_string GOTO[MAX_NUM_VN];        // 假设非终结符最多20个，lazy 2
} TABLE_ITEM[COUNT];        // 分析表有多少行(项目集有多少个), COUNT就取多少，可以malloc???

struct CHARS{
	int len_vn;				// 非终结符的个数
	char vn[MAX_NUM_VN][MAX_LEN_VN];// 非终结符, 最长为3, S' + '\0'
	int len_vt;				// 终结符的个数
	char vt[MAX_NUM_VT][MAX_LEN_VT];// 终结符
} *V;

struct status_stack{    // 状态栈
    int idx;
    char stack[MAX_STACK_SIZE][ITEM_LEN];
};
struct char_stack{      // 符号栈
    int idx;
    char stack[MAX_STACK_SIZE][MAX_LEN_VT];
};
// struct input_type{
//     int type;               // type = 1 -> 非终结符
//     char *input[ITEM_LEN];  // type = 0-> 终结符
// };
int _STEP;
struct analysis_item{
    int step;                       // 步骤
    struct status_stack stat_stk;   // 状态栈
    struct char_stack char_stk;     // 符号栈
    char str_now[MAX_LEN_VT];       // 输入串 -> 当前遇到的字符
    char Action[ITEM_LEN];          // ACTION
    char Goto[ITEM_LEN];              // GOTO
} analyses[128];

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
		if (strcmp(vt, V->vt[i]) == 0)
			return i;
	}
	return -1;
}
int get_vn_no(char* vn){
	// 找到这个非终结符的编号，
	// 因为非终结符的 顺序都是按照V中的顺序来的，所以非终结符的顺序唯一，只需要确定其编号
	for (int i = 0; i < V->len_vn; ++ i)
		if (strcmp(vn, V->vn[i]) == 0)
			return i;
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
			if (0 == line_len)
				continue; //空行
		}
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
			buf[line_len - 1] = '\0', line_len--;
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
	while (loc > 0 && line[loc] == ' '){ loc --; }; // 跳过空格

	return loc;
}

int get_production_right(char* line){
	// 找到产生式的右边的开始的位置
	int loc = 0;
	for(; line[loc] && line[loc] != '>'; ++ loc){};	// 找到 > 的位置
	loc ++;
	while (line[loc] && line[loc] == ' '){ loc ++; }; // 跳过空格
    // to do, to be
	if (loc > strlen(line)) return -1; // 错误
	return loc;
}
int add_item_to_set(struct lr_item_set* set, int i){ // 参数i表示是第几个产生式
    // 每一次往一个项目集中添加一个新的项目的时候，这个项目的·都在产生式右边的最左边。
    struct lr_item it;
    it.operated = false;
    strcpy(it.item, lines[i]);
    it.loc = get_production_right(it.item);
    bool flag = true;
    for (int j = 0; j < set->cnt; ++ j)
        // 没有重复（·的位置和字符串都不重复）的才添加进去
        if (set->item_set[j].loc == it.loc && strcmp(set->item_set[j].item, it.item) == 0)
            flag = false;
            
    if (flag){
        set->item_set[set->cnt ++] = it;
        if (set->cnt >= NUM_PER_SET) return -1; 
    }
        // 超长了 
    return 1;
}
bool is_front_repeated(struct lr_item_set* S, char str[]){
    // 在移进的时候是否遇到重复的字符（串
    for (int i = 0; i < S->cnt_next_status; ++ i)
        if (strcmp(str, S->next[i].edge) == 0)  
            return true;
    return false;
}
bool equal_prefix(char *s, char *t){
    // s是直到他的长度 的，所以s要放在前面
    // printf("---compares:\n");
    for (int i = 0; s[i]; ++ i)
        if (s[i] != t[i]) 
            return false;
    return true;
}

struct lr_item_set* init_lr_item_set(){
    // 初始化一个lr_item_set并返回
    struct lr_item_set *S = (struct lr_item_set*)malloc(sizeof(struct lr_item_set));
    ALL_LR_ITEM_SET[UID] = S;   // 记录在指针数组中
    // 记得初始化值！
    S->status = UID ++, S->core = 0, S->cnt = 0, S->cnt_next_status = 0; 
    S->can_reduce = false;      // 是否可以规约，初始为false       
    // 将opearated置为false
    memset(S->item_set, 0, NUM_PER_SET * sizeof(struct lr_item));
    // for (int i = 0; i < 10; ++ i)
    //     if (!S->item_set[i].operated) printf("\n\nhhhhhhhhhhhhhhhhhh\n\n");
    // 将对应的next_status中的字符串置0
    memset(S->next, 0, MAX_STATUS_NEXT * (sizeof(struct next_status)));
    return S; 
}
void del_lr_item_set(struct lr_item_set **S){
    UID --;     // UID为全局变量！
    printf("%s was freed!!\n", (*S)->item_set[0].item);
    free(*S);
}
int is_itemset_repeated(struct lr_item_set* S){
    // to be optimized , to do
    // 判断是否有一样的项目集, 即核一样，在新增加完项目集并且把核添加进去了后判断。
    // 如果有重复的，就把重复的那个的UID返回回去
    // 无重复返回-1
    // printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    for (int i = 0; i < UID - 1; ++ i){ // < UID - 1, 因为不包括自己
        // 先找核中项目数一样的
        if (ALL_LR_ITEM_SET[i]->core == S->core){
            // printf("in comp:%s, %s\n", ALL_LR_ITEM_SET[i]->item_set[0].item, S->item_set[0].item);
            bool flag[COUNT] = {false};
            // printf("S-cnt:%d, A-cnt:%d\n", S->core, ALL_LR_ITEM_SET[i]->core);
            // printf("'''''''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
            for (int j = 0; j < S->core; ++ j){
                for (int k = 0; k < ALL_LR_ITEM_SET[i]->core; ++ k){
                    if (ALL_LR_ITEM_SET[i]->item_set[k].loc == S->item_set[j].loc)
                        if (strcmp(ALL_LR_ITEM_SET[i]->item_set[k].item, S->item_set[j].item) == 0)
                            flag[j] = true;
                    // printf("A: %d, %s\n", ALL_LR_ITEM_SET[i]->item_set[k].loc, ALL_LR_ITEM_SET[i]->item_set[k].item);
                    // printf("B: %d, %s\n", S->item_set[j].loc, S->item_set[j].item);
                }
            }
            int k;
            for (k = 0; k < S->core; ++ k)
                if (!flag[k]) break;
            if (k >= S->core) return i;
        }
    }
    // printf("*********************************************************\n");
    // printf("true!!!!!!!!!!!!!!\n");
    return -1;
}
bool is_item_left(struct lr_item_set* S){
    for (int i = 0; i < S->cnt; ++ i)
        if (!S->item_set[i].operated)
            return true;
    return false;
}
void shift(struct lr_item_set* S);
void expand(struct lr_item_set* S){
    // 项目集的 核 开始扩张
    printf("UID:%d, cnt:%d\n", S->status, S->cnt);
    int scnt = S->cnt;  // 循环过程中，S->cnt会发生改变！！！！
    for(int i = 0; i < S->cnt; ++ i){
        printf("expand() %d, %1c, %s\n", S->item_set[i].loc, S->item_set[i].item[S->item_set[i].loc], S->item_set[i].item);
    }
    printf("???????????????????\n");
    for (int i = 0; i < S->cnt; ++ i){
        int loc = S->item_set[i].loc;
        // 达到了末尾，即是一个规约状态（ LR(0), SLR(1)待定
        // to be optomized, to do
        char ch = S->item_set[i].item[loc]; 
        printf("ct_loc:%d, ch:%c, ct_produce:%s\n", loc, ch, S->item_set[i].item);

        if (ch == '\0') continue;
        // 当前的第一个符号，如果是一个非终结符，则要在I0中添加项目，如果不是直接忽略
        if (is_vn(ch)) 
            for (int j = 1; j < line_num; ++ j){    // 第0行是S',不考虑它
                int left = get_production_left(lines[j]);
                // 大于1就出错了，因为最长的S'的left才是1
                if (left > 0) {
                    printf("left:%d, in line %d:%s  error left, unrecommended space ' '!\n", left, j, lines[j]); 
                    exit(-1);
                }     
                if (ch == lines[j][left]) add_item_to_set(S, j);
            }  
    }
    if (CONTINUE_ != UID){
        CONTINUE_ = UID;
        shift(S);
    }
        
    // static int x = 0;
    // if (x <= 80){
    //     shift(S);
    // }
    // x ++;
    
    // // 这个重复是在shift里面判断就行（判断核是不是有重复
    // if (is_itemset_repeated(S)){ 
    //     // if (pre != NULL){
    //     //     pre->cnt_next_status --;
    //     //     pre->next[pre->cnt_next_status].status = pre->status;   // UID记录
    //     //     // to be optimized,  to do
    //     //     strcpy(pre->next[pre->cnt_next_status].edge, "?");
    //     //     pre->cnt_next_status ++;
    //     // }

    //     // del_lr_item_set(&S);
    // } else{
    //     // shift(S);
    // }
}

void shift(struct lr_item_set* S){
    // 移进
    printf("shift() cnt:%d\n", S->cnt);
    for (int i = 0; i < S->cnt; ++ i){
        int loc = S->item_set[i].loc;
        char c = S->item_set[i].item[loc];
        if (c == '\0') {
            S->can_reduce = true;   // ·到达末尾，可以进行规约
            continue;  // to do, to be optimized
        }
        char tmp[MAX_LEN_VT];
        if (is_vn(c)) tmp[0] = c, tmp[1] = '\0';// 非终结符，移进一位
        // 终结符，移进相应长度的位数, 
        // 词法分析的时候没错，is_prefix()这里一定不会出错.
        else {
            char *s = is_prefix(S->item_set[i].item + loc);
            if (s == NULL){
                printf("ct:%s\n", S->item_set[i].item + loc);
                for (int i = 0; i < S->cnt; ++ i){
                    printf("%d, %s\n", S->item_set[i].loc, S->item_set[i].item);
                }
                printf("error!!!!!!\n");
                exit(-1);
            }
            strcpy(tmp, s);
        }
       
        if (!is_front_repeated(S, tmp)){
            // 如果移进那个的字符没有重复，才新建一个项目集（新建项目集的依据！
            // printf("tmp:%s\n", tmp);
            struct lr_item_set* new = init_lr_item_set();
            // 把信息复制过去
            S->next[S->cnt_next_status].status = new->status;   // UID记录
            strcpy(S->next[S->cnt_next_status].edge, tmp);      // 边记录
            S->cnt_next_status ++;
            // 找到了，给这个新项目集添加 核
            int core = 0;
            for (int j = 0; j < S->cnt; ++ j){
                if (S->item_set[j].operated) continue;
                // printf("comp:%s, %s\n", tmp, S->item_set[j].item + S->item_set[j].loc);
                int a_loc = S->item_set[j].loc;
                if (equal_prefix(tmp, S->item_set[j].item + S->item_set[j].loc)){
                    // printf("%s equals %s\n", tmp, S->item_set[j].item + S->item_set[j].loc);
                    core ++;
                    strcpy(new->item_set[new->cnt].item, S->item_set[j].item);
                    int k;  // 跳过空格
                    for (k = a_loc + strlen(tmp); new->item_set[new->cnt].item[k] && new->item_set[new->cnt].item[k] == ' '; ++ k){};
                    new->item_set[new->cnt].loc = k;    // 更新loc
                    printf("%d, %d, %d == ", k, a_loc, strlen(tmp));
                    printf("!!!!! UID is:%d, a_loc:%d, %s\n", UID - 1, k, new->item_set[new->cnt].item);
                    new->cnt ++;
                    S->item_set[j].operated = true; // 标记为已经扫描过
                    // printf("%s is operated!!!!!\n", tmp);
                    // to be optimized. 
                    // 并且如果扫描到了结尾了-----处理一下to do
                    int new_loc = new->item_set[new->cnt].loc;
                    if (S->item_set[i].item[new_loc] == '\0') {
                        // · 到结尾了to do
                    }
                }  
            }
            new->core = core;   // 核中项目的个数
            // to do, to be optimized
            int res = is_itemset_repeated(new);
            printf("%d new core is:\n", new->core);
            for (int x = 0; x < new->core; ++ x){
                int y = new->item_set[x].loc;
                printf("--%d, %1c, %s\n", y, new->item_set[x].item[y], new->item_set[x].item);
            }
            printf("res:%d\n", res);
            if (res != -1){
                S->cnt_next_status --;
                S->next[S->cnt_next_status].status = ALL_LR_ITEM_SET[res]->status;   // UID记录
                strcpy(S->next[S->cnt_next_status].edge, tmp);
                S->cnt_next_status ++;
                del_lr_item_set(&new);
            }
            else expand(new);
            // expand(new);
        }
        
        printf("===%d, %c, %s\n", S->item_set[i].loc, S->item_set[i].item[S->item_set[i].loc], S->item_set[i].item);
    }
}
/*移进-归约冲突（shift-reduce conflict）*/
void reduce(){
    // 规约
}

void init(struct lr_item_set** S){
    // 求初始的第一个 项目集。
    UID = 0;
    CONTINUE_ = -1;
    // *S = (struct lr_item_set*)malloc(sizeof(struct lr_item_set));
    // (*S)->status = UID ++, (*S)->cnt = 0, (*S)->cnt_next_status = 0;        // 记得初始化值！
    (*S) = init_lr_item_set();
    add_item_to_set(*S, 0);
    (*S)->core = 1;
    expand(*S);
    printf("%d, %d, %s\n", (*S)->cnt, (*S)->item_set[0].loc, (*S)->item_set[0].item);
    
    // char ct_vn = (*S)->item_set[0].item[(*S)->item_set[0].loc];   
    // // 当前的第一个符号，如果是一个非终结符，则要在I0中添加项目，如果不是直接忽略
    // if (is_vn(ct_vn)) 
    //     for (int i = 0; i < line_num; ++ i){
    //         int left = get_production_left(lines[i]);
    //         // 大于1就出错了，因为最长的S'的left才是1
    //         if (left > 1) {printf("error left!\n"); break;}    
    //         if (ct_vn == lines[i][left]) add_item_to_set(*S, i);
    //     }
    
    for (int i = 0; i < (*S)->cnt; ++ i)
        printf("%d, %s\n", (*S)->item_set[i].loc, (*S)->item_set[i].item);
}
int get_production_no(char *prod){
    // 获取产生式的编号
    for (int i = 0; i < line_num; ++ i)
        if (strcmp(lines[i], prod) == 0)
            return i;
    return -1;
}

int **r_action(){
    int **Action=(int **)malloc(UID * sizeof(int*));
    for (int i = 0; i < UID; i++) 
        Action[i] = (int *)malloc(V->len_vt * sizeof(int));
    for(int i = 0; i < UID; i ++) {
        printf("action: %d \n",i);
        for (int j = 0; j < V->len_vt; ++ j) { 
            if (TABLE_ITEM[i].ACTION[j][0] == 'S')   
                Action[i][j] = atoi(TABLE_ITEM[i].ACTION[j] + 1);
            else if (TABLE_ITEM[i].ACTION[j][0] == 'a')    
                Action[i][j] = UID; //获取数字部分， acc
            else if(TABLE_ITEM[i].ACTION[j][0]=='r')    
                Action[i][j] = 100 * (atoi(TABLE_ITEM[i].ACTION[j] + 1));
            else if(strlen(TABLE_ITEM[i].ACTION[j]) == 0)
                Action[i][j] = -1;
            else {
                printf("error in r_action");
                exit(-1);
            }
            printf("%d ",Action[i][j]);
        }
        printf("\n");
    }
    return Action;
}
int **r_goto() {
    //分配空间
    int **Goto=(int **)malloc(UID * sizeof(int*));
    for (int i = 0; i < UID; i++) Goto[i] = (int *)malloc(V->len_vn * sizeof(int));
    
    for(int i = 0; i < UID; i ++) {
        printf("goto: %d \n", i);
        for (int j = 0; j < V->len_vn; ++j) {
            if(strlen(TABLE_ITEM[i].GOTO[j]) != 0) {
                Goto[i][j] = atoi(TABLE_ITEM[i].GOTO[j]);   //获取数字部分
                printf("%d ", Goto[i][j]);
                continue;
            } else {   
                Goto[i][j] = -1;
                printf("%d ",Goto[i][j]);
            } 
        }
        printf("\n");
    }
    return Goto;
}


void read_fisrt_follow_sets(){
    for (int i = 0; i < V->len_vn; ++i) strcpy(FIRST_[i].vn, V->vn[i]);
    for (int i = 0; i < V->len_vn; ++i) strcpy(FOLLOW_[i].vn, V->vn[i]);
    FILE* fp = fopen("first-follow-set.txt", "r");
    if (fp == NULL){
        printf("read %s failed.", "first-follow-set.txt");
        exit(-1);
    }
    char buf[128];
    
    int mode = 0;   // mode = 0 表示读取first集，1 表示读取follow集
    int cnt = 0;

    while (fgets(buf, LINE_MAX, fp)){
		int line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r', 空格' '
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1] || ' ' == buf[line_len - 1]){
			buf[line_len - 1] = '\0';
			line_len--;
		}
        if (0 == line_len){
            cnt = 0;
            mode = 1;
            continue; //空行
        }
        int loc = 0;
		for (loc = 0; buf[loc] != ':'; ++ loc){};
        loc ++;
        printf("%s\n", buf + loc);
        // 依次读取first集和follow集
        int no = 0;
        printf("cnt:%d\n", cnt);
        while (buf[loc]){ // 双指针。。。
            int j = loc;
            for (; buf[j] && buf[j] != ' '; ++ j){};
            // printf("%s, ", buf + j);
            if (mode == 0){
                FIRST_[cnt].cnt ++;
                FIRST_[cnt].set[no][j - loc] = '\0';
                printf("FIRST_ appended %s, ", buf + loc);
                strncpy(FIRST_[cnt].set[no], buf + loc, j - loc);
                printf("%s\n", FIRST_[cnt].set[no]);
                no ++;
            } else {
                FOLLOW_[cnt].cnt ++;
                FOLLOW_[cnt].set[no][j - loc] = '\0';
                strncpy(FOLLOW_[cnt].set[no ++], buf + loc, j - loc);
            }
            // cnt = cnt % V->len_vn;
            if (buf[j]) loc = j + 1;
            else loc = j;
            // printf("%s\n", buf + loc);
        }
        cnt ++;
        // printf("\n");
	}

    fclose(fp);
}
int is_in_follow_set(char *vn, char *s){
    // 判断vn的follow集包不包含s
    // printf("vn:%s\n", vn);
    int no = get_vn_no(vn);
    if (no == -1){
        printf("find vn is:%s\n", vn);
        printf("find error!\n");
        exit(-1);
    }
    for (int i = 0; i < FOLLOW_[no].cnt; ++ i)
        if (strcmp(FOLLOW_[no].set[i], s) == 0)
            return i;
    return -1;
}
bool is_null_unite_sets(char *v1, char *v2){
    // 判断两个vn的follow集是不是有交集
    int n1 = get_vn_no(v1), n2 = get_vn_no(v2);
    for (int i = 0; i < FOLLOW_[n1].cnt; ++ i)
        for (int j = 0; j < FOLLOW_[n2].cnt; ++ j)
            if (strcmp(FOLLOW_[n1].set[i], FOLLOW_[n2].set[j]) == 0)
                return false;
    return true;
}

void lr_table_generator(){
    // TABLE_ITEM是全局变量，默认初始化为0了
    memset(TABLE_ITEM, 0, COUNT * sizeof(struct table_item));
    for (int i = 0; i < V->len_vt; ++ i)
        printf("%s, ", V->vt[i]);
    printf("\n");
    for (int i = 0; i < UID; ++ i){
        TABLE_ITEM[i].status = i;
        bool reduce = ALL_LR_ITEM_SET[i]->can_reduce;
        int which = -1; // 如果是有移进-规约冲突，which记录下来是哪一个vn
        int rs[COUNT] = {0};    // 记录哪些项目可以进行规约了
        int num_rs = 0;
        int ts[COUNT] = {0};    // 可以移进的项目的非终结符的序号
        int num_ts = 0;
        int no_ts[COUNT] = {0}; // 可以移进的项目的序号
        int num_no_ts = 0;

        if (reduce){
            // 查看是否有规约-规约冲突
            for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt; ++ j){    // 可以规约的只在核里面
                int loc = ALL_LR_ITEM_SET[i]->item_set[j].loc;
                if (ALL_LR_ITEM_SET[i]->item_set[j].item[loc] == '\0'){
                    // if ()
                    which = j;
                    rs[num_rs ++] = j;
                } else {    // is_prefix返回的是一个非终结符 或者 NULL
                    char *s = is_prefix(ALL_LR_ITEM_SET[i]->item_set[j].item + loc);
                    if (s == NULL) continue;
                    else {  
                        // 记录哪些是可以移进的，放在if (reduce)里面是因为这里才可能有移进-规约冲突
                        int no = get_vt_no(s);
                        ts[num_ts ++] = no;
                        no_ts[num_no_ts ++] = j;
                    }
                }
            }
            printf("attention: %d-", i);
            for (int k = 0; k < num_rs; ++ k) printf("%d ", rs[k]);
            printf("-");
            for (int k = 0; k < num_no_ts; ++ k) printf("%d ", no_ts[k]);
            printf("\n");
            printf("eeee-num_rs is:%d \n", num_rs);

            // 判断是否有包含关系 e.g. Follow(A) 包含 非终结符
            
            for (int h = 0; h < num_ts; ++ h){
                int is_error = 0;   // 不符合P139的条件，有两个及以上的可规约项目的follow集中包含同一个非终结符
                for (int k = 0; k < num_rs; ++ k){
                    // 拿到产生式左边的终结符
                    int t = get_production_left(ALL_LR_ITEM_SET[i]->item_set[rs[k]].item);
                    char tmp_vn[5] = {0};
                    for (int p = 0; p <= t; ++ p) tmp_vn[p] = ALL_LR_ITEM_SET[i]->item_set[rs[k]].item[p];
                    tmp_vn[t + 1] = '\0';
                    printf("item:%s, ", ALL_LR_ITEM_SET[i]->item_set[rs[k]].item);
                    printf("tmp_vn:%s, ct_vt:%s\n", tmp_vn, V->vt[ts[h]]);
                    if (is_in_follow_set(tmp_vn, V->vt[ts[h]]) != -1){
                        is_error ++;
                        if (is_error >= 2){
                            printf("%d\n", i);
                            printf("%s is in follow set!", V->vt[ts[h]]);
                            exit(-1);
                        }
                        
                    }
                }   
            }
            // if (num_rs == 0) {
            //     printf("eeeeeee!\n");
            //     exit(-1);
            // }
            if (num_rs == 1) { // 只有一条·到达末尾的，在其FOLLOW集里面就能规约
                // 拿到产生式左边的终结符
                printf("look:%d, %s", i, ALL_LR_ITEM_SET[i]->item_set[rs[0]].item);
                int t = get_production_left(ALL_LR_ITEM_SET[i]->item_set[rs[0]].item);
                char tmp_vn[5] = {0};
                for (int p = 0; p <= t; ++ p) tmp_vn[p] = ALL_LR_ITEM_SET[i]->item_set[rs[0]].item[p];
                tmp_vn[t + 1] = '\0';
                // int no = get_vn_no(tmp_vn);
                char tmp[5] = {0};
                int no = get_production_no(ALL_LR_ITEM_SET[i]->item_set[rs[0]].item);
                tmp[0] = 'r';
                itoa(no, tmp + 1, 10);
                printf(":::%d . %s . %s . ", i, tmp_vn, tmp);
                
                for (int j = 0; j < V->len_vt; ++ j)
                    if (is_in_follow_set(tmp_vn, V->vt[j]) != -1){
                        printf("%s -> %s, ", tmp_vn, V->vt[j]);
                        strcpy(TABLE_ITEM[i].ACTION[j], tmp);
                    }
                printf("\n");
            } else if (num_rs > 1){    // 有多个，判断是否有规约规约冲突
                for (int k = 0; k < num_rs; ++ k){
                    for (int h = k + 1; h < num_rs; ++ h){
                        char tmp1[5], tmp2[5];
                        int len = get_production_left(ALL_LR_ITEM_SET[i]->item_set[k].item);
                        for (int m = 0; m <= len; ++ m)
                            tmp1[m] = ALL_LR_ITEM_SET[i]->item_set[k].item[m];
                        tmp1[len + 1] = '\0';
                        len = get_production_left(ALL_LR_ITEM_SET[i]->item_set[h].item);
                        for (int m = 0; m <= len; ++ m)
                            tmp2[m] = ALL_LR_ITEM_SET[i]->item_set[h].item[m];
                        tmp2[len + 1] = '\0';

                        if (!is_null_unite_sets(tmp1, tmp2)){
                            printf("%s %s follow set unite is not null!\n", tmp1, tmp2);
                            exit(-1);
                        }
                    }
                }
                // // 有规约-规约冲突，查看follow集，判断SLR1 能不能解决。
                // printf("!!!reduce-reduce conflict!!!\n");
                // // exit(-1);
            }
                
        }
        for(int j = 0; j < ALL_LR_ITEM_SET[i]->cnt_next_status; ++ j){
            char tmp[10];
            int no = get_vt_no(ALL_LR_ITEM_SET[i]->next[j].edge);
            
            if (no != -1){
                // 终结符，添加到ACTION
                tmp[0] = 'S';
                itoa(ALL_LR_ITEM_SET[i]->next[j].status, tmp + 1, 10);  // int to str
                strcpy(TABLE_ITEM[i].ACTION[no], tmp);
                // 移进-规约冲突的处理！
                if (ALL_LR_ITEM_SET[i]->can_reduce){
                    for (int y = 0; y < num_rs; ++ y){
                        // if (is_in_follow_set(, V->vt[no]))
                    }
                    int t = get_production_left(lines[which]);
                    char tmp_vn[5] = {0};
                    for (int p = 0; p <= t; ++ p) tmp_vn[p] = lines[which][p];
                    tmp_vn[t + 1] = '\0';
                    // 这个项目可以移进，但同时又是一个规约项目，shift-reduce conflict!
                    if (is_in_follow_set(tmp_vn, V->vt[no]) != -1){
                        printf("vt in follow error!\n");
                        exit(-1);
                    } else {
                        
                        
                    }
                }
                // strcpy(TABLE_ITEM[i].ACTION[no], ALL_LR_ITEM_SET[i]->next[j].edge);    
            } else if ((no = get_vn_no(ALL_LR_ITEM_SET[i]->next[j].edge)) != -1){
                // 非终结符，添加到GOTO
                itoa(ALL_LR_ITEM_SET[i]->next[j].status, tmp, 10);  // int to str
                strcpy(TABLE_ITEM[i].GOTO[no], tmp);
            } else {
                printf("error error error\n");
                exit(-1);
            }
        }
    }
}
char *get_input(char buf[]){
    // 面临的输入一定是终结符！！！
    int loc = 0;
    for (; buf[loc] && buf[loc] != ','; ++ loc){};      // 找到,
    for (loc ++; buf[loc] && buf[loc] == ' '; ++ loc);  // 跳过空格
    if (!buf[loc]){
        printf("fatal error!\n");
        exit(-1);
    }
    char *s = is_prefix(buf + loc);
    if (s != NULL) return s;
    printf("error in get_input!\n");
    exit(-1);
    return NULL;
}
char *get_next_status(char *ct, char *input){
    // 输入串里面只可能有终结符！
    int in_no = get_vt_no(input);
    int ct_no = atoi(ct + 1);
    return TABLE_ITEM[ct_no].ACTION[in_no];
}
int count_production_right_num(int line){
    int res = 0;
    int loc = 0;
    for (; lines[line][loc] && lines[line][loc] != '>'; ++ loc){};
    ++ loc;
    for (; lines[line][loc] && lines[line][loc] == ' '; ++ loc){};
    if (lines[line][loc] == '\0'){
        printf("error in count_production_right_num!\n");
        exit(-1);
    }
    
    while (lines[line][loc]){   // 双指针
        if (is_vn(lines[line][loc])) {
            loc += 1;
        } else {
            char *s = is_prefix(lines[line] + loc);
            if (s == NULL){
                printf("error in count_production_right_num!\n");
                exit(-1);
            } else {
                loc += strlen(s);
            }
        }
        res ++;
        for (; lines[line][loc] && lines[line][loc] == ' '; ++ loc){};
    }
    if (res <= 0){
        printf("unexpected error in count_production_right_num!\n");
        exit(-1);
    }
    return res;
}
void grammar_analyse(){
    _STEP = 0;   // STEP初值为1 ！！！！
    // struct analysis_item *analyse = (struct analysis_item *)malloc(sizeof(struct analysis_item));
    // memset(analyse, 0, sizeof(struct analysis_item));
    // 读入词法分析的结果并进行语法分析
    FILE* lex_reader = fopen("lex_res.txt", "r");
    if (lex_reader == NULL) {
        printf("read %s error!\n", "les_res.txt");
        exit(-1);
    }
    char buf[LINE_MAX];
    while (fgets(buf, LINE_MAX, lex_reader)){
		int line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r', 空格' '
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1] || ' ' == buf[line_len - 1])
			buf[line_len - 1] = '\0', line_len--;
        if (0 == line_len) continue; //空行
        char *input = get_input(buf);// 当前面临的输入
        if (_STEP == 0){     // 第一次，进行初始化，状态初始为0，符号栈初始为 # 
            analyses[_STEP].step = _STEP;   
            strcpy(analyses[_STEP].stat_stk.stack[analyses[_STEP].stat_stk.idx], "S0\0");
            strcpy(analyses[_STEP].char_stk.stack[analyses[_STEP].char_stk.idx ++], "#\0");
            // printf("buf:%s, input:%s\n", buf, input);
            strcpy(analyses[_STEP].str_now, input);
            char *next_st = get_next_status(analyses[_STEP].stat_stk.stack[analyses[_STEP].stat_stk.idx], input);
            strcpy(analyses[_STEP].Action, next_st);
            analyses[_STEP].stat_stk.idx ++;
            int stk_idx = analyses[_STEP].stat_stk.idx;
            strcpy(analyses[_STEP].stat_stk.stack[stk_idx], next_st);
            analyses[_STEP].stat_stk.idx ++;
            _STEP ++;    // 遇到非终结符，直接_STEP + 1 
        } else {
            // 取状态栈栈顶元素
            int stk_idx = analyses[_STEP].stat_stk.idx - 1;
            char *top = analyses[_STEP].stat_stk.stack[stk_idx];
            char *next_st = get_next_status(top, input);
            if (next_st[0] == 'S') {
                // 执行ACTION动作
                analyses[_STEP].step = _STEP;
                strcpy(analyses[_STEP].stat_stk.stack[analyses[_STEP].stat_stk.idx ++], next_st);
                strcpy(analyses[_STEP].char_stk.stack[analyses[_STEP].char_stk.idx ++], input);
                strcpy(analyses[_STEP].str_now, input);
                strcpy(analyses[_STEP].Action, next_st);
                _STEP ++;
            } else if (next_st[0] == 'r') {
                int num = atoi(next_st + 1);
                // 出栈！
            }
        }
	}
	if (0 == feof){
		printf("fgets error\n"); // 未读到文件末尾
		return;
	}
    fclose(lex_reader);
}
int main(int argc, char *argv[]){
    FILE* fp = fopen("item_set.txt", "w");
    if (NULL == fp){
        printf("open %s failed.\n", "item_set.txt\0");
        return -1;
	}
    read_lines(argv[1]);
    for (int i = 0; i < line_num; ++ i)
        printf("%s\n", lines[i]);
    // 获得所有的终结符和非终结符
    V = (struct CHARS *)malloc(sizeof(struct CHARS));
	V->len_vn = V->len_vt = 0;
    get_vs(argv[1]);
    // 这里的终结符要加一个 # 
    strcpy(V->vt[V->len_vt ++], "#\0");
    // 初始化求所有的项目集
    struct lr_item_set* S;
    init(&S);
    // ALL_LR_ITEM_SET = (struct lr_item_set*)malloc(COUNT * sizeof(struct lr_item_set));
    // printf("len:%d\n", S->cnt);

    shift(S);
    read_fisrt_follow_sets();

    // printf("UID:%d\n", UID);
    printf("next num:%d\n", S->cnt_next_status);
    for (int i = 0; i < S->cnt_next_status; ++ i){
        printf("%d, %s\n", S->next[i].status, S->next[i].edge);
    }
    
    printf("====================\n");
    for (int i = 0; i < UID; ++ i){
        printf("UID:%d  ", i);
        fprintf(fp, "%d\n", i);
        if (ALL_LR_ITEM_SET[i]->can_reduce) printf("yes");
        else printf("no");
        printf("\n");
        for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt_next_status; ++ j){
            printf("(%s, %d) ", ALL_LR_ITEM_SET[i]->next[j].edge, ALL_LR_ITEM_SET[i]->next[j].status);
            fprintf(fp, "(%s, %d) ", ALL_LR_ITEM_SET[i]->next[j].edge, ALL_LR_ITEM_SET[i]->next[j].status);
        }
        fprintf(fp, "\n");
        printf("\ncore:\n");
        for (int j = 0; j < ALL_LR_ITEM_SET[i]->core; ++ j){
            int loc = ALL_LR_ITEM_SET[i]->item_set[j].loc;
            printf("%d, %d, %c, %s\n", ALL_LR_ITEM_SET[i]->core, loc, ALL_LR_ITEM_SET[i]->item_set[j].item[loc], ALL_LR_ITEM_SET[i]->item_set[j].item);
        }
        printf("\n");
        for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt; ++ j){
            int loc = ALL_LR_ITEM_SET[i]->item_set[j].loc;
            printf("%d, %d, %c, %s\n", ALL_LR_ITEM_SET[i]->core, loc, ALL_LR_ITEM_SET[i]->item_set[j].item[loc], ALL_LR_ITEM_SET[i]->item_set[j].item);
            fprintf(fp, "%d~%s\n", ALL_LR_ITEM_SET[i]->item_set[j].loc, ALL_LR_ITEM_SET[i]->item_set[j].item);
        }
        printf("------------------------------\n");
    }
    
    printf("first&follow set\n");
    {
        for (int i = 0; i < V->len_vn; ++ i){
            printf("%s: ", V->vn[i]);
            for (int j = 0; j < FIRST_[i].cnt; ++ j)
                printf("%s, ", FIRST_[i].set[j]);
            printf("\n");
        }
        printf("\n-------------------\n");
        for (int i = 0; i < V->len_vn; ++ i){
            printf("%s: ", V->vn[i]);
            for (int j = 0; j < FOLLOW_[i].cnt; ++ j)
                printf("%s, ", FOLLOW_[i].set[j]);
            printf("\n");
        }
    }

    lr_table_generator();
    // 输出slr1分析表
    {    // printf("`````````````````````````````````````\n");
        printf("+----------------------------------------------------------------------------------------------------------------------+\n");
        printf("|    |                                  ACTION                                         |              GOTO             |\n");
        printf("|----+---------------------------------------------------------------------------------+-------------------------------|\n");
        printf("|%4s|", "stat");
        for (int i = 0; i < V->len_vt; ++ i)
            printf("%3s|", V->vt[i]);
        // printf("%3s|", "#");
        for (int i = 0; i < V->len_vn; ++ i)
            printf(" %-2s|", V->vn[i]);
        printf("\n");
        for (int i = 0; i < UID; ++ i){
            printf("| %-2d |", TABLE_ITEM[i].status);
            int sep = 0;
            int back = 0;
            for (int j = 0; j < V->len_vt; ++ j){
                if (TABLE_ITEM[i].ACTION[j][1] == '0')
                    strcpy(TABLE_ITEM[i].ACTION[j], "acc\0");

                int len1 = strlen(V->vt[j]);
                int len2 = strlen(TABLE_ITEM[i].ACTION[j]);
                if (len1 > len2 && len2 > 0){
                    sep = (len1 - len2) / 2;
                    for (int i = 0; i < sep; ++ i) printf(" ");
                    back = (len1 - len2) - sep;
                }
                else if (len2 <= 3 && len2 > 0){
                    sep = (3 - len2) / 2;
                    for (int i = 0; i < sep; ++ i) printf(" ");
                    back = (3 - len2) - sep;
                } else if (len2 == 0){
                    int x = len1 > 3 ? len1 : 3;
                    sep = x / 2;
                    for (int j = 0; j < sep; ++ j) printf(" ");
                    back = x - sep;
                }
                printf("%s", TABLE_ITEM[i].ACTION[j]);
                for (int x = 0; x < back; ++ x) printf(" ");
                printf("|");
            }
            // if (TABLE_ITEM[i].status == 1){ // 状态1是接受
            //     printf("acc|");
            // } else
            //     printf("   |");
            for (int j = 0; j < V->len_vn; ++ j){
                int len1 = strlen(V->vn[j]);
                int len2 = strlen(TABLE_ITEM[i].GOTO[j]);
                if (len1 > len2 && len2 > 0){
                    sep = (len1 - len2) / 2;
                    for (int i = 0; i < sep; ++ i) printf(" ");
                    back = (len1 - len2) - sep;
                }
                else if (len2 <= 3 && len2 > 0){
                    sep = (3 - len2) / 2;
                    for (int i = 0; i < sep; ++ i) printf(" ");
                    back = (3 - len2) - sep;
                } else if (len2 == 0){
                    int x = len1 > 3 ? len1 : 3;
                    sep = x / 2;
                    for (int j = 0; j < sep; ++ j) printf(" ");
                    back = x - sep;
                }
                printf("%s", TABLE_ITEM[i].GOTO[j]);
                for (int x = 0; x < back; ++ x) printf(" ");
                printf("|");
            }
            printf("\n");
        }
        printf("+----------------------------------------------------------------------------------------------------------------------+\n");
    }
    
    // int **gotos = r_goto();
    // int **actions = r_action();

    // // 输出所有的终结符和非终结符
    // {
    //     printf("%d Vns: \n", V->len_vn);
    //     for (int i = 0; i < V->len_vn; ++i){
    //         printf("%s", V->vn[i]);
    //         if (i < V->len_vn - 1) printf(", ");
    //     }
    //     printf("\n");
    //     printf("%d Vts: \n", V->len_vt);
    //     for (int i = 0; i < V->len_vt; ++i){
    //         printf("(%d, %s), ", strlen(V->vt[i]), V->vt[i]);
    //         if (i < V->len_vn - 1) printf(", ");
    //     }
    //     printf("\n");
    // }
    grammar_analyse();
    printf("%d, %s, %s, %s, %s\n", analyses[0].step, analyses[0].str_now, analyses[0].stat_stk.stack[1], analyses[0].char_stk.stack[0], analyses[0].Action);
    
    fclose(fp);
    
    return 0;
}
