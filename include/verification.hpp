/**
 * @file verification.hpp
 * @brief Declaration of the golden reference output computation.
 *
 * compute_expected() takes a 2D image (vector of rows) and a threshold,
 * and returns the exact expected output bitstream after applying the
 * 9‑tap FIR filter + threshold. Used for validation in test mode.
 */
#ifndef VERIFICATION_HPP
#define VERIFICATION_HPP

#include <vector>
#include <cstdint>

std::vector<uint8_t> compute_expected(const std::vector<std::vector<uint8_t>>& rows, double threshold);

#endif