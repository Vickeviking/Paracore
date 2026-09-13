/* tests/para_test.hpp — ett litet testramverk byggt för samtidighetsbuggar.
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
 *     ser likadan ut, vilket är precis vad vi vill — och para::not_built()
 *     också, vilket är vad en stub ska göra.
 *
 *  3. RACE-TESTER ÄR MÄRKTA. PARA_TEST_RACE registrerar ett test som är
 *     meningsfullt att köra MÅNGA gånger — `make stress` kör bara dem, 200
 *     varv, under ThreadSanitizer. Ett samtidighetstest som kördes en gång och
 *     gick igenom har inte bevisat något; det har bara inte råkat misslyckas.
 *
 * Användning:
 *
 *     PARA_TEST(min_grej_funkar) {
 *         para::Mutex m;
 *         std::lock_guard g{m};
 *         PARA_ASSERT(true);
 *     }
 *
 * ── Vad som ändrades från C-versionen ─────────────────────────────────────
 *
 * Registreringen använde `__attribute__((constructor))`, en GCC-utvidgning.
 * Här är den ett objekt med statisk lagringstid vars konstruktor registrerar
 * testet — samma effekt, och det är standard-C++ i stället för en utvidgning.
 *
 * Och PARA_ASSERT_NOT_BUILT tar numera en MODUL, inte ett uttryck. Se
 * src/core/modules.cpp: byggplanen bor på ett ställe, och testsviten läser
 * den i stället för att gissa ur returkoder.
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
    Race = 1 /* körs upprepat av `make stress` */
};

void register_test(const char *name, Fn fn, Tag tag, const char *file, int line) noexcept;

/* Anropas av assert-makrona i barnprocessen. Återvänder aldrig. */
[[noreturn]] void fail(const char *file, int line, const char *fmt, ...) noexcept;

/* Objektet som registrerar. Ett per PARA_TEST, med statisk lagringstid. */
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
            ::para::test::fail(__FILE__, __LINE__, "PARA_ASSERT(%s) föll", #cond);                 \
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
            ::para::test::fail(__FILE__, __LINE__, "%s gav %d (%.*s), väntade %d (%.*s)", #expr,   \
                               static_cast<int>(para_st_), static_cast<int>(got_.size()),          \
                               got_.data(), static_cast<int>(para_ex_),                            \
                               static_cast<int>(want_.size()), want_.data());                      \
        }                                                                                          \
    } while (0)

#define PARA_ASSERT_OK(expr) PARA_ASSERT_STATUS(expr, ::para::Status::Ok)

/* Ett Result<T> som ska bära ett fel. */
#define PARA_ASSERT_ERR(expr, expected)                                                            \
    do {                                                                                           \
        auto para_r_ = (expr);                                                                     \
        if (para_r_.has_value()) {                                                                 \
            ::para::test::fail(__FILE__, __LINE__, "%s lyckades, väntade ett fel", #expr);         \
        }                                                                                          \
        PARA_ASSERT_STATUS(para_r_.error(), expected);                                             \
    } while (0)

/* Ett Result<T> som ska bära ett värde — ger värdet tillbaka. */
#define PARA_UNWRAP(out, expr)                                                                     \
    auto para_res_##out = (expr);                                                                  \
    do {                                                                                           \
        if (!para_res_##out.has_value()) {                                                         \
            const auto e_ = ::para::to_string(para_res_##out.error());                             \
            ::para::test::fail(__FILE__, __LINE__, "%s misslyckades: %.*s", #expr,                 \
                               static_cast<int>(e_.size()), e_.data());                            \
        }                                                                                          \
    } while (0);                                                                                   \
    auto &out = *para_res_##out

/* För byggplanen: dokumenterar i testsviten att modulen ännu inte är byggd.
 * När du vänder raden i src/core/modules.cpp faller det här testet — och DET
 * är signalen att gå hit och skriva ett riktigt test. `make progress` läser
 * samma tabell. */
#define PARA_ASSERT_NOT_BUILT(module)                                                              \
    do {                                                                                           \
        if (::para::is_built(module)) {                                                            \
            const auto n_ = ::para::module_name(module);                                           \
            ::para::test::fail(__FILE__, __LINE__,                                                 \
                               "MODUL %.*s är byggd nu — ta bort den här raden och skriv "         \
                               "riktiga tester för den",                                           \
                               static_cast<int>(n_.size()), n_.data());                            \
        }                                                                                          \
    } while (0)

#endif /* PARACORE_TESTS_PARA_TEST_HPP */
