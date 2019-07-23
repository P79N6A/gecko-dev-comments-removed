








































#include "rclock.h"
#include <prlog.h>

RCLock::RCLock()
{
    lock = PR_NewLock();  
    PR_ASSERT(NULL != lock);
}  

RCLock::~RCLock()
{
    if (NULL != lock) PR_DestroyLock(lock);
    lock = NULL;
}  

void RCLock::Acquire()
{
    PR_ASSERT(NULL != lock);
    PR_Lock(lock);
}  

void RCLock::Release()
{
    PRStatus rv;
    PR_ASSERT(NULL != lock);
    rv = PR_Unlock(lock);
    PR_ASSERT(PR_SUCCESS == rv);
}  



