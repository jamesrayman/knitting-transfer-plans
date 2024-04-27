#include "util.h"
#include <bit>


StopWatch::StopWatch() {
    start();
}
void StopWatch::start() {
    start_time = std::chrono::steady_clock::now();
}
double StopWatch::stop() {
    return std::chrono::duration<double, std::ratio<1>>(
        std::chrono::steady_clock::now() - start_time
    ).count();
}

unsigned int log_offsets(unsigned long long offsets) {
    int n = std::popcount(offsets);

    // calculate x = floor(log_2(n+1))
    n++;
    int x = 0;
    while (n > 1) {
        x++;
        n >>= 1;
    }
    return x;
}
