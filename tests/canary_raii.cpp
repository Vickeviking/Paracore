/* KANARIEFÅGEL 5 — ett undantag som lämnar en kritisk sektion låst.
 *
 * NY I C++-VERSIONEN, och den kunde inte finnas i C-versionen: C har inga
 * undantag, och därmed inte den här buggklassen.
 *
 * Programmet tar ett lås för hand, kastar, fångar undantaget utanför — och
 * unlock() nås aldrig. Låset är taget för alltid. Nästa tråd som vill ha det
 * hänger, och `make canary` kräver att watchdogen dödar programmet.
 *
 * ── Varför den är värd en egen kanariefågel ───────────────────────────────
 *
 * Den delar verktyg med kanariefågel 3 (watchdogen), vilket bryter mot
 * mönstret "ett program, ett verktyg". Den finns ändå, för att den bevisar
 * något annat: att en hel buggklass blev MÖJLIG i och med språkbytet.
 *
 * Varje gång du skriver `m.lock()` i stället för `std::lock_guard g{m}` har
 * du skrivit det här programmet. Skillnaden mot det här är bara att din
 * throw ligger tre funktioner ned i en allokering som råkade misslyckas.
 *
 * Kanariefågel 3 säger "watchdogen fungerar". Den här säger "så här ser en
 * hängning ut när den beror på ett undantag", och den skillnaden är exakt vad
 * du behöver känna igen klockan två på natten.
 *
 * FIXA DEN ALDRIG. Men gör om den en gång: byt de två handskrivna raderna mot
 *
 *     std::lock_guard g{m};
 *
 * kör om, och se att programmet avslutas normalt. Det är hela RAII-argumentet
 * på tre rader, mätt i stället för påstått. Sätt sedan tillbaka.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>
#include <stdexcept>

namespace {

para::Mutex m;

void kritisk_sektion_utan_vakt() {
    m.lock(); /* ← utan std::lock_guard. Det ÄR buggen. */
    throw std::runtime_error("något gick fel mitt i den kritiska sektionen");
    m.unlock(); /* nås aldrig — och kompilatorn varnar inte, för raden ÄR nåbar
                  * så länge kompilatorn inte vet att throw alltid sker */
}

} // namespace

int main() {
    try {
        kritisk_sektion_utan_vakt();
    } catch (const std::exception &e) {
        std::printf("fångade: %s\n", e.what());
        std::printf("och nu är låset taget för alltid. nästa tråd hänger.\n");
        std::fflush(stdout);
    }

    /* Nästa tråd hänger. Watchdogen ska döda oss här. */
    {
        para::Thread t{[] {
            m.lock();
            m.unlock();
        }};
    }

    std::printf("den här raden ska aldrig nås\n");
    return 0;
}
