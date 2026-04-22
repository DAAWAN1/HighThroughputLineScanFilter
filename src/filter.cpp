/**
 * @file filter.cpp
 * @brief Implementation of the 9‑tap FIR filter + threshold decision.
 *
 * Maintains a 9‑element sliding window of uint8 pixel values.
 * For every input pixel (fed one by one via process_pair), the window is shifted
 * and the dot product with constant coefficients (filter_constants.hpp) is computed.
 * The result is compared against a threshold TV → output bit 1 (≥ TV) or 0 (< TV).
 *
 * The class processes two pixels per call to reduce overhead.
 * It also tracks execution time per pair and flags any violation of the T_ns budget.
 */
#include "filter.hpp"
#include "filter_constants.hpp"
#include "timer.hpp"

Filter::Filter(double threshold, long long T_ns)
    : threshold_(threshold), T_ns_(T_ns) {}

Filter::OutputPair Filter::process_pair(std::array<uint8_t, 2> input_pair) {
    HighResTimer work_timer;
    work_timer.start();

    // First pixel
    push_single(input_pair[0]);
    bool first_valid = (received_ >= 9);
    uint8_t b0 = first_valid ? next_bit_single() : 0;
    if (!first_valid) (void)next_bit_single();  // consume dummy increment

    // Second pixel
    push_single(input_pair[1]);
    bool second_valid = (received_ >= 9);
    uint8_t b1 = second_valid ? next_bit_single() : 0;
    if (!second_valid) (void)next_bit_single();

    long long elapsed = work_timer.elapsed_ns();
    total_filter_work_ns_ += elapsed;
    if (elapsed > max_filter_ns_) max_filter_ns_ = elapsed;
    if (elapsed > T_ns_) time_violation_ = true;

    return {b0, b1, first_valid, second_valid};
}

Filter::OutputPair Filter::flush_pair() {
    return process_pair({0, 0});
}

size_t Filter::valid_output_count() const { return valid_output_count_; }
bool Filter::time_violation() const { return time_violation_; }
long long Filter::total_work_ns() const { return total_filter_work_ns_; }
long long Filter::max_work_ns() const { return max_filter_ns_; }

void Filter::push_single(uint8_t val) {
    for (int i = 0; i < 8; ++i) window_[i] = window_[i + 1];
    window_[8] = val;
    ++received_;
}

uint8_t Filter::next_bit_single() {
    double sum = 0.0;
    for (int i = 0; i < 9; ++i)
        sum += static_cast<double>(window_[i]) * FILTER_COEFF[i];
    ++valid_output_count_;
    return (sum >= threshold_) ? 1 : 0;
}