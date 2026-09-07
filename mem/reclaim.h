/* mem/reclaim.h — säker minnesåtervinning. Kursens svåraste modul.
 *
 * STATUS: STUB — du bygger den i MODUL 9 (AMP 10.6, McKenney).
 *
 * DET HÄR ÄR TILLÄGGET TILL DIN URSPRUNGLIGA TRÄDSTRUKTUR, och skälet är
 * enkelt: utan det här biblioteket är varje lock-free struktur i ds/ antingen
 * ett minnesläckage eller en use-after-free. Det finns ingen tredje möjlighet,
 * och det är den insikt de flesta kursböcker viker undan från.
 *
 * Problemet: din pop läser `top`, läser `top->next`, och CAS:ar. Mellan
 * läsningen och CAS:en kan en annan tråd ha poppat noden och FRIGJORT den.
 * Din läsning av `->next` är då en use-after-free. Att inte frigöra alls är
 * det enda som räddar dig, och det är inte en lösning.
 *
 * Tre svar, i den ordning du ska bygga dem:
 *
 *   PARA_RECLAIM_LEAK    frigör aldrig. Referensen — och det du faktiskt kör
 *                        med i modul 7 och 8. Ärligt namngiven.
 *
 *   PARA_RECLAIM_TAGGED  räknare i pekarens oanvända bitar, eller dubbelbrett
 *                        CAS (cmpxchg16b på x86-64, LSE casp på aarch64).
 *                        Löser ABA men INTE use-after-free. Räkna på när taggen
 *                        går runt — svaret är i sekunder, inte i år.
 *
 *   PARA_RECLAIM_HAZARD  Michael: varje tråd publicerar de pekare den just nu
 *                        läser; den som retirerar en nod skannar publikationerna
 *                        och skjuter upp free för det som är i bruk.
 *                        Gränsen för hur mycket som kan vara oåtervunnet är
 *                        BEVISBAR — härled den själv, den är O(trådar × hazards).
 *
 *   PARA_RECLAIM_EPOCH   Fraser: billigare i det vanliga fallet (ingen skrivning
 *                        per läst pekare), men EN enda fastnad läsare håller
 *                        hela epoken och minnet växer obegränsat. Mät det med
 *                        en tråd som sover mitt i en läsning. RCU i kärnan är
 *                        samma idé med ett schemaläggartrick i stället för en
 *                        räknare.
 *
 * KLART-KRITERIUM (milstolpe 9): både Treiberstacken och MS-kön återvinner
 * genom en hazard-domän, `make asan` är tyst efter 8 trådar × 60 sekunder,
 * och du har mätt vad domänen kostar i genomströmning jämfört med att läcka.
 */
#ifndef PARACORE_MEM_RECLAIM_H
#define PARACORE_MEM_RECLAIM_H

#include <stddef.h>
#include <core/status.h>

typedef enum para_reclaim_kind {
    PARA_RECLAIM_LEAK = 0,
    PARA_RECLAIM_TAGGED,
    PARA_RECLAIM_HAZARD,
    PARA_RECLAIM_EPOCH
} para_reclaim_kind;

typedef struct para_domain para_domain;
typedef void (*para_free_fn)(void *node);

/* `hazards_per_thread` är hur många pekare en tråd kan hålla samtidigt.
 * Treiber behöver 1, Michael–Scott behöver 2, Harris-listan behöver 3.
 * Att välja för lågt ger inte ett fel — det ger en tyst use-after-free.
 * Assert:a i skyddsfunktionen. */
para_status para_domain_init(para_domain **out, para_reclaim_kind kind, unsigned hazards_per_thread,
                             para_free_fn free_fn);
void para_domain_destroy(para_domain *d);

/* Varje tråd registrerar sig en gång. Avregistrering återlämnar platsen. */
para_status para_domain_register_thread(para_domain *d);
void para_domain_unregister_thread(para_domain *d);

/* Publicera att du läser `p` på plats `slot`. Måste följas av en OMLÄSNING
 * av källan och en kontroll att den fortfarande pekar på `p` — annars hann
 * någon retirera den mellan din läsning och din publicering. Det steget är
 * det folk glömmer, och det är det enda som gör konstruktionen korrekt. */
void *para_hazard_protect(para_domain *d, unsigned slot, void *const *source);
void para_hazard_clear(para_domain *d, unsigned slot);

/* Logiskt borttagen. Fysisk frigöring sker när ingen längre skyddar noden. */
para_status para_domain_retire(para_domain *d, void *node);

/* Kör en återvinningsomgång nu. Anropas normalt automatiskt av retire. */
size_t para_domain_reclaim(para_domain *d);

/* Rapporten vill ha den här kurvan över tid. */
size_t para_domain_retired_count(const para_domain *d);

#endif /* PARACORE_MEM_RECLAIM_H */
