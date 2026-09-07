/* ds/set.h — samma mängd, fem synkroniseringsstrategier.
 *
 * STATUS: STUB — du bygger dem i MODUL 7 (AMP kapitel 9).
 *
 * Det här är period 2:s viktigaste modul, och kapitlet är genialt för att det
 * håller DATASTRUKTUREN konstant och varierar bara synkroniseringen. Samma
 * kontrakt — add, remove, contains — fem gånger:
 *
 *   PARA_SET_COARSE       ett lås om hela listan.
 *   PARA_SET_FINE         hand-over-hand: lås två noder i taget. Första riktiga
 *                         ordningsdisciplinen, och första chansen till deadlock
 *                         om du släpper i fel ordning.
 *   PARA_SET_OPTIMISTIC   gå utan lås, lås sedan och VALIDERA att du fortfarande
 *                         är där du tror. Validering är det nya begreppet, och
 *                         det bär resten av perioden.
 *   PARA_SET_LAZY         logisk borttagning via en marked-bit, så att contains
 *                         blir WAIT-FREE och aldrig tar ett lås alls.
 *                         Kapitlets viktigaste steg.
 *   PARA_SET_LOCKFREE     Harris/Michael: lågbiten i pekaren bär
 *                         borttagningsflaggan, CAS på pekare-med-flagga.
 *
 * FÖR VARJE VERSION ska du skriva ned linjäriseringspunkten — inklusive för
 * en `contains` som returnerar false. För LAZY och LOCKFREE är svaret inte
 * uppenbart, och det är hela poängen. Skriv dem i docs/linearization.md.
 */
#ifndef PARACORE_DS_SET_H
#define PARACORE_DS_SET_H

#include <stddef.h>
#include <stdint.h>
#include <core/status.h>

typedef enum para_set_kind {
    PARA_SET_COARSE = 0,
    PARA_SET_FINE,
    PARA_SET_OPTIMISTIC,
    PARA_SET_LAZY,
    PARA_SET_LOCKFREE
} para_set_kind;

typedef struct para_set para_set;

para_status para_set_init(para_set **out, para_set_kind kind);
void para_set_destroy(para_set *s);

para_status para_set_add(para_set *s, uint64_t key);    /* PARA_ERR_BUSY = fanns redan */
para_status para_set_remove(para_set *s, uint64_t key); /* PARA_ERR_NOTFOUND */
int para_set_contains(const para_set *s, uint64_t key);
size_t para_set_size_approx(const para_set *s);

#endif /* PARACORE_DS_SET_H */
