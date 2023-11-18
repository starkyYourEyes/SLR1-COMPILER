// SLR1只针对有LR0分析表有冲突的进行分析，
// 1. 移进-规约冲突
// 2. 规约-规约冲突
// first 集 和 follow集合要事先生成！
// 非终结符集中多了一个 # ！
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "first_follow.h"
#include "slr1.h"


int main(int argc, char *argv[]){
    FILE* fp = fopen("files/item_set.txt", "w");
    if (NULL == fp){
        printf("open %s failed.\n", "files/item_set.txt\0");
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

    shift(S);
    // cal_first();
    // cal_follow();
    read_fisrt_follow_sets();

    // printf("UID:%d\n", UID);
    printf("next num:%d\n", S->cnt_next_status);
    for (int i = 0; i < S->cnt_next_status; ++ i){
        printf("%d, %s\n", S->next[i].status, S->next[i].edge);
    }
    
    printf("============================================================\n");
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
    fclose(fp);

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
    for (int i = 0; i < stat_stk.idx; ++ i)
        printf("%d ", stat_stk.stack[i]);
    printf("\n");
    for (int i = 0; i < char_stk.idx; ++ i)
        printf("%s ", char_stk.stack[i]);
    printf("\n");

    return 0;
}
