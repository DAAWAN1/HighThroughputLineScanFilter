/**
 * @file test_utils.hpp
 * @brief Simple unit test macros for the evaluation test suite.
 *
 * Provides TEST_ASSERT, TEST_ASSERT_EQ, and RUN_TEST macros.
 * Used exclusively by tests/test.cpp – not part of the main pipeline.
 */
#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <string>
#include <cstdlib>

// Simple test assertion macros
#define TEST_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ << " - " << msg << std::endl; \
            std::exit(1); \
        } \
    } while (0)

#define TEST_ASSERT_EQ(a, b, msg) \
    TEST_ASSERT((a) == (b), msg)

#define TEST_ASSERT_NEAR(a, b, epsilon, msg) \
    TEST_ASSERT(((a) - (b)) <= (epsilon) && ((b) - (a)) <= (epsilon), msg)

// Test runner macro
#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        test_func(); \
        std::cout << "PASSED" << std::endl; \
    } while (0)

#endif // TEST_UTILS_HPP