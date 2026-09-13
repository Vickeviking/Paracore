/* Byggplanen som tester.
 *
 * Varje rad här säger "den här modulen är inte byggd ännu". När du vänder
 * modulens rad i src/core/modules.cpp FALLER testet — och det är signalen att
 * komma hit, ta bort raden, och skriva riktiga tester för det du precis
 * byggde. `make progress` läser samma tabell.
 *
 * Det är avsiktligt att kursplanen är körbar. En TODO-lista i en README blir
 * inaktuell; en TODO-lista som är en testsvit kan inte bli det.
 *
 * TVÅ SORTERS PÅSTÅENDEN PER MODUL, och skillnaden är värd att förstå:
 *
 *   PARA_ASSERT_NOT_BUILT(Module::X)   läser byggplanen. Faller när DU säger
 *                                      att modulen är klar.
 *
 *   PARA_ASSERT_ERR(...NotBuilt)       anropar stubben. Faller när koden
 *                                      börjar svara på riktigt.
 *
 * Att båda finns är inte dubbelarbete: den första fångar "jag vände raden men
 * byggde inget", den andra fångar "jag byggde det men glömde vända raden".
 * Båda har hänt.
 */
#include "para_test.hpp"

#include <paracore.hpp>

#include <string>

PARA_TEST(m02_minnesmodellen_inte_genomarbetad) {
    PARA_ASSERT_NOT_BUILT(para::Module::MemoryModel);
    /* Hjälpmedlen finns dock — modulen handlar om att förstå dem. */
    para::Backoff b{8};
    b.once();
    PARA_ASSERT(b.limit() >= 1);
}

PARA_TEST(m03_omsesidig_uteslutning_inte_byggd) {
    PARA_ASSERT_NOT_BUILT(para::Module::MutualExclusion);
    /* Namnen finns redan — mätskripten i modul 5 vill ha dem, och de kommer
     * ur typen så att de inte kan hamna i otakt med den. */
    PARA_ASSERT(std::string{para::PetersonLock::name()} == "peterson");
    PARA_ASSERT(std::string{para::FilterLock::name()} == "filter");
    PARA_ASSERT(std::string{para::BakeryLock::name()} == "bakery");
}

PARA_TEST(m04_spinlas_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::Spinlocks);
    /* Namnen finns redan — mätskripten vill ha dem, och de kommer ur typen
     * så att de inte kan hamna i otakt med den. */
    PARA_ASSERT(std::string{para::McsLock::name()} == "mcs");
    PARA_ASSERT(std::string{para::TasLock::name()} == "tas");
    PARA_ASSERT(std::string{para::ArrayLock::name()} == "alock");
}

PARA_TEST(m05_pool_rwlock_semafor_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::Monitors);
    PARA_ASSERT_ERR(para::ThreadPool::create(4, 64), para::Status::NotBuilt);
}

PARA_TEST(m06_matriggen_inte_byggd) {
    PARA_ASSERT_NOT_BUILT(para::Module::BenchRig);
    const para::bench::Config cfg{.name = "tom", .min_threads = 1, .max_threads = 1};
    PARA_ASSERT_ERR(para::bench::run(cfg, [](unsigned, unsigned) -> std::size_t { return 0; }),
                    para::Status::NotBuilt);
}

PARA_TEST(m07_mangderna_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::Sets);
    para::LazySet<std::uint64_t> s;
    PARA_ASSERT_STATUS(s.add(1), para::Status::NotBuilt);
}

PARA_TEST(m08_koer_och_stackar_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::QueuesStacks);
    para::MichaelScottQueue<int> q;
    PARA_ASSERT_STATUS(q.push(1), para::Status::NotBuilt);
    para::TreiberStack<int> st;
    PARA_ASSERT_STATUS(st.push(1), para::Status::NotBuilt);
    para::SpscRing<int, 1024> ring;
    PARA_ASSERT_STATUS(ring.try_push(1), para::Status::NotBuilt);
}

PARA_TEST(m09_minnesatervinning_inte_byggd) {
    PARA_ASSERT_NOT_BUILT(para::Module::Reclamation);
    struct Node {
        int v;
    };
    para::HazardDomain<Node, 2> d;
    PARA_ASSERT_STATUS(d.retire(nullptr), para::Status::NotBuilt);

    /* LeakDomain ÄR byggd, och den räknar. Referensen ska fungera från dag
     * ett — annars går modul 7 och 8 inte att köra alls. */
    para::LeakDomain<Node> leak;
    PARA_ASSERT_OK(leak.retire(nullptr));
    PARA_ASSERT_EQ(leak.retired_count(), 1);
}

PARA_TEST(m10_hashtabellerna_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::HashMaps);
    para::StripedMap<std::uint64_t, int> m{1024, 64};
    PARA_ASSERT_STATUS(m.put(1, 2), para::Status::NotBuilt);
}

PARA_TEST(m11_barriar_skiplista_pqueue_inte_byggda) {
    PARA_ASSERT_NOT_BUILT(para::Module::SkipLists);
    para::SenseBarrier b{4};
    PARA_ASSERT_ERR(b.arrive_and_wait(), para::Status::NotBuilt);
    para::LockFreeSkipList<std::uint64_t, int> sl;
    PARA_ASSERT_STATUS(sl.add(1, 2), para::Status::NotBuilt);
    para::PriorityQueue<std::uint64_t, int> pq;
    PARA_ASSERT_STATUS(pq.push(1, 2), para::Status::NotBuilt);
}

PARA_TEST(m12_schemalaggaren_inte_byggd) {
    PARA_ASSERT_NOT_BUILT(para::Module::Scheduler);
    PARA_ASSERT_ERR(para::Scheduler::create(4), para::Status::NotBuilt);
}

/* ── och en rad som INTE ska falla ────────────────────────────────────────
 *
 * Modul 1 är byggd. Att den står här är hur sviten bevisar att
 * PARA_ASSERT_NOT_BUILT faktiskt läser tabellen och inte bara alltid säger
 * ja: om is_built() vore trasig och returnerade false för allt, skulle det
 * här testet gå igenom och alla andra också — och sviten hade varit
 * dekoration. */
PARA_TEST(m01_repot_ar_byggt) {
    PARA_ASSERT(para::is_built(para::Module::Repo));
}
