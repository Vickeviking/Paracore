/* src/core/modules.cpp — BYGGPLANEN. Den enda filen du ändrar för att säga
 * att en modul är klar.
 *
 * ══════════════════════════════════════════════════════════════════════════
 *  NÄR DU HAR BYGGT EN MODUL: vänd dess rad till true. Då faller testet i
 *  tests/test_notbuilt.cpp — och DET är signalen att gå dit, ta bort raden,
 *  och skriva riktiga tester för det du precis byggde.
 *
 *  `make progress` läser den här tabellen. Den är byggplanen i körbar form,
 *  och det är därför den ligger i koden och inte i en README: en TODO-lista
 *  i en README blir inaktuell, en TODO-lista som testsviten läser kan inte
 *  bli det.
 * ══════════════════════════════════════════════════════════════════════════
 *
 * (C-versionen räknade i stället antalet `return PARA_ERR_NOTIMPL` i src/.
 * Det fungerade så länge varje stub var en funktion som KUNDE returnera en
 * kod. I C++ måste `void lock()` uppfylla Lockable och kan inte returnera
 * något alls, så räkningen hade blivit fel i samma stund som modul 4
 * påbörjades. Ett ställe är bättre än sjutton ändå.)
 */
#include <core/status.hpp>

#include <cstdio>
#include <cstdlib>

namespace para {

namespace {

struct Row {
    Module m;
    bool built;
    const char *name;
};

/* ── byggplanen ──────────────────────────────────────────────────────────── */
constexpr Row kModules[] = {
    {Module::Repo, true, "1  monorepot som bevisapparat"},
    {Module::MemoryModel, false, "2  C++-minnesmodellen, mätt och inte trodd"},
    {Module::MutualExclusion, false, "3  ömsesidig uteslutning som bevis"},
    {Module::Spinlocks, false, "4  spinlås, kontention och cachen"},
    {Module::Monitors, false, "5  monitorer, rättvisa och trådpoolen"},
    {Module::BenchRig, false, "6  riggen: att mäta så siffran betyder något"},
    {Module::Sets, false, "7  listor: fem synkroniseringsstrategier"},
    {Module::QueuesStacks, false, "8  köer, stackar och elimination"},
    {Module::Reclamation, false, "9  minnesåtervinning: ABA, hazard pointers"},
    {Module::HashMaps, false, "10 hashtabeller: från ett lås till split-ordering"},
    {Module::SkipLists, false, "11 skiplistor, prioritetsköer, barriärer"},
    {Module::Scheduler, false, "12 slutprovet: work-stealing-schemaläggare"},
};

const Row *find(Module m) noexcept {
    for (const Row &r : kModules) {
        if (r.m == m) {
            return &r;
        }
    }
    return nullptr;
}

} // namespace

bool is_built(Module m) noexcept {
    const Row *r = find(m);
    return r != nullptr && r->built;
}

std::string_view module_name(Module m) noexcept {
    const Row *r = find(m);
    return (r != nullptr) ? std::string_view{r->name} : std::string_view{"okänd modul"};
}

void not_built(Module m, std::string_view what) noexcept {
    /* std::fprintf och inte std::print: den här körs ofta under en sanitizer
     * eller i en kraschande process, och printf är det som är kvar när
     * iostreams tillstånd inte går att lita på. */
    std::fprintf(stderr,
                 "\nparacore: %.*s är inte byggd ännu.\n"
                 "          MODUL %.*s fyller den.\n"
                 "          Vänd raden i src/core/modules.cpp när du har byggt den.\n\n",
                 static_cast<int>(what.size()), what.data(),
                 static_cast<int>(module_name(m).size()), module_name(m).data());
    std::abort();
}

} // namespace para
