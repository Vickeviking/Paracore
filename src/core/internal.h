/* Internt för src/core. Ligger medvetet INTE i core/ — det som ligger i
 * core/ är publikt, och gränsen ska gå att se i filträdet. */
#ifndef PARACORE_SRC_CORE_INTERNAL_H
#define PARACORE_SRC_CORE_INTERNAL_H

#include <core/status.h>

void para_set_os_error(int err);

/* errno -> para_status, och spara den råa koden för felsökning. */
para_status para_from_errno(int err);

#endif
