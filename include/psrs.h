#ifndef PSRS_H
#define PSRS_H

#include "macro.h"

#include <mpi.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Ensure all the members are of builtin types so MPI can transmit them
 * easily without worrying about custom defined types.
 */
struct arguments {
        int length;
        int procnum;
        bool outphase;
};


static int argument_parse(struct arguments *result, int argc, char *argv[]);
static void argument_bcast(struct arguments *arg);



#endif /* PSRS_H */
