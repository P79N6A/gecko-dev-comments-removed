










































#include "prolock.h"
#include "prlog.h"
#include "prerror.h"

PR_IMPLEMENT(PROrderedLock *) 
    PR_CreateOrderedLock( 
        PRInt32 order,
        const char *name
)
{
#ifdef XP_MAC
#pragma unused( order, name )
#endif
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
} 


PR_IMPLEMENT(void) 
    PR_DestroyOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} 


PR_IMPLEMENT(void) 
    PR_LockOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
} 


PR_IMPLEMENT(PRStatus) 
    PR_UnlockOrderedLock( 
        PROrderedLock *lock 
)
{
#ifdef XP_MAC
#pragma unused( lock )
#endif
    PR_ASSERT(!"Not implemented"); 
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
} 
