/* KANARIEFÅGEL 2 — en LATENT deadlock (ABBA-låsordning) som aldrig inträffar.
 *
 * Den viktigaste av de fem, och avsiktligt konstruerad så att den ALDRIG
 * fastnar: en grind (`G` + `C`) släpper inte fram tråd 2:s B→A förrän tråd 1:s
 * A→B är helt klar. De två kan omöjligen mötas. Programmet kör igenom på
 * nolltid, varje gång, på varje maskin.
 *
 * Och ändå är buggen där. Om de två ordningarna någon gång utfördes samtidigt
 * skulle trådarna deadlocka, och så här ser verkliga låsordningsbuggar ut:
 * latenta i månader, gröna i CI, och sedan hänger produktionen en tisdag för
 * att lasten råkade bli hög.
 *
 * `make canary` kräver att helgrind rapporterar "lock order violated" på ett
 * program som fungerade perfekt. Det är hela skälet att verktyget finns:
 *
 *     ett test kan bara visa att buggen inte inträffade den här gången.
 *     helgrind visar att den KAN inträffa.
 *
 * ── Varför BÅDA trådarna skapas innan någon joinas ─────────────────────────
 *
 * Inte stil, utan en verklig krock med verktyget. Den första versionen gjorde
 * create(t1); join(t1); create(t2); join(t2) — alltså skapade en tråd EFTER
 * att en annan hade joinats. Det får helgrind 3.25.1 att krascha internt:
 *
 *     Helgrind: hg_main.c:5411 (hg_handle_client_request):
 *               Assertion 'found' failed.
 *
 * Den kraschen ser i utskriften nästan ut som "hittade inget", och just den
 * förväxlingen är vad kanariefåglarna finns för att omöjliggöra. Den upptäcktes
 * också precis som den skulle: samma repo gick grönt på en maskin och rött på
 * nästa. Kör dem på båda.
 *
 * Det här är också anledningen till att trådarna ligger i ett eget scope
 * nedan i stället för att skapas och joinas var för sig — std::jthread joinar
 * i destruktorn, och destruktorerna körs i omvänd ordning vid scopets slut,
 * alltså efter att båda har skapats. Formen som undviker helgrind-kraschen
 * blev den naturliga formen i C++.
 *
 * FIXA ALDRIG ABBA-ORDNINGEN NEDAN. Grinden får du gärna göra elegantare.
 *
 * ── Och en sak du ska prova ────────────────────────────────────────────────
 *
 * Byt de två kritiska sektionerna mot
 *
 *     std::scoped_lock guard{A, B};      respektive     std::scoped_lock guard{B, A};
 *
 * och kör helgrind igen. Buggen är BORTA, trots att ordningen i koden
 * fortfarande är omvänd — std::lock provar och backar av i stället för att
 * låsa i tur och ordning. Det är kanariefågel 2:s bugg, löst i standarden,
 * och det fungerar med dina egna lås i samma stund som de uppfyller
 * para::Lockable. Sätt sedan tillbaka den här versionen.
 */
#include <core/mutex.hpp>
#include <core/thread.hpp>

#include <cstdio>
#include <mutex>

namespace {

para::Mutex A;
para::Mutex B;

/* Grinden som gör kollisionen omöjlig — och därmed poängen tydlig. */
para::Mutex G;
para::CondVar C;
bool first_done = false;

void lock_ab() {
    {
        std::lock_guard ga{A};
        std::lock_guard gb{B}; /* A → B */
    }
    {
        std::unique_lock lk{G};
        first_done = true;
        C.notify_one();
    }
}

void lock_ba() {
    {
        std::unique_lock lk{G};
        C.wait(lk, [] { return first_done; });
    }
    {
        std::lock_guard gb{B};
        std::lock_guard ga{A}; /* B → A — och det är buggen */
    }
}

} // namespace

int main() {
    {
        para::Thread t1{lock_ab};
        para::Thread t2{lock_ba}; /* båda skapade före någon join */
    }
    std::printf("kördes igenom utan att hänga — och är ändå trasig.\n"
                "helgrind ska säga 'lock order ... violated'.\n");
    return 0;
}
