/**
 * @file data_generator.hpp
 * @brief Declaration of the DataGenerator class – pixel source with precise timing.
 *
 * Provides two modes:
 * - Test mode: reads from CSV file, row‑major, two pixels per call.
 * - Random mode: generates uniform random uint8 values.
 *
 * The next() method blocks (busy‑wait) until the next pair is due according to T_ns.
 * It returns a pair of pixels (std::array<uint8_t,2>) and updates internal indices.
 */
#ifndef DATA_GENERATOR_HPP
#define DATA_GENERATOR_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <random>
#include "timer.hpp"

class DataGenerator {
public:
    DataGenerator(int m, long long T_ns, bool test_mode, const std::string& csv_path);
    std::array<uint8_t, 2> next();
    bool done() const;
    long long total_work_ns() const;

private:
    int m_;
    long long T_ns_;
    bool test_mode_;
    std::vector<std::vector<uint8_t>> test_rows_;
    size_t row_idx_ = 0;
    size_t col_idx_ = 0;
    std::mt19937 rng_;
    HighResTimer timer_;
    long long next_pair_time_ = 0;
    long long total_gen_work_ns_ = 0;
};

#endif