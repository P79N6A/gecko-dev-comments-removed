









































#include "primpl.h"

#ifdef WINCE
typedef DWORD (*IntervalFuncType)(void);
static IntervalFuncType intervalFunc;
#endif

void
_PR_MD_INTERVAL_INIT()
{
#ifdef WINCE
    HMODULE mmtimerlib = LoadLibraryW(L"mmtimer.dll");  
    if (mmtimerlib) {
        intervalFunc = (IntervalFuncType)GetProcAddress(mmtimerlib,
                                                        "timeGetTime");
    } else {
        intervalFunc = &GetTickCount;
    }
#endif
}

PRIntervalTime 
_PR_MD_GET_INTERVAL()
{
    
#ifdef WINCE
    return (*intervalFunc)();
#else
    return timeGetTime();
#endif
}

PRIntervalTime 
_PR_MD_INTERVAL_PER_SEC()
{
    return 1000;
}
