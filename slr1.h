#include "first_follow.h"
#include <stdlib.h>

#define COUNT 96            // 最多有多少个项目集
#define MAX_LEN_PRODUCTION 20

#define MAX_STATUS_NEXT 20  // 每一个项目集通过移进而到达的新的项目集的最大个数
#define NUM_PER_SET 20      // 每一个项目集中最多的项目数
#define MAX_STACK_SIZE 128
#define MAX_STEP 1024       // 分析过程的最大步骤数
struct next_status{
    int status;                     // 指向的下一个项目集的UID
    char edge[MAX_LEN_VT];          // 通过那条边指向下一个项目集，即通过什么字符到达的
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
    int status;                         // 每一行的编号，也即项目集编号
    // int action_idx;
    cpp_string ACTION[MAX_NUM_VT];      // 假设终结符最多40个，lazy
    // int goto_idx;
    cpp_string GOTO[MAX_NUM_VN];        // 假设非终结符最多20个，lazy 2
} TABLE_ITEM[COUNT];                    // 分析表有多少行(项目集有多少个), COUNT就取多少，可以malloc???


struct status_stack{                    // 状态栈
    int idx;
    int stack[MAX_STACK_SIZE];
} stat_stk;
struct char_stack{                      // 符号栈
    int idx;
    char stack[MAX_STACK_SIZE][MAX_LEN_VT];
} char_stk;

int _STEP;                              // 分析过程中的每一行的编号（步骤
int current_line;                       // 语法分析正在进行分析的行号, 报错定位行号
FILE* analyse_res;                      // 语法分析的步骤写入到文件
struct analysis_item{
    int step;                           // 步骤
    // struct status_stack stat_stk;    // 状态栈
    // struct char_stack char_stk;      // 符号栈
    char str_now[MAX_LEN_VT];           // 输入串 -> 当前遇到的字符
    char Action[ITEM_LEN];              // ACTION
    char Goto[ITEM_LEN];                // GOTO
} analyses[MAX_STEP];

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
bool is_front_repeated(struct lr_item_set* S, char str[]){ // 在移进的时候是否遇到重复的字符（串
    for (int i = 0; i < S->cnt_next_status; ++ i)
        if (strcmp(str, S->next[i].edge) == 0)  
            return true;
    return false;
}
bool equal_prefix(char *s, char *t){ // 判断*t开始的字符串前strlen(s)个字符是不是和s相等，其中s长度已知
    // s是直到他的长度 的，所以s要放在前面
    // printf("---compares:\n");
    for (int i = 0; s[i]; ++ i)
        if (s[i] != t[i]) 
            return false;
    return true;
}

struct lr_item_set* init_lr_item_set(){ // 初始化一个lr_item_set并返回
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
void del_lr_item_set(struct lr_item_set **S){ // 删除一个项目集（有重复）
    UID --;     // UID为全局变量！
    // printf("%s was freed!!\n", (*S)->item_set[0].item);
    free(*S);
}
int is_itemset_repeated(struct lr_item_set* S){ // 判断是否有一样的项目集
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
// bool is_item_left(struct lr_item_set* S){ // 判断是否已经操作过这个项目集
//     for (int i = 0; i < S->cnt; ++ i)
//         if (!S->item_set[i].operated)
//             return true;
//     return false;
// }
void shift(struct lr_item_set* S);  // 函数声明，方便在expand()中调用
void expand(struct lr_item_set* S){ // 项目集的 核 开始扩张
    // printf("UID:%d, cnt:%d\n", S->status, S->cnt);
    int scnt = S->cnt;  // 循环过程中，S->cnt会发生改变！！！！
    // for(int i = 0; i < S->cnt; ++ i){
    //     printf("expand() %d, %1c, %s\n", S->item_set[i].loc, S->item_set[i].item[S->item_set[i].loc], S->item_set[i].item);
    // }
    // printf("???????????????????\n");
    for (int i = 0; i < S->cnt; ++ i){
        int loc = S->item_set[i].loc;
        // 达到了末尾，即是一个规约状态（ LR(0), SLR(1)待定
        // to be optomized, to do
        char ch = S->item_set[i].item[loc]; 
        // printf("ct_loc:%d, ch:%c, ct_produce:%s\n", loc, ch, S->item_set[i].item);

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
void shift(struct lr_item_set* S){ // 移进
    // printf("shift() cnt:%d\n", S->cnt);
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
                    // printf("%d, %d, %d == ", k, a_loc, strlen(tmp));
                    // printf("!!!!! UID is:%d, a_loc:%d, %s\n", UID - 1, k, new->item_set[new->cnt].item);
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
            // printf("%d new core is:\n", new->core);
            for (int x = 0; x < new->core; ++ x){
                int y = new->item_set[x].loc;
            }
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
        
    }
}

void init(struct lr_item_set** S){ // 求初始的第一个 项目集。
    UID = 0;
    CONTINUE_ = -1;
    // *S = (struct lr_item_set*)malloc(sizeof(struct lr_item_set));
    // (*S)->status = UID ++, (*S)->cnt = 0, (*S)->cnt_next_status = 0;        // 记得初始化值！
    (*S) = init_lr_item_set();
    add_item_to_set(*S, 0);
    (*S)->core = 1;
    expand(*S);
    // printf("%d, %d, %s\n", (*S)->cnt, (*S)->item_set[0].loc, (*S)->item_set[0].item);
    
    // char ct_vn = (*S)->item_set[0].item[(*S)->item_set[0].loc];   
    // // 当前的第一个符号，如果是一个非终结符，则要在I0中添加项目，如果不是直接忽略
    // if (is_vn(ct_vn)) 
    //     for (int i = 0; i < line_num; ++ i){
    //         int left = get_production_left(lines[i]);
    //         // 大于1就出错了，因为最长的S'的left才是1
    //         if (left > 1) {printf("error left!\n"); break;}    
    //         if (ct_vn == lines[i][left]) add_item_to_set(*S, i);
    //     }
    
    // for (int i = 0; i < (*S)->cnt; ++ i)
    //     printf("%d, %s\n", (*S)->item_set[i].loc, (*S)->item_set[i].item);
}
int get_production_no(char *prod){ // 获取产生式的编号
    for (int i = 0; i < line_num; ++ i)
        if (strcmp(lines[i], prod) == 0)
            return i;
    return -1;
}

void read_fisrt_follow_sets(){ // 读取文件中的first集和follow集
    for (int i = 0; i < V->len_vn; ++i) strcpy(FIRST_[i].vn, V->vn[i]);
    for (int i = 0; i < V->len_vn; ++i) strcpy(FOLLOW_[i].vn, V->vn[i]);
    FILE* fp = fopen("files/first-follow-set.txt", "r");
    if (fp == NULL){
        printf("read %s failed.", "files/first-follow-set.txt");
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
        // printf("%s\n", buf + loc);
        // 依次读取first集和follow集
        int no = 0;
        // printf("cnt:%d\n", cnt);
        while (buf[loc]){ // 双指针。。。
            int j = loc;
            for (; buf[j] && buf[j] != ' '; ++ j){};
            // printf("%s, ", buf + j);
            if (mode == 0){
                FIRST_[cnt].cnt ++;
                FIRST_[cnt].set[no][j - loc] = '\0';
                // printf("FIRST_ appended %s, ", buf + loc);
                strncpy(FIRST_[cnt].set[no], buf + loc, j - loc);
                // printf("%s\n", FIRST_[cnt].set[no]);
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

int is_in_follow_set(char *vn, char *s){ // 判断vn的follow集包不包含s
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
bool is_null_unite_sets(char *v1, char *v2){ // 判断两个vn的follow集是不是有交集
    int n1 = get_vn_no(v1), n2 = get_vn_no(v2);
    for (int i = 0; i < FOLLOW_[n1].cnt; ++ i)
        for (int j = 0; j < FOLLOW_[n2].cnt; ++ j)
            if (strcmp(FOLLOW_[n1].set[i], FOLLOW_[n2].set[j]) == 0)
                return false;
    return true;
}

void lr_table_generator(){ // 生成SLR1分析表
    // TABLE_ITEM是全局变量，默认初始化为0了
    memset(TABLE_ITEM, 0, COUNT * sizeof(struct table_item));
    // for (int i = 0; i < V->len_vt; ++ i)
    //     printf("%s, ", V->vt[i]);
    // printf("\n");
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

            // 判断是否有包含关系 e.g. Follow(A) 包含 非终结符
            
            for (int h = 0; h < num_ts; ++ h){
                int is_error = 0;   // 不符合P139的条件，有两个及以上的可规约项目的follow集中包含同一个非终结符
                for (int k = 0; k < num_rs; ++ k){
                    // 拿到产生式左边的终结符
                    int t = get_production_left(ALL_LR_ITEM_SET[i]->item_set[rs[k]].item);
                    char tmp_vn[5] = {0};
                    for (int p = 0; p <= t; ++ p) tmp_vn[p] = ALL_LR_ITEM_SET[i]->item_set[rs[k]].item[p];
                    tmp_vn[t + 1] = '\0';
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
            if (num_rs == 1) { // 只有一条·到达末尾的，在其FOLLOW集里面就能规约
                // 拿到产生式左边的终结符
                int t = get_production_left(ALL_LR_ITEM_SET[i]->item_set[rs[0]].item);
                char tmp_vn[5] = {0};
                for (int p = 0; p <= t; ++ p) tmp_vn[p] = ALL_LR_ITEM_SET[i]->item_set[rs[0]].item[p];
                tmp_vn[t + 1] = '\0';
                // int no = get_vn_no(tmp_vn);
                char tmp[5] = {0};
                int no = get_production_no(ALL_LR_ITEM_SET[i]->item_set[rs[0]].item);
                tmp[0] = 'r';
                itoa(no, tmp + 1, 10);
                
                for (int j = 0; j < V->len_vt; ++ j)
                    if (is_in_follow_set(tmp_vn, V->vt[j]) != -1){
                        // printf("%s -> %s, ", tmp_vn, V->vt[j]);
                        strcpy(TABLE_ITEM[i].ACTION[j], tmp);
                    }
                // printf("\n");
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
int get_current_line(char buf[]){ // 获取正在进行分析的行号。
    int loc = 0, res = 0;
    for (; buf[loc] && buf[loc] != ','; ++ loc);
    for (++ loc; buf[loc] && buf[loc] != ','; ++ loc);  // 找到第二个逗号的位置
    for (loc ++; buf[loc] && buf[loc] == ' '; ++ loc);  // 跳过空格
    if (buf[loc] < '0' || buf[loc] > '9') {
        printf("error in get_current_line()!\n");
        exit(-1);
    }
    for (; buf[loc] && (buf[loc] >= '0' && buf[loc] <= '9'); ++ loc)
        res = 10 * res + (buf[loc] - '0');
    return res;
}
void out_stk(int mode, FILE *fp){ // 打印栈内的数据 mode = 1 -> 状态栈，mode = 0 -> 符号栈
    if (mode == 1) {
        for (int i = 0; i  < stat_stk.idx; ++ i){
            printf("%d", stat_stk.stack[i]);
            if (fp != NULL) fprintf(fp, "%d", stat_stk.stack[i]);
            if (i < stat_stk.idx - 1) {
                printf(".");
                if (fp != NULL) fprintf(fp, ".");
            }
        }
    } else if (mode == 0) {
        for (int i = 0; i < char_stk.idx; ++ i){
            printf("%s", char_stk.stack[i]);
            if (fp != NULL) fprintf(fp, "%s", char_stk.stack[i]);
            if (i < char_stk.idx - 1) {
                printf(" ");
                if (fp != NULL) fprintf(fp, " ");
            }
        }
    } else {
        printf("bad argument 'mode' in out_stk()\n");
        exit(-1);
    }
}
void out_slr1_table_item(){ // 输入分析过程中的每一步（每一行
    printf("|(%2d)| ", _STEP + 1);
    fprintf(analyse_res, "|(%2d)| ", _STEP + 1);
    out_stk(1, analyse_res); 
    printf(" | "); 
    fprintf(analyse_res, " | ");
    
    out_stk(0, analyse_res);
    printf(" | %s | ", analyses[_STEP].str_now);
    fprintf(analyse_res, " | %s | ", analyses[_STEP].str_now);
    printf("%s | %3s |\n", analyses[_STEP].Action, analyses[_STEP].Goto);
    fprintf(analyse_res, "%s | %3s |\n", analyses[_STEP].Action, analyses[_STEP].Goto);
}

char *get_input(char buf[]){ // 获取分析过程中面临的输入
    // 获取面临的输入一定是终结符！！！
    int loc = 0;
    for (; buf[loc] && buf[loc] != ','; ++ loc){};      // 找到,即（**, **）中的第二项
    for (loc ++; buf[loc] && buf[loc] == ' '; ++ loc);  // 跳过空格
    if (!buf[loc]){
        printf("fatal error!\n");
        exit(-1);
    }
    char *s = is_prefix(buf + loc);
    return s;
}
char *get_next_status(int ct, char *input, int mode){ // 获取分析过程中的下一个动作，S* or r**
    // 输入串里面只可能有终结符！
    // mode == 1 表示是在S动作之后寻找下一个状态
    // mode == 0 表示是在r动作之后寻找下一个状态
    if (mode == 1){         // S
        int in_no = get_vt_no(input);
        if (strlen(TABLE_ITEM[ct].ACTION[in_no]) == 0) {
            strcpy(analyses[_STEP].str_now, input);
            printf("error step:\n");
            fprintf(analyse_res, "error step:\n");
            out_slr1_table_item();
            printf("line %d:an error occured when finding S%d, to:%s\n", current_line, ct, input);
            printf("syntax error!%s", input);
            exit(-1);
        }
        return TABLE_ITEM[ct].ACTION[in_no];
    } else if (mode == 0) { // r
        int in_no = get_vn_no(input);
                if (strlen(TABLE_ITEM[ct].GOTO[in_no]) == 0) {
            printf("error step:\n");
            strcpy(analyses[_STEP].str_now, input);
            out_slr1_table_item();
            printf("line %d:an error occured when finding GOTO %d, to:%s\n", current_line, ct, input);
            printf("syntax error!%s", input);
            exit(-1);
        }
        return TABLE_ITEM[ct].GOTO[in_no];
    } else {                // error
        printf("argument mode error!\n");
        exit(-1);
    }
}
int count_production_right_num(int line){ // 获取产生式右边的元素（Vn|Vt）的个数】
    // line表示第几个产生式，即lines中的编号
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
void syntax_analyse(){ // 根据 SLR1分析表 进行语法分析
    _STEP = 0;
    // struct analysis_item *analyse = (struct analysis_item *)malloc(sizeof(struct analysis_item));
    // memset(analyse, 0, sizeof(struct analysis_item));
    // 读入词法分析的结果并进行语法分析
    FILE* lex_reader = fopen("files/lex_res.txt", "r");
    analyse_res = fopen("files/slr1_process.txt", "w");
    if (lex_reader == NULL) {
        printf("read %s error!\n", "files/les_res.txt");
        exit(-1);
    }
    if (analyse_res == NULL) {
        printf("write %s error!\n", "files/slr1_process.txt");
        exit(-1);
    }
    char buf[LINE_MAX];
    // int debug = 0;
    char *input;
    char *next_st;                  // 下一个状态
    while (fgets(buf, LINE_MAX, lex_reader)){
        // if (debug ++ == 19) return;
		int line_len = strlen(buf);
		// 排除换行符‘\n’ windos文本排除回车符'\r', 空格' '
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1] || ' ' == buf[line_len - 1])
			buf[line_len - 1] = '\0', line_len--;
        if (0 == line_len) continue;    // 空行
        input = get_input(buf);   // 当前面临的输入
        current_line = get_current_line(buf);
        // printf("get_input res:%s\n", input);
        if (input == NULL) {
            printf("error in get_input()!\n");
            exit(-1);
        }

        if (_STEP == 0){     // 第一次，进行初始化，状态初始为0，符号栈初始为 # 
            analyses[_STEP].step = _STEP;   
            stat_stk.stack[stat_stk.idx] = 0;
            strcpy(char_stk.stack[char_stk.idx ++], "#\0");
            // printf("buf:%s, input:%s\n", buf, input);
            strcpy(analyses[_STEP].str_now, input);
            char *next_st = get_next_status(stat_stk.stack[stat_stk.idx], input, 1);
            // printf("%s\n", next_st);
            strcpy(analyses[_STEP].Action, next_st);
            stat_stk.idx ++;
            out_slr1_table_item();
            stat_stk.stack[stat_stk.idx ++] = atoi(next_st + 1);
            strcpy(char_stk.stack[char_stk.idx ++], input);
            _STEP ++;    // 遇到非终结符，直接_STEP + 1 
        } else {
            // 取  状态栈  栈顶元素
            // printf("---current input:%s\n", input);
                //             out_slr1_table_item();
            int stk_idx = stat_stk.idx - 1;
            int top = stat_stk.stack[stk_idx];
            // printf("top:%d\n", top);
            next_st = get_next_status(top, input, 1);
            // printf("next_st:%s\n", next_st);
ACTION_S:
            if (next_st[0] == 'S') {
                // 执行ACTION动作
                analyses[_STEP].step = _STEP;
                strcpy(analyses[_STEP].str_now, input);
                strcpy(analyses[_STEP].Action, next_st);
                out_slr1_table_item();
                stat_stk.stack[stat_stk.idx ++] = atoi(next_st + 1);
                strcpy(char_stk.stack[char_stk.idx ++], input);
                _STEP ++;
            } else if (next_st[0] == 'r') {
                while (next_st[0] == 'r'){
                    // printf("else next_st:%s\n", next_st);
                    strcpy(analyses[_STEP].Action, next_st);    // record
                    strcpy(analyses[_STEP].str_now, input); 
                    int line = atoi(next_st + 1);               // 第几条产生式
                    int num = count_production_right_num(line); // 产生式右边元素的数目
                    int left = get_production_left(lines[line]);
                    char tmp[2] = {0};  // 规约得到的非终结符
                    tmp[0] = lines[line][left], tmp[1] = '\0';
                    int top = stat_stk.stack[stat_stk.idx - num - 1];
                    next_st = get_next_status(top, tmp, 0);   // 查GOTO表
                    // printf("next_St:%s\n", next_st);
                    strcpy(analyses[_STEP].Goto, next_st);

                    out_slr1_table_item();
                  
                    stat_stk.idx -= num;// 出栈！
                    char_stk.idx -= num;// 出栈！

                    strcpy(char_stk.stack[char_stk.idx ++], tmp);
                    strcpy(analyses[_STEP].str_now, input);  
                    top = stat_stk.stack[stat_stk.idx - 1];
                    // printf("top:%d, tmp:%s, ", top, tmp);
                    next_st = get_next_status(top, tmp, 0);   // 查GOTO表
                    // printf("next_St:%s\n", next_st);
                    strcpy(analyses[_STEP].Goto, next_st);
                    stat_stk.stack[stat_stk.idx ++] = atoi(next_st);
                    
                    // printf("stk top:%d, input:%s\n", stat_stk.stack[stat_stk.idx - 1], input);
                    next_st = get_next_status(stat_stk.stack[stat_stk.idx - 1], input, 1);
                    // printf("sss:next_St:%s\n", next_st);

                    _STEP ++;
                }
                if (next_st[0] == 'S') {
                    goto ACTION_S;
                } else if (next_st[0] == 'a') { // 规约之后可以接收了！
                    // 接受！
                    strcpy(analyses[_STEP].Action, "acc\0");
                    out_slr1_table_item();
                    printf("accepted!\n");
                    
                }
            } else if (next_st[0] == 'a') {     // 判断是不是acc, 也许没用，因为接受都是在规约之后
            //     // 接受！
                printf("accepted!%s\n", analyses[_STEP].Action);
                strcpy(analyses[_STEP].Action, "acc\0");
                out_slr1_table_item();
            } else {
                printf("unexpected status!\n");
                exit(-1);
            }
        }
	}

	if (0 == feof){
		printf("fgets error\n"); // 未读到文件末尾
		return;
	}
    fclose(lex_reader);
}

void slr1_runner(){
    FILE* fp = fopen("files/item_set.txt", "w");
    if (NULL == fp){
        printf("open %s failed.\n", "files/item_set.txt\0");
        exit(-1);
        return;
	}

    // 这里的终结符要加一个 # 
    strcpy(V->vt[V->len_vt ++], "#\0");
    // 初始化求所有的项目集
    struct lr_item_set* S;
    init(&S);
    shift(S);

    fclose(fp);
    lr_table_generator();
    // 输出slr1分析表
    printf("slr1 table:\n");
    {    
        printf("+--------------------------------------------------------------------------------------------------------------------------+\n");
        printf("|    |                                     ACTION                                          |              GOTO             |\n");
        printf("|----+-------------------------------------------------------------------------------------+-------------------------------|\n");
        printf("|%4s|", "stat");
        for (int i = 0; i < V->len_vt; ++ i)
            printf("%3s|", V->vt[i]);
        // printf("%3s|", "#");
        for (int i = 0; i < V->len_vn; ++ i)
            printf(" %-2s|", V->vn[i]);
        printf("\n");
        printf("|----+-------------------------------------------------------------------------------------+-------------------------------|\n");

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
        printf("+--------------------------------------------------------------------------------------------------------------------------+\n");
    }

    printf("analyse process:\n");
    syntax_analyse();
    // for (int i = 0; i < stat_stk.idx; ++ i)
    //     printf("%d ", stat_stk.stack[i]);
    // printf("\n");
    // for (int i = 0; i < char_stk.idx; ++ i)
    //     printf("%s ", char_stk.stack[i]);
    // printf("\n");
}
