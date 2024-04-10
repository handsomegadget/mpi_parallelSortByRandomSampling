#include "../include/macro.h"
#include "../include/psrs.h"
#include "../include/convert.h"
#include "../include/sort.h"
#include <errno.h>
#include <getopt.h>      
#include <inttypes.h>    
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      


int main(int argc, char *argv[])
{
        MPI_Init(&argc, &argv);
        int rank;  //获得进程的序号
        struct arguments arg = { }; //每个进程都有一个
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank == 0) { //只有主进程能获得参数
                if (argument_parse(&arg, argc, argv) < 0) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
        }
        argument_bcast(&arg); //由主进程将参数传递给其他进程
        start_sort(&arg); //开始排序
        MPI_Barrier(MPI_COMM_WORLD); //等待所有进程结束
        MPI_Finalize();
        return 0;
}

/*从命令行获取参数*/
static int argument_parse(struct arguments *result, int argc, char *argv[])
{
        static const char *const OPT_STR = "l:o";
        static const struct option OPTS[] = {
                {"length",   required_argument, NULL, 'l'}, //数组长度
                {"outphase",   no_argument, NULL, 'o'}};    //是否输出各个阶段的用时
        if (result == NULL || argc == 0 || argv == NULL) {
                errno = EINVAL;
                return -1;
        }

        result->outphase = false; 
        int opt = 0;
        while (-1 != (opt = getopt_long(argc, argv, OPT_STR, OPTS, NULL))) {
                switch (opt) {
                case 0:
                        break;
                case 'l': {
                        if (!int_convert(result->length, optarg)) {
                                fprintf(stderr, "Length is too large or not valid\n");
                                return -1;
                        }
                        break;
                }
                case 'o':
                        result->outphase = true;
                        break;
                }
        }
        MPI_Comm_size(MPI_COMM_WORLD, &(result->procnum));
        if (result->length % result->procnum != 0) {
                fprintf(stderr,"Length is not divisible by the number of "
                           "processes");
                return -1;
        }

}
/* arg 结构体中成员的值从根进程（排名为0的进程）
广播到 MPI 通信组中的所有其他进程。
这样，所有的进程都能获得相同的arg结构体中的值*/
static void argument_bcast(struct arguments *arg)
{
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->length), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->procnum), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->outphase), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
}

