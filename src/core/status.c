#include <core/status.h>

static _Thread_local int g_last_os_error;

void para_set_os_error(int err); /* intern; se src/core/internal.h */

void para_set_os_error(int err) {
    g_last_os_error = err;
}

int para_last_os_error(void) {
    return g_last_os_error;
}

const char *para_strerror(para_status st) {
    switch (st) {
    case PARA_OK:
        return "ok";
    case PARA_ERR_INVAL:
        return "ogiltigt argument";
    case PARA_ERR_NOMEM:
        return "slut på minne";
    case PARA_ERR_AGAIN:
        return "inte just nu, försök igen";
    case PARA_ERR_BUSY:
        return "upptagen";
    case PARA_ERR_TIMEDOUT:
        return "tidsgränsen gick ut";
    case PARA_ERR_CLOSED:
        return "stängd";
    case PARA_ERR_FULL:
        return "full";
    case PARA_ERR_EMPTY:
        return "tom";
    case PARA_ERR_NOTFOUND:
        return "hittades inte";
    case PARA_ERR_OS:
        return "systemanropet misslyckades";
    case PARA_ERR_NOTIMPL:
        return "inte byggd ännu (se modulen i huvudfilen)";
    }
    return "okänd status";
}
