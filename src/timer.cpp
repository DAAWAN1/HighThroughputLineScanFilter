/**
 * @file timer.cpp
 * @brief High‑precision timer using Windows QueryPerformanceCounter.
 *
 * Provides nanosecond‑resolution timing by measuring the elapsed time
 * between construction/start and the elapsed_ns() call.
 * Used for both end‑to‑end throughput measurement and per‑module profiling.
 */
#include "timer.hpp"

HighResTimer::HighResTimer() {
    QueryPerformanceFrequency(&freq_);
}

void HighResTimer::start() {
    QueryPerformanceCounter(&start_);
}

long long HighResTimer::elapsed_ns() const {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (now.QuadPart - start_.QuadPart) * 1'000'000'000LL / freq_.QuadPart;
}