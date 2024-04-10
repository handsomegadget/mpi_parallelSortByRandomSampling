#ifndef SORT_H
#define SORT_H

#include "macro.h"
#include "list.h"
#include "psrs.h"

#include <stdbool.h>
#include <stddef.h>

struct arr_segment { //一个数组段
        long *head; //首元素的地址
        int size; //数组段的长度
};

struct segment_block { //每个进程管理自己的区块，区块内可能含有多个数组段
        bool clean; 
        int size; //有多少个数组段
        struct arr_segment part[]; //数组段的列表
};

struct procargs { // 每个进程的参数，灵感来自于xv6操作系统
        unsigned int root; // 是否是根进程
        int id; // 进程的ID
        int procnum; // 本次运行的进程总数
        long *head; // 进程自己分到的数组的首地址
        int size; //进程自己分到的数组段大小
        int max_sample_size; // 最大采样大小
        int total_size;// 全局排序任务的数组大小
};

enum sort_stat { 
        MEAN,
        SORT_STAT_SIZE
};

enum psrs_phase {
        PHASE1,
        PHASE2,
        PHASE3,
        PHASE4,
        PHASE_COUNT
};

void
start_sort(const struct arguments *const arg);

static int
sequential_sort(double ssort_time[], const struct arguments *const arg);

static void
parallel_sort(double psort_stats[], const struct arguments *const arg);

static void
psort_wrapper(double elapsed[], const struct arguments *const arg);

static void
local_scatter(long array[], struct procargs *const arg);

static void
local_sort(struct arr_segment *const local_samples,
           const struct procargs *const arg);

int 
segment_block_init(struct segment_block **self, bool clean, int size);
int 
segment_block_destroy(struct segment_block **self);
static void
arr_segment_form(struct segment_block *const blk,
               struct arr_segment *const pivots,
               const struct procargs *const arg);
static void
arr_segment_send(struct segment_block *const blk_copy,
               struct segment_block *const blk,
               const int sid,
               int *const pindex,
               const struct procargs *const arg);
               static void
arr_segment_merge(struct arr_segment *const result,
                struct segment_block *blk_copy,
                const struct procargs *const arg);
static int 
long_compare(const void *left, const void *right);

static void
psort_start(double elapsed[],
            long array[],
            struct procargs *const arg);

static void
pivots_bcast(struct arr_segment *const pivots,
             struct arr_segment *const local_samples,
             const struct procargs *const arg);

static void
arr_segment_exchange(struct segment_block *const blk_copy,
                   struct segment_block *blk,
                   const struct procargs *const arg);
static int
binary_search(int *const index,
           const long value,
           const long array[],
           const int size);

static void
output_write(double data[], const struct arguments *const arg);

static int array_merge(long output[],
                       const long left[],
                       const size_t lsize,
                       const long right[],
                       const size_t rsize);
static inline void
mpi_recv_check(const MPI_Status *const status,
               MPI_Datatype datatype,
               const int count);
#endif /* SORT_H */