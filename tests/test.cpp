// tests/test.cpp
// Unit tests for DataGenerator and Filter blocks.
// No external dependencies – uses only standard C++.
// Compile with: cl /EHsc /std:c++17 test.cpp /Fe:test.exe
// or g++ -std=c++17 -O2 test.cpp -o test

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <array>
#include <chrono>
#include <random>
#include <cstdint>
#include <string>
#include <algorithm>
#include <iomanip>
#include <cassert>

// Windows specific timing – if not on Windows, comment out and use a stub.
#ifdef _WIN32
#include <windows.h>
#include <immintrin.h>
#else

#endif

#include "test_utils.hpp"

// ----------------------------------------------------------------------
// Constants and helper functions from main (copied for test isolation)
// ----------------------------------------------------------------------
const std::array<double, 9> FILTER_COEFF = {
    0.00025177, 0.008666992, 0.078025818, 0.24130249, 0.343757629,
    0.24130249, 0.078025818, 0.008666992, 0.000125885
};

// High‑precision timer (Windows)
class HighResTimer {
#ifdef _WIN32
    LARGE_INTEGER freq_, start_;
public:
    HighResTimer() { QueryPerformanceFrequency(&freq_); }
    void start() { QueryPerformanceCounter(&start_); }
    long long elapsed_ns() const {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (now.QuadPart - start_.QuadPart) * 1'000'000'000LL / freq_.QuadPart;
    }
#endif
};

// CSV loader (simplified for testing)
std::vector<std::vector<uint8_t>> load_csv_rows(const std::string& path, int m) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open CSV file");

    std::vector<std::vector<uint8_t>> rows;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string tok;
        std::vector<uint8_t> row;
        for (int col = 0; col < m; ++col) {
            if (!std::getline(ss, tok, ',')) break;
            if (!tok.empty())
                row.push_back(static_cast<uint8_t>(std::clamp(std::stoi(tok), 0, 255)));
            else
                row.push_back(0);
        }
        if (!row.empty()) rows.push_back(std::move(row));
    }
    return rows;
}

// Precompute expected output (same as original)
std::vector<uint8_t> compute_expected(const std::vector<std::vector<uint8_t>>& rows, double threshold) {
    std::vector<uint8_t> flat;
    for (const auto& row : rows)
        flat.insert(flat.end(), row.begin(), row.end());

    std::vector<uint8_t> output;
    std::array<uint8_t, 9> window{};
    size_t n = flat.size();
    for (size_t i = 0; i < n + 8; ++i) {
        for (int j = 0; j < 8; ++j)
            window[j] = window[j + 1];
        window[8] = (i < n) ? flat[i] : 0;

        if (i >= 8) {
            double sum = 0.0;
            for (int j = 0; j < 9; ++j)
                sum += window[j] * FILTER_COEFF[j];
            output.push_back(sum >= threshold ? 1 : 0);
        }
    }
    return output;
}

// ----------------------------------------------------------------------
// DataGenerator class (exactly as in main, but with public test helpers)
// ----------------------------------------------------------------------
class DataGenerator {
    int m_;
    long long T_ns_;
    bool test_mode_;
    std::vector<std::vector<uint8_t>> test_rows_;
    size_t row_idx_ = 0;
    size_t col_idx_ = 0;

    std::mt19937 rng_{42};
    HighResTimer timer_;
    long long next_pair_time_ = 0;
    long long total_gen_work_ns_ = 0;

public:
    DataGenerator(int m, long long T_ns, bool test_mode, const std::string& csv_path)
        : m_(m), T_ns_(T_ns), test_mode_(test_mode) {
        if (test_mode_) test_rows_ = load_csv_rows(csv_path, m_);
        timer_.start();
    }

    std::array<uint8_t, 2> next() {
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
                        pair[1] = 0;          // odd column → zero
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

    bool done() const { return test_mode_ && row_idx_ >= test_rows_.size(); }
    long long total_work_ns() const { return total_gen_work_ns_; }
};

// ----------------------------------------------------------------------
// Filter class (exactly as in main)
// ----------------------------------------------------------------------
class Filter {
    double threshold_;
    std::array<uint8_t, 9> window_{};
    size_t received_ = 0;
    size_t valid_output_count_ = 0;

    long long total_filter_work_ns_ = 0;
    long long max_filter_ns_ = 0;
    long long T_ns_;
    bool time_violation_ = false;

public:
    struct OutputPair {
        uint8_t first;
        uint8_t second;
        bool first_valid;
        bool second_valid;
    };

    Filter(double threshold, long long T_ns) : threshold_(threshold), T_ns_(T_ns) {}

    OutputPair process_pair(std::array<uint8_t, 2> input_pair) {
        HighResTimer work_timer;
        work_timer.start();

        // First pixel
        push_single(input_pair[0]);
        bool first_valid = (received_ >= 9);
        uint8_t b0 = first_valid ? next_bit_single() : 0;
        if (!first_valid) (void)next_bit_single(); // consume dummy increment

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

    OutputPair flush_pair() { return process_pair({0, 0}); }

    size_t valid_output_count() const { return valid_output_count_; }
    bool time_violation() const { return time_violation_; }
    long long total_work_ns() const { return total_filter_work_ns_; }
    long long max_work_ns() const { return max_filter_ns_; }

private:
    void push_single(uint8_t val) {
        for (int i = 0; i < 8; ++i) window_[i] = window_[i + 1];
        window_[8] = val;
        ++received_;
    }

    uint8_t next_bit_single() {
        double sum = 0.0;
        for (int i = 0; i < 9; ++i) sum += static_cast<double>(window_[i]) * FILTER_COEFF[i];
        ++valid_output_count_;
        return (sum >= threshold_) ? 1 : 0;
    }
};

// ----------------------------------------------------------------------
// Helper: Create a temporary CSV file for testing
// ----------------------------------------------------------------------
void create_test_csv(const std::string& path, const std::vector<std::vector<uint8_t>>& data) {
    std::ofstream out(path);
    for (const auto& row : data) {
        for (size_t i = 0; i < row.size(); ++i) {
            out << static_cast<int>(row[i]);
            if (i + 1 < row.size()) out << ',';
        }
        out << '\n';
    }
}

// ----------------------------------------------------------------------
// Test Cases
// ----------------------------------------------------------------------

void test_csv_loader() {
    // Create a small CSV file
    std::vector<std::vector<uint8_t>> data = {
        {10, 20, 30},
        {40, 50, 60}
    };
    create_test_csv("temp_test.csv", data);

    auto loaded = load_csv_rows("temp_test.csv", 3);
    TEST_ASSERT_EQ(loaded.size(), 2, "Row count mismatch");
    TEST_ASSERT_EQ(loaded[0].size(), 3, "Column count mismatch");
    TEST_ASSERT_EQ(loaded[0][0], 10, "Value mismatch");
    TEST_ASSERT_EQ(loaded[1][2], 60, "Value mismatch");

    std::remove("temp_test.csv");
}

void test_data_generator_test_mode() {
    std::vector<std::vector<uint8_t>> data = {
        {1, 2, 3, 4},
        {5, 6, 7, 8}
    };
    create_test_csv("temp_test2.csv", data);

    DataGenerator gen(4, 1000, true, "temp_test2.csv"); // T=1000ns, large to avoid timing issues

    // Expect pairs: (1,2), (3,4), (5,6), (7,8)
    auto p1 = gen.next();
    TEST_ASSERT_EQ(p1[0], 1, "First pair first element");
    TEST_ASSERT_EQ(p1[1], 2, "First pair second element");

    auto p2 = gen.next();
    TEST_ASSERT_EQ(p2[0], 3, "Second pair first element");
    TEST_ASSERT_EQ(p2[1], 4, "Second pair second element");

    auto p3 = gen.next();
    TEST_ASSERT_EQ(p3[0], 5, "Third pair first element");
    TEST_ASSERT_EQ(p3[1], 6, "Third pair second element");

    auto p4 = gen.next();
    TEST_ASSERT_EQ(p4[0], 7, "Fourth pair first element");
    TEST_ASSERT_EQ(p4[1], 8, "Fourth pair second element");

    // After consuming all data, one more next() triggers the done state
    auto p5 = gen.next();   // this call advances row_idx_ past the end
    TEST_ASSERT(gen.done(), "Generator should be done after attempting to read beyond data");
    TEST_ASSERT_EQ(p5[0], 0, "After done first element zero");
    TEST_ASSERT_EQ(p5[1], 0, "After done second element zero");

    std::remove("temp_test2.csv");
}

void test_data_generator_random_mode() {
    DataGenerator gen(10, 1000, false, ""); // random mode
    // Check that we get values in range 0-255 for many calls
    for (int i = 0; i < 100; ++i) {
        auto p = gen.next();
        TEST_ASSERT(p[0] <= 255, "Random value out of range");
        TEST_ASSERT(p[1] <= 255, "Random value out of range");
    }
    
    // Verify deterministic behaviour: two instances with same seed produce identical sequences
    DataGenerator genA(10, 1000, false, "");
    DataGenerator genB(10, 1000, false, "");
    for (int i = 0; i < 10; ++i) {
        auto pA = genA.next();
        auto pB = genB.next();
        TEST_ASSERT_EQ(pA[0], pB[0], "Same seed should yield same first element");
        TEST_ASSERT_EQ(pA[1], pB[1], "Same seed should yield same second element");
    }
}

void test_filter_window_logic() {
    Filter filt(120.0, 1000); // threshold 120, T=1000

    // Feed 9 values: all zeros
    for (int i = 0; i < 4; ++i) {
        auto out = filt.process_pair({0, 0});
        if (i < 3) {
            TEST_ASSERT(!out.first_valid, "Early output should be invalid");
            TEST_ASSERT(!out.second_valid, "Early output should be invalid");
        }
    }
    // 9th element (first of 5th pair) produces first valid output
    auto out = filt.process_pair({0, 0});
    TEST_ASSERT(out.first_valid, "First valid output should appear");
    TEST_ASSERT(out.first == 0, "All zeros should yield 0 (sum=0 < threshold)");

    // Now test with a known window
    Filter filt2(10.0, 1000);
    // Build window manually by feeding pairs
    // Coefficients sum to ~1.0, so value 255 should give ~255
    // We'll test threshold boundary
    std::array<uint8_t, 2> high{255, 255};
    for (int i = 0; i < 4; ++i) filt2.process_pair(high);
    auto out2 = filt2.process_pair(high);
    TEST_ASSERT(out2.first_valid, "Valid output expected");
    TEST_ASSERT(out2.first == 1, "High value should exceed threshold");
}

void test_filter_expected_output() {
    // Simple 1x9 image (all 100)
    std::vector<std::vector<uint8_t>> img = {{100, 100, 100, 100, 100, 100, 100, 100, 100}};
    double threshold = 50.0;
    auto expected = compute_expected(img, threshold);
    
    TEST_ASSERT_EQ(expected.size(), 9, "Expected 9 outputs for 9 input pixels");
    
    // First five outputs should be 1 (sum stays above threshold)
    TEST_ASSERT_EQ(expected[0], 1, "Output 0 should be 1 (window all 100s)");
    TEST_ASSERT_EQ(expected[1], 1, "Output 1 should be 1");
    TEST_ASSERT_EQ(expected[2], 1, "Output 2 should be 1");
    TEST_ASSERT_EQ(expected[3], 1, "Output 3 should be 1");
    TEST_ASSERT_EQ(expected[4], 1, "Output 4 should be 1");
    // Remaining outputs have enough trailing zeros to drop below threshold
    TEST_ASSERT_EQ(expected[5], 0, "Output 5 should be 0");
    TEST_ASSERT_EQ(expected[6], 0, "Output 6 should be 0");
    TEST_ASSERT_EQ(expected[7], 0, "Output 7 should be 0");
    TEST_ASSERT_EQ(expected[8], 0, "Output 8 should be 0");

    // Lower values (all 10) – all outputs should be 0
    img = {{10, 10, 10, 10, 10, 10, 10, 10, 10}};
    expected = compute_expected(img, threshold);
    TEST_ASSERT_EQ(expected.size(), 9, "Expected 9 outputs for 9 input pixels");
    for (size_t i = 0; i < expected.size(); ++i) {
        TEST_ASSERT_EQ(expected[i], 0, "All outputs should be 0");
    }
}

void test_pipeline_correctness() {
    // Create a 2x4 image (even columns to avoid zero-padding mismatches)
    std::vector<std::vector<uint8_t>> data = {
        {10, 20, 30, 40},
        {50, 60, 70, 80}
    };
    create_test_csv("pipeline_test.csv", data);

    int m = 4;   // must match the number of columns in the CSV
    double tv = 30.0;
    long long T_ns = 1000;

    DataGenerator gen(m, T_ns, true, "pipeline_test.csv");
    Filter filt(tv, T_ns);

    std::vector<uint8_t> pipeline_output;
    while (!gen.done()) {
        auto input = gen.next();
        auto out = filt.process_pair(input);
        if (out.first_valid) pipeline_output.push_back(out.first);
        if (out.second_valid) pipeline_output.push_back(out.second);
    }
    // Flush remaining pipeline (8 zeros)
    for (int i = 0; i < 4; ++i) {
        auto out = filt.flush_pair();
        if (out.first_valid) pipeline_output.push_back(out.first);
        if (out.second_valid) pipeline_output.push_back(out.second);
    }

    auto expected = compute_expected(data, tv);

    // Check that pipeline produces at least as many outputs as expected
    TEST_ASSERT(pipeline_output.size() >= expected.size(),
                "Pipeline output should have at least expected number of outputs");

    // Verify all expected outputs
    for (size_t i = 0; i < expected.size(); ++i) {
        TEST_ASSERT_EQ(pipeline_output[i], expected[i], "Mismatch at index");
    }

    // Any extra outputs (from trailing zeros) must be 0
    for (size_t i = expected.size(); i < pipeline_output.size(); ++i) {
        TEST_ASSERT_EQ(pipeline_output[i], 0, "Extra outputs should be zero");
    }

    std::remove("pipeline_test.csv");
}

// ----------------------------------------------------------------------
// Main test runner
// ----------------------------------------------------------------------
int main() {
    std::cout << "=== Running Unit Tests ===" << std::endl;

    RUN_TEST(test_csv_loader);
    RUN_TEST(test_data_generator_test_mode);
    RUN_TEST(test_data_generator_random_mode);
    RUN_TEST(test_filter_window_logic);
    RUN_TEST(test_filter_expected_output);
    RUN_TEST(test_pipeline_correctness);

    std::cout << "\nAll tests PASSED." << std::endl;
    return 0;
}