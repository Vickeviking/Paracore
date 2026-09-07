/* hashmap.c
 *
 * MODUL 10 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <ds/hashmap.h>

struct para_map {
    int unused;
};

para_status para_map_init(para_map **out, para_map_kind kind, unsigned stripes) {
    (void)out;
    (void)kind;
    (void)stripes;
    return PARA_ERR_NOTIMPL;
}
void para_map_destroy(para_map *m) {
    (void)m;
}
para_status para_map_put(para_map *m, uint64_t key, void *value) {
    (void)m;
    (void)key;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_map_get(const para_map *m, uint64_t key, void **out) {
    (void)m;
    (void)key;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
para_status para_map_remove(para_map *m, uint64_t key, void **out) {
    (void)m;
    (void)key;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
size_t para_map_size_approx(const para_map *m) {
    (void)m;
    return 0u;
}
