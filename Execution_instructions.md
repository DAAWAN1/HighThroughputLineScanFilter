### 1. Prerequisites / Environment Setup

**Recommended Environment (as per problem statement):**
- **OS**: Windows 10 or 11 (64-bit)
- **IDE / Compiler**:**Visual Studio 2017, 2019, or 2022** (Community Edition is sufficient)**OR** MinGW-w64 with `g++` (for command-line builds)
- **C++ Standard**: **C++17** (required)
- **No external libraries** needed — only standard C++ + Windows SDK (`<windows.h>`, `QueryPerformanceCounter`)

**Important for High Performance (<100 ns throughput):**
- The code sets **REALTIME_PRIORITY_CLASS** and **THREAD_PRIORITY_TIME_CRITICAL**.
- Run the program as **Administrator** for best results.

---

### 2. Folder Structure

```
HighThroughputLineScanFilter/
├── README.md
├── Execution_instrcuctions.md
├── test_input.csv
├── src/
│   ├── main.cpp
│   ├── data_generator.cpp
│   ├── filter.cpp
│   ├── csv_loader.cpp
│   ├── verification.cpp
│   └── timer.cpp
├── include/
│   ├── data_generator.hpp
│   ├── filter.hpp
│   ├── filter_constants.hpp
│   ├── csv_loader.hpp
│   ├── verification.hpp
│   ├── timer.hpp
│   └── test_utils.hpp
├── tests/
|    └── test.cpp
├── test_csv/
│   ├── test_input.csv
│   ├── test_alternating.csv
│   ├── test_bright_defect.csv
│   └── test_dark_defect.csv
```

---

### 3. Step-by-Step Execution (Command Line – g++ / MinGW)

**Step 1:** Open **Command Prompt** or **PowerShell** and go to the solution folder.

**Step 2:** Compile with maximum optimization:
```bash
g++ -std=c++17 -O3 -march=native -Iinclude src/*.cpp -o main.exe
```

**Step 3:** Run with the provided test input:
```bash
# Using the provided test_input.csv (4 rows x 8 columns)
.\main.exe 8 120.0 100 0 test_csv\test_input.csv

# Using dark defect pattern
.\main.exe 10 180.0 100 0 test_csv\test_dark_defect.csv

# Using bright defect pattern
.\main.exe 10 180.0 100 0 test_csv\test_bright_defect.csv

# Using alternating pattern (0,255,0,255,...)
.\main.exe 16 128.0 100 0 test_csv\test_alternating.csv

# Random mode (no CSV needed) – runs indefinitely, press Ctrl+C to stop
.\main.exe 1024 100.0 500 1
```

**Expected Output:**
```
Warning: memory requirement (≤ m) cannot be strictly met for m < 9 because filter needs 9 bytes. Continuing anyway.
[VERIFICATION] PASSED (32 bits matched)

=== Performance ===
Total pixels: 40
Total elapsed time: 2200 ns
Avg per pixel (including wait): 55.0 ns
Throughput <100ns: YES

=== Per-Module Profiling ===
DataGenerator total work time: 200 ns
Filter total work time: 700 ns
Filter max work per pair: 100 ns
Process time violation (any pair > T_ns): NO
```

**Step 4:** Run the Unit Tests (highly recommended):
```bash
g++ -std=c++17 -O2 -Iinclude tests/test.cpp -o test.exe
.\test.exe
```
→ All tests should show **PASSED**.

### 4. Command Line Arguments Explained

```bash
main.exe <m> <TV> <T_ns> <mode> [csv_path]
```

| Argument   | Description                                      | Example          |
|------------|--------------------------------------------------|------------------|
| `m`        | Number of columns per row                        | `8`              |
| `TV`       | Threshold Value (double)                         | `120.0`          |
| `T_ns`     | Process time in nanoseconds (≥ 100)              | `100`            |
| `mode`     | `0` = Test mode (from CSV)<br>`1` = Random mode  | `0`              |
| `csv_path` | Optional – path to CSV file (only for mode=0)    | `test_input.csv` |

---

### 5. Special Notes

- **For m < 9**: You will see a warning (this is expected and correct — the filter needs 9 samples).
- **CSV Format**: Plain text, comma-separated values (0–255). Each line = one row.
- **Output**: After running, you get:
  - Verification result (PASSED / FAILED)
  - Full performance report
  - Per-module timing (DataGenerator + Filter)
  - Memory usage note