#include "slr1.h"
#include "lexical.h"
/*
第一个参数——项目的文法文件(files/grammar.txt)
第二个参数——源文件(files/code.txt)
*/
int main(int argc, char *argv[]){
//    printf("%s\n", _pgmptr);
    FILE* w_res = fopen("./files/first-follow-set.txt", "w"); // 将first集和follow集写入文件中
	if (NULL == w_res){
		printf("write %s failed.\n", "./files/first-follow-set.txt");
		return -1;
	}
	V = (struct CHARS *)malloc(sizeof(struct CHARS));
	V->len_vn = V->len_vt = 0;

    get_vs(argv[1]);    // argv[1] -> grammar.txt
    printf("print Vns and Vts:\n");
    {
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
    }

    read_lines(argv[1]);

    get_first_set();

    printf("\nfirst sets:\n");
    // 打印first集并写入到文件中
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

    cal_follow();
    
    printf("\nfollow sets:\n");
	{
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
    }
	fclose(w_res);

    printf("\nlex res:\n");
    lex_runner(argv[2]);
    printf("\nslr1 analyse:\n");
    slr1_runner();




    return 0;
}