






































#include "cpr.h"
#include "cpr_types.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"

extern cpr_status_e cprSocketInit(void);
extern cpr_status_e cprSocketCleanup(void);

#ifdef WIN32_7960
extern void win32_stdio_init(void);
#endif


static boolean pre_init_called = FALSE;


cprRegion_t timerRegion;
cprPool_t timerPool;











CRITICAL_SECTION criticalSection;

cprRC_t
cprPreInit (void)
{
    static const int8_t fname[] = "cprPreInit";

    


    if (pre_init_called equals TRUE) {
        return CPR_SUCCESS;
    }
    pre_init_called = TRUE;

    
    InitializeCriticalSection(&criticalSection);

    
    if (cprSocketInit() == CPR_FAILURE) {
        CPR_ERROR("%s: socket init failed\n", fname);
        return CPR_FAILURE;
    }

    


    timerRegion = cprCreateRegion("CPR - Timers");
    if (timerRegion) {
        timerPool = cprCreatePool(timerRegion, "Timers - 32 bytes", 10, 32);
        if (timerPool == NULL) {
            CPR_ERROR("%s buffer pool creation failed.\n", fname);
            return (CPR_FAILURE);
        }
    } else {
        CPR_ERROR("%s buffer region creation failed.\n", fname);
        return (CPR_FAILURE);
    }

#ifdef WIN32_7960
    win32_stdio_init();
#endif
    return (CPR_SUCCESS);
}











cprRC_t
cprPostInit (void)
{
    



    return (CPR_SUCCESS);

}

