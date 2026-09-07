/* reclaim.c
 *
 * MODUL 9 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <mem/reclaim.h>

struct para_domain {
    int unused;
};

para_status para_domain_init(para_domain **out, para_reclaim_kind kind, unsigned hazards_per_thread,
                             para_free_fn free_fn) {
    (void)out;
    (void)kind;
    (void)hazards_per_thread;
    (void)free_fn;
    return PARA_ERR_NOTIMPL;
}
void para_domain_destroy(para_domain *d) {
    (void)d;
}
para_status para_domain_register_thread(para_domain *d) {
    (void)d;
    return PARA_ERR_NOTIMPL;
}
void para_domain_unregister_thread(para_domain *d) {
    (void)d;
}
void *para_hazard_protect(para_domain *d, unsigned slot, void *const *source) {
    (void)d;
    (void)slot;
    (void)source;
    return NULL;
}
void para_hazard_clear(para_domain *d, unsigned slot) {
    (void)d;
    (void)slot;
}
para_status para_domain_retire(para_domain *d, void *node) {
    (void)d;
    (void)node;
    return PARA_ERR_NOTIMPL;
}
size_t para_domain_reclaim(para_domain *d) {
    (void)d;
    return 0u;
}
size_t para_domain_retired_count(const para_domain *d) {
    (void)d;
    return 0u;
}
