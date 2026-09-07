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
#include <core/thread.h>
#include <stdio.h>

static long shared_counter; /* medvetet inte _Atomic. Det är hela poängen. */

static void *bump(void *arg) {
    (void)arg;
    for (int i = 0; i < 100000; i++) {
        shared_counter++; /* läs-modifiera-skriv utan synkronisering */
    }
    return NULL;
}

int main(void) {
    para_thread *a = NULL;
    para_thread *b = NULL;
    para_thread_create(&a, bump, NULL);
    para_thread_create(&b, bump, NULL);
    para_thread_join(a, NULL);
    para_thread_join(b, NULL);
    printf("räknaren blev %ld (väntat 200000 om ingen kapplöpning fanns)\n", shared_counter);
    return 0;
}
