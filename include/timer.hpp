/**
 * @file timer.hpp
 * @brief Declaration of a high‑resolution timer using Windows QPC.
 *
 * Provides start() and elapsed_ns() methods.
 * The constructor automatically obtains the performance counter frequency.
 */
#ifndef TIMER_HPP
#define TIMER_HPP

#include <windows.h>

class HighResTimer {
    LARGE_INTEGER freq_, start_;
public:
    HighResTimer();
    void start();
    long long elapsed_ns() const;
};

#endif // TIMER_HPP