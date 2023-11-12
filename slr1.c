// SLR1只针对有LR0分析表有冲突的进行分析，
// 1. 移进-规约冲突
// 2. 规约-规约冲突
#define COUNT 20
struct item // SLR分析表的每一行。
{
    int status; // 每一行的编号，也即项目集编号
    char ACTION[40][5];
    char GOTO[20][5];
    /* data */
} item[COUNT];  // 分析表有多少行(项目集有多少个), COUNT就取多少，可以malloc???
