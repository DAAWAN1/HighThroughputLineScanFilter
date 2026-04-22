/**
 * @file filter.hpp
 * @brief Declaration of the Filter class – 9‑tap FIR + threshold.
 *
 * Processes a stream of pixel pairs. Internally maintains a 9‑element sliding window.
 * After every two pixels, it outputs up to two decision bits (valid after at least 9 pixels received).
 * Also tracks per‑pair execution time and detects violations of the T_ns budget.
 */
#ifndef FILTER_HPP
#define FILTER_HPP

#include <array>
#include <cstdint>

class Filter {
public:
    struct OutputPair {
        uint8_t first;
        uint8_t second;
        bool first_valid;
        bool second_valid;
    };

    Filter(double threshold, long long T_ns);
    OutputPair process_pair(std::array<uint8_t, 2> input_pair);
    OutputPair flush_pair();          // feed zeros to drain remaining window
    size_t valid_output_count() const;
    bool time_violation() const;
    long long total_work_ns() const;
    long long max_work_ns() const;

private:
    double threshold_;
    std::array<uint8_t, 9> window_{};
    size_t received_ = 0;
    size_t valid_output_count_ = 0;
    long long total_filter_work_ns_ = 0;
    long long max_filter_ns_ = 0;
    long long T_ns_;
    bool time_violation_ = false;

    void push_single(uint8_t val);
    uint8_t next_bit_single();
};

#endif