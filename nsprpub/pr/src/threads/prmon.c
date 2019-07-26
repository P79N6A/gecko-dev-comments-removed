




#include "primpl.h"








static void _PR_PostNotifyToMonitor(PRMonitor *mon, PRBool broadcast)
{
    PR_ASSERT(mon != NULL);
    PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mon);

    


    if (broadcast)
        mon->notifyTimes = -1;
    else if (mon->notifyTimes != -1)
        mon->notifyTimes += 1;
}

static void _PR_PostNotifiesFromMonitor(PRCondVar *cv, PRIntn times)
{
    PRStatus rv;

    



    PR_ASSERT(cv != NULL);
    PR_ASSERT(times != 0);
    if (times == -1) {
        rv = PR_NotifyAllCondVar(cv);
        PR_ASSERT(rv == PR_SUCCESS);
    } else {
        while (times-- > 0) {
            rv = PR_NotifyCondVar(cv);
            PR_ASSERT(rv == PR_SUCCESS);
        }
    }
}




PR_IMPLEMENT(PRMonitor*) PR_NewMonitor()
{
    PRMonitor *mon;
    PRStatus rv;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    mon = PR_NEWZAP(PRMonitor);
    if (mon == NULL) {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }

    rv = _PR_InitLock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    if (rv != PR_SUCCESS)
        goto error1;

    mon->owner = NULL;

    rv = _PR_InitCondVar(&mon->entryCV, &mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    if (rv != PR_SUCCESS)
        goto error2;

    rv = _PR_InitCondVar(&mon->waitCV, &mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    if (rv != PR_SUCCESS)
        goto error3;

    mon->notifyTimes = 0;
    mon->entryCount = 0;
    mon->name = NULL;
    return mon;

error3:
    _PR_FreeCondVar(&mon->entryCV);
error2:
    _PR_FreeLock(&mon->lock);
error1:
    PR_Free(mon);
    return NULL;
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
    PR_ASSERT(mon != NULL);
    _PR_FreeCondVar(&mon->waitCV);
    _PR_FreeCondVar(&mon->entryCV);
    _PR_FreeLock(&mon->lock);
#if defined(DEBUG)
    memset(mon, 0xaf, sizeof(PRMonitor));
#endif
    PR_Free(mon);
}




PR_IMPLEMENT(void) PR_EnterMonitor(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRStatus rv;

    PR_ASSERT(mon != NULL);
    PR_Lock(&mon->lock);
    if (mon->entryCount != 0) {
        if (mon->owner == me)
            goto done;
        while (mon->entryCount != 0) {
            rv = PR_WaitCondVar(&mon->entryCV, PR_INTERVAL_NO_TIMEOUT);
            PR_ASSERT(rv == PR_SUCCESS);
        }
    }
    
    PR_ASSERT(mon->notifyTimes == 0);
    PR_ASSERT(mon->owner == NULL);
    mon->owner = me;

done:
    mon->entryCount += 1;
    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
}






PR_IMPLEMENT(PRBool) PR_TestAndEnterMonitor(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRStatus rv;

    PR_ASSERT(mon != NULL);
    PR_Lock(&mon->lock);
    if (mon->entryCount != 0) {
        if (mon->owner == me)
            goto done;
        rv = PR_Unlock(&mon->lock);
        PR_ASSERT(rv == PR_SUCCESS);
        return PR_FALSE;
    }
    
    PR_ASSERT(mon->notifyTimes == 0);
    PR_ASSERT(mon->owner == NULL);
    mon->owner = me;

done:
    mon->entryCount += 1;
    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    return PR_TRUE;
}




PR_IMPLEMENT(PRStatus) PR_ExitMonitor(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRStatus rv;

    PR_ASSERT(mon != NULL);
    PR_Lock(&mon->lock);
    
    PR_ASSERT(mon->entryCount > 0);
    PR_ASSERT(mon->owner == me);
    if (mon->entryCount == 0 || mon->owner != me)
    {
        rv = PR_Unlock(&mon->lock);
        PR_ASSERT(rv == PR_SUCCESS);
        return PR_FAILURE;
    }

    mon->entryCount -= 1;  
    if (mon->entryCount == 0)
    {
        
        
        mon->owner = NULL;
        if (mon->notifyTimes != 0) {
            _PR_PostNotifiesFromMonitor(&mon->waitCV, mon->notifyTimes);
            mon->notifyTimes = 0;
        }
        rv = PR_NotifyCondVar(&mon->entryCV);
        PR_ASSERT(rv == PR_SUCCESS);
    }
    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    return PR_SUCCESS;
}





PR_IMPLEMENT(PRIntn) PR_GetMonitorEntryCount(PRMonitor *mon)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRStatus rv;
    PRIntn count = 0;

    PR_Lock(&mon->lock);
    if (mon->owner == me)
        count = mon->entryCount;
    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    return count;
}

PR_IMPLEMENT(void) PR_AssertCurrentThreadInMonitor(PRMonitor *mon)
{
#if defined(DEBUG) || defined(FORCE_PR_ASSERT)
    PRStatus rv;

    PR_Lock(&mon->lock);
    PR_ASSERT(mon->entryCount != 0 &&
              mon->owner == _PR_MD_CURRENT_THREAD());
    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
#endif
}


















PR_IMPLEMENT(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime ticks)
{
    PRStatus rv;
    PRUint32 saved_entries;
    PRThread *saved_owner;

    PR_ASSERT(mon != NULL);
    PR_Lock(&mon->lock);
    
    PR_ASSERT(mon->entryCount > 0);
    
    PR_ASSERT(mon->owner == _PR_MD_CURRENT_THREAD());  

    
    saved_entries = mon->entryCount;
    mon->entryCount = 0;
    saved_owner = mon->owner;
    mon->owner = NULL;
    
    if (mon->notifyTimes != 0) {
        _PR_PostNotifiesFromMonitor(&mon->waitCV, mon->notifyTimes);
        mon->notifyTimes = 0;
    }
    rv = PR_NotifyCondVar(&mon->entryCV);
    PR_ASSERT(rv == PR_SUCCESS);

    rv = PR_WaitCondVar(&mon->waitCV, ticks);
    PR_ASSERT(rv == PR_SUCCESS);

    while (mon->entryCount != 0) {
        rv = PR_WaitCondVar(&mon->entryCV, PR_INTERVAL_NO_TIMEOUT);
        PR_ASSERT(rv == PR_SUCCESS);
    }
    PR_ASSERT(mon->notifyTimes == 0);
    
    mon->entryCount = saved_entries;
    mon->owner = saved_owner;

    rv = PR_Unlock(&mon->lock);
    PR_ASSERT(rv == PR_SUCCESS);
    return rv;
}






PR_IMPLEMENT(PRStatus) PR_Notify(PRMonitor *mon)
{
    _PR_PostNotifyToMonitor(mon, PR_FALSE);
    return PR_SUCCESS;
}






PR_IMPLEMENT(PRStatus) PR_NotifyAll(PRMonitor *mon)
{
    _PR_PostNotifyToMonitor(mon, PR_TRUE);
    return PR_SUCCESS;
}



PRUint32 _PR_MonitorToString(PRMonitor *mon, char *buf, PRUint32 buflen)
{
    PRUint32 nb;

    if (mon->owner) {
	nb = PR_snprintf(buf, buflen, "[%p] owner=%d[%p] count=%ld",
			 mon, mon->owner->id, mon->owner, mon->entryCount);
    } else {
	nb = PR_snprintf(buf, buflen, "[%p]", mon);
    }
    return nb;
}
