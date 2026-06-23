#ifndef TEST_PROCESS_HPP
#define TEST_PROCESS_HPP

#include <string>
#include <iostream>
#include <functional>

static int _tests_run    = 0;
static int _tests_passed = 0;
static int _tests_failed = 0;

#define ASSERT_EQ(expected, actual)                                             \
    do {                                                                        \
        _tests_run++;                                                           \
        if ((expected) == (actual)) {                                           \
            _tests_passed++;                                                    \
            std::cout << "  [OK] " << #actual << " == " << #expected << "\n";   \
        } else {                                                                \
            _tests_failed++;                                                    \
            std::cout << "  [FAIL] " << #actual                                 \
                      << "\n    esperado : " << (expected)                      \
                      << "\n    obtido   : " << (actual) << "\n";               \
            std::cout << "    em: " << __FILE__ << ":" << __LINE__ << "\n";     \
        }                                                                       \
    } while (0)

#define ASSERT_TRUE(expr)                                                       \
    do {                                                                        \
        _tests_run++;                                                           \
        if (expr) {                                                             \
            _tests_passed++;                                                    \
            std::cout << "  [OK] " << #expr << "\n";                            \
        } else {                                                                \
            _tests_failed++;                                                    \
            std::cout << "  [FAIL] " << #expr << " é falso\n";                  \
            std::cout << "    em: " << __FILE__ << ":" << __LINE__ << "\n";     \
        }                                                                       \
    } while (0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define RUN_TEST(name, fn)                                                      \
    do {                                                                        \
        std::cout << "\n[TEST] " << (name) << "\n";                             \
        fn();                                                                   \
    } while (0)

#define PRINT_RESULTS()                                                         \
    do {                                                                        \
        std::cout << "\n========================================\n";            \
        std::cout << "Resultados: " << _tests_passed << "/" << _tests_run       \
                  << " passaram";                                               \
        if (_tests_failed > 0)                                                  \
            std::cout << "  (" << _tests_failed << " falhas)";                  \
        std::cout << "\n========================================\n";            \
        return (_tests_failed == 0) ? 0 : 1;                                    \
    } while (0)

#endif // TEST_PROCESS_HPP