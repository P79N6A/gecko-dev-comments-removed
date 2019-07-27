








































































































#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_timers.h"
#include "cpr_darwin_timers.h"
#include "plat_api.h"
#include "plat_debug.h"

#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>




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
    




    debug_bind_keyword("cpr-info", &cprInfo);

    return CPR_SUCCESS;
}

