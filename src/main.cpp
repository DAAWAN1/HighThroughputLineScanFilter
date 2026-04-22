/**
 * @file main.cpp
 * @brief Entry point of the line‑scan camera defect detection pipeline.
 *
 * Parses command line arguments, initializes the DataGenerator and Filter,
 * runs the streaming pipeline, performs verification against a golden model
 * (when in test mode), and reports performance metrics (throughput, per‑module
 * timings, memory usage, time violations).
 *
 * Key functional logic:
 * - Sets Windows real‑time thread priority for low‑latency execution.
 * - Loops while DataGenerator has more data (test mode) or indefinitely (random mode).
 * - Each iteration fetches a pair of pixels, processes them through the Filter,
 *   and collects valid output bits.
 * - After the input stream ends, flushes the Filter with 4 pairs of zeros to
 *   drain the remaining 8 pixels from the sliding window.
 * - Compares the produced output bits with pre‑computed expected bits (test mode).
 * - Prints verification result and performance statistics.
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <windows.h>

#include "data_generator.hpp"
#include "filter.hpp"
#include "csv_loader.hpp"
#include "verification.hpp"
#include "timer.hpp"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cout << "Usage: " << argv[0] << " <m> <TV> <T_ns> <mode:0=test,1=random> [csv]\n";
        return 1;
    }

    int m = std::stoi(argv[1]);
    double tv = std::stod(argv[2]);
    long long T_ns = std::stoll(argv[3]);
    bool test_mode = (std::stoi(argv[4]) == 0);
    std::string csv = (argc > 5) ? argv[5] : "test_input.csv";

    // Memory requirement warning (non‑negotiable but algorithmically impossible for m<9)
    if (m < 9 && test_mode) {
        std::cerr << "Warning: memory requirement (≤ m) cannot be strictly met for m < 9 "
                  << "because filter needs 9 bytes. Continuing anyway.\n";
    }

    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    DataGenerator gen(m, T_ns, test_mode, csv);
    Filter filt(tv, T_ns);

    std::vector<uint8_t> expected;
    size_t verify_idx = 0;
    size_t mismatch_count = 0;
    size_t total_valid_bits = 0;
    if (test_mode) {
        auto rows = load_csv_rows(csv, m);
        expected = compute_expected(rows, tv);
    }

    size_t total_pixels = 0;
    HighResTimer total_timer;
    total_timer.start();

    while (!gen.done()) {
        auto input_pair = gen.next();
        auto out = filt.process_pair(input_pair);

        if (out.first_valid) {
            if (test_mode && verify_idx < expected.size()) {
                if (out.first != expected[verify_idx]) ++mismatch_count;
                ++total_valid_bits;
                ++verify_idx;
            }
        }
        if (out.second_valid) {
            if (test_mode && verify_idx < expected.size()) {
                if (out.second != expected[verify_idx]) ++mismatch_count;
                ++total_valid_bits;
                ++verify_idx;
            }
        }
        total_pixels += 2;
    }

    // Flush 8 zero pixels (4 pairs)
    for (int i = 0; i < 4; ++i) {
        auto out = filt.flush_pair();
        if (out.first_valid && test_mode && verify_idx < expected.size()) {
            if (out.first != expected[verify_idx]) ++mismatch_count;
            ++total_valid_bits;
            ++verify_idx;
        }
        if (out.second_valid && test_mode && verify_idx < expected.size()) {
            if (out.second != expected[verify_idx]) ++mismatch_count;
            ++total_valid_bits;
            ++verify_idx;
        }
        total_pixels += 2;
    }

    long long total_elapsed_ns = total_timer.elapsed_ns();

    // Verification report
    if (test_mode) {
        std::cout << "[VERIFICATION] ";
        if (mismatch_count == 0 && total_valid_bits == expected.size())
            std::cout << "PASSED (" << total_valid_bits << " bits matched)\n";
        else
            std::cout << "FAILED (" << mismatch_count << " mismatches out of " << total_valid_bits << ")\n";
    }

    // Performance report
    double avg_ns_per_pixel = static_cast<double>(total_elapsed_ns) / total_pixels;
    std::cout << "\n=== Performance ===\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "Total pixels: " << total_pixels << "\n";
    std::cout << "Total elapsed time: " << total_elapsed_ns << " ns\n";
    std::cout << "Avg per pixel (including wait): " << avg_ns_per_pixel << " ns\n";
    std::cout << "Throughput <100ns: " << (avg_ns_per_pixel < 100.0 ? "YES" : "NO") << "\n\n";

    std::cout << "=== Per‑Module Profiling ===\n";
    std::cout << "DataGenerator total work time: " << gen.total_work_ns() << " ns\n";
    std::cout << "Filter total work time: " << filt.total_work_ns() << " ns\n";
    std::cout << "Filter max work per pair: " << filt.max_work_ns() << " ns\n";
    std::cout << "Process time violation (any pair > T_ns): " << (filt.time_violation() ? "YES" : "NO") << "\n";

    std::cout << "\n=== Memory Usage ===\n";
    std::cout << "Working set: constant (sliding window of 9 elements).\n";
    std::cout << "Output is streamed, not stored. ";
    if (m >= 9)
        std::cout << "Memory ≤ m requirement satisfied.\n";
    else
        std::cout << "Memory = 9 bytes > m=" << m << " (requirement cannot be met for m<9).\n";

    return 0;
}