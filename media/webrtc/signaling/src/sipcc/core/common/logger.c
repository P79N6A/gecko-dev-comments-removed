



#include "cpr_types.h"
#include "cpr_string.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "stdarg.h"
#include "logger.h"
#include "logmsg.h"
#include "phone_debug.h"
#include "text_strings.h"
#include "uiapi.h"
#include "platform_api.h"
#include "prot_configmgr.h"

#define MAX_LOG_CACHE_ENTRIES 20







void
log_msg (int phrase_index, ...)
{
    char phrase_buf[LOG_MAX_LEN * 4];
    char status_msg[LOG_MAX_LEN * 4];
    va_list ap;

    


    if (phrase_index == 0) {
        return;
    }

    


    if (platGetPhraseText(phrase_index, phrase_buf, (LOG_MAX_LEN * 4)) == CPR_FAILURE) {
        return;
    }

    


    va_start(ap, phrase_index);
    vsprintf(status_msg, phrase_buf, ap);
    va_end(ap);

    err_msg("%%%s\n", status_msg);

    





    switch (phrase_index) {
    case LOG_REG_MSG:
    case LOG_REG_RED_MSG:
    case LOG_REG_AUTH_MSG:
    case LOG_REG_AUTH_HDR_MSG:
    case LOG_REG_AUTH_SCH_MSG:
    case LOG_REG_CANCEL_MSG:
    case LOG_REG_AUTH:
    case LOG_REG_AUTH_ACK_TMR:
    case LOG_REG_AUTH_NO_CRED:
    case LOG_REG_AUTH_UNREG_TMR:
    case LOG_REG_RETRY:
    case LOG_REG_UNSUPPORTED:
    case LOG_REG_AUTH_SERVER_ERR:
    case LOG_REG_AUTH_GLOBAL_ERR:
    case LOG_REG_AUTH_UNKN_ERR:
        return;

    default:
        break;
    }

    ui_log_status_msg(status_msg);
}


