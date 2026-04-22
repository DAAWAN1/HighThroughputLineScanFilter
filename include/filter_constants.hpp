/**
 * @file filter_constants.hpp
 * @brief Constant FIR filter coefficients (9‑tap symmetric kernel).
 *
 * The coefficients correspond to a low‑pass / smoothing filter.
 * The sum of all coefficients is approximately 1.0.
 * Index order: [K-4, K-3, K-2, K-1, K, K+1, K+2, K+3, K+4].
 */
#ifndef FILTER_CONSTANTS_HPP
#define FILTER_CONSTANTS_HPP

#include <array>

const std::array<double, 9> FILTER_COEFF = {
    0.00025177, 0.008666992, 0.078025818, 0.24130249, 0.343757629,
    0.24130249, 0.078025818, 0.008666992, 0.000125885
};

#endif