/**
 * @file csv_loader.cpp
 * @brief Simple CSV parser for loading pixel data from a file.
 *
 * Reads a comma‑separated file where each line represents one row of the 2D image.
 * Values are clamped to [0, 255] and stored as uint8_t.
 * Empty lines are skipped.
 */
#include "csv_loader.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

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
        row.reserve(m);
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