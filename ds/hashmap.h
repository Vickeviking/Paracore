/* ds/hashmap.h — fyra hashtabeller, i den ordning AMP kapitel 13 motiverar dem.
 *
 * STATUS: STUB — du bygger dem i MODUL 10.
 *
 *   PARA_MAP_GLOBAL     ett lås om hela tabellen. Referensen.
 *   PARA_MAP_STRIPED    L lås över N hinkar, lock[hash % L]. Den första riktiga
 *                       skalningsvinsten. Svep L = 1, 8, 64, 1024 och hitta
 *                       där vinsten planar ut — svaret handlar om cachelinjer,
 *                       inte om lås.
 *   PARA_MAP_REFINABLE  striped OCH omstrukturerbar. Modulens svåra del: att
 *                       fördubbla tabellen medan andra trådar läser. Ta alla
 *                       lås i bestämd ordning, markera med en ägarflagga, och
 *                       låt en tråd som redan börjat på den gamla tabellen
 *                       upptäcka det och göra om. Mät OMSTRUKTURERINGSKLIPPET:
 *                       genomströmning sekund för sekund runt en resize.
 *   PARA_MAP_SPLIT      lock-free, rekursiv split-ordering (Shalev–Shavit).
 *                       Den vackra idén: håll ALLA element i EN lock-free lista
 *                       sorterad på BITREVERSERAD nyckel, och låt hinkarna vara
 *                       pekare in i listan. Att fördubbla hinkarna flyttar då
 *                       inte ett enda element — den nya hinken är bara en ny
 *                       ingångspunkt i en lista som redan är rätt sorterad.
 *
 * Listan i PARA_MAP_SPLIT ÄR modul 7:s Harris-lista. Återanvänd den. Går den
 * inte att återanvända är det ett gränssnittsfel i modul 7, och det är värt
 * att gå tillbaka och fixa där i stället för att kopiera koden hit.
 */
#ifndef PARACORE_DS_HASHMAP_H
#define PARACORE_DS_HASHMAP_H

#include <stddef.h>
#include <stdint.h>
#include <core/status.h>

typedef enum para_map_kind {
    PARA_MAP_GLOBAL = 0,
    PARA_MAP_STRIPED,
    PARA_MAP_REFINABLE,
    PARA_MAP_SPLIT
} para_map_kind;

typedef struct para_map para_map;

/* `stripes` gäller STRIPED/REFINABLE; 0 = välj själv utifrån hardware_concurrency. */
para_status para_map_init(para_map **out, para_map_kind kind, unsigned stripes);
void para_map_destroy(para_map *m);

para_status para_map_put(para_map *m, uint64_t key, void *value);
para_status para_map_get(const para_map *m, uint64_t key, void **out); /* PARA_ERR_NOTFOUND */
para_status para_map_remove(para_map *m, uint64_t key, void **out);
size_t para_map_size_approx(const para_map *m);

#endif /* PARACORE_DS_HASHMAP_H */
