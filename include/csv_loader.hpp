/**
 * @file csv_loader.hpp
 * @brief Declaration of the CSV loader function.
 *
 * load_csv_rows() reads a comma‑separated file where each line is one row.
 * It expects exactly 'm' columns per row (or fewer; missing values become 0).
 * Returns a vector of rows, each row a vector of uint8_t pixels.
 */
#ifndef CSV_LOADER_HPP
#define CSV_LOADER_HPP

#include <vector>
#include <cstdint>
#include <string>

std::vector<std::vector<uint8_t>> load_csv_rows(const std::string& path, int m);

#endif