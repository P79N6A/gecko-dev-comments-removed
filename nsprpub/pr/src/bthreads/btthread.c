




































#include <kernel/OS.h>
#include <support/TLS.h>

#include "prlog.h"
#include "primpl.h"
#include "prcvar.h"
#include "prpdce.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>


#define BT_THREAD_PRIMORD   0x01    /* this is the primordial thread */
#define BT_THREAD_SYSTEM    0x02    /* this is a system thread */
#define BT_THREAD_JOINABLE  0x04	/* this is a joinable thread */

struct _BT_Bookeeping
{
    PRLock *ml;                 
	sem_id cleanUpSem;		

    PRInt32 threadCount;	

} bt_book = { NULL, B_ERROR, 0 };


#define BT_TPD_LIMIT 128	/* number of TPD slots we'll provide (arbitrary) */





static int32 tpd_beosTLSSlots[BT_TPD_LIMIT];
static PRThreadPrivateDTOR tpd_dtors[BT_TPD_LIMIT];

static vint32 tpd_slotsUsed=0;	
static int32 tls_prThreadSlot;	







static PRLock *joinSemLock;

static PRUint32 _bt_MapNSPRToNativePriority( PRThreadPriority priority );
static PRThreadPriority _bt_MapNativeToNSPRPriority( PRUint32 priority );
static void _bt_CleanupThread(void *arg);
static PRThread *_bt_AttachThread();

void
_PR_InitThreads (PRThreadType type, PRThreadPriority priority,
                 PRUintn maxPTDs)
{
    PRThread *primordialThread;
    PRUint32  beThreadPriority;

	
	joinSemLock = PR_NewLock();
	if (joinSemLock == NULL)
	{
		PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
		return;
    }

    


    
    primordialThread = PR_NEWZAP(PRThread);
    if( NULL == primordialThread )
    {
        PR_SetError( PR_OUT_OF_MEMORY_ERROR, 0 );
        return;
    }

	primordialThread->md.joinSem = B_ERROR;

    



    beThreadPriority = _bt_MapNSPRToNativePriority( priority );
    
    set_thread_priority( find_thread( NULL ), beThreadPriority );
    
    primordialThread->priority = priority;


	
    primordialThread->state |= BT_THREAD_PRIMORD;
	if (type == PR_SYSTEM_THREAD)
		primordialThread->state |= BT_THREAD_SYSTEM;

    




	
	tls_prThreadSlot = tls_allocate();

    




	tls_set(tls_prThreadSlot, primordialThread);
    
	
    bt_book.ml = PR_NewLock();
    if( NULL == bt_book.ml )
    {
    	PR_SetError( PR_OUT_OF_MEMORY_ERROR, 0 );
	return;
    }
}

PRUint32
_bt_MapNSPRToNativePriority( PRThreadPriority priority )
    {
    switch( priority )
    {
    	case PR_PRIORITY_LOW:	 return( B_LOW_PRIORITY );
	case PR_PRIORITY_NORMAL: return( B_NORMAL_PRIORITY );
	case PR_PRIORITY_HIGH:	 return( B_DISPLAY_PRIORITY );
	case PR_PRIORITY_URGENT: return( B_URGENT_DISPLAY_PRIORITY );
	default:		 return( B_NORMAL_PRIORITY );
    }
}

PRThreadPriority
_bt_MapNativeToNSPRPriority(PRUint32 priority)
    {
	if (priority < B_NORMAL_PRIORITY)
		return PR_PRIORITY_LOW;
	if (priority < B_DISPLAY_PRIORITY)
		return PR_PRIORITY_NORMAL;
	if (priority < B_URGENT_DISPLAY_PRIORITY)
		return PR_PRIORITY_HIGH;
	return PR_PRIORITY_URGENT;
}

PRUint32
_bt_mapNativeToNSPRPriority( int32 priority )
{
    switch( priority )
    {
    	case PR_PRIORITY_LOW:	 return( B_LOW_PRIORITY );
	case PR_PRIORITY_NORMAL: return( B_NORMAL_PRIORITY );
	case PR_PRIORITY_HIGH:	 return( B_DISPLAY_PRIORITY );
	case PR_PRIORITY_URGENT: return( B_URGENT_DISPLAY_PRIORITY );
	default:		 return( B_NORMAL_PRIORITY );
    }
}


void _bt_CleanupThread(void *arg)
{
	PRThread *me = PR_GetCurrentThread();
	int32 i;

	
	for (i = 0; i < tpd_slotsUsed; i++)
	{
		void *oldValue = tls_get(tpd_beosTLSSlots[i]);
		if ( oldValue != NULL && tpd_dtors[i] != NULL )
			(*tpd_dtors[i])(oldValue);
	}

	
	if (me->state & BT_THREAD_JOINABLE)
	{
		
		PR_Lock(joinSemLock);

		if (me->md.is_joining)
		{
			


			delete_sem(me->md.joinSem);

			PR_Unlock(joinSemLock);

		}
		else
    {
			


			me->md.joinSem = create_sem(0, "join sem");

			
			PR_Unlock(joinSemLock);

			
			while (acquire_sem(me->md.joinSem) == B_INTERRUPTED);
	    }
	}

	
	if ((me->state & BT_THREAD_SYSTEM) == 0)
	{
		
    PR_Lock( bt_book.ml );

		
	bt_book.threadCount--;

		if (bt_book.threadCount == 0 && bt_book.cleanUpSem != B_ERROR) {
			


			delete_sem(bt_book.cleanUpSem);
	}

    PR_Unlock( bt_book.ml );
	}

	
	PR_DELETE(me);
}






static void*
_bt_root (void* arg)
	{
    PRThread *thred = (PRThread*)arg;
    PRIntn rv;
    void *privData;
    status_t result;
    int i;

	
	tls_set(tls_prThreadSlot, thred);

    thred->startFunc(thred->arg);  

	
	_bt_CleanupThread(NULL);

	return 0;
}

PR_IMPLEMENT(PRThread*)
    PR_CreateThread (PRThreadType type, void (*start)(void* arg), void* arg,
                     PRThreadPriority priority, PRThreadScope scope,
                     PRThreadState state, PRUint32 stackSize)
{
    PRUint32 bePriority;

    PRThread* thred;

    if (!_pr_initialized) _PR_ImplicitInitialization();

	thred = PR_NEWZAP(PRThread);
 	if (thred == NULL)
	{
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }

    thred->md.joinSem = B_ERROR;

        thred->arg = arg;
        thred->startFunc = start;
        thred->priority = priority;

	if( state == PR_JOINABLE_THREAD )
	{
	    thred->state |= BT_THREAD_JOINABLE;
	}

        

	PR_Lock( bt_book.ml );

	if (type == PR_USER_THREAD)
	{
	    bt_book.threadCount++;
        }

	PR_Unlock( bt_book.ml );

	bePriority = _bt_MapNSPRToNativePriority( priority );

        thred->md.tid = spawn_thread((thread_func)_bt_root, "moz-thread",
                                     bePriority, thred);
        if (thred->md.tid < B_OK) {
            PR_SetError(PR_UNKNOWN_ERROR, thred->md.tid);
            PR_DELETE(thred);
			return NULL;
        }

        if (resume_thread(thred->md.tid) < B_OK) {
            PR_SetError(PR_UNKNOWN_ERROR, 0);
            PR_DELETE(thred);
			return NULL;
        }

    return thred;
    }

PR_IMPLEMENT(PRThread*)
	PR_AttachThread(PRThreadType type, PRThreadPriority priority,
					PRThreadStack *stack)
{
	
	return PR_GetCurrentThread();
}

PR_IMPLEMENT(void)
	PR_DetachThread()
{
	
}

PR_IMPLEMENT(PRStatus)
    PR_JoinThread (PRThread* thred)
{
    status_t eval, status;

    PR_ASSERT(thred != NULL);

	if ((thred->state & BT_THREAD_JOINABLE) == 0)
    {
	PR_SetError( PR_INVALID_ARGUMENT_ERROR, 0 );
	return( PR_FAILURE );
    }

	
	PR_Lock(joinSemLock);
	
	if (thred->md.is_joining)
	{
		

		PR_Unlock(joinSemLock);
		return PR_FAILURE;
	}

	
	thred->md.is_joining = PR_TRUE;

	if (thred->md.joinSem == B_ERROR)
	{
		

		thred->md.joinSem = create_sem(0, "join sem");

		
		PR_Unlock(joinSemLock);

		
		while (acquire_sem(thred->md.joinSem) == B_INTERRUPTED);

	}
	else
	{
		

		delete_sem(thred->md.joinSem);
		
		PR_Unlock(joinSemLock);
    }

	
    wait_for_thread(thred->md.tid, &eval);

    return PR_SUCCESS;
}

PR_IMPLEMENT(PRThread*)
    PR_GetCurrentThread ()
{
    PRThread* thred;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    thred = (PRThread *)tls_get( tls_prThreadSlot);
	if (thred == NULL)
	{
		

		thred = _bt_AttachThread();
	}
    PR_ASSERT(NULL != thred);

    return thred;
}

PR_IMPLEMENT(PRThreadScope)
    PR_GetThreadScope (const PRThread* thred)
{
    PR_ASSERT(thred != NULL);
    return PR_GLOBAL_THREAD;
}

PR_IMPLEMENT(PRThreadType)
    PR_GetThreadType (const PRThread* thred)
{
    PR_ASSERT(thred != NULL);
    return (thred->state & BT_THREAD_SYSTEM) ?
        PR_SYSTEM_THREAD : PR_USER_THREAD;
}

PR_IMPLEMENT(PRThreadState)
    PR_GetThreadState (const PRThread* thred)
{
    PR_ASSERT(thred != NULL);
    return (thred->state & BT_THREAD_JOINABLE)?
    					PR_JOINABLE_THREAD: PR_UNJOINABLE_THREAD;
}

PR_IMPLEMENT(PRThreadPriority)
    PR_GetThreadPriority (const PRThread* thred)
{
    PR_ASSERT(thred != NULL);
    return thred->priority;
}  

PR_IMPLEMENT(void) PR_SetThreadPriority(PRThread *thred,
                                        PRThreadPriority newPri)
{
    PRUint32 bePriority;

    PR_ASSERT( thred != NULL );

    thred->priority = newPri;
    bePriority = _bt_MapNSPRToNativePriority( newPri );
    set_thread_priority( thred->md.tid, bePriority );
}

PR_IMPLEMENT(PRStatus)
    PR_NewThreadPrivateIndex (PRUintn* newIndex,
                              PRThreadPrivateDTOR destructor)
{
	int32    index;

    if (!_pr_initialized) _PR_ImplicitInitialization();

	
	index = atomic_add( &tpd_slotsUsed, 1 );
	if (index >= BT_TPD_LIMIT)
	{
		
		atomic_add( &tpd_slotsUsed, -1 );
		PR_SetError( PR_TPD_RANGE_ERROR, 0 );
	    return( PR_FAILURE );
	}

	

	tpd_beosTLSSlots[index] = tls_allocate();

	
	tpd_dtors[index] = destructor;

    *newIndex = (PRUintn)index;

    return( PR_SUCCESS );
}

PR_IMPLEMENT(PRStatus)
    PR_SetThreadPrivate (PRUintn index, void* priv)
{
	void *oldValue;

    



    if(index < 0 || index >= tpd_slotsUsed || index >= BT_TPD_LIMIT)
    {
		PR_SetError( PR_TPD_RANGE_ERROR, 0 );
	return( PR_FAILURE );
    }

	

	oldValue = tls_get(tpd_beosTLSSlots[index]);
	if (oldValue != NULL && tpd_dtors[index] != NULL)
		(*tpd_dtors[index])(oldValue);

	
	tls_set(tpd_beosTLSSlots[index], priv);

	    return( PR_SUCCESS );
	}

PR_IMPLEMENT(void*)
    PR_GetThreadPrivate (PRUintn index)
{
	
	if (index < 0 || index >= tpd_slotsUsed || index >= BT_TPD_LIMIT)
    {   
		PR_SetError( PR_TPD_RANGE_ERROR, 0 );
		return NULL;
    }

	
	return tls_get( tpd_beosTLSSlots[index] );
	}


PR_IMPLEMENT(PRStatus)
    PR_Interrupt (PRThread* thred)
{
    PRIntn rv;

    PR_ASSERT(thred != NULL);

    










    rv = suspend_thread( thred->md.tid );
    if( rv != B_NO_ERROR )
    {
        
        PR_SetError( PR_UNKNOWN_ERROR, rv );
        return PR_FAILURE;
    }

    rv = resume_thread( thred->md.tid );
    if( rv != B_NO_ERROR )
    {
        PR_SetError( PR_UNKNOWN_ERROR, rv );
        return PR_FAILURE;
    }

    return PR_SUCCESS;
}

PR_IMPLEMENT(void)
    PR_ClearInterrupt ()
{
}

PR_IMPLEMENT(PRStatus)
    PR_Yield ()
{
    

    snooze(100);
}

#define BT_MILLION 1000000UL

PR_IMPLEMENT(PRStatus)
    PR_Sleep (PRIntervalTime ticks)
{
    bigtime_t tps;
    status_t status;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    tps = PR_IntervalToMicroseconds( ticks );

    status = snooze(tps);
    if (status == B_NO_ERROR) return PR_SUCCESS;

    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, status);
    return PR_FAILURE;
}

PR_IMPLEMENT(PRStatus)
    PR_Cleanup ()
{
    PRThread *me = PR_GetCurrentThread();

    PR_ASSERT(me->state & BT_THREAD_PRIMORD);
    if ((me->state & BT_THREAD_PRIMORD) == 0) {
        return PR_FAILURE;
    }

    PR_Lock( bt_book.ml );

	if (bt_book.threadCount != 0)
    {
		

		bt_book.cleanUpSem = create_sem(0, "cleanup sem");
    }

    PR_Unlock( bt_book.ml );

	


	while (acquire_sem(bt_book.cleanUpSem) == B_INTERRUPTED);

    return PR_SUCCESS;
}

PR_IMPLEMENT(void)
    PR_ProcessExit (PRIntn status)
{
    exit(status);
}

PRThread *_bt_AttachThread()
{
	PRThread *thread;
	thread_info tInfo;

	
	PR_ASSERT(tls_get(tls_prThreadSlot) == NULL);

	
	thread = PR_NEWZAP(PRThread);
	if (thread == NULL)
	{
		PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
		return NULL;
	}

	
	get_thread_info(find_thread(NULL), &tInfo);

	
	thread->md.tid = tInfo.thread;
	thread->md.joinSem = B_ERROR;
	thread->priority = _bt_MapNativeToNSPRPriority(tInfo.priority);

	
	thread->state = 0;

	
	PR_Lock(bt_book.ml);
	bt_book.threadCount++;
	PR_Unlock(bt_book.ml);

	
	tls_set(tls_prThreadSlot, thread);
	
	


	on_exit_thread(_bt_CleanupThread, NULL);
	
	return thread;
}
