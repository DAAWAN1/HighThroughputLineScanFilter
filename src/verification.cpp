/**
 * @file verification.cpp
 * @brief Golden reference implementation of the filter + threshold.
 *
 * Computes the expected output bits for a given 2D image (vector of rows) and threshold.
 * The algorithm follows the same logic as the production Filter class:
 * - Slide a 9‑element window over the flattened pixel stream.
 * - Pad the end with 8 zeros.
 * - For each window position compute dot product with FILTER_COEFF and compare to TV.
 * Used only in test mode to validate the pipeline.
 */
#include "verification.hpp"
#include "filter_constants.hpp"
#include <array>

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