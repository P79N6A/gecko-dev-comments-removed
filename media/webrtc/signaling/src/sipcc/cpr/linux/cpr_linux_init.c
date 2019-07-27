














































































































#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_timers.h"
#include "cpr_linux_timers.h"
#include "plat_api.h"
#include <errno.h>
#if defined(WEBRTC_GONK)
#include <linux/msg.h>
#include <linux/ipc.h>
#else
#include <sys/msg.h>
#include <sys/ipc.h>
#endif
#include "plat_debug.h"




static boolean pre_init_called = FALSE;








int32_t cprInfo = TRUE;















cprRC_t
cprPreInit (void)
{
    static const char fname[] = "cprPreInit";
    int32_t returnCode;

    


    if (pre_init_called == TRUE) {
        return CPR_SUCCESS;
    }
    pre_init_called = TRUE;
    (void)fname;
    (void)returnCode;
    return CPR_SUCCESS;
}

















cprRC_t
cprPostInit (void)
{




    return CPR_SUCCESS;
}

