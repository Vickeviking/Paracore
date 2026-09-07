#include "para_test.h"

#include <bench/bench.h>

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define PARA_MAX_TESTS 512
#define PARA_DEFAULT_TIMEOUT_MS 10000u

typedef struct {
    const char *name;
    para_test_fn fn;
    para_test_tag tag;
    const char *file;
    int line;
} entry;

static entry g_tests[PARA_MAX_TESTS];
static int g_count;

void para_test_register(const char *name, para_test_fn fn, para_test_tag tag, const char *file,
                        int line) {
    if (g_count >= PARA_MAX_TESTS) {
        fprintf(stderr, "para_test: fler än %d tester; höj PARA_MAX_TESTS\n", PARA_MAX_TESTS);
        _exit(2);
    }
    g_tests[g_count].name = name;
    g_tests[g_count].fn = fn;
    g_tests[g_count].tag = tag;
    g_tests[g_count].file = file;
    g_tests[g_count].line = line;
    g_count++;
}

void para_test_fail(const char *file, int line, const char *fmt, ...) {
    va_list ap;
    fprintf(stderr, "        %s:%d: ", file, line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    fflush(stderr);
    _exit(1);
}

/* ── körning ────────────────────────────────────────────────────────────── */

typedef enum { R_PASS, R_FAIL, R_TIMEOUT, R_SIGNAL } outcome;

static outcome run_forked(const entry *e, unsigned timeout_ms, int *sig_out, double *ms_out) {
    fflush(NULL);
    uint64_t t0 = para_now_ns();
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        _exit(2);
    }
    if (pid == 0) {
        e->fn();
        _exit(0);
    }

    /* Föräldern pollar. En sekunds upplösning hade varit enklare, men då
     * mäter vi inte testets tid — och testtiden är hur man ser att ett test
     * håller på att bli en deadlock innan det blir det. */
    const unsigned poll_us = 500;
    uint64_t deadline = t0 + (uint64_t)timeout_ms * 1000000ull;
    int status = 0;
    for (;;) {
        pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) {
            break;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "waitpid: %s\n", strerror(errno));
            _exit(2);
        }
        if (para_now_ns() >= deadline) {
            kill(pid, SIGKILL);
            (void)waitpid(pid, &status, 0);
            *ms_out = (double)(para_now_ns() - t0) / 1e6;
            return R_TIMEOUT;
        }
        struct timespec ts = {0, (long)poll_us * 1000L};
        nanosleep(&ts, NULL);
    }

    *ms_out = (double)(para_now_ns() - t0) / 1e6;
    if (WIFSIGNALED(status)) {
        *sig_out = WTERMSIG(status);
        return R_SIGNAL;
    }
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0) ? R_PASS : R_FAIL;
}

static void usage(const char *argv0) {
    printf("användning: %s [flaggor] [filter]\n"
           "  --list             lista testerna och sluta\n"
           "  --repeat N         kör varje test N gånger (default 1)\n"
           "  --timeout MS       watchdog per test (default %u)\n"
           "  --race-only        kör bara PARA_TEST_RACE-märkta tester\n"
           "  --no-fork          kör i processen (för valgrind utan trace-children)\n"
           "  filter             delsträng som testnamnet måste innehålla\n",
           argv0, PARA_DEFAULT_TIMEOUT_MS);
}

int main(int argc, char **argv) {
    unsigned repeat = 1;
    unsigned timeout_ms = PARA_DEFAULT_TIMEOUT_MS;
    int race_only = 0;
    int no_fork = 0;
    const char *filter = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--list") == 0) {
            for (int t = 0; t < g_count; t++) {
                printf("%s%s\n", g_tests[t].name,
                       g_tests[t].tag == PARA_TAG_RACE ? "  [race]" : "");
            }
            return 0;
        } else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
            repeat = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--timeout") == 0 && i + 1 < argc) {
            timeout_ms = (unsigned)strtoul(argv[++i], NULL, 10);
        } else if (strcmp(argv[i], "--race-only") == 0) {
            race_only = 1;
        } else if (strcmp(argv[i], "--no-fork") == 0) {
            no_fork = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            filter = argv[i];
        }
    }

    const char *env = getenv("PARA_TEST_TIMEOUT_MS");
    if (env != NULL) {
        timeout_ms = (unsigned)strtoul(env, NULL, 10);
    }

    int passed = 0, failed = 0, skipped = 0;
    printf("paracore: %d registrerade tester, watchdog %u ms, %u varv\n", g_count, timeout_ms,
           repeat);

    for (int t = 0; t < g_count; t++) {
        const entry *e = &g_tests[t];
        if (race_only && e->tag != PARA_TAG_RACE) {
            skipped++;
            continue;
        }
        if (filter != NULL && strstr(e->name, filter) == NULL) {
            skipped++;
            continue;
        }
        for (unsigned r = 0; r < repeat; r++) {
            double ms = 0.0;
            int sig = 0;
            outcome o;
            if (no_fork) {
                uint64_t t0 = para_now_ns();
                e->fn();
                ms = (double)(para_now_ns() - t0) / 1e6;
                o = R_PASS;
            } else {
                o = run_forked(e, timeout_ms, &sig, &ms);
            }
            switch (o) {
            case R_PASS:
                if (r + 1 == repeat) {
                    printf("  ok       %-44s %7.1f ms\n", e->name, ms);
                    passed++;
                }
                break;
            case R_FAIL:
                printf("  FEL      %-44s %7.1f ms   (%s:%d)\n", e->name, ms, e->file, e->line);
                failed++;
                r = repeat;
                break;
            case R_TIMEOUT:
                printf("  TIMEOUT  %-44s %7.1f ms   MÖJLIG DEADLOCK — %s:%d\n", e->name, ms,
                       e->file, e->line);
                printf("           kör: valgrind --tool=helgrind för låsordningen\n");
                failed++;
                r = repeat;
                break;
            case R_SIGNAL:
                printf("  SIGNAL   %-44s %7.1f ms   %s (%s:%d)\n", e->name, ms, strsignal(sig),
                       e->file, e->line);
                failed++;
                r = repeat;
                break;
            }
        }
    }

    printf("\n%d ok, %d fel, %d hoppade\n", passed, failed, skipped);
    if (passed == 0 && failed == 0) {
        printf("INGA TESTER KÖRDES — det är ett fel, inte ett grönt resultat.\n");
        return 1;
    }
    return failed == 0 ? 0 : 1;
}
