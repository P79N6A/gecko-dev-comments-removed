























































#include "prlog.h"
#include "prinit.h"
#include "prpdce.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_PR_DCETHREADS)

PRIntn failed_already=0;
PRIntn debug_mode=0;

static PRIntn prmain(PRIntn argc, char **argv)
{
    PRStatus rv;
    PRLock *ml = PR_NewLock();
    PRCondVar *cv = PRP_NewNakedCondVar();
    PRIntervalTime tenmsecs = PR_MillisecondsToInterval(10);

    rv = PRP_TryLock(ml);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1; 
    
    rv = PRP_TryLock(ml);
    PR_ASSERT(PR_FAILURE == rv);
    if ((rv != PR_FAILURE) & (!debug_mode)) failed_already=1; 

    rv = PRP_NakedNotify(cv);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1; 

    rv = PRP_NakedBroadcast(cv);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1; 

    rv = PRP_NakedWait(cv, ml, tenmsecs);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1;     

    PR_Unlock(ml);    
        
    rv = PRP_NakedNotify(cv);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1;     

    rv = PRP_NakedBroadcast(cv);
    PR_ASSERT(PR_SUCCESS == rv);
    if ((rv != PR_SUCCESS) & (!debug_mode)) failed_already=1;     

    PRP_DestroyNakedCondVar(cv);
    PR_DestroyLock(ml);

    if (debug_mode) printf("Test succeeded\n");

    return 0;

}  

#endif 

int main(int argc, char **argv)
{

#if defined(_PR_DCETHREADS)
    PR_Initialize(prmain, argc, argv, 0);
    if(failed_already)    
        return 1;
    else
        return 0;
#else
    return 0;
#endif
}  



