#include "para_test.hpp"

#include <bench/bench.hpp>

#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdarg>
#include <cstring>
#include <ctime>

/* ── the registry ──────────────────────────────────────────────────────────── */

namespace {

constexpr int kMaxTests = 512;
constexpr unsigned kDefaultTimeoutMs = 10000;

struct Entry {
    const char *name;
    ::para::test::Fn fn;
    ::para::test::Tag tag;
    const char *file;
    int line;
};

/* A raw array and not a std::vector, on purpose: registration happens during
 * static initialisation, and a std::vector growing then depends on its own
 * constructor having run already. That is the "static initialization order
 * fiasco", and a fixed array with constant initialisation has no constructor
 * to wait for. Same reason g_count is an int and not a std::atomic: all
 * registration happens before main, on one thread. */
Entry g_tests[kMaxTests];
int g_count = 0;

} // namespace

namespace para::test {

void register_test(const char *name, Fn fn, Tag tag, const char *file, int line) noexcept {
    if (g_count >= kMaxTests) {
        std::fprintf(stderr, "para_test: more than %d tests; raise kMaxTests\n", kMaxTests);
        _exit(2);
    }
    g_tests[g_count] = Entry{name, fn, tag, file, line};
    ++g_count;
}

void fail(const char *file, int line, const char *fmt, ...) noexcept {
    std::va_list ap;
    std::fprintf(stderr, "        %s:%d: ", file, line);
    va_start(ap, fmt);
    std::vfprintf(stderr, fmt, ap);
    va_end(ap);
    std::fputc('\n', stderr);
    std::fflush(stderr);
    _exit(1);
}

} // namespace para::test

/* ── running ────────────────────────────────────────────────────────────── */

namespace {

using para::test::Tag;

enum class Outcome { Pass, Fail, Timeout, Signal };

struct RunResult {
    Outcome outcome{Outcome::Pass};
    int sig{0};
    double ms{0.0};
};

RunResult run_forked(const para::test::Fn fn, unsigned timeout_ms) noexcept {
    std::fflush(nullptr);
    RunResult res;
    const std::uint64_t t0 = para::bench::now_ns();
    const pid_t pid = fork();
    if (pid < 0) {
        std::fprintf(stderr, "fork: %s\n", std::strerror(errno));
        _exit(2);
    }
    if (pid == 0) {
        fn();
        _exit(0);
    }

    /* The parent polls. One-second resolution would have been simpler, but
     * then we would not measure the test's time — and the test time is how
     * you see that a test is turning into a deadlock before it becomes one. */
    constexpr long kPollNs = 500 * 1000;
    const std::uint64_t deadline = t0 + static_cast<std::uint64_t>(timeout_ms) * 1000000ULL;
    int status = 0;
    for (;;) {
        const pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            break;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::fprintf(stderr, "waitpid: %s\n", std::strerror(errno));
            _exit(2);
        }
        if (para::bench::now_ns() >= deadline) {
            kill(pid, SIGKILL);
            (void)waitpid(pid, &status, 0);
            res.ms = static_cast<double>(para::bench::now_ns() - t0) / 1e6;
            res.outcome = Outcome::Timeout;
            return res;
        }
        const timespec ts{0, kPollNs};
        nanosleep(&ts, nullptr);
    }

    res.ms = static_cast<double>(para::bench::now_ns() - t0) / 1e6;
    if (WIFSIGNALED(status)) {
        res.sig = WTERMSIG(status);
        res.outcome = Outcome::Signal;
        return res;
    }
    res.outcome = (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? Outcome::Pass : Outcome::Fail;
    return res;
}

void usage(const char *argv0) noexcept {
    std::printf("usage: %s [options] [filter]\n"
                "  --list             list the tests and exit\n"
                "  --repeat N         run every test N times (default 1)\n"
                "  --timeout MS       watchdog per test (default %u)\n"
                "  --race-only        run only PARA_TEST_RACE-tagged tests\n"
                "  --no-fork          run in-process (for valgrind without trace-children)\n"
                "  filter             substring the test name must contain\n",
                argv0, kDefaultTimeoutMs);
}

} // namespace

int main(int argc, char **argv) {
    unsigned repeat = 1;
    unsigned timeout_ms = kDefaultTimeoutMs;
    bool race_only = false;
    bool no_fork = false;
    const char *filter = nullptr;

    for (int i = 1; i < argc; ++i) {
        const std::string_view a{argv[i]};
        if (a == "--list") {
            for (int t = 0; t < g_count; ++t) {
                std::printf("%s%s\n", g_tests[t].name,
                            g_tests[t].tag == Tag::Race ? "  [race]" : "");
            }
            return 0;
        } else if (a == "--repeat" && i + 1 < argc) {
            repeat = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 10));
        } else if (a == "--timeout" && i + 1 < argc) {
            timeout_ms = static_cast<unsigned>(std::strtoul(argv[++i], nullptr, 10));
        } else if (a == "--race-only") {
            race_only = true;
        } else if (a == "--no-fork") {
            no_fork = true;
        } else if (a == "--help" || a == "-h") {
            usage(argv[0]);
            return 0;
        } else {
            filter = argv[i];
        }
    }

    if (const char *env = std::getenv("PARA_TEST_TIMEOUT_MS"); env != nullptr) {
        timeout_ms = static_cast<unsigned>(std::strtoul(env, nullptr, 10));
    }

    int passed = 0;
    int failed = 0;
    int skipped = 0;
    std::printf("paracore: %d registered tests, watchdog %u ms, %u rounds\n", g_count, timeout_ms,
                repeat);

    for (int t = 0; t < g_count; ++t) {
        const Entry &e = g_tests[t];
        if (race_only && e.tag != Tag::Race) {
            ++skipped;
            continue;
        }
        if (filter != nullptr && std::string_view{e.name}.find(filter) == std::string_view::npos) {
            ++skipped;
            continue;
        }
        for (unsigned r = 0; r < repeat; ++r) {
            RunResult res;
            if (no_fork) {
                const std::uint64_t t0 = para::bench::now_ns();
                e.fn();
                res.ms = static_cast<double>(para::bench::now_ns() - t0) / 1e6;
                res.outcome = Outcome::Pass;
            } else {
                res = run_forked(e.fn, timeout_ms);
            }
            switch (res.outcome) {
            case Outcome::Pass:
                if (r + 1 == repeat) {
                    std::printf("  ok       %-44s %7.1f ms\n", e.name, res.ms);
                    ++passed;
                }
                break;
            case Outcome::Fail:
                std::printf("  FAIL     %-44s %7.1f ms   (%s:%d)\n", e.name, res.ms, e.file,
                            e.line);
                ++failed;
                r = repeat;
                break;
            case Outcome::Timeout:
                std::printf("  TIMEOUT  %-44s %7.1f ms   POSSIBLE DEADLOCK — %s:%d\n", e.name,
                            res.ms, e.file, e.line);
                std::printf("           run: valgrind --tool=helgrind for the lock order\n");
                ++failed;
                r = repeat;
                break;
            case Outcome::Signal:
                std::printf("  SIGNAL   %-44s %7.1f ms   %s (%s:%d)\n", e.name, res.ms,
                            strsignal(res.sig), e.file, e.line);
                ++failed;
                r = repeat;
                break;
            }
        }
    }

    std::printf("\n%d ok, %d failed, %d skipped\n", passed, failed, skipped);
    if (passed == 0 && failed == 0) {
        std::printf("NO TESTS RAN — that is a failure, not a green result.\n");
        return 1;
    }
    return failed == 0 ? 0 : 1;
}
