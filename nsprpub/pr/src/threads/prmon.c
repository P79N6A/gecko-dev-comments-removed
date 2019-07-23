




































#include "primpl.h"






PR_IMPLEMENT(PRMonitor*) PR_NewMonitor()
{
    PRMonitor *mon;
	PRCondVar *cvar;
	PRLock *lock;

    mon = PR_NEWZAP(PRMonitor);
    if (mon) {
		lock = PR_NewLock();
	    if (!lock) {
			PR_DELETE(mon);
			return 0;
    	}

	    cvar = PR_NewCondVar(lock);
	    if (!cvar) {
	    	PR_DestroyLock(lock);
			PR_DELETE(mon);
			return 0;
    	}
    	mon->cvar = cvar;
	mon->name = NULL;
    }
    return mon;
}

PR_IMPLEMENT(PRMonitor*) PR_NewNamedMonitor(const char* name)
{
    PRMonitor* mon = PR_NewMonitor();
    if (mon)
        mon->name = name;
    return mon;
}






PR_IMPLEMENT(void) PR_DestroyMonitor(PRMonitor *mon)
{
	PR_DestroyLock(mon->cvar->lock);
    PR_DestroyCondVar(mon->cvar);
    PR_DELETE(mon);
}




PR_IMPLEMENT(void) PR_EnterMonitor(PRMonitor *mon)
{
    if (mon->cvar->lock->owner == _PR_MD_CURRENT_THREAD()) {
		mon->entryCount++;
    } else {
		PR_Lock(mon->cvar->lock);
		mon->entryCount = 1;
    }
}






PR_IMPLEMENT(PRBool) PR_TestAndEnterMonitor(PRMonitor *mon)
{
    if (mon->cvar->lock->owner == _PR_MD_CURRENT_THREAD()) {
		mon->entryCount++;
		return PR_TRUE;
    } else {
		if (PR_TestAndLock(mon->cvar->lock)) {
	    	mon->entryCount = 1;
	   	 	return PR_TRUE;
		}
    }
    return PR_FALSE;
}




PR_IMPLEMENT(PRStatus) PR_ExitMonitor(PRMonitor *mon)
{
    if (mon->cvar->lock->owner != _PR_MD_CURRENT_THREAD()) {
        return PR_FAILURE;
    }
    if (--mon->entryCount == 0) {
		return PR_Unlock(mon->cvar->lock);
    }
    return PR_SUCCESS;
}





PR_IMPLEMENT(PRIntn) PR_GetMonitorEntryCount(PRMonitor *mon)
{
    return (mon->cvar->lock->owner == _PR_MD_CURRENT_THREAD()) ?
        mon->entryCount : 0;
}





PR_IMPLEMENT(void) PR_AssertCurrentThreadInMonitor(PRMonitor *mon)
{
    PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(mon->cvar->lock);
}


















PR_IMPLEMENT(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime ticks)
{
    PRUintn entryCount;
	PRStatus status;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (mon->cvar->lock->owner != me) return PR_FAILURE;

    entryCount = mon->entryCount;
    mon->entryCount = 0;

	status = _PR_WaitCondVar(me, mon->cvar, mon->cvar->lock, ticks);

    mon->entryCount = entryCount;

    return status;
}






PR_IMPLEMENT(PRStatus) PR_Notify(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    if (mon->cvar->lock->owner != me) return PR_FAILURE;
    PR_NotifyCondVar(mon->cvar);
    return PR_SUCCESS;
}






PR_IMPLEMENT(PRStatus) PR_NotifyAll(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    if (mon->cvar->lock->owner != me) return PR_FAILURE;
    PR_NotifyAllCondVar(mon->cvar);
    return PR_SUCCESS;
}



PRUint32 _PR_MonitorToString(PRMonitor *mon, char *buf, PRUint32 buflen)
{
    PRUint32 nb;

    if (mon->cvar->lock->owner) {
	nb = PR_snprintf(buf, buflen, "[%p] owner=%d[%p] count=%ld",
			 mon, mon->cvar->lock->owner->id,
			 mon->cvar->lock->owner, mon->entryCount);
    } else {
	nb = PR_snprintf(buf, buflen, "[%p]", mon);
    }
    return nb;
}
