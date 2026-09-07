/* exec/pool.h — trådpoolen. Period 1:s arbetshäst.
 *
 * STATUS: STUB — du bygger den i MODUL 5.
 *
 * Fast antal arbetare som drar jobb ur en BEGRÄNSAD kö. Begränsad, inte
 * obegränsad: en kö utan tak är inte en design, det är ett minnesläckage med
 * extra steg. När kön är full måste den som lämnar in vänta — och det är
 * backpressure, systemets enda sätt att säga "jag hinner inte".
 *
 * Två avstängningslägen, för att de svarar på olika frågor:
 *   PARA_POOL_DRAIN  kör klart allt som redan lämnats in. "Vi stänger."
 *   PARA_POOL_NOW    sluta plocka nya jobb, rapportera hur många som aldrig
 *                    kördes. "Det brinner." Antalet är returvärdet, för en
 *                    avstängning som tyst tappar jobb är en bugg med gott
 *                    uppförande.
 *
 * KLART-KRITERIUM (milstolpe 5): 10^6 jobb genom poolen under `make tsan`
 * utan fynd, ren avstängning i båda lägena, och `make asan` rapporterar noll
 * läckta jobb.
 *
 * FÄLLAN du ska framkalla med flit en gång: låt ett jobb i poolen vänta på
 * ett para_future från ett annat jobb i SAMMA pool, med bara en arbetare.
 * Det är en deadlock, testriggens watchdog fångar den, och den har ett namn
 * (thread pool starvation). Modul 12:s work-stealing-schemaläggare är svaret.
 */
#ifndef PARACORE_EXEC_POOL_H
#define PARACORE_EXEC_POOL_H

#include <stddef.h>
#include <core/status.h>
#include <core/task.h>

typedef struct para_pool para_pool;

typedef enum para_pool_shutdown { PARA_POOL_DRAIN = 0, PARA_POOL_NOW } para_pool_shutdown;

/* `workers` = 0 betyder para_hardware_concurrency().
 * `queue_capacity` = 0 är ett fel (PARA_ERR_INVAL), inte "obegränsad". */
para_status para_pool_init(para_pool **out, unsigned workers, size_t queue_capacity);

/* Blockerar när kön är full. */
para_status para_pool_submit(para_pool *p, para_task_fn fn, void *arg);

/* PARA_ERR_FULL i stället för att vänta. Det anropet mäter din backpressure. */
para_status para_pool_try_submit(para_pool *p, para_task_fn fn, void *arg);

/* Som submit, men ger ett future. Anroparen äger det och måste släppa det. */
para_status para_pool_submit_future(para_pool *p, para_task_fn fn, void *arg, para_future **out);

/* Vänta tills kön är tom OCH ingen arbetare kör. Inte samma sak som
 * avstängning — poolen tar emot jobb igen efteråt. */
para_status para_pool_wait_idle(para_pool *p);

/* `unrun` får vara NULL. Efter detta är `p` ogiltig. */
para_status para_pool_shutdown_and_destroy(para_pool *p, para_pool_shutdown mode, size_t *unrun);

unsigned para_pool_worker_count(const para_pool *p);

#endif /* PARACORE_EXEC_POOL_H */
