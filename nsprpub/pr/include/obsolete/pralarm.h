




























#if !defined(pralarm_h)
#define pralarm_h

#include "prtypes.h"
#include "prinrval.h"


PR_BEGIN_EXTERN_C





typedef struct PRAlarm PRAlarm;
typedef struct PRAlarmID PRAlarmID;

typedef PRBool (PR_CALLBACK *PRPeriodicAlarmFn)(
    PRAlarmID *id, void *clientData, PRUint32 late);























NSPR_API(PRAlarm*) PR_CreateAlarm(void);



















NSPR_API(PRStatus) PR_DestroyAlarm(PRAlarm *alarm);





































NSPR_API(PRAlarmID*) PR_SetAlarm(
    PRAlarm *alarm, PRIntervalTime period, PRUint32 rate,
    PRPeriodicAlarmFn function, void *clientData);























NSPR_API(PRStatus) PR_ResetAlarm(
	PRAlarmID *id, PRIntervalTime period, PRUint32 rate);

PR_END_EXTERN_C

#endif 


