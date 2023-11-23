#include "first_follow.h"
#include "lexical.h"
#include <stdlib.h>
#include <iostream>
#include <stack>
#include <string>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <iomanip>
using namespace std;

#define COUNT 128            // 最多有多少个项目集
#define MAX_LEN_PRODUCTION 20

#define MAX_STATUS_NEXT 20  // 每一个项目集通过移进而到达的新的项目集的最大个数
#define NUM_PER_SET 20      // 每一个项目集中最多的项目数
#define MAX_STACK_SIZE 128
#define MAX_STEP 512       // 分析过程的最大步骤数
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
    cpp_string ACTION[MAX_NUM_VT];      // 假设终结符最多40个，lazy
    cpp_string GOTO[MAX_NUM_VN];        // 假设非终结符最多20个，lazy 2
} TABLE_ITEM[COUNT];                    // 分析表有多少行(项目集有多少个), COUNT就取多少，可以malloc???

int _STEP;                      // 分析过程中的每一行的编号（步骤
int current_line;               // 语法分析正在进行分析的行号, 报错定位行号
FILE* analyse_res;              // 语法分析的步骤写入到文件
struct analysis_item{
    int step;                   // 步骤
    char str_now[MAX_LEN_VT];   // 输入串 -> 当前遇到的字符
    char Action[ITEM_LEN];      // ACTION
    char Goto[ITEM_LEN];        // GOTO
} analyses[MAX_STEP];
bool syntax_success = false;

class symbol {
    public:
        string varName{};        // 变量名
        string valueStr{"0"};    // 变量的值，字符串形式,初始化为0
        int PLACE{-1};           // 该变量在符号表中的位置,初始化为-1
        vector<int> truelist;    // 值为真所指向的四元式的标号
        vector<int> falselist;   // 值为假所指向的四元式的标号
        string rawName{""};      // 规约之前的名字，原始名字，123, <, >...
};
struct quad {           // 四元式
    string op;          // 操作符
    int arg1Index;      // 源操作数1的符号表地址
    int arg2Index;      // 源操作数2的符号表地址
    symbol result;      // 目的操作数
};

struct status_stack{    // 状态栈
    int idx;
    int stack[MAX_STACK_SIZE];
} stat_stk;
struct char_stack{      // 符号栈
    int idx;
    symbol stack[MAX_STACK_SIZE];
} char_stk;

vector<quad> quads;         // 四元式序列
vector<symbol> symbolTable; // 符号表
map<string, int> ENTRY;     // 用于查变量的符号表入口地址
int tempVarNum = 0;         // 临时变量个数
symbol newtemp(){           // 生成新的临时变量
    tempVarNum ++;
    return symbol{string("T" + to_string(tempVarNum))};
}
set<string> oprts = {"<", ">", "<=", ">=", "==", "!="}; // map--rop

void GEN(const string& op, int arg1, int arg2, symbol &result){ // 产生一个四元式
    // 运算符、参数1在符号表的编号、参数2在符号表的编号，结果符号
    // 产生一个四元式，并填入四元式序列表
    quads.push_back(quad{op, arg1, arg2, result}); //插入到四元式序列中

    if (op == "-") {        // 将临时变量result注册进入符号表
        result.PLACE = symbolTable.size();
        result.valueStr = "-" + symbolTable[arg1].valueStr;
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "+"){ // 将临时变量result注册进入符号表
        result.PLACE = symbolTable.size();
        result.valueStr = to_string(atoi(symbolTable[arg1].valueStr.c_str()) + atoi(symbolTable[arg2].valueStr.c_str()));
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "$"){ // 将临时变量result注册进入符号表
        result.PLACE = symbolTable.size();
        result.valueStr = to_string(atoi(symbolTable[arg1].valueStr.c_str()) - atoi(symbolTable[arg2].valueStr.c_str()));
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "*"){ // 将临时变量result注册进入符号表
        result.PLACE = symbolTable.size();
        result.valueStr = to_string(atoi(symbolTable[arg1].valueStr.c_str()) * atoi(symbolTable[arg2].valueStr.c_str()));
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "/") { // 将临时变量result注册进入符号表
        result.PLACE = symbolTable.size();
        int x = stoi(symbolTable[arg1].valueStr);
        int y = stoi(symbolTable[arg2].valueStr);
        if (y == 0) {
            cout << "\nline " << current_line + 1 << ": " << "zero division error!\n";
            exit(-1);
        }
        int z = x / y;
        result.valueStr = to_string(z);
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == ":="){ 
        result.valueStr = symbolTable[arg1].valueStr;
        
        if (symbolTable[arg1].varName == string("true") || symbolTable[arg1].varName == string("false")){
            result.PLACE = symbolTable.size();
            symbolTable.push_back(result);
            ENTRY[result.varName] = result.PLACE;
        }
        // if (result.rawName == "y") cout << "yeyesyes" << symbolTable[arg1].valueStr << endl;
        symbolTable[ENTRY[result.rawName]].valueStr = symbolTable[arg1].valueStr;
        // for (auto &res : symbolTable)   // 回去填补symbolTable中变量的值
        //     if (res.varName == result.varName || res.varName == result.rawName)
        //         res.valueStr = result.valueStr;
    } else if (op == "or"){
        result.PLACE = symbolTable.size();
        int res = -1;
        if (atoi(symbolTable[arg1].valueStr.c_str()) || atoi(symbolTable[arg2].valueStr.c_str()))
            res = 1;
        else res = 0;
        result.valueStr = to_string(res);
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "and"){
        result.PLACE = symbolTable.size();
        int res = -1;
        if (atoi(symbolTable[arg1].valueStr.c_str()) && atoi(symbolTable[arg2].valueStr.c_str()))
            res = 1;
        else res = 0;
        result.valueStr = to_string(res);
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "not"){
        result.PLACE = symbolTable.size();
        if (symbolTable[arg1].valueStr == "1")
            result.valueStr = "0";
        else if (symbolTable[arg1].valueStr == "0")
            result.valueStr = "1";
        else {
            cout << "\nerror in not()\n";
            exit(-1);
        }
        symbolTable.push_back(result);
        ENTRY[result.varName] = result.PLACE;
    } else if (op == "goto"){
        // for no error report
    } else if (oprts.count(op)){
        // for no error report  
    } else{
        cout << "\nunexpected operator in GEN()!\n";
        exit(-1);
    }
}

void out_quad(vector<quad> &v){ // 输出所有的四元式
    FILE* fp = fopen("files/quads.txt", "w");
    if (fp == NULL) {
        printf("\nwrite %s failed.", "files/quads.txt");
        exit(-1);
    }
    int idx = 0;
    for (auto & quad : v){
        if (quad.op == "goto"){
            cout << setw(2) << idx ++ << ". (" << quad.op << ", ";
            quad.arg1Index != -1 ? cout << quad.arg1Index : cout << "_";
            cout << ")" << endl;
            fprintf(fp, "%d.(goto, %d)\n", idx - 1, quad.arg1Index);
            continue;
        } 
        cout << setw(2) << idx ++  << ". (" << quad.op << ", ";
        fprintf(fp, "%d.(%s, ", idx - 1, quad.op.c_str());
        if (quad.arg1Index != -1) {
            cout << symbolTable[quad.arg1Index].varName;
            fprintf(fp, "%s, ", symbolTable[quad.arg1Index].varName.c_str());
        } else {    
            cout << "_";
            fprintf(fp, "_, ");
        }
        cout << ", ";
        if (quad.arg2Index != -1) {
            cout << symbolTable[quad.arg2Index].varName;
            fprintf(fp, "%s, ", symbolTable[quad.arg2Index].varName.c_str());
        } else {    
            cout << "_";
            fprintf(fp, "_, ");
        }
        fprintf(fp, "%s)\n", quad.result.varName.c_str());
        cout << ", " << quad.result.varName << ")" << endl;
    }
    fclose(fp);

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
    // 将对应的next_status中的字符串置0
    memset(S->next, 0, MAX_STATUS_NEXT * (sizeof(struct next_status)));
    return S; 
}
void del_lr_item_set(struct lr_item_set **S){ // 删除一个项目集（有重复）
    UID --;     // UID为全局变量！
    free(*S);
}
int is_itemset_repeated(struct lr_item_set* S){ // 判断是否有一样的项目集
    // to be optimized , to do
    // 判断是否有一样的项目集, 即核一样，在新增加完项目集并且把核添加进去了后判断。
    // 如果有重复的，就把重复的那个的UID返回回去
    // 无重复返回-1
    for (int i = 0; i < UID - 1; ++ i){ // < UID - 1, 因为不包括自己
        // 先找核中项目数一样的
        if (ALL_LR_ITEM_SET[i]->core == S->core){
            bool flag[COUNT] = {false};
            for (int j = 0; j < S->core; ++ j){
                for (int k = 0; k < ALL_LR_ITEM_SET[i]->core; ++ k){
                    if (ALL_LR_ITEM_SET[i]->item_set[k].loc == S->item_set[j].loc)
                        if (strcmp(ALL_LR_ITEM_SET[i]->item_set[k].item, S->item_set[j].item) == 0)
                            flag[j] = true;
                }
            }
            int k;
            for (k = 0; k < S->core; ++ k)
                if (!flag[k]) break;
            if (k >= S->core) return i;
        }
    }
    return -1;
}
void shift(struct lr_item_set* S);  // 函数声明，方便在expand()中调用
void expand(struct lr_item_set* S){ // 项目集的 核 开始扩张
    int scnt = S->cnt;  // 循环过程中，S->cnt会发生改变！！！！
    for (int i = 0; i < S->cnt; ++ i){
        int loc = S->item_set[i].loc;
        // to be optomized, to do
        char ch = S->item_set[i].item[loc]; 
        if (ch == '\0') continue;
        // 当前的第一个符号，如果是一个非终结符，则要在I0中添加项目，如果不是直接忽略
        if (is_vn(ch)) 
            for (int j = 1; j < line_num; ++ j){    // 第0行是S',不考虑它
                int left = get_production_left(lines[j]);
                // 大于1就出错了，因为最长的S'的left才是1
                if (left > 0) {
                    printf("\nleft:%d, in line %d:%s  error left, unrecommended space ' '!\n", left, j, lines[j]); 
                    exit(-1);
                }     
                if (ch == lines[j][left]) add_item_to_set(S, j);
            }  
    }
    if (CONTINUE_ != UID){
        CONTINUE_ = UID;
        shift(S);
    }
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
                printf("\nfatal error!!!!!!\n");
                exit(-1);
            }
            strcpy(tmp, s);
        }
       
        if (!is_front_repeated(S, tmp)){
            // 如果移进那个的字符没有重复，才新建一个项目集（新建项目集的依据！
            struct lr_item_set* new_set = init_lr_item_set();
            // 把信息复制过去
            S->next[S->cnt_next_status].status = new_set->status;   // UID记录
            strcpy(S->next[S->cnt_next_status].edge, tmp);      // 边记录
            S->cnt_next_status ++;
            // 找到了，给这个新项目集添加 核
            int core = 0;
            for (int j = 0; j < S->cnt; ++ j){
                if (S->item_set[j].operated) continue;
                int a_loc = S->item_set[j].loc;
                if (equal_prefix(tmp, S->item_set[j].item + S->item_set[j].loc)){
                    core ++;
                    strcpy(new_set->item_set[new_set->cnt].item, S->item_set[j].item);
                    int k;  // 跳过空格
                    for (k = a_loc + strlen(tmp); new_set->item_set[new_set->cnt].item[k] && new_set->item_set[new_set->cnt].item[k] == ' '; ++ k){};
                    new_set->item_set[new_set->cnt].loc = k;    // 更新loc
                    new_set->cnt ++;
                    S->item_set[j].operated = true; // 标记为已经扫描过
                    int new_loc = new_set->item_set[new_set->cnt].loc;
                    if (S->item_set[i].item[new_loc] == '\0') {
                        // · 到结尾了to do
                    }
                }  
            }
            new_set->core = core;   // 核中项目的个数
            // to do, to be optimized
            int res = is_itemset_repeated(new_set);
            // printf("%d new_set core is:\n", new_set->core);
            for (int x = 0; x < new_set->core; ++ x){
                int y = new_set->item_set[x].loc;
            }
            if (res != -1){
                S->cnt_next_status --;
                S->next[S->cnt_next_status].status = ALL_LR_ITEM_SET[res]->status;   // UID记录
                strcpy(S->next[S->cnt_next_status].edge, tmp);
                S->cnt_next_status ++;
                del_lr_item_set(&new_set);
            }
            else expand(new_set);
        }
    }
}

void init(struct lr_item_set** S){ // 求初始的第一个 项目集。
    UID = 0;
    CONTINUE_ = -1; // 记得初始化值！
    (*S) = init_lr_item_set();
    add_item_to_set(*S, 0);
    (*S)->core = 1;
    expand(*S);
}
int get_production_no(char *prod){ // 获取产生式的编号
    for (int i = 0; i < line_num; ++ i)
        if (strcmp(lines[i], prod) == 0)
            return i;
    return -1;
}
int is_in_follow_set(char *vn, char *s){ // 判断vn的follow集包不包含s
    int no = get_vn_no(vn);
    if (no == -1){
        printf("find vn is:%s\n", vn);
        printf("\nfind error!\n");
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
    for (int i = 0; i < UID; ++ i){
        TABLE_ITEM[i].status = i;
        bool reduce = ALL_LR_ITEM_SET[i]->can_reduce;
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
                            printf("\n%s is in follow set!", V->vt[ts[h]]);
                            exit(-1);
                        }
                    }
                }   
            }
            if (num_rs == 1) { // 只有一条·到达末尾的，在其FOLLOW集里面就规约，但是会产生移进-规约冲突，下面解决
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
            } else if (num_rs > 1){    // 有多个，判断是否有规约-规约冲突
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
                            printf("\n%s %s follow set unite is not null!\n", tmp1, tmp2);
                            exit(-1);
                        }
                    }
                }
            }
        }
        for(int j = 0; j < ALL_LR_ITEM_SET[i]->cnt_next_status; ++ j){
            char tmp[10];
            int no = get_vt_no(ALL_LR_ITEM_SET[i]->next[j].edge);
            if (no != -1){
                // 终结符，添加到ACTION
                tmp[0] = 'S';
                itoa(ALL_LR_ITEM_SET[i]->next[j].status, tmp + 1, 10);  // int to str
                char save_r[MAX_LEN_PRODUCTION] = {0};
                strcpy(save_r, TABLE_ITEM[i].ACTION[no]);
                strcpy(TABLE_ITEM[i].ACTION[no], tmp);
                // 移进-规约冲突的处理！
                if (ALL_LR_ITEM_SET[i]->can_reduce){
                    for (int y = 0; y < num_rs; ++ y){
                        int t = get_production_left(ALL_LR_ITEM_SET[i]->item_set[rs[y]].item);
                        char tmp_vn[5] = {0};
                        for (int p = 0; p <= t; ++ p) tmp_vn[p] = ALL_LR_ITEM_SET[i]->item_set[rs[y]].item[p];
                        tmp_vn[t + 1] = '\0';
                        // 这个项目可以移进，但同时又是一个规约项目，shift-reduce conflict!
                        if (is_in_follow_set(tmp_vn, V->vt[no]) != -1){
                            bool can_solve = false;
                            if (strlen(save_r) != 0){ // 尝试用优先级解决冲突, - * / not的优先级比较高
                                char *iitem = ALL_LR_ITEM_SET[i]->item_set[rs[y]].item;
                                for (int q = get_production_right(iitem); iitem[q]; ++ q){

                                    if (iitem[q] == '-' || iitem[q] == '*' || iitem[q] == '/'){
                                        can_solve = true;
                                        // cout << strlen(iitem) << " " << "item:" << iitem << " " << "iitem[q]:" << iitem[q] << endl;
                                        break;
                                    } else if (iitem[q] == 'n' && iitem[q + 1] && iitem[q + 1] == 'o' && iitem[q + 2] && iitem[q + 2] == 't'){
                                        can_solve = true;
                                        break;
                                    } else if (iitem[q] == 'o' && iitem[q + 1] && iitem[q + 1] == 'r') {
                                        // and 和 or 的优先级 -> 从左到右计算
                                        can_solve = true;
                                        break;
                                    } else if (iitem[q] == 'a' && iitem[q + 1] && iitem[q + 1] == 'n' && iitem[q + 2] && iitem[q + 2] == 'd') {
                                        // and 和 or 的优先级 -> 从左到右计算
                                        can_solve = true;
                                        break;
                                    }
                                }
                            }
                            if (can_solve) {
                                strcpy(TABLE_ITEM[i].ACTION[no], save_r);
                                strcpy(ALL_LR_ITEM_SET[i]->next[j].edge, "");
                            } else {
                                // 利用优先级可以 《人工》解决一切冲突 ！
                            }
                        } else {
                            // do nothing
                        }
                    }
                }
            } else if ((no = get_vn_no(ALL_LR_ITEM_SET[i]->next[j].edge)) != -1){
                // 非终结符，添加到GOTO
                itoa(ALL_LR_ITEM_SET[i]->next[j].status, tmp, 10);  // int to str
                strcpy(TABLE_ITEM[i].GOTO[no], tmp);
            } else {
                printf("\nerror error error\n");
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
        printf("\nerror in get_current_line()!\n");
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
            char s[20];
            strcpy(s, char_stk.stack[i].varName.c_str());
            if (!is_keyword(s) && (is_alpha(s[0]) || is_digit(s[0])) && !is_vn(s[0])){
                printf("id");
                if (fp != NULL) fprintf(fp, "id");
            } else {
                printf("%s", char_stk.stack[i].varName.c_str());
                if (fp != NULL) fprintf(fp, "%s", char_stk.stack[i].varName.c_str());
            }
            if (i < char_stk.idx - 1) {
                printf(" ");
                if (fp != NULL) fprintf(fp, " ");
            }
        }
    } else {
        printf("\nbad argument 'mode' in out_stk()\n");
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
        printf("\nfatal error!\n");
        exit(-1);
    }
    char *s = is_prefix(buf + loc);
    return s;
}

string missing_check(){ // if有then搭配，begin有end搭配，那你呢。
    int cnt_then = 0, cnt_if = 0, cnt_begin = 0, cnt_end = 0;
    for (int i = char_stk.idx - 1; i >= 0; -- i){
        if (char_stk.stack[i].varName == "then") cnt_then ++;
        else if (char_stk.stack[i].varName == "if") cnt_if ++;
        else if (char_stk.stack[i].varName == "begin") cnt_begin ++;
        else if (char_stk.stack[i].varName == "end") cnt_end ++;
    }
    if (cnt_begin == cnt_end && cnt_if == cnt_then) return "";
    else if (cnt_if > cnt_then) return "then";
    else if (cnt_if < cnt_then) return "if";
    else if (cnt_begin > cnt_end) return "end";
    else if (cnt_begin < cnt_end)  return "begin";
    return "";
}

char *get_next_status(int ct, char *input, int mode){ // 获取分析过程中的下一个动作，ACTION or GOTO
    // mode == 1 表示是查ACTION表
    // mode == 0 表示是查GOTO  表
    if (mode == 1){         // S
        int in_no = get_vt_no(input);
        if (strlen(TABLE_ITEM[ct].ACTION[in_no]) == 0) {
            map<string, string> p;
            p["end"] = "begin", p["begin"] = "end";
            p["if"] = "then", p["then"] = "if";
            strcpy(analyses[_STEP].str_now, input);
            printf("error step:\n");
            fprintf(analyse_res, "error step:\n");
            out_slr1_table_item();
            printf("syntax error!\n");
            string missing_res = missing_check();
            
            if (missing_res == "then") printf("\nline %d: missing '%s' after '%s'.", current_line + 1, missing_res.c_str(), p[missing_res].c_str());
            else if (string(input) == string("#") and missing_res == "end") printf("\nline %d: missing '%s' after '%s'.", current_line + 1, missing_res.c_str(), p[missing_res].c_str());
            else printf("\nline %d: An error occured near '%s'!\n", current_line + 1, input);
            exit(-1);
        }
        return TABLE_ITEM[ct].ACTION[in_no];
    } else if (mode == 0) { // r
        int in_no = get_vn_no(input);
                if (strlen(TABLE_ITEM[ct].GOTO[in_no]) == 0) {
            map<string, string> p;
            p["end"] = "begin", p["begin"] = "end";
            p["if"] = "then", p["then"] = "if";
            printf("error step:\n");
            strcpy(analyses[_STEP].str_now, input);
            out_slr1_table_item();
            // printf("line %d:an error occured when finding GOTO %d, to:%s\n", current_line, ct, input);
            printf("syntax error!\n");
            string missing_res = missing_check();
            if (missing_res == "then") printf("\nline %d: missing '%s' after '%s'.", current_line + 1, missing_res.c_str(), p[missing_res].c_str());
            else if (string(input) == string("#") and missing_res == "end") printf("\nline %d: missing '%s' after '%s'.", current_line + 1, missing_res.c_str(), p[missing_res].c_str());
            else printf("\nline %d: An error occured near '%s'!\n", current_line + 1, input);
            exit(-1);
        }
        return TABLE_ITEM[ct].GOTO[in_no];
    } else {                // error
        printf("\nargument mode error!\n");
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
        printf("\nerror in count_production_right_num!\n");
        exit(-1);
    }
    while (lines[line][loc]){   // 双指针
        if (is_vn(lines[line][loc])) {
            loc += 1;
        } else {
            char *s = is_prefix(lines[line] + loc);
            if (s == NULL){
                printf("\nerror in count_production_right_num!\n");
                exit(-1);
            } else {
                loc += strlen(s);
            }
        }
        res ++;
        for (; lines[line][loc] && lines[line][loc] == ' '; ++ loc){};
    }
    if (res <= 0){
        printf("\nunexpected error in count_production_right_num!\n");
        exit(-1);
    }
    return res;
}

void backpatch(vector<int>& v, int gotostm, string from=""){ // 回填
    if (from == "or" || from == "and") goto OR_AND; // 布尔表达式的回填

    for (auto ls:v){    // if 语句的回填
        auto tmp_quad = quads[ls];
        if (tmp_quad.op == "goto") quads[ls].arg1Index = gotostm;
        else if (oprts.count(tmp_quad.op)) quads[ls].result.varName = to_string(gotostm);
    }  
    return;
OR_AND:
    for (const auto &e : v)
        if (quads[e].op == "goto") quads[e].arg1Index = gotostm;
        else quads[e].result.varName = to_string(gotostm);
}
vector<int> merge(vector<int>& v1, vector<int>& v2){ // 两个链 merge
    vector<int> ans;
    ans.insert(ans.end(), v1.begin(), v1.end());
    ans.insert(ans.end(), v2.begin(), v2.end());
    return ans;
}

bool is_declared(string s="", string value=""){ // 查看某个符号是否有效
    if (value == "" or value == "_") return false;
    if (is_digit(s[0]) or s[0] == 'T') return true;    // 跳过数字
    if (is_alpha(s[0]))
        for (int i = 0; i < symbolTable.size() - 1; ++ i)
            if (s == symbolTable[i].varName or s == symbolTable[i].rawName)
                return true;
    return false;

}

void syntax_analyse(){ // 根据 SLR1分析表 进行语法分析 + 语义计算
    _STEP = 0;
    // 读入词法分析的结果并进行语法分析
    FILE* lex_reader = fopen("files/lex_res.txt", "r");
    analyse_res = fopen("files/slr1_process.txt", "w");
    if (lex_reader == NULL) {
        printf("\nread %s error!\n", "files/les_res.txt");
        exit(-1);
    }
    if (analyse_res == NULL) {
        printf("\nwrite %s error!\n", "files/slr1_process.txt");
        exit(-1);
    }
    
    char buf[LINE_MAX];   
    stack<int> gotostm;     // M.gotostm
    stack<string> semantic; // 语义栈，用于算术表达式的计算
    stack<symbol> stk_symbol_before_then;   // 用于if的回填, 可以嵌套if

    char *input;            // 当前读入的字符
    char *next_st;          // 下一个状态
    while (fgets(buf, LINE_MAX, lex_reader)){
		int line_len = strlen(buf); // while循环 排除换行符‘\n’ windos文本排除回车符'\r', 空格' '
		while ('\n' == buf[line_len - 1] || '\r' == buf[line_len - 1] || ' ' == buf[line_len - 1]) buf[line_len - 1] = '\0', line_len--;
        if (0 == line_len) continue;// 空行

        input = get_input(buf);     // 当前面临的输入
        current_line = get_current_line(buf);
        if (input == NULL) {
            printf("\nline %d: error in get_input()!\n", current_line + 1);
            exit(-1);
        }
        
        string now_get = string(input); //读头符号
        symbol tempSym;
        tempSym.rawName = now_get;
        // 将id, int和true、false登记到符号表
        if (now_get == "id" || now_get == "true" || now_get == "false"){
            string tmp_s = string(buf + 1);             
            tmp_s = tmp_s.substr(0, tmp_s.find(','));   // tmps取实际值)(词法分析结果的三元组中的第一项)，数字or标识符
            semantic.push(tmp_s); // 语义栈
            tempSym.rawName = tmp_s;
            // 将语义加入符号表，并添加入口地址映射
            tempSym.varName = tmp_s;
            if (is_digit(tmp_s.at(0))) tempSym.valueStr = tmp_s;            // 数字 
            else if (strcmp(input, "true\0") == 0) tempSym.valueStr = "1";  // bool true
            else if (strcmp(input, "false\0") == 0) tempSym.valueStr = "0"; // bool false 
            else tempSym.valueStr = string("_");                            // 起初标识符的值设置为 _ 

            // 记录读入的变量或者数值的PLACE（读入重复的标识符的处理！！！！to do）
            
            if (ENTRY.find(tempSym.varName) == ENTRY.end()){
                tempSym.PLACE = symbolTable.size();      
                symbolTable.push_back(tempSym);
                ENTRY[tempSym.varName] = tempSym.PLACE;
            } else {
                tempSym = symbolTable[ENTRY[tempSym.varName]];
            }
        } else { 
            if (now_get == "rop"){  // rop 类也有自己原来的值，rawName
                string tmp_s = string(buf + 1);             
                tmp_s = tmp_s.substr(0, tmp_s.find(','));   // 实际的符号，< > >= <= == 
                tempSym.valueStr = tmp_s;       // input == < > <= >= ...
            }
            tempSym.varName = string(input);    //
        }
        if (_STEP == 0){     // 第一次，进行初始化，状态初始为0，符号栈初始为 # 
            analyses[_STEP].step = _STEP;   
            stat_stk.stack[stat_stk.idx] = 0;
            char_stk.stack[char_stk.idx ++].varName = string("#\0");
            strcpy(analyses[_STEP].str_now, input);
            char *next_st = get_next_status(stat_stk.stack[stat_stk.idx], input, 1);
            strcpy(analyses[_STEP].Action, next_st);
            stat_stk.idx ++;
            out_slr1_table_item();      // 遇到的第一个符号一定是begin！
            stat_stk.stack[stat_stk.idx ++] = atoi(next_st + 1);
            char_stk.stack[char_stk.idx ++] = tempSym;
            _STEP ++;    // 遇到非终结符，直接_STEP + 1 
        } else {
            int stk_idx = stat_stk.idx - 1; // 取  状态栈  栈顶元素
            int top = stat_stk.stack[stk_idx];
            next_st = get_next_status(top, input, 1);
ACTION_S:
            if (next_st[0] == 'S') {
                // 执行ACTION动作
                analyses[_STEP].step = _STEP;
                strcpy(analyses[_STEP].str_now, input);
                strcpy(analyses[_STEP].Action, next_st);
                out_slr1_table_item();
                stat_stk.stack[stat_stk.idx ++] = atoi(next_st + 1);
                char_stk.stack[char_stk.idx ++] = tempSym;
                // or/and/then 入栈要记录一下位置，方便回填
                if (tempSym.varName == "or") {
                    gotostm.push(quads.size()); // 记录M指向的stm的位置, 回填
                } else if (tempSym.varName == "then"){
                    // then 入栈了，可以回填truelist
                    stk_symbol_before_then.push(char_stk.stack[char_stk.idx - 2]);
                    if (stk_symbol_before_then.top().truelist.size())
                        backpatch(stk_symbol_before_then.top().truelist, quads.size());
                    gotostm.push(quads.size()); // 记录M指向的stm的位置
                } else if (tempSym.varName == "and"){
                    gotostm.push(quads.size()); // 记录M指向的stm的位置
                }
                _STEP ++;
            } else if (next_st[0] == 'r') {
/*================================！语法分析 + 语法制导的语义分析！================================*/
                while (next_st[0] == 'r'){
                    strcpy(analyses[_STEP].Action, next_st);    // record
                    strcpy(analyses[_STEP].str_now, input); 
                    int line = atoi(next_st + 1);               // 第几条产生式
                    int num = count_production_right_num(line); // 产生式右边元素的数目
                    int left = get_production_left(lines[line]);
                    char tmp[2] = {0};  // 规约得到的非终结符
                    tmp[0] = lines[line][left], tmp[1] = '\0';
                    int top = stat_stk.stack[stat_stk.idx - num - 1];
                    next_st = get_next_status(top, tmp, 0);     // 查GOTO表
                    strcpy(analyses[_STEP].Goto, next_st);
                    out_slr1_table_item();

/*=============================------！语义计算！------=============================*/
                    int nnnn = 8;       // 算术表达式
                    int mmmm = 12;      // 算术表达式
                    int bbbb = mmmm + 4;// 布尔表达式
                    int PLACE = -1;     // 这个PLACE用来赋值给规约之后的栈顶的符号！

                    symbol res;         // 记录表达式计算过后的结果
                        // 算数&布尔表达式的语义计算
                        if (line == nnnn){ // A->id:=E, A为赋值语句
                            symbol E, id;
                            E = char_stk.stack[char_stk.idx - 1];
                            id = char_stk.stack[char_stk.idx - 3];
                            string e_name = semantic.top();
                            if (is_digit(id.rawName[0]) or id.rawName == "true" or id.rawName == "false"){
                                cout << "\nline " << current_line + 1 << ": You can't assign value to a constant.\n";
                                exit(-1);
                            } else if (!is_declared(E.rawName, E.valueStr) and !is_declared(e_name, symbolTable[ENTRY[e_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << e_name << "' is not declared!\n";
                                exit(-1);
                            }
                            GEN(":=", E.PLACE, -1, id);
                        } else if (line >= nnnn + 1 && line <= mmmm){ // E->E+$*/E
                            string opt[4] = {"+", "$", "*", "/"};
                            symbol T = newtemp();
                            symbol E1 = char_stk.stack[char_stk.idx - 3];
                            symbol E2 = char_stk.stack[char_stk.idx - 1];
                            
                            // 得到语义栈顶的两个元素
                            string e2_name = semantic.top();
                            semantic.pop();
                            string e1_name = semantic.top();
                            semantic.pop();
                            
                            GEN(opt[line - (nnnn + 1)], E1.PLACE, E2.PLACE, T);
                            if (!is_declared(E1.rawName, E1.valueStr) and !is_declared(e1_name, symbolTable[ENTRY[e1_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << e1_name << "' is not declared!\n";
                                exit(-1);
                            } else if (!is_declared(E2.rawName, E2.valueStr) and !is_declared(e2_name, symbolTable[ENTRY[e2_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << e2_name << "' is not declared!\n";
                                exit(-1);
                            }
                            semantic.push(T.varName);
                            // 保存这次规约的结果。
                            PLACE = T.PLACE;
                            res = T;
                        } else if (line == mmmm + 1){ // E-> -E
                            symbol T = newtemp();
                            symbol E1 = char_stk.stack[char_stk.idx - 1];
                            string e_name = semantic.top();
                            if (!is_declared(E1.rawName, E1.valueStr) and !is_declared(e_name, symbolTable[ENTRY[e_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << E1.rawName << "' is not declared!\n";
                                exit(-1);
                            }
                            semantic.pop();
                            semantic.push(T.varName);
                            GEN("-", E1.PLACE, -1, T);
                            // save res
                            PLACE = T.PLACE;
                            res = T;
                        } else if (line == mmmm + 2){ // E->(E)
                            res = char_stk.stack[char_stk.idx - 2];
                            PLACE = ENTRY[semantic.top()];
                        } else if (line == mmmm + 3){ // E->id
                            // symbol id = 
                            res = char_stk.stack[char_stk.idx - 1];
                            PLACE = res.PLACE;
                        } 
                        
/*=============================------！布尔表达式！------=============================*/
                        else if (line == bbbb or line == bbbb + 1) { // B->B or B , B->B and B
                            string opt[4] = {"or", "and"};
                            symbol E1 = char_stk.stack[char_stk.idx - 3];
                            symbol E2 = char_stk.stack[char_stk.idx - 1];
                            if (opt[line - bbbb] == "or"){
                                if (gotostm.empty()) {
                                    cout << "\nerror, no gotostm yet\n";
                                    exit(-1);
                                }
                                backpatch(E1.falselist, gotostm.top(), "or");
                                gotostm.pop();
                                // merge
                                res = symbol{"_"};
                                res.truelist = merge(E1.truelist, E2.truelist);
                                res.falselist  = E2.falselist;
                            } else {
                                if (gotostm.empty()) {
                                    cout << "\nerror, no gotostm yet\n";
                                    exit(-1);
                                }
                                backpatch(E1.truelist, gotostm.top(), "and");
                                gotostm.pop();
                                // merge
                                res = symbol{"_"};
                                res.falselist = merge(E1.falselist, E2.falselist);
                                res.truelist  = E2.truelist;
                            }
                        } else if (line == bbbb + 2) { // B->not B
                            symbol t = char_stk.stack[char_stk.idx - 1];
                            res.truelist = t.falselist, res.falselist = t.truelist, PLACE = t.PLACE; 
                        } else if (line == bbbb + 3) { // B->(B)
                            symbol t = char_stk.stack[char_stk.idx - 2];
                            res.truelist = t.truelist, res.falselist = t.falselist, PLACE = t.PLACE; 
                        } else if (line == bbbb + 4) { // B->E rop E impotanct
                            symbol E1 = char_stk.stack[char_stk.idx - 3];
                            symbol E2 = char_stk.stack[char_stk.idx - 1];
                            res = symbol{"_"};
                            // 得到语义栈顶的两个元素
                            string e2_name = semantic.top();
                            semantic.pop();
                            string e1_name = semantic.top();
                            semantic.pop();
                            // 将这两个元素放回去
                            semantic.push(e1_name); semantic.push(e2_name);
                            string opr = char_stk.stack[char_stk.idx - 2].valueStr;
                            res.truelist.push_back(quads.size());
                            res.falselist.push_back(quads.size() + 1);
                            
                            if (!is_declared(E1.rawName, E1.valueStr) and !is_declared(e1_name, symbolTable[ENTRY[e1_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << e1_name << "' is not declared!\n";
                                exit(-1);
                            } else if (!is_declared(E2.rawName, E2.valueStr) and !is_declared(e2_name, symbolTable[ENTRY[e2_name]].valueStr)) {
                                cout << "\nline " << current_line + 1 << ": '" << e2_name << "' is not declared!\n";
                                exit(-1);
                            }
                            GEN(opr, ENTRY[e1_name], ENTRY[e2_name], res);
                            GEN("goto", -1, -1, tempSym);   // tempSym没什么用
        
                        } else if (line == bbbb + 5 or line == bbbb + 6) { // B->true, B->false
                            symbol tmp_sym;
                            res = symbolTable[ENTRY[semantic.top()]];
                            if (line == bbbb + 5) res.truelist.push_back(quads.size()); // 添加true链
                            else res.falselist.push_back(quads.size());                 // false链
                            GEN("goto", -1, -1, tmp_sym);   // tmp_sym仅仅是一个占位作用
                            PLACE = res.PLACE;  
                        } else if (line == 4) { // C -> if B then 
                            // 因为B是bool表达式，此时回填truelist
                            symbol tmp_huitian = char_stk.stack[char_stk.idx - 2];
                            backpatch(tmp_huitian.truelist, quads.size(), "or");
                        } else if (line == 1) { // S -> CS 即S-> if B then S,得到falselist
                            // 回填false链，
                            while (stk_symbol_before_then.size()){
                                symbol symbol_before_then = stk_symbol_before_then.top();
                                if (symbol_before_then.falselist.size())
                                    backpatch(symbol_before_then.falselist, quads.size(), "or");
                                // 至此，所有的true与falselist都已经填好了，直接全部pop
                                stk_symbol_before_then.pop();
                            }
                        }
                    
                    stat_stk.idx -= num, char_stk.idx -= num;   // 出栈！
                    // 把规约之后的结果保存到栈顶，同时对于规约后的字符的名字不能修改（还是产生式左边的终结符varName = string(tmp)
                    if ((res.PLACE != -1) || res.truelist.size() || res.falselist.size()) 
                        char_stk.stack[char_stk.idx] = res, char_stk.stack[char_stk.idx ++].varName = string(tmp);
                    else char_stk.stack[char_stk.idx ++].varName = string(tmp), char_stk.stack[char_stk.idx - 1].PLACE = PLACE;
  
                    strcpy(analyses[_STEP].str_now, input);
                    top = stat_stk.stack[stat_stk.idx - 1];
                    next_st = get_next_status(top, tmp, 0);   // 查GOTO表
                    strcpy(analyses[_STEP].Goto, next_st);
                    stat_stk.stack[stat_stk.idx ++] = atoi(next_st);
                    next_st = get_next_status(stat_stk.stack[stat_stk.idx - 1], input, 1);
                    _STEP ++;
                }

/*================================！over！================================*/
                if (next_st[0] == 'S') goto ACTION_S;
                else if (next_st[0] == 'a') {   // 规约之后可以接受了！
                    strcpy(analyses[_STEP].Action, "acc\0");
                    strcpy(analyses[_STEP].str_now, "#\0");
                    out_slr1_table_item();
                    printf("accepted!\n");
                    syntax_success = true;
                    return;
                }
            } else if (next_st[0] == 'a') {     // 判断是不是acc, 也许没用，因为接受都是在规约之后(即在上面那里接受)
                strcpy(analyses[_STEP].Action, "acc\0");
                strcpy(analyses[_STEP].str_now, "#\0");
                out_slr1_table_item();
                printf("accepted! %s\n", analyses[_STEP].Action);
                syntax_success = true;
                return;
            } else {
                printf("\nexpected status!\n");
                exit(-1);
            }
        }
	}
	out_slr1_table_item();
    if (!syntax_success) {
        map<string, string> p;
        p["end"] = "begin", p["begin"] = "end";
        p["if"] = "then", p["then"] = "if";
        string missing_res = missing_check();
        if (missing_res != "") printf("\nline %d: missing '%s' after '%s'.", current_line + 1, missing_res.c_str(), p[missing_res].c_str());
        else printf("\nmissing '#' at the end of source code file!\n");
        exit(-1);
    }
    if (0 == feof){
		printf("fgets error\n"); // 未读到文件末尾
		return;
	}
    fclose(lex_reader);
}

void slr1_runner(){ // SLR1分析，启动！
    FILE* fp = fopen("files/slr1_item_set.txt", "w");
    if (NULL == fp){
        printf("\nopen %s failed.\n", "files/slr1_item_set.txt\0");
        exit(-1);
        return;
	}
    // 这里的终结符要加一个 # 
    strcpy(V->vt[V->len_vt ++], "#\0");
    // 初始化求所有的项目集
    struct lr_item_set* S;
    init(&S);
    shift(S);

    lr_table_generator();
    // 输出slr1分析表
    printf("slr1 table:\n");
    {   FILE* fp_table = fopen("files/slr1_table.txt", "w");
        if (fp_table == NULL) {
            printf("\nwrite %s failed.", "files/slr1_table.txt");
            exit(-1);
        }
        printf("+------------------------------------------------------------------------------------------------------------------------------+\n");
        printf("|    |                                       ACTION                                            |              GOTO             |\n");
        printf("|----+-----------------------------------------------------------------------------------------+-------------------------------|\n");
        printf("|%4s|", "stat");
        for (int i = 0; i < V->len_vt; ++ i){
            printf("%3s|", V->vt[i]);
            fprintf(fp_table, "%s ", V->vt[i]);
        }
        // printf("%3s|", "#");
        for (int i = 0; i < V->len_vn; ++ i){
            printf(" %-2s|", V->vn[i]);
            fprintf(fp_table, "%s ", V->vn[i]);
        }
        fprintf(fp_table, "\n");
        printf("\n");
        printf("|----+-----------------------------------------------------------------------------------------+-------------------------------|\n");

        for (int i = 0; i < UID; ++ i){
            printf("| %-2d |", TABLE_ITEM[i].status);
            fprintf(fp_table, "%d|", TABLE_ITEM[i].status);
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
                fprintf(fp_table, "%s|", TABLE_ITEM[i].ACTION[j]);
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
                fprintf(fp_table, "%s|", TABLE_ITEM[i].GOTO[j]);
                for (int x = 0; x < back; ++ x) printf(" ");
                printf("|");
            }
            printf("\n");
            fprintf(fp_table, "%\n");
        }
        printf("+------------------------------------------------------------------------------------------------------------------------------+\n");
        fclose(fp_table);
    }

    // 将项目集写入到文件中
    {
        for (int i = 0; i < UID; ++i) {
//        printf("UID:%d  ", i);
            fprintf(fp, "%d\n", i);
//        printf("\n");
            for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt_next_status; ++j) {
//            printf("(%s, %d) ", ALL_LR_ITEM_SET[i]->next[j].edge, ALL_LR_ITEM_SET[i]->next[j].status);
                if (strlen(ALL_LR_ITEM_SET[i]->next[j].edge) > 0)
                    fprintf(fp, "(%s, %d) ", ALL_LR_ITEM_SET[i]->next[j].edge, ALL_LR_ITEM_SET[i]->next[j].status);
            }
            fprintf(fp, "\n");
//        printf("\ncore:\n");
            for (int j = 0; j < ALL_LR_ITEM_SET[i]->core; ++j) {
                int loc = ALL_LR_ITEM_SET[i]->item_set[j].loc;
//            printf("%d, %d, %c, %s\n", ALL_LR_ITEM_SET[i]->core, loc, ALL_LR_ITEM_SET[i]->item_set[j].item[loc], ALL_LR_ITEM_SET[i]->item_set[j].item);
            }
//        printf("\n");
            for (int j = 0; j < ALL_LR_ITEM_SET[i]->cnt; ++j) {
                int loc = ALL_LR_ITEM_SET[i]->item_set[j].loc;
//            printf("%d, %d, %c, %s\n", ALL_LR_ITEM_SET[i]->core, loc, ALL_LR_ITEM_SET[i]->item_set[j].item[loc], ALL_LR_ITEM_SET[i]->item_set[j].item);
                fprintf(fp, "%d~%s\n", ALL_LR_ITEM_SET[i]->item_set[j].loc, ALL_LR_ITEM_SET[i]->item_set[j].item);
            }
//        printf("------------------------------\n");
        }
    }
    fclose(fp);

    printf("analyse process:\n");
    syntax_analyse();

    cout << "\nsymbolTable" << endl;
    for (auto & it : symbolTable)
        cout << it.varName << " " << it.valueStr << " " << it.PLACE << " " << it.truelist.size() << " " << it.falselist.size()  << endl;
    cout << endl;

    cout << "ENTRY" << endl;
    for (auto & it : ENTRY)
        cout << it.first << " " << it.second << endl;

    cout << "\nquads.(len) = " << quads.size() << endl;
    out_quad(quads);

}
