/* tests/para_test.hpp — a small test framework built for concurrency bugs.
 *
 * Three things set it apart from "assert in a main":
 *
 *  1. EVERY TEST RUNS IN ITS OWN PROCESS (fork), with a WATCHDOG. A test that
 *     does not finish within the time limit is reported as TIMEOUT and
 *     killed. That is what a deadlock looks like, and it is the whole reason
 *     for the design: a hung test in an ordinary test binary hangs the whole
 *     suite and CI, and you have no idea which test it was. Here you get the
 *     test's name, its file, and the other tests keep running.
 *
 *  2. A CRASH IS A RESULT, NOT THE END. SIGSEGV/SIGABRT in a test is caught
 *     by the parent and reported with the signal name. A sanitizer's abort
 *     looks the same, which is exactly what we want — and so does
 *     para::not_built(), which is what a stub should do.
 *
 *  3. RACE TESTS ARE TAGGED. PARA_TEST_RACE registers a test that is worth
 *     running MANY times — `make stress` runs only those, 200 rounds, under
 *     ThreadSanitizer. A concurrency test that ran once and passed has not
 *     proven anything; it just did not happen to fail.
 *
 * Usage — the macro takes ONE argument, the test's name, which must be a
 * valid C++ identifier (it becomes a function name):
 *
 *     PARA_TEST(my_thing_works) {
 *         para::Mutex m;
 *         std::lock_guard g{m};
 *         PARA_ASSERT(true);
 *     }
 *
 * The assertions are PARA_ASSERT, PARA_ASSERT_EQ, PARA_ASSERT_STATUS,
 * PARA_ASSERT_OK, PARA_ASSERT_ERR, PARA_UNWRAP and PARA_ASSERT_NOT_BUILT —
 * nothing else. (Googletest-style `TEST(Suite, Name)` and `CHECK_EQ` do not
 * exist here.)
 *
 * ── What changed from the C version ───────────────────────────────────────
 *
 * Registration used `__attribute__((constructor))`, a GCC extension. Here it
 * is an object with static storage duration whose constructor registers the
 * test — same effect, and it is standard C++ instead of an extension.
 *
 * And PARA_ASSERT_NOT_BUILT now takes a MODULE, not an expression. See
 * src/core/modules.cpp: the build plan lives in one place, and the test suite
 * reads it instead of guessing from return codes.
 */
#ifndef PARACORE_TESTS_PARA_TEST_HPP
#define PARACORE_TESTS_PARA_TEST_HPP

#include <core/status.hpp>

#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace para::test {

using Fn = void (*)();

enum class Tag {
    Plain = 0,
    Race = 1 /* run repeatedly by `make stress` */
};

void register_test(const char *name, Fn fn, Tag tag, const char *file, int line) noexcept;

/* Called by the assert macros in the child process. Never returns. */
[[noreturn]] void fail(const char *file, int line, const char *fmt, ...) noexcept;

/* The object that registers. One per PARA_TEST, with static storage duration. */
struct Registrar {
    Registrar(const char *name, Fn fn, Tag tag, const char *file, int line) noexcept {
        register_test(name, fn, tag, file, line);
    }
};

} // namespace para::test

#define PARA_TEST__(name, tag)                                                                     \
    static void name();                                                                            \
    static const ::para::test::Registrar para_reg_##name{#name, name, tag, __FILE__, __LINE__};    \
    static void name()

#define PARA_TEST(name) PARA_TEST__(name, ::para::test::Tag::Plain)
#define PARA_TEST_RACE(name) PARA_TEST__(name, ::para::test::Tag::Race)

#define PARA_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            ::para::test::fail(__FILE__, __LINE__, "PARA_ASSERT(%s) failed", #cond);               \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_EQ(a, b)                                                                       \
    do {                                                                                           \
        const long long para_a_ = static_cast<long long>(a);                                       \
        const long long para_b_ = static_cast<long long>(b);                                       \
        if (para_a_ != para_b_) {                                                                  \
            ::para::test::fail(__FILE__, __LINE__, "%s == %s: %lld != %lld", #a, #b, para_a_,      \
                               para_b_);                                                           \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_STATUS(expr, expected)                                                         \
    do {                                                                                           \
        const ::para::Status para_st_ = (expr);                                                    \
        const ::para::Status para_ex_ = (expected);                                                \
        if (para_st_ != para_ex_) {                                                                \
            const auto got_ = ::para::to_string(para_st_);                                         \
            const auto want_ = ::para::to_string(para_ex_);                                        \
            ::para::test::fail(__FILE__, __LINE__, "%s gave %d (%.*s), expected %d (%.*s)", #expr, \
                               static_cast<int>(para_st_), static_cast<int>(got_.size()),          \
                               got_.data(), static_cast<int>(para_ex_),                            \
                               static_cast<int>(want_.size()), want_.data());                      \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_OK(expr) PARA_ASSERT_STATUS(expr, ::para::Status::Ok)

/* A Result<T> that must carry an error. */
#define PARA_ASSERT_ERR(expr, expected)                                                            \
    do {                                                                                           \
        auto para_r_ = (expr);                                                                     \
        if (para_r_.has_value()) {                                                                 \
            ::para::test::fail(__FILE__, __LINE__, "%s succeeded, expected an error", #expr);      \
        }                                                                                          \
        PARA_ASSERT_STATUS(para_r_.error(), expected);                                             \
    } while (0)

/* A Result<T> that must carry a value — hands the value back. */
#define PARA_UNWRAP(out, expr)                                                                     \
    auto para_res_##out = (expr);                                                                  \
    do {                                                                                           \
        if (!para_res_##out.has_value()) {                                                         \
            const auto e_ = ::para::to_string(para_res_##out.error());                             \
            ::para::test::fail(__FILE__, __LINE__, "%s failed: %.*s", #expr,                       \
                               static_cast<int>(e_.size()), e_.data());                            \
        }                                                                                          \
    } while (0);                                                                                   \
    auto &out = *para_res_##out

/* For the build plan: documents in the test suite that the module is not
 * built yet. When you flip the row in src/core/modules.cpp this test fails —
 * and THAT is the signal to go here and write a real test. `make progress`
 * reads the same table. */
#define PARA_ASSERT_NOT_BUILT(module)                                                              \
    do {                                                                                           \
        if (::para::is_built(module)) {                                                            \
            const auto n_ = ::para::module_name(module);                                           \
            ::para::test::fail(__FILE__, __LINE__,                                                 \
                               "MODULE %.*s is built now — delete this row and write real "        \
                               "tests for it",                                                     \
                               static_cast<int>(n_.size()), n_.data());                            \
        }                                                                                          \
    } while (0)

#endif /* PARACORE_TESTS_PARA_TEST_HPP */
