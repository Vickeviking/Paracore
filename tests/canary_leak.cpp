/* KANARIEFÅGEL 4 — en avsiktlig minnesläcka.
 *
 * `make canary` kräver att AddressSanitizer (och valgrind memcheck) FÄLLER
 * den. Modul 9 handlar om att frigöra minne i lock-free-strukturer utan att
 * dra undan det för någon annan; om ASan inte hittar en läcka på 32 byte
 * kommer den inte att hitta din heller.
 *
 * NEW OCH INTE MALLOC, med flit: i C++ går minne oftare förlorat genom en
 * `new` vars `delete` aldrig körs — för att en tidig retur, ett undantag
 * eller en ägarskapsoklarhet kom emellan — än genom en glömd free(). Läckan
 * ska se ut som de läckor du faktiskt kommer att skriva.
 */
#include <cstdio>
#include <new>

int main() {
    /* new[] utan delete[]. Med flit. Att det INTE är ett unique_ptr är
     * poängen: varje läcka i modern C++ börjar med en rå ägare. */
    int *p = new (std::nothrow) int[8];
    if (p == nullptr) {
        return 1;
    }
    p[0] = 1;
    std::printf("allokerade 32 byte på %p och glömde dem\n", static_cast<void *>(p));
    return 0;
}
