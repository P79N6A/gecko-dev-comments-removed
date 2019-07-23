




































#include "primpl.h"
#include "obsolete/prsem.h"






PR_IMPLEMENT(PRSemaphore*) PR_NewSem(PRUintn value)
{
    PRSemaphore *sem;
    PRCondVar *cvar;
    PRLock *lock;

    sem = PR_NEWZAP(PRSemaphore);
    if (sem) {
#ifdef HAVE_CVAR_BUILT_ON_SEM
        _PR_MD_NEW_SEM(&sem->md, value);
#else
        lock = PR_NewLock();
        if (!lock) {
            PR_DELETE(sem);
            return NULL;
    	}

        cvar = PR_NewCondVar(lock);
        if (!cvar) {
            PR_DestroyLock(lock);
            PR_DELETE(sem);
            return NULL;
    	}
    	sem->cvar = cvar;
    	sem->count = value;
#endif
    }
    return sem;
}






PR_IMPLEMENT(void) PR_DestroySem(PRSemaphore *sem)
{
#ifdef HAVE_CVAR_BUILT_ON_SEM
    _PR_MD_DESTROY_SEM(&sem->md);
#else
    PR_ASSERT(sem->waiters == 0);

    PR_DestroyLock(sem->cvar->lock);
    PR_DestroyCondVar(sem->cvar);
#endif
    PR_DELETE(sem);
}















PR_IMPLEMENT(PRStatus) PR_WaitSem(PRSemaphore *sem)
{
	PRStatus status = PR_SUCCESS;

#ifdef HAVE_CVAR_BUILT_ON_SEM
	return _PR_MD_WAIT_SEM(&sem->md);
#else
	PR_Lock(sem->cvar->lock);
	while (sem->count == 0) {
		sem->waiters++;
		status = PR_WaitCondVar(sem->cvar, PR_INTERVAL_NO_TIMEOUT);
		sem->waiters--;
		if (status != PR_SUCCESS)
			break;
	}
	if (status == PR_SUCCESS)
		sem->count--;
	PR_Unlock(sem->cvar->lock);
#endif
	
	return (status);
}






PR_IMPLEMENT(void) PR_PostSem(PRSemaphore *sem)
{
#ifdef HAVE_CVAR_BUILT_ON_SEM
	_PR_MD_POST_SEM(&sem->md);
#else
	PR_Lock(sem->cvar->lock);
	if (sem->waiters)
		PR_NotifyCondVar(sem->cvar);
	sem->count++;
	PR_Unlock(sem->cvar->lock);
#endif
}

#if DEBUG






PR_IMPLEMENT(PRUintn) PR_GetValueSem(PRSemaphore *sem)
{
	PRUintn rv;

#ifdef HAVE_CVAR_BUILT_ON_SEM
	rv = _PR_MD_GET_VALUE_SEM(&sem->md);
#else
	PR_Lock(sem->cvar->lock);
	rv = sem->count;
	PR_Unlock(sem->cvar->lock);
#endif
	
	return rv;
}
#endif
