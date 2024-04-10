#include "../include/macro.h"
#include "../include/sort.h"

#include "../include/arraygen.h"
#include "../include/list.h"
#include "../include/psrs.h"
#include "../include/timing.h"
#include <chrono>
#include <errno.h>
#include <math.h>    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void start_sort(const struct arguments *const arg)
{
        int rank = 0;
        double ssort_time[SORT_STAT_SIZE]; //单元素数组，便于传递地址参数
        double psort_time[SORT_STAT_SIZE];
        double psort_per_phase_time[PHASE_COUNT];
        //获取进程序号
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        //如果只有一个进程，就直接用串行排序
        if (arg->procnum == 1) {
                if (sequential_sort(ssort_time, arg) < 0) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }

        } else {
                if (arg->outphase) {
                        parallel_sort(psort_per_phase_time, arg);
                } else {
                        parallel_sort(psort_time, arg);
                }
        }
        //主进程负责输出结果
        if (0 == rank) { 
                if (1 == arg->procnum) {
                        output_write(ssort_time, arg);
                } else if (1 < arg->procnum) {
                        if (arg->outphase) {
                                output_write(psort_per_phase_time, arg);
                        } else {
                                output_write(psort_time, arg);
                        }
                }
        }

        MPI_Barrier(MPI_COMM_WORLD);
}

/*输出排序时间*/
static void
output_write(double data[], const struct arguments *const arg)
{
        if (NULL == data || NULL == arg) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        if (1 == arg->procnum) {
                puts("Mean Sorting Time");
                printf("%f\n", data[MEAN]);              
        } else{
                if (arg->outphase) {
                                puts("Phase 1, Phase 2, Phase 3, Phase 4");
                                printf("%f, %f, %f, %f\n",
                                       data[PHASE1],
                                       data[PHASE2],
                                       data[PHASE3],
                                       data[PHASE4]);
                } else {
                                puts("Mean Sorting Time");
                                printf("%f\n", data[MEAN]);
                        }
                }
        
}
/*串行排序*/
static int
sequential_sort(double ssort_time[], const struct arguments *const arg)
{
        double avg_time = 0;
        long *array = NULL;
        Generator generator;
        std::chrono::high_resolution_clock::time_point start;
        double elapsed;
        if (NULL == ssort_time || NULL == arg) {
                errno = EINVAL;
                return -1;
        }

        if (0 > generator.array_generate(&array, arg->length)) {
                return -1;
        }

        timing_reset(start);
        for (size_t iteration = 0U; iteration < 10; ++iteration) {
                timing_start(start);
                qsort(array, arg->length, sizeof(long), long_compare);
                timing_stop(elapsed, start);
                timing_reset(start);
                avg_time += (elapsed/10);
                if (0 > generator.array_generate(&array, arg->length)) {
                        return -1;
                }
                elapsed = .0;
        }

        generator.array_destroy(&array);
        ssort_time[MEAN] = avg_time;
        return 0;
}

/*PSRS*/
static void
parallel_sort(double psort_stats[], const struct arguments *const arg)
{
        double per_phase_sort_time[PHASE_COUNT];
        double avg_per_phase_sort_time[PHASE_COUNT];
        double total_sort_time = 0;
        memset(per_phase_sort_time, 0, sizeof per_phase_sort_time);
        memset(avg_per_phase_sort_time, 0, sizeof per_phase_sort_time);

        for (unsigned int i = 0; i < 10; ++i) { //运行十次，对时间取平均数
                psort_wrapper(per_phase_sort_time, arg);           
                for (int j = 0; j < PHASE_COUNT; ++j) {
                        avg_per_phase_sort_time[j] += (per_phase_sort_time[j]/10); //每个phase的运行时间
                        total_sort_time += (per_phase_sort_time[j]/10); //总运行时间
                }       
                MPI_Barrier(MPI_COMM_WORLD); 
        }

        if (arg->outphase) { //如果要分段输出
                for (int j = 0; j < PHASE_COUNT; ++j) {
                                psort_stats[j] += per_phase_sort_time[j];
                        }
        } else { //如果要输出整块的时间
                psort_stats[0] = total_sort_time;
        }
}

static void
psort_wrapper(double elapsed[], const struct arguments *const arg)
{
        long *array = NULL;
        Generator generator;
        /* 每个进程需要处理的元素 */
        int chunk_size = (int)ceil((double)arg->length / arg->procnum);
        struct procargs process_info;

        memset(&process_info, 0, sizeof(struct procargs));
        MPI_Comm_rank(MPI_COMM_WORLD, &(process_info.id));
        process_info.total_size = arg->length;
        process_info.procnum = arg->procnum;

        if (0 == process_info.id) { //rank为0的是主进程
                process_info.root = true;
        } else {
                process_info.root = false;
        }

        if (process_info.root) {
                if (0 > generator.array_generate(&array, arg->length)) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }

        }
        MPI_Barrier(MPI_COMM_WORLD);
        if (arg->procnum != (process_info.id + 1)) {
                process_info.size = chunk_size;
        } else {
                if (0 != (arg->length % chunk_size)) {
                        process_info.size = arg->length % chunk_size;
                        process_info.max_sample_size = process_info.size;
                } else {
                        process_info.size = chunk_size;
                        process_info.max_sample_size = arg->procnum;
                }
        }

        psort_start(elapsed, array, &process_info);

        if (process_info.root) {
                generator.array_destroy(&array);
        }
}
static void
psort_start(double elapsed[],
            long array[],
            struct procargs *const arg)
{
        std::chrono::high_resolution_clock::time_point start;

        struct arr_segment local_samples;
        struct arr_segment pivots;
        struct segment_block *blk = NULL;
        struct segment_block *blk_copy = NULL;
        struct arr_segment result;
        timing_reset(start);
        MPI_Barrier(MPI_COMM_WORLD);
        /*第一阶段：均匀划分、局部排序、正则采样*/
        if (arg->root) { //由主进程计时
                timing_start(start);
        }
        local_scatter(array, arg);
        local_sort(&local_samples, arg);
        MPI_Barrier(MPI_COMM_WORLD);
        if (arg->root) {
                timing_stop((elapsed[PHASE1]), start);
                timing_reset(start);
                timing_start(start);
        }
  
        /*第二阶段*/
        pivots_bcast(&pivots, &local_samples, arg);
        if (arg->procnum != pivots.size + 1) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        if (segment_block_init(&blk, false, pivots.size + 1) < 0) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        if (arg->root) {
                timing_stop((elapsed[PHASE2]), start);
                timing_reset(start);
                timing_start(start);
        }
         /*第三阶段：用主元分割数组，相互交换数组块*/
        arr_segment_form(blk, &pivots, arg);
        MPI_Barrier(MPI_COMM_WORLD);
        
       
        if (0 > segment_block_init(&blk_copy, true, pivots.size + 1)) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
     
        arr_segment_exchange(blk_copy, blk, arg);
           /*第四阶段：，归并排序，主进程合并结果*/
        if (arg->root) {
                timing_stop((elapsed[PHASE3]), start);
                timing_reset(start);
                timing_start(start);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        arr_segment_merge(&result, blk_copy, arg);
        MPI_Barrier(MPI_COMM_WORLD);
        /* 结束psrs，验证结果 */
        if (arg->root) {
                timing_stop((elapsed[PHASE4]), start); //结束第四阶段的计时
                /*验证排序结果是否正确*/
                long *cmp = (long *)calloc(arg->total_size, sizeof(long));
                memcpy(cmp, result.head, arg->total_size * sizeof(long));
                qsort(cmp, arg->total_size, sizeof(long), long_compare);
                if(memcmp(cmp, result.head, arg->total_size * sizeof(long))!=0){
                       puts("The Result is Wrong!");
                       MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); 
                }
                free(cmp);
                free(result.head);
        }
        MPI_Barrier(MPI_COMM_WORLD);
}

static void
local_scatter(long array[], struct procargs *const arg)
{
        arg->head = (long *)calloc(arg->size, sizeof(long));
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->max_sample_size),
                  1,
                  MPI_INT,
                  arg->procnum - 1,
                  MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);

        /* 将 array 中的数据按照指定的方式分发给每个进程 */
        MPI_Scatter(array,
                    arg->size,
                    MPI_LONG,
                    arg->head,
                    arg->size,
                    MPI_LONG,
                    0,
                    MPI_COMM_WORLD);


}

static void
local_sort(struct arr_segment *const local_samples,
           const struct procargs *const arg)
{
        /* 采样步长 */
        int window = arg->total_size / (arg->procnum * arg->procnum);
        List local_sample_list;

        memset(local_samples, 0, sizeof(struct arr_segment));
        /*局部排序*/
        qsort(arg->head, arg->size, sizeof(long), long_compare);
        /*先用链表存储采样结果*/
        for (int idx = 0, picked = 0;
             idx < arg->size && picked < arg->max_sample_size;
             idx += window, ++picked) {
                if (0 > local_sample_list.add(arg->head[idx])) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
                if (0U == window) {
                        break;
                }
        }
        local_samples->size = local_sample_list.getSize();
        local_samples->head = (long *)calloc(local_samples->size,
                                             sizeof(long));
        if (NULL == local_samples->head) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        /*将采样结果放入local_samples内*/
        long *array = new long[local_samples->size];
        local_sample_list.copyTo(array);
        memcpy(local_samples->head, array, local_samples->size * sizeof(long));

        delete[] array;
        MPI_Barrier(MPI_COMM_WORLD);


}

static void pivots_bcast(struct arr_segment *const pivots,
                         struct arr_segment *const local_samples,
                         const struct procargs *const arg) {
    int pivot_step = 0;
    List pivot_list;
    struct arr_segment total_samples;

    // 检查输入参数是否为NULL，如果是，则调用MPI_Abort终止MPI程序
    if (NULL == pivots || NULL == local_samples || NULL == arg) {
        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    // 计算每个进程应该选取的主元的步长
    pivot_step = arg->procnum;
    // 初始化pivots和total_samples结构体
    memset(pivots, 0, sizeof(struct arr_segment));
    memset(&total_samples, 0, sizeof(struct arr_segment));

    // 使用MPI_Reduce将每个进程的局部采样点数目汇总到根进程的total_samples.size中
    MPI_Reduce(&(local_samples->size),
               &(total_samples.size),
               1,
               MPI_INT,
               MPI_SUM,
               0,
               MPI_COMM_WORLD);

    // 如果是根进程，为total_samples.head分配空间
    if (arg->root) {
        total_samples.head = (long *)calloc(total_samples.size,
                                            sizeof(long));
    }

    // 等待所有进程执行完MPI_Reduce
    MPI_Barrier(MPI_COMM_WORLD);

    // 使用MPI_Gather收集所有进程的局部采样点数据到total_samples.head中
    MPI_Gather(local_samples->head,
               local_samples->size,
               MPI_LONG,
               total_samples.head,
               local_samples->size,
               MPI_LONG,
               0,
               MPI_COMM_WORLD);
    // 释放local_samples.head的内存
    free(local_samples->head);
    local_samples->head = NULL;

    // 如果是根进程
    if (arg->root) {
        // 对total_samples.head进行全局排序
        qsort(total_samples.head,
              total_samples.size,
              sizeof(long),
              long_compare);

        // 从排序后的采样点中选取每个进程所需的主元
        for (int i = pivot_step, count = 0;
             i < total_samples.size && count < arg->procnum - 1;
             i += arg->procnum, ++count) {
            // 将选取的主元添加到pivot_list中
            if (0 > pivot_list.add(total_samples.head[i])) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
            }
        }
        // 释放total_samples.head的内存
        free(total_samples.head);

        // 初始化变量
        long value;
        total_samples.head = NULL;
        
        // 将选取的主元存储到pivots结构体中
        pivots->size = pivot_list.getSize();
        pivots->head = (long *)calloc(pivots->size, sizeof(long));

        // 检查内存分配是否成功
        if (NULL == pivots->head) {
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        // 将主元数据复制到pivots->head中
        long *array = new long[pivots->size];
        pivot_list.copyTo(array);
        memcpy(pivots->head, array, pivots->size * sizeof(long));

        // 释放array的内存
        delete[] array;
    }

    // 等待所有进程执行完上述操作
    MPI_Barrier(MPI_COMM_WORLD);

    // 使用MPI_Bcast广播pivots->size到所有进程
    MPI_Bcast(&(pivots->size),
              1,
              MPI_INT,
              0,
              MPI_COMM_WORLD);

    // 如果不是根进程，为pivots->head分配空间
    if (false == arg->root) {
        pivots->head = (long *)calloc(pivots->size, sizeof(long));

        // 检查内存分配是否成功
        if (NULL == pivots->head) {
            MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
    }

    // 等待所有进程执行完上述操作
    MPI_Barrier(MPI_COMM_WORLD);

    // 使用MPI_Bcast广播pivots->head到所有进程
    MPI_Bcast(pivots->head,
              pivots->size,
              MPI_LONG,
              0,
              MPI_COMM_WORLD);
}

/**
 * @brief 初始化分段块结构体
 * 
 * @param self 分段块结构体的指针的指针，用于存储创建的结构体的地址
 * @param clean 分段块是否需要清空的标志，true表示需要清空，false表示不清空
 * @param size 分段块中数组段的大小
 * @return int 返回操作状态，成功返回0，失败返回-1
 */
int segment_block_init(struct segment_block **self, bool clean, int size)
{
    struct segment_block *blk = NULL;
    size_t total_size = sizeof(struct segment_block) +
                        size * sizeof(struct arr_segment);
    if (NULL == self || 0 >= size) {
        errno = EINVAL;
        return -1;
    }
    blk = (struct segment_block *)malloc(total_size);
    if (NULL == blk) {
        return -1;
    }
    // 将分配的内存清零，包括结构体和数组段的部分
    memset(blk, 0, total_size);
    // 设置分段块结构体的clean和size成员变量的值
    blk->clean = clean;
    blk->size = size;
    *self = blk;
    return 0;
}


int segment_block_destroy(struct segment_block **self)
{
        struct segment_block *blk = NULL;
        if (NULL == self) {
                errno = EINVAL;
                return -1;
        }
        blk = *self;
        if (blk->clean) {
                for (int i = 0; i < blk->size; ++i) {
                        free(blk->part[i].head);
                }
        }
        free(blk);
        *self = NULL;
        return 0;
}

static void
arr_segment_form(struct segment_block *const blk,
               struct arr_segment *const pivots,
               const struct procargs *const arg)
{
        int part_idx = 0;
        int sub_idx = 0;
        int prev_part_size = 0;
        long pivot = 0;
        int terminate = 0;
        int terminate_from = 0;
        if (NULL == blk || NULL == pivots || NULL == arg) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        
        blk->part[part_idx].head = arg->head;
        for (int pivot_idx = 0; pivot_idx < pivots->size; ++pivot_idx) {
                pivot = pivots->head[pivot_idx];


                if (0 > binary_search(&sub_idx,
                                   pivot,
                                   blk->part[part_idx].head,
                                   arg->size - prev_part_size)) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
                

                blk->part[part_idx].size = sub_idx;
                prev_part_size += sub_idx;
                if(arg->size - prev_part_size == 0){
                        terminate = 1;
                        terminate_from = part_idx + 1;
                        break;
                }
                blk->part[part_idx + 1].head = blk->part[part_idx].head +\
                                               blk->part[part_idx].size;
                ++part_idx;}

        if(terminate){
                for(int i = terminate_from; i <= pivots->size; i++){
                        blk->part[i].head = NULL;
                        blk->part[i].size = 0;
                }
        }
        else{blk->part[part_idx].size = arg->size - prev_part_size;}
                
        free(pivots->head);
        MPI_Barrier(MPI_COMM_WORLD);

}

static void
arr_segment_merge(struct arr_segment *const result,
                struct segment_block *blk_copy,
                const struct procargs *const arg)
{       
        const int N = arg->procnum + 1;
        struct arr_segment running_result;
        struct arr_segment temp_result[N];
        struct arr_segment merge_dump;
        MPI_Status recv_status;
        if (NULL == result || NULL == blk_copy || NULL == arg) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        memset(result, 0, sizeof(struct arr_segment));
        memset(&recv_status, 0, sizeof(MPI_Status));

        running_result = blk_copy->part[0];
        for (int i = 1; i < arg->procnum; ++i) {
                merge_dump.size = running_result.size + blk_copy->part[i].size;
                if(blk_copy->part[i].size>0){
                merge_dump.head = (long *)calloc(merge_dump.size, sizeof(long));
                if (NULL == merge_dump.head) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
                array_merge(merge_dump.head,
                            running_result.head,
                            running_result.size,
                            blk_copy->part[i].head,
                            blk_copy->part[i].size);
                running_result = merge_dump;}
        }

        temp_result[arg->id].size = running_result.size;
        temp_result[arg->id].head = (long *)calloc(running_result.size, sizeof(long));
        memcpy(temp_result[arg->id].head, running_result.head,running_result.size*sizeof(long));
        if (0 > segment_block_destroy(&blk_copy)) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        if (arg->root) {
                result->size = arg->total_size;
                result->head = (long *)calloc(arg->total_size, sizeof(long));

                if (NULL == result->head) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        
        for (int i = 1,merged_size = 0;
             i < arg->procnum;
             ++i) {
                if (arg->root) {
                        
                               MPI_Recv(&merged_size,
                                 1,
                                 MPI_INT,
                                 i,
                                 MPI_ANY_TAG,
                                 MPI_COMM_WORLD,
                                 &recv_status);
                        mpi_recv_check(&recv_status,
                                       MPI_INT,
                                       1); 
                        
                        
                        if(merged_size>0){
                                temp_result[i].size = merged_size;
                                temp_result[i].head = (long *)calloc(merged_size, sizeof(long));
                        MPI_Recv(temp_result[i].head,
                                 merged_size,
                                 MPI_LONG,
                                 i,
                                 MPI_ANY_TAG,
                                 MPI_COMM_WORLD,
                                 &recv_status);
                        mpi_recv_check(&recv_status,
                                       MPI_LONG,
                                       merged_size);
                        }

                } else if (i == arg->id) {
                        MPI_Ssend(&temp_result[i].size,
                                  1,
                                  MPI_INT,
                                  0,
                                  0,
                                  MPI_COMM_WORLD);

                        MPI_Ssend(temp_result[i].head,
                                  temp_result[i].size,
                                  MPI_LONG,
                                  0,
                                  0,
                                  MPI_COMM_WORLD);
                }
                MPI_Barrier(MPI_COMM_WORLD);
        }
        MPI_Barrier(MPI_COMM_WORLD);

        
        if(arg->root){
               int lastsize = 0;
                for(int i = 0; i < arg->procnum; i++){
                        memcpy((result->head + lastsize), temp_result[i].head, (temp_result[i].size) * sizeof(long));
                        lastsize += temp_result[i].size;
                        
                }
        }
        MPI_Barrier(MPI_COMM_WORLD);
}

static void
arr_segment_exchange(struct segment_block *const blk_copy,
                   struct segment_block *blk,
                   const struct procargs *const arg)
{
        int part_idx = 0;

        if (NULL == blk_copy || NULL == blk || NULL == arg) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        for (int i = 0; i < arg->procnum; ++i) {
                arr_segment_send(blk_copy, blk, i, &part_idx, arg);
        }

        if (0 > segment_block_destroy(&blk)) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        free(arg->head);


}
static void
arr_segment_send(struct segment_block *const blk_copy,
               struct segment_block *const blk,
               const int sid,
               int *const pindex,
               const struct procargs *const arg)
{
        MPI_Status recv_status;

        if (NULL == blk_copy || NULL == blk ||\
            0 > sid || NULL == pindex || 0 > *pindex || NULL == arg) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }

        memset(&recv_status, 0, sizeof(MPI_Status));
        /*如果是sender自己*/
        if (sid == arg->id) {
                blk_copy->part[*pindex].size = blk->part[arg->id].size;
                if(blk_copy->part[*pindex].size>0){
                blk_copy->part[*pindex].head = (long *)\
                                               calloc(blk->part[arg->id].size,
                                                      sizeof(long));

                memcpy(blk_copy->part[*pindex].head, blk->part[arg->id].head,
                       blk->part[arg->id].size * sizeof(long));}
                else blk_copy->part[*pindex].head = NULL;
                ++*pindex;
        }
        for (int j = 0; j < arg->procnum; ++j) {
                if (sid != j) {
                        if (sid == arg->id) {
                                MPI_Ssend(&(blk->part[j].size),
                                          1,
                                          MPI_INT,
                                          j,
                                          0,
                                          MPI_COMM_WORLD);
                                if(blk->part[j].size!=0){
                                MPI_Ssend(blk->part[j].head,
                                          blk->part[j].size,
                                          MPI_LONG,
                                          j,
                                          0,
                                          MPI_COMM_WORLD);}
                        } else if (j == arg->id) {
                                MPI_Recv(&(blk_copy->part[*pindex].size),
                                         1,
                                         MPI_INT,
                                         sid,
                                         MPI_ANY_TAG,
                                         MPI_COMM_WORLD,
                                         &recv_status);
                                mpi_recv_check(&recv_status,
                                               MPI_INT,
                                               1);
                                if(blk_copy->part[*pindex].size>0){
                                blk_copy->part[*pindex].head =(long *)calloc(\
                                                blk_copy->part[*pindex].size,
                                                sizeof(long));
                                if (NULL == blk_copy->part[*pindex].head) {
                                        MPI_Abort(MPI_COMM_WORLD,
                                                  EXIT_FAILURE);
                                }
                                MPI_Recv(blk_copy->part[*pindex].head,
                                         blk_copy->part[*pindex].size,
                                         MPI_LONG,
                                         sid,
                                         MPI_ANY_TAG,
                                         MPI_COMM_WORLD,
                                         &recv_status);
                                
                                mpi_recv_check(&recv_status,
                                               MPI_LONG,
                                               blk_copy->part[*pindex].size);}

                                else{
                                        blk_copy->part[*pindex].head = NULL;
                                }
                                ++*pindex;
                        }
                }
                MPI_Barrier(MPI_COMM_WORLD);
        }
}
static int long_compare(const void *left, const void *right)
{
        const long left_long = *((const long *)left);
        const long right_long = *((const long *)right);

        return (left_long < right_long ? -1 : left_long > right_long ? 1 : 0);
}

static int
binary_search(int *const index,
           const long value,
           const long array[],
           const int size)
{
        int start, middle, end;

        if (NULL == index || NULL == array || 0 >= size) {
                errno = EINVAL;
                return -1;
        }

        start = 0;
        end = size - 1;
        middle = (start + end) / 2;

        while (start <= end) {
                if (array[middle] <= value) {
                        start = middle + 1;
                } else if (array[middle] > value) {
                        end = middle - 1;
                }
                middle = (start + end) / 2;
        }

        *index = start;

        return 0;
}
static int array_merge(long output[],
                       const long left[],
                       const size_t lsize,
                       const long right[],
                       const size_t rsize)
{
        size_t lindex = 0U, rindex = 0U, oindex = 0U;
        if(lsize == 0){
                
                for(int i = 0; i < rsize; i++){
                        output[i] = right[i];
                }
        }
        if(rsize == 0){
                for(int i = 0; i < lsize; i++){
                        output[i] = left[i];

                }
        }
        // 
        // // 合并两个已排序数组
        for (; lindex < lsize && rindex < rsize; ++oindex) {
                if (left[lindex] < right[rindex]) {
                        output[oindex] = left[lindex];
                        ++lindex;
                } else {
                        output[oindex] = right[rindex];
                        ++rindex;
                }
        }
        if (lindex < lsize) {
                for (; lindex < lsize; ++lindex, ++oindex) {
                        output[oindex] = left[lindex];
                }
        }

        if (rindex < rsize) {
                for (; rindex < rsize; ++rindex, ++oindex) {
                        output[oindex] = right[rindex];
                }
        }
        return 0;
}

// 检验接收到的数量和期待的数量是否一致
static inline void
mpi_recv_check(const MPI_Status *const status,
               MPI_Datatype datatype,
               const int count)
{
        int received = 0;

        if (NULL == status || count < 0) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
        MPI_Get_count(status, datatype, &received);
        if (received != count) {
                MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
        }
}