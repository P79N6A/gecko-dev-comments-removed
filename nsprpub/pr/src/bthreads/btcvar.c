




































#include <kernel/OS.h>

#include "primpl.h"












PR_IMPLEMENT(PRCondVar*)
    PR_NewCondVar (PRLock *lock)
{
    PRCondVar *cv = PR_NEW( PRCondVar );
    PR_ASSERT( NULL != lock );
    if( NULL != cv )
    {
	cv->lock = lock;
	cv->sem = create_sem(0, "CVSem");
	cv->handshakeSem = create_sem(0, "CVHandshake");
	cv->signalSem = create_sem( 0, "CVSignal");
	cv->signalBenCount = 0;
	cv->ns = cv->nw = 0;
	PR_ASSERT( cv->sem >= B_NO_ERROR );
	PR_ASSERT( cv->handshakeSem >= B_NO_ERROR );
	PR_ASSERT( cv->signalSem >= B_NO_ERROR );
    }
    return cv;
} 







PR_IMPLEMENT(void)
    PR_DestroyCondVar (PRCondVar *cvar)
{
    status_t result = delete_sem( cvar->sem );
    PR_ASSERT( result == B_NO_ERROR );
    
    result = delete_sem( cvar->handshakeSem );
    PR_ASSERT( result == B_NO_ERROR );

    result = delete_sem( cvar->signalSem );
    PR_ASSERT( result == B_NO_ERROR );

    PR_DELETE( cvar );
}





























PR_IMPLEMENT(PRStatus)
    PR_WaitCondVar (PRCondVar *cvar, PRIntervalTime timeout)
{
    status_t err;
    if( timeout == PR_INTERVAL_NO_WAIT ) 
    {
        PR_Unlock( cvar->lock );
        PR_Lock( cvar->lock );
        return PR_SUCCESS;
    }

    if( atomic_add( &cvar->signalBenCount, 1 ) > 0 ) 
    {
        if (acquire_sem(cvar->signalSem) == B_INTERRUPTED) 
        {
            atomic_add( &cvar->signalBenCount, -1 );
            return PR_FAILURE;
        }
    }
    cvar->nw += 1;
    if( atomic_add( &cvar->signalBenCount, -1 ) > 1 ) 
    {
        release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
    }

    PR_Unlock( cvar->lock );
    if( timeout==PR_INTERVAL_NO_TIMEOUT ) 
    {
    	err = acquire_sem(cvar->sem);
    } 
    else 
    {
    	err = acquire_sem_etc(cvar->sem, 1, B_RELATIVE_TIMEOUT, PR_IntervalToMicroseconds(timeout) );
    }

    if( atomic_add( &cvar->signalBenCount, 1 ) > 0 ) 
    {
        while (acquire_sem(cvar->signalSem) == B_INTERRUPTED);
    }

    if (cvar->ns > 0)
    {
        release_sem_etc(cvar->handshakeSem, 1, B_DO_NOT_RESCHEDULE);
        cvar->ns -= 1;
    }
    cvar->nw -= 1;
    if( atomic_add( &cvar->signalBenCount, -1 ) > 1 ) 
    {
        release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
    }

    PR_Lock( cvar->lock );
    if(err!=B_NO_ERROR) 
    {
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}














PR_IMPLEMENT(PRStatus)
    PR_NotifyCondVar (PRCondVar *cvar)
{
    status_t err ;
    if( atomic_add( &cvar->signalBenCount, 1 ) > 0 ) 
    {
        if (acquire_sem(cvar->signalSem) == B_INTERRUPTED) 
        {
            atomic_add( &cvar->signalBenCount, -1 );
            return PR_FAILURE;
        }
    }
    if (cvar->nw > cvar->ns)
    {
        cvar->ns += 1;
        release_sem_etc(cvar->sem, 1, B_DO_NOT_RESCHEDULE);
        if( atomic_add( &cvar->signalBenCount, -1 ) > 1 ) 
        {
            release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
        }

        while (acquire_sem(cvar->handshakeSem) == B_INTERRUPTED) 
        {
            err = B_INTERRUPTED; 
        }
    }
    else
    {
        if( atomic_add( &cvar->signalBenCount, -1 ) > 1 )
        {
            release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
        }
    }
    return PR_SUCCESS; 
}









PR_IMPLEMENT(PRStatus)
    PR_NotifyAllCondVar (PRCondVar *cvar)
{
    int32 handshakes;
    status_t err = B_OK;

    if( atomic_add( &cvar->signalBenCount, 1 ) > 0 ) 
    {
        if (acquire_sem(cvar->signalSem) == B_INTERRUPTED) 
        {
            atomic_add( &cvar->signalBenCount, -1 );
            return PR_FAILURE;
        }
    }

    if (cvar->nw > cvar->ns)
    {
        handshakes = cvar->nw - cvar->ns;
        cvar->ns = cvar->nw;				
        release_sem_etc(cvar->sem, handshakes, B_DO_NOT_RESCHEDULE);	
        if( atomic_add( &cvar->signalBenCount, -1 ) > 1 ) 
        {
            release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
        }

        while (acquire_sem_etc(cvar->handshakeSem, handshakes, 0, 0) == B_INTERRUPTED) 
        {
            err = B_INTERRUPTED; 
        }
    }
    else
    {
        if( atomic_add( &cvar->signalBenCount, -1 ) > 1 ) 
        {
            release_sem_etc(cvar->signalSem, 1, B_DO_NOT_RESCHEDULE);
        }
    }
    return PR_SUCCESS;
}
