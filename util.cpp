#include "util.h"


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
