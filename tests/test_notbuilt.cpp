/* The build plan as tests.
 *
 * Every row here says "this module is not built yet". When you flip the
 * module's row in src/core/modules.cpp the test FAILS — and that is the
 * signal to come here, delete the row, and write real tests for what you just
 * built. `make progress` reads the same table.
 *
 * It is deliberate that the curriculum is executable. A TODO list in a README
 * goes stale; a TODO list that is a test suite cannot.
 *
 * Test names carry the Arcturon track's module number (m02 = track module 2).
 *
 * TWO KINDS OF ASSERTION PER MODULE, and the difference is worth
 * understanding:
 *
 *   PARA_ASSERT_NOT_BUILT(Module::X)   reads the build plan. Fails when YOU
 *                                      say the module is done.
 *
 *   PARA_ASSERT_ERR(...NotBuilt)       calls the stub. Fails when the code
 *                                      starts answering for real.
 *
 * Having both is not duplication: the first catches "I flipped the row but
 * built nothing", the second catches "I built it but forgot to flip the row".
 * Both have happened.
 *
 * Module 1 (the memory model) has no row: it is built, it lives in
 * playground/ (the litmus rig), and its library helpers — Backoff,
 * CacheAligned — have real tests in tests/test_core.cpp.
 */
#include "para_test.hpp"

#include <paracore.hpp>

#include <string>

PARA_TEST(m02_mutual_exclusion_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::MutualExclusion);
    /* PetersonLock is built (tests/test_peterson.cpp); filter and bakery are
     * not. The names exist already — the bench scripts in module 5 want them,
     * and they come from the type so they cannot drift out of sync with it. */
    PARA_ASSERT(std::string{para::FilterLock::name()} == "filter");
    PARA_ASSERT(std::string{para::BakeryLock::name()} == "bakery");
}

PARA_TEST(m03_spinlocks_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::Spinlocks);
    /* The names exist already — the bench scripts want them, and they come
     * from the type so they cannot drift out of sync with it. */
    PARA_ASSERT(std::string{para::McsLock::name()} == "mcs");
    PARA_ASSERT(std::string{para::TasLock::name()} == "tas");
    PARA_ASSERT(std::string{para::ArrayLock::name()} == "alock");
}

PARA_TEST(m04_pool_rwlock_semaphore_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::Monitors);
    PARA_ASSERT_ERR(para::ThreadPool::create(4, 64), para::Status::NotBuilt);
}

PARA_TEST(m05_bench_rig_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::BenchRig);
    const para::bench::Config cfg{.name = "empty", .min_threads = 1, .max_threads = 1};
    PARA_ASSERT_ERR(para::bench::run(cfg, [](unsigned, unsigned) -> std::size_t { return 0; }),
                    para::Status::NotBuilt);
}

PARA_TEST(m06_sets_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::Sets);
    para::LazySet<std::uint64_t> s;
    PARA_ASSERT_STATUS(s.add(1), para::Status::NotBuilt);
}

PARA_TEST(m07_queues_and_stacks_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::QueuesStacks);
    para::MichaelScottQueue<int> q;
    PARA_ASSERT_STATUS(q.push(1), para::Status::NotBuilt);
    para::TreiberStack<int> st;
    PARA_ASSERT_STATUS(st.push(1), para::Status::NotBuilt);
    para::SpscRing<int, 1024> ring;
    PARA_ASSERT_STATUS(ring.try_push(1), para::Status::NotBuilt);
}

PARA_TEST(m08_reclamation_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::Reclamation);
    struct Node {
        int v;
    };
    para::HazardDomain<Node, 2> d;
    PARA_ASSERT_STATUS(d.retire(nullptr), para::Status::NotBuilt);

    /* LeakDomain IS built, and it counts. The reference has to work from day
     * one — otherwise modules 6 and 7 cannot run at all. */
    para::LeakDomain<Node> leak;
    PARA_ASSERT_OK(leak.retire(nullptr));
    PARA_ASSERT_EQ(leak.retired_count(), 1);
}

PARA_TEST(m09_hash_maps_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::HashMaps);
    para::StripedMap<std::uint64_t, int> m{1024, 64};
    PARA_ASSERT_STATUS(m.put(1, 2), para::Status::NotBuilt);
}

PARA_TEST(m10_barrier_skiplist_pqueue_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::SkipLists);
    para::SenseBarrier b{4};
    PARA_ASSERT_ERR(b.arrive_and_wait(), para::Status::NotBuilt);
    para::LockFreeSkipList<std::uint64_t, int> sl;
    PARA_ASSERT_STATUS(sl.add(1, 2), para::Status::NotBuilt);
    para::PriorityQueue<std::uint64_t, int> pq;
    PARA_ASSERT_STATUS(pq.push(1, 2), para::Status::NotBuilt);
}

PARA_TEST(m11_scheduler_not_built) {
    PARA_ASSERT_NOT_BUILT(para::Module::Scheduler);
    PARA_ASSERT_ERR(para::Scheduler::create(4), para::Status::NotBuilt);
}

/* ── and rows that must NOT fail ──────────────────────────────────────────
 *
 * Rows 0 and 1 are built. Having them here is how the suite proves that
 * PARA_ASSERT_NOT_BUILT actually reads the table and does not just always say
 * yes: if is_built() were broken and returned false for everything, the rows
 * above would pass and so would everything else — and the suite would be
 * decoration. */
PARA_TEST(m00_repo_is_built) {
    PARA_ASSERT(para::is_built(para::Module::Repo));
}

PARA_TEST(m01_memory_model_is_built) {
    PARA_ASSERT(para::is_built(para::Module::MemoryModel));
}
