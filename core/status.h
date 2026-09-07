/* core/status.h — Paracores felmodell.
 *
 * En enda enum genom hela biblioteket. Inget errno-läckage ut genom det
 * publika API:t: en anropare ska aldrig behöva veta att det var pthread
 * under. Varje funktion som kan misslyckas returnerar para_status, och
 * utdata går via pekarargument.
 *
 * Regeln: PARA_OK är 0, allt annat är negativt. `if (st != PARA_OK)` är
 * den enda kontroll som finns; `if (!st)` är förbjuden av samma skäl som
 * `if (!strcmp())` är det — den läser fel.
 */
#ifndef PARACORE_CORE_STATUS_H
#define PARACORE_CORE_STATUS_H

typedef enum para_status {
    PARA_OK = 0,
    PARA_ERR_INVAL = -1,    /* ogiltigt argument (NULL, 0 trådar, ...) */
    PARA_ERR_NOMEM = -2,    /* allokering misslyckades */
    PARA_ERR_AGAIN = -3,    /* resursen fanns inte just nu; försök igen */
    PARA_ERR_BUSY = -4,     /* upptagen (trylock som inte fick låset) */
    PARA_ERR_TIMEDOUT = -5, /* tidsgränsen gick ut */
    PARA_ERR_CLOSED = -6,   /* kön/poolen är stängd för nya jobb */
    PARA_ERR_FULL = -7,     /* begränsad kö full och anroparen ville inte vänta */
    PARA_ERR_EMPTY = -8,    /* inget att hämta */
    PARA_ERR_NOTFOUND = -9, /* nyckeln finns inte */
    PARA_ERR_OS = -10,      /* systemanropet sa nej; se para_last_os_error() */
    PARA_ERR_NOTIMPL = -99  /* du har inte byggt den här ännu. Det är meningen. */
} para_status;

/* Läsbar text för en status. Aldrig NULL, aldrig allokerad. */
const char *para_strerror(para_status st);

/* Den råa errno-koden bakom det senaste PARA_ERR_OS på DENNA tråd.
 * Finns för felsökning och felmeddelanden — inte för kontrollflöde. */
int para_last_os_error(void);

#endif /* PARACORE_CORE_STATUS_H */
