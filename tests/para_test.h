/* tests/para_test.h — ett litet testramverk byggt för samtidighetsbuggar.
 *
 * Tre saker skiljer det från "assert i en main":
 *
 *  1. VARJE TEST KÖRS I EN EGEN PROCESS (fork), med en WATCHDOG. Ett test som
 *     inte blir klart inom tidsgränsen rapporteras som TIMEOUT och dödas.
 *     Det är så en deadlock ser ut, och det är hela skälet till konstruktionen:
 *     ett hängt test i en vanlig testbinär hänger hela sviten och CI, och du
 *     får ingen aning om vilket test det var. Här får du testets namn, dess
 *     fil, och de andra testerna kör vidare.
 *
 *  2. EN KRASCH ÄR ETT RESULTAT, INTE SLUTET. SIGSEGV/SIGABRT i ett test
 *     fångas av föräldern och rapporteras med signalnamn. Sanitizerns abort
 *     ser likadan ut, vilket är precis vad vi vill.
 *
 *  3. RACE-TESTER ÄR MÄRKTA. PARA_TEST_RACE registrerar ett test som är
 *     meningsfullt att köra MÅNGA gånger — `make stress` kör bara dem, 200 varv,
 *     under ThreadSanitizer. Ett samtidighetstest som kördes en gång och gick
 *     igenom har inte bevisat något; det har bara inte råkat misslyckas.
 *
 * Användning:
 *
 *     PARA_TEST(min_grej_funkar) {
 *         para_mutex *m = NULL;
 *         PARA_ASSERT_OK(para_mutex_init(&m));
 *         PARA_ASSERT(m != NULL);
 *         para_mutex_destroy(m);
 *     }
 */
#ifndef PARACORE_TESTS_PARA_TEST_H
#define PARACORE_TESTS_PARA_TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <core/status.h>

typedef void (*para_test_fn)(void);

typedef enum para_test_tag {
    PARA_TAG_PLAIN = 0,
    PARA_TAG_RACE = 1 /* körs upprepat av `make stress` */
} para_test_tag;

void para_test_register(const char *name, para_test_fn fn, para_test_tag tag, const char *file,
                        int line);

/* Anropas av assert-makrona i barnprocessen. Återvänder aldrig. */
void para_test_fail(const char *file, int line, const char *fmt, ...);

#define PARA_TEST__(name, tag)                                                                     \
    static void name(void);                                                                        \
    __attribute__((constructor)) static void para_reg_##name(void) {                               \
        para_test_register(#name, name, tag, __FILE__, __LINE__);                                  \
    }                                                                                              \
    static void name(void)

#define PARA_TEST(name) PARA_TEST__(name, PARA_TAG_PLAIN)
#define PARA_TEST_RACE(name) PARA_TEST__(name, PARA_TAG_RACE)

#define PARA_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            para_test_fail(__FILE__, __LINE__, "PARA_ASSERT(%s) föll", #cond);                     \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_EQ(a, b)                                                                       \
    do {                                                                                           \
        long long para_a_ = (long long)(a);                                                        \
        long long para_b_ = (long long)(b);                                                        \
        if (para_a_ != para_b_) {                                                                  \
            para_test_fail(__FILE__, __LINE__, "%s == %s: %lld != %lld", #a, #b, para_a_,          \
                           para_b_);                                                               \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_STATUS(expr, expected)                                                         \
    do {                                                                                           \
        para_status para_st_ = (expr);                                                             \
        para_status para_ex_ = (expected);                                                         \
        if (para_st_ != para_ex_) {                                                                \
            para_test_fail(__FILE__, __LINE__, "%s gav %d (%s), väntade %d (%s)", #expr,           \
                           (int)para_st_, para_strerror(para_st_), (int)para_ex_,                  \
                           para_strerror(para_ex_));                                               \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_OK(expr) PARA_ASSERT_STATUS(expr, PARA_OK)

/* För stubbarna: dokumenterar i testsviten att modulen ännu inte är byggd.
 * När du bygger den faller det här testet — och DET är signalen att gå hit
 * och skriva ett riktigt test. `make progress` räknar dem. */
#define PARA_ASSERT_NOT_BUILT_YET(expr) PARA_ASSERT_STATUS(expr, PARA_ERR_NOTIMPL)

#endif /* PARACORE_TESTS_PARA_TEST_H */
