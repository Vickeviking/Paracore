/* KANARIEFÅGEL 1 — en avsiktlig datakapplöpning.
 *
 * Det här programmet är TRASIGT MED FLIT och ska aldrig fixas.
 *
 * `make canary` bygger det under ThreadSanitizer och kräver att TSan FÄLLER
 * det. Går det igenom har din sanitizer slutat fungera — fel flaggor, fel
 * länkordning, en -fno-sanitize som smugit in via ett beroende — och då är
 * varje grönt TSan-resultat du fått sedan dess värdelöst.
 *
 * Det är den enda testtyp som skyddar mot att verktygen tyst går sönder, och
 * den kostar tjugo rader. Ta aldrig bort den.
 */
#include <core/thread.hpp>

#include <cstdio>

namespace {
/* Medvetet inte std::atomic. Det är hela poängen. */
long shared_counter = 0;

void bump() {
    for (int i = 0; i < 100000; ++i) {
        shared_counter++; /* läs-modifiera-skriv utan synkronisering */
    }
}
} // namespace

int main() {
    {
        para::Thread a{bump};
        para::Thread b{bump};
    } /* jthread joinar här */
    std::printf("räknaren blev %ld (väntat 200000 om ingen kapplöpning fanns)\n", shared_counter);
    return 0;
}
