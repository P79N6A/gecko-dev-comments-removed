




































#if defined(_PRMWAIT_H)
#else
#define _PRMWAIT_H

#include "prio.h"
#include "prtypes.h"
#include "prclist.h"

PR_BEGIN_EXTERN_C






















typedef struct PRWaitGroup PRWaitGroup;























typedef enum PRMWStatus
{
    PR_MW_PENDING = 1,
    PR_MW_SUCCESS = 0,
    PR_MW_FAILURE = -1,
    PR_MW_TIMEOUT = -2,
    PR_MW_INTERRUPT = -3
} PRMWStatus;








typedef struct PRMemoryDescriptor
{
    void *start;                
    PRSize length;              
} PRMemoryDescriptor;








typedef struct PRMWaitClientData PRMWaitClientData;

















typedef struct PRRecvWait 
{
    PRCList internal;           

    PRFileDesc *fd;             
    PRMWStatus outcome;         
    PRIntervalTime timeout;     

    PRInt32 bytesRecv;          
    PRMemoryDescriptor buffer;  
    PRMWaitClientData *client;  
} PRRecvWait;










typedef struct PRMWaitEnumerator PRMWaitEnumerator;

































NSPR_API(PRStatus) PR_AddWaitFileDesc(PRWaitGroup *group, PRRecvWait *desc);

































NSPR_API(PRRecvWait*) PR_WaitRecvReady(PRWaitGroup *group);


































NSPR_API(PRStatus) PR_CancelWaitFileDesc(PRWaitGroup *group, PRRecvWait *desc);


























NSPR_API(PRRecvWait*) PR_CancelWaitGroup(PRWaitGroup *group);


























NSPR_API(PRWaitGroup*) PR_CreateWaitGroup(PRInt32 size);






















NSPR_API(PRStatus) PR_DestroyWaitGroup(PRWaitGroup *group);





















NSPR_API(PRMWaitEnumerator*) PR_CreateMWaitEnumerator(PRWaitGroup *group);

















NSPR_API(PRStatus) PR_DestroyMWaitEnumerator(PRMWaitEnumerator* enumerator);






















NSPR_API(PRRecvWait*) PR_EnumerateWaitGroup(
    PRMWaitEnumerator *enumerator, const PRRecvWait *previous);
   
PR_END_EXTERN_C

#endif 


