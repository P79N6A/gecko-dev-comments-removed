










































#include "prolock.h"
#include "prlog.h"
#include "prerror.h"

PR_IMPLEMENT(PROrderedLock *) 
    PR_CreateOrderedLock( 
        PRInt32 order,
        const char *name
)
{
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
} 


PR_IMPLEMENT(void) 
    PR_DestroyOrderedLock( 
        PROrderedLock *lock 
)
{
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} 


PR_IMPLEMENT(void) 
    PR_LockOrderedLock( 
        PROrderedLock *lock 
)
{
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} 


PR_IMPLEMENT(PRStatus) 
    PR_UnlockOrderedLock( 
        PROrderedLock *lock 
)
{
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
} 
