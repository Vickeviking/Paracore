/* src/core/modules.cpp — THE BUILD PLAN. The only file you change to say that
 * a module is done.
 *
 * ══════════════════════════════════════════════════════════════════════════
 *  WHEN YOU HAVE BUILT A MODULE: flip its row to true. The matching test in
 *  tests/test_notbuilt.cpp then fails — and THAT is the signal to go there,
 *  delete the row, and write real tests for what you just built.
 *
 *  `make progress` reads this table. It is the build plan in executable
 *  form, and that is why it lives in the code and not in a README: a TODO
 *  list in a README goes stale, a TODO list the test suite reads cannot.
 * ══════════════════════════════════════════════════════════════════════════
 *
 * ══════════════════════════════════════════════════════════════════════════
 *  THE NUMBERING IS THE ARCTURON TRACK'S, one to one.
 *
 *  The study track "Paracore" in Arcturon has eleven modules, and rows 1–11
 *  below carry the same number and the same title. A number in a header
 *  ("MODULE 7 fills this file") is always the track's number, so a lesson
 *  that says "module 3" and a header that says "module 3" mean the same
 *  thing.
 *
 *  Row 0 is the repo itself — the Makefile, the test rig and the five
 *  canaries. It is not a track module (the track starts with implementation
 *  directly), but it stays here because it is the row that proves
 *  `is_built()` actually reads the table
 *  (tests/test_notbuilt.cpp::m00_repo_is_built).
 *
 *  Status mirrors the track: a row is true when every lesson of that module
 *  is completed in Arcturon AND the code exists here.
 * ══════════════════════════════════════════════════════════════════════════
 *
 * (The C version counted the number of `return PARA_ERR_NOTIMPL` in src/
 * instead. That worked as long as every stub was a function that COULD return
 * a code. In C++, `void lock()` has to satisfy Lockable and cannot return
 * anything, so the count would have been wrong the moment the spinlock module
 * started. One place beats seventeen anyway.)
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

/* ── the build plan ──────────────────────────────────────────────────────── */
constexpr Row kModules[] = {
    {Module::Repo, true, "0  the repo as a proof machine (prerequisite)"},
    {Module::MemoryModel, true, "1  the memory model, measured and not believed"},
    {Module::MutualExclusion, false, "2  mutual exclusion, built from atomics"},
    {Module::Spinlocks, false, "3  spinlocks, contention and the cache"},
    {Module::Monitors, false, "4  monitors, fairness and the thread pool"},
    {Module::BenchRig, false, "5  the bench rig: measuring so the number means something"},
    {Module::Sets, false, "6  sets: five synchronisation strategies on one data structure"},
    {Module::QueuesStacks, false, "7  queues, stacks and elimination"},
    {Module::Reclamation, false, "8  memory reclamation: ABA, hazard pointers and epochs"},
    {Module::HashMaps, false, "9  hash tables: from one lock to split-ordering"},
    {Module::SkipLists, false, "10 skip lists, priority queues and barriers"},
    {Module::Scheduler, false, "11 the final exam: a work-stealing scheduler"},
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
    return (r != nullptr) ? std::string_view{r->name} : std::string_view{"unknown module"};
}

void not_built(Module m, std::string_view what) noexcept {
    /* std::fprintf and not std::print: this often runs under a sanitizer or in
     * a crashing process, and printf is what is left when iostreams' state
     * cannot be trusted. */
    std::fprintf(stderr,
                 "\nparacore: %.*s is not built yet.\n"
                 "          MODULE %.*s fills it.\n"
                 "          Flip the row in src/core/modules.cpp when you have built it.\n\n",
                 static_cast<int>(what.size()), what.data(),
                 static_cast<int>(module_name(m).size()), module_name(m).data());
    std::abort();
}

} // namespace para
