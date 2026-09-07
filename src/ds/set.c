/* set.c
 *
 * MODUL 7 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <ds/set.h>

struct para_set {
    int unused;
};

para_status para_set_init(para_set **out, para_set_kind kind) {
    (void)out;
    (void)kind;
    return PARA_ERR_NOTIMPL;
}
void para_set_destroy(para_set *s) {
    (void)s;
}
para_status para_set_add(para_set *s, uint64_t key) {
    (void)s;
    (void)key;
    return PARA_ERR_NOTIMPL;
}
para_status para_set_remove(para_set *s, uint64_t key) {
    (void)s;
    (void)key;
    return PARA_ERR_NOTIMPL;
}
int para_set_contains(const para_set *s, uint64_t key) {
    (void)s;
    (void)key;
    return 0;
}
size_t para_set_size_approx(const para_set *s) {
    (void)s;
    return 0u;
}
