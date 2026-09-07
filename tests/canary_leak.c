/* KANARIEFÅGEL 3 — en avsiktlig minnesläcka.
 *
 * `make canary` kräver att AddressSanitizer (och valgrind memcheck) FÄLLER
 * den. Modul 9 handlar om att frigöra minne i lock-free-strukturer utan att
 * dra undan det för någon annan; om ASan inte hittar en läcka på 32 byte
 * kommer den inte att hitta din heller.
 */
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    void *p = malloc(32);
    if (p == NULL) {
        return 1;
    }
    /* Ingen free. Med flit. */
    printf("allokerade 32 byte på %p och glömde dem\n", p);
    return 0;
}
