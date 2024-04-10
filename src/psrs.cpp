#include "../include/macro.h"
#define PSRS_PSRS_ONLY
#include "../include/psrs.h"
#undef PSRS_PSRS_ONLY

#include "../include/convert.h"
#include "../include/sort.h"

#include <errno.h>
#include <getopt.h>      /* getopt_long() */
#include <inttypes.h>    /* uintmax_t */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>      /* strdup() */


int main(int argc, char *argv[])
{
        /*
         *  only the root process calls the argument_parse
         * function and passes the parsed results to all the rest process(es).
         */
        MPI_Init(&argc, &argv);
        int rank;
        struct arguments arg = { };

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if (rank == 0) {
                if (0 > argument_parse(&arg, argc, argv)) {
                        MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
        }
        argument_bcast(&arg);
        start_sort(&arg);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        return EXIT_SUCCESS;
}

static int argument_parse(struct arguments *result, int argc, char *argv[])
{
        /* NOTE: All the flags followed by an extra colon require arguments. */
        static const char *const OPT_STR = "l:o";
        static const struct option OPTS[] = {
                {"length",   required_argument, NULL, 'l'},
                {"outphase",   no_argument, NULL, 'o'}};
        if (result == NULL || argc == 0 || argv == NULL) {
                errno = EINVAL;
                return -1;
        }

        result->outphase = false;
        int opt = 0;
        while (-1 != (opt = getopt_long(argc, argv, OPT_STR, OPTS, NULL))) {
                /*
                 * NOTE:
                 * 'sscanf' is an unsafe function since it can cause unwanted
                 * overflow and memory corruption, which has a negative impact
                 * on the robustness of the program.
                 *
                 * Reference:
                 * https://www.akkadia.org/drepper/defprogramming.pdf
                 */
                switch (opt) {
                /*
                 * 'case 0:' only triggered when any one of the flag
                 * field in the 'struct option' is set to non-NULL value;
                 * in this program this branch is never taken.
                 */
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

        if (0 != result->length % result->procnum) {
                fprintf(stderr,"Length is not divisible by the number of "
                           "process(es)");
                return -1;
        }

}

static void argument_bcast(struct arguments *arg)
{
        /*
         * Depends on how the processes are actually scheduled,
         * the first barrier is mainly waiting for root process.
         * */
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->length), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->procnum), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Bcast(&(arg->outphase), 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
}

