
















































#if !defined(prinrval_h)
#define prinrval_h

#include "prtypes.h"

PR_BEGIN_EXTERN_C





typedef PRUint32 PRIntervalTime;









#define PR_INTERVAL_MIN 1000UL
#define PR_INTERVAL_MAX 100000UL












#define PR_INTERVAL_NO_WAIT 0UL
#define PR_INTERVAL_NO_TIMEOUT 0xffffffffUL
























NSPR_API(PRIntervalTime) PR_IntervalNow(void);

















NSPR_API(PRUint32) PR_TicksPerSecond(void);


















NSPR_API(PRIntervalTime) PR_SecondsToInterval(PRUint32 seconds);
NSPR_API(PRIntervalTime) PR_MillisecondsToInterval(PRUint32 milli);
NSPR_API(PRIntervalTime) PR_MicrosecondsToInterval(PRUint32 micro);


















NSPR_API(PRUint32) PR_IntervalToSeconds(PRIntervalTime ticks);
NSPR_API(PRUint32) PR_IntervalToMilliseconds(PRIntervalTime ticks);
NSPR_API(PRUint32) PR_IntervalToMicroseconds(PRIntervalTime ticks);

PR_END_EXTERN_C


#endif 


