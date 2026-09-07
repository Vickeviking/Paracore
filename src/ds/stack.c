/* stack.c
 *
 * MODUL 8 fyller den här filen. Tills dess svarar allting
 * PARA_ERR_NOTIMPL — vilket testerna räknar med och `make progress` räknar.
 */
#include <ds/stack.h>

struct para_stack {
    int unused;
};

para_status para_stack_init(para_stack **out, para_stack_kind kind) {
    (void)out;
    (void)kind;
    return PARA_ERR_NOTIMPL;
}
void para_stack_destroy(para_stack *s) {
    (void)s;
}
para_status para_stack_push(para_stack *s, void *value) {
    (void)s;
    (void)value;
    return PARA_ERR_NOTIMPL;
}
para_status para_stack_pop(para_stack *s, void **out) {
    (void)s;
    (void)out;
    return PARA_ERR_NOTIMPL;
}
size_t para_stack_size_approx(const para_stack *s) {
    (void)s;
    return 0u;
}
