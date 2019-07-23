




































#include <kernel/OS.h>

#include "primpl.h"








PR_IMPLEMENT(PRMonitor*)
    PR_NewMonitor (void)
{
    PRMonitor *mon;
    PRCondVar *cvar;
    PRLock    *lock;

    mon = PR_NEWZAP( PRMonitor );
    if( mon )
    {
	lock = PR_NewLock();
	if( !lock )
        {
	    PR_DELETE( mon );
	    return( 0 );
	}

	cvar = PR_NewCondVar( lock );
	if( !cvar )
	{
	    PR_DestroyLock( lock );
	    PR_DELETE( mon );
	    return( 0 );
	}

	mon->cvar = cvar;
	mon->name = NULL;
    }

    return( mon );
}

PR_IMPLEMENT(PRMonitor*) PR_NewNamedMonitor(const char* name)
{
    PRMonitor* mon = PR_NewMonitor();
    if( mon )
    {
        mon->name = name;
    }
    return mon;
}







PR_IMPLEMENT(void)
    PR_DestroyMonitor (PRMonitor *mon)
{
    PR_DestroyLock( mon->cvar->lock );
    PR_DestroyCondVar( mon->cvar );
    PR_DELETE( mon );
}






PR_IMPLEMENT(void)
    PR_EnterMonitor (PRMonitor *mon)
{
    if( mon->cvar->lock->owner == find_thread( NULL ) )
    {
	mon->entryCount++;

    } else
    {
	PR_Lock( mon->cvar->lock );
	mon->entryCount = 1;
    }
}






PR_IMPLEMENT(PRStatus)
    PR_ExitMonitor (PRMonitor *mon)
{
    if( mon->cvar->lock->owner != find_thread( NULL ) )
    {
	return( PR_FAILURE );
    }
    if( --mon->entryCount == 0 )
    {
	return( PR_Unlock( mon->cvar->lock ) );
    }
    return( PR_SUCCESS );
}

















PR_IMPLEMENT(PRStatus)
    PR_Wait (PRMonitor *mon, PRIntervalTime ticks)
{
    PRUint32 entryCount;
    PRUintn  status;
    PRThread *meThread;
    thread_id me = find_thread( NULL );
    meThread = PR_GetCurrentThread();

    if( mon->cvar->lock->owner != me ) return( PR_FAILURE );

    entryCount = mon->entryCount;
    mon->entryCount = 0;

    status = PR_WaitCondVar( mon->cvar, ticks );

    mon->entryCount = entryCount;

    return( status );
}






PR_IMPLEMENT(PRStatus)
    PR_Notify (PRMonitor *mon)
{
    if( mon->cvar->lock->owner != find_thread( NULL ) )
    {
	return( PR_FAILURE );
    }

    PR_NotifyCondVar( mon->cvar );
    return( PR_SUCCESS );
}






PR_IMPLEMENT(PRStatus)
    PR_NotifyAll (PRMonitor *mon)
{
    if( mon->cvar->lock->owner != find_thread( NULL ) )
    {
	return( PR_FAILURE );
    }

    PR_NotifyAllCondVar( mon->cvar );
    return( PR_SUCCESS );
}





PR_IMPLEMENT(PRIntn)
    PR_GetMonitorEntryCount(PRMonitor *mon)
{
    return( (mon->cvar->lock->owner == find_thread( NULL )) ?
            mon->entryCount : 0 );
}





PR_IMPLEMENT(void)
    PR_AssertCurrentThreadInMonitor(PRMonitor *mon)
{
    PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(mon->cvar->lock);
}
