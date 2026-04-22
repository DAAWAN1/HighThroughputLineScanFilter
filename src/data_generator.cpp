/**
 * @file data_generator.cpp
 * @brief Implementation of the DataGenerator class – emulates a line‑scan camera.
 *
 * Provides pixel pairs at a fixed time interval T (nanoseconds) using either:
 * - Test mode: reads pixel data from a CSV file (row‑major order, two columns at a time)
 * - Random mode: generates random uint8 values using a Mersenne Twister (seed = 42)
 *
 * Timing is enforced by a busy‑wait spinloop with _mm_pause() to minimise jitter.
 * The total work time (excluding waiting) is accumulated for profiling.
 */
#include "data_generator.hpp"
#include "csv_loader.hpp"
#include "timer.hpp"
#include <random>
#include <immintrin.h>

DataGenerator::DataGenerator(int m, long long T_ns, bool test_mode, const std::string& csv_path)
    : m_(m), T_ns_(T_ns), test_mode_(test_mode), rng_(42) {
    if (test_mode_) test_rows_ = load_csv_rows(csv_path, m_);
    timer_.start();
}

std::array<uint8_t, 2> DataGenerator::next() {
    // Wait until the next pair is due (busy‑wait with pause instruction)
    while (timer_.elapsed_ns() < next_pair_time_) _mm_pause();

    HighResTimer work_timer;
    work_timer.start();

    std::array<uint8_t, 2> pair{0, 0};
    if (test_mode_) {
        if (row_idx_ < test_rows_.size()) {
            const auto& row = test_rows_[row_idx_];
            if (col_idx_ < row.size()) {
                pair[0] = row[col_idx_];
                if (col_idx_ + 1 < row.size()) {
                    pair[1] = row[col_idx_ + 1];
                    col_idx_ += 2;
                } else {
                    pair[1] = 0;          // odd column → zero padding
                    col_idx_ += 1;
                }
                if (col_idx_ >= row.size()) {
                    ++row_idx_;
                    col_idx_ = 0;
                }
            }
        }
    } else {
        pair[0] = static_cast<uint8_t>(rng_() & 0xFF);
        pair[1] = static_cast<uint8_t>(rng_() & 0xFF);
    }

    total_gen_work_ns_ += work_timer.elapsed_ns();
    next_pair_time_ += T_ns_;
    return pair;
}

bool DataGenerator::done() const {
    return test_mode_ && row_idx_ >= test_rows_.size();
}

long long DataGenerator::total_work_ns() const {
    return total_gen_work_ns_;
}