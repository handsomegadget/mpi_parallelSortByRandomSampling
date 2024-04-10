#ifndef TIMING_H
#define TIMING_H

#include "macro.h"
#include <chrono>
#include <time.h>

int timing_start(std::chrono::time_point<std::chrono::high_resolution_clock>& start) ;
int timing_reset(std::chrono::time_point<std::chrono::high_resolution_clock>& start) ;
int timing_stop(double& elapsed, const std::chrono::time_point<std::chrono::high_resolution_clock>& start);

#endif /* TIMING_H */
