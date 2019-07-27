



#include "cpr_types.h"
#include "cpr_locks.h"
#include "cpr_win_locks.h" 
#include "cpr_debug.h"
#include "cpr_memory.h"
#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include <windows.h>


extern CRITICAL_SECTION criticalSection;












void
cprDisableSwap (void)
{
    EnterCriticalSection(&criticalSection);
}











void
cprEnableSwap (void)
{
    LeaveCriticalSection(&criticalSection);
}
