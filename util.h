#include <cstddef>
#include <chrono>

#ifndef UTIL_H
#define UTIL_H

inline std::size_t hash_combine(std::size_t x, std::size_t y) {
    return x ^ (y + 0x5e7a3ddcc8414e72 + (x << 12) + (x >> 3));
}

class NotImplemented { };

class StopWatch {
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time;

public:
    StopWatch();
    void start();
    double stop();
};

unsigned int log_offsets(unsigned long long);

#endif
