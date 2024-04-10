#include "../include/timing.h"


int timing_start(std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
    start = std::chrono::high_resolution_clock::now();
    return 0;
}

int timing_reset(std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
    start = std::chrono::time_point<std::chrono::high_resolution_clock>();
    return 0;
}

int timing_stop(double& elapsed, const std::chrono::time_point<std::chrono::high_resolution_clock>& start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    elapsed = static_cast<double>(duration) / 1e9; // Convert nanoseconds to seconds
    return 0;
}
