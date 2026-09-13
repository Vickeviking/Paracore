/* PROBE — är den här byggkonfigurationens dubbelbreda CAS äkta?
 *
 * Det här är INTE en kanariefågel. En kanariefågel är ett trasigt program som
 * ett verktyg måste fälla. Det här är ett probe: samma familj som
 * build/<MODE>/.tsan-works, alltså en fråga om vad den här maskinen och den
 * här kompilatorn FAKTISKT klarar. Skillnaden spelar roll, för de två ska
 * aldrig se likadana ut i utskriften.
 *
 * Frågan den svarar på:
 *
 *     std::atomic<TaggedPtr<T>> — är den lock-free, eller tar den tyst ett
 *     bibliotekslås?
 *
 * Det är modul 9:s PARA_RECLAIM_TAGGED som står och faller med svaret. En
 * "lock-free" Treiberstack vars CAS i själva verket är ett mutex inne i
 * libatomic är inte lock-free. Den är en låst stack med sämre kod, och
 * ingenting i programmet säger ifrån.
 *
 * ── Och svaret är inte vad de flesta tror ─────────────────────────────────
 *
 * På EN OCH SAMMA x86-64-maskin, med samma -mcx16:
 *
 *     clang++   is_always_lock_free  →  true
 *     g++       is_always_lock_free  →  false
 *
 * GCC vägrar kalla cmpxchg16b lock-free, för att en atomär LÄSNING av 16 byte
 * måste kunna ske på skrivskyddat minne och cmpxchg16b skriver alltid. Clang
 * gör en annan avvägning. Ingen av dem har fel; de svarar på olika frågor.
 *
 * PÅ AARCH64 (Pi 5, gunnar) säger g++ 14.2 också nej — och där hjälper INGEN
 * flagga. Mätt 13 sep 2026, alla fyra gav "LÅST":
 *
 *     (inga flaggor)  -march=armv8.2-a+lse  -mcpu=native  -mcpu=cortex-a76+lse
 *
 * CPU:n har `atomics` i /proc/cpuinfo, alltså finns LSE och därmed CASP. GCC:s
 * beslut handlar inte om instruktionen utan om samma läsbarhetsgaranti som på
 * x86. Att prova flaggorna ändå är rätt reflex; att skriva ned att de inte
 * hjälpte är det som gör att du slipper prova igen om tre månader.
 *
 * Poängen är att du får VETA vilket svar ditt bygge gav, innan du bygger en
 * datastruktur ovanpå antagandet.
 */
#include <mem/reclaim.hpp>
#include <sync/atomic.hpp>

#include <atomic>
#include <cstdio>

namespace {
struct Node {
    int v;
};
} // namespace

int main() {
    const bool ptr_lf = para::is_lock_free_v<Node *>;
    const bool tagged_lf = para::tagged_ptr_is_lock_free_v<Node>;

    std::printf("kompilator        : ");
#if defined(__clang__)
    std::printf("clang %d.%d\n", __clang_major__, __clang_minor__);
#elif defined(__GNUC__)
    std::printf("gcc %d.%d\n", __GNUC__, __GNUC_MINOR__);
#else
    std::printf("okänd\n");
#endif
    std::printf("arkitektur        : %s\n",
#if defined(__x86_64__)
                "x86-64"
#elif defined(__aarch64__)
                "aarch64"
#else
                "okänd"
#endif
    );
    std::printf("sizeof(TaggedPtr) : %zu byte\n", sizeof(para::TaggedPtr<Node>));
    std::printf("atomic<Node*>     : %s\n", ptr_lf ? "lock-free" : "LÅST (libatomic)");
    std::printf("atomic<TaggedPtr> : %s\n", tagged_lf ? "lock-free" : "LÅST (libatomic)");

    /* En enkel pekare som inte är lock-free betyder att något är djupt fel i
     * bygget — ingen plattform Paracore stöder saknar det. DET fäller probet. */
    if (!ptr_lf) {
        std::printf("\nFEL: inte ens atomic<T*> är lock-free här. Kontrollera bygget.\n");
        return 1;
    }

    /* Att den taggade pekaren INTE är lock-free är ett giltigt svar, inte ett
     * fel. Exitkod 2 säger "svaret är nej" och skiljer det från "kunde inte
     * kontrollera" (vilket aldrig händer här) och från "allt är bra". */
    return tagged_lf ? 0 : 2;
}
