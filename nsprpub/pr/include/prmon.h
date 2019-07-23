




































#ifndef prmon_h___
#define prmon_h___

#include "prtypes.h"
#include "prinrval.h"

PR_BEGIN_EXTERN_C

typedef struct PRMonitor PRMonitor;








NSPR_API(PRMonitor*) PR_NewMonitor(void);







NSPR_API(void) PR_DestroyMonitor(PRMonitor *mon);






NSPR_API(void) PR_EnterMonitor(PRMonitor *mon);






NSPR_API(PRStatus) PR_ExitMonitor(PRMonitor *mon);

















NSPR_API(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime ticks);






NSPR_API(PRStatus) PR_Notify(PRMonitor *mon);






NSPR_API(PRStatus) PR_NotifyAll(PRMonitor *mon);






#if defined(DEBUG) || defined(FORCE_PR_ASSERT)
#define PR_ASSERT_CURRENT_THREAD_IN_MONITOR( mon) \
    PR_AssertCurrentThreadInMonitor(mon)
#else
#define PR_ASSERT_CURRENT_THREAD_IN_MONITOR( mon)
#endif


NSPR_API(void) PR_AssertCurrentThreadInMonitor(PRMonitor *mon);

PR_END_EXTERN_C

#endif 
