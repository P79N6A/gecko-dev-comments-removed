








































#include "rccv.h"

#include <prlog.h>
#include <prerror.h>
#include <prcvar.h>

RCCondition::RCCondition(class RCLock *lock): RCBase()
{
    cv = PR_NewCondVar(lock->lock);
    PR_ASSERT(NULL != cv);
    timeout = PR_INTERVAL_NO_TIMEOUT;
}  

RCCondition::~RCCondition()
{
    if (NULL != cv) PR_DestroyCondVar(cv);
}  

PRStatus RCCondition::Wait()
{
    PRStatus rv;
    PR_ASSERT(NULL != cv);
    if (NULL == cv)
    {
        SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        rv = PR_FAILURE;
    }
    else
        rv = PR_WaitCondVar(cv, timeout.interval);
    return rv;
}  

PRStatus RCCondition::Notify()
{
    return PR_NotifyCondVar(cv);
}  

PRStatus RCCondition::Broadcast()
{
    return PR_NotifyAllCondVar(cv);
}  

PRStatus RCCondition::SetTimeout(const RCInterval& tmo)
{
    if (NULL == cv)
    {
        SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        return PR_FAILURE;
    }
    timeout = tmo;
    return PR_SUCCESS;
}  

RCInterval RCCondition::GetTimeout() const { return timeout; }


