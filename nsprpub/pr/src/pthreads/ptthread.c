










































#if defined(_PR_PTHREADS) || defined(_PR_DCETHREADS)

#include "prlog.h"
#include "primpl.h"
#include "prpdce.h"

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#ifdef SYMBIAN



#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#endif







static PRIntn pt_schedpriv = 0;
extern PRLock *_pr_sleeplock;

static struct _PT_Bookeeping
{
    PRLock *ml;                 
    PRCondVar *cv;              
    PRInt32 system, user;       
    PRUintn this_many;          
    pthread_key_t key;          
    PRThread *first, *last;     
#if defined(_PR_DCETHREADS) || defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    PRInt32 minPrio, maxPrio;   
#endif
} pt_book = {0};

static void _pt_thread_death(void *arg);
static void _pt_thread_death_internal(void *arg, PRBool callDestructors);
static void init_pthread_gc_support(void);

#if defined(_PR_DCETHREADS) || defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
static PRIntn pt_PriorityMap(PRThreadPriority pri)
{
#ifdef NTO
    




    return 10;
#else
    return pt_book.minPrio +
	    pri * (pt_book.maxPrio - pt_book.minPrio) / PR_PRIORITY_LAST;
#endif
}
#endif

#if defined(GC_LEAK_DETECTOR) && (__GLIBC__ >= 2) && defined(__i386__) 

#include <setjmp.h>

typedef struct stack_frame stack_frame;

struct stack_frame {
    stack_frame* next;
    void* pc;
};

static stack_frame* GetStackFrame()
{
    jmp_buf jb;
    stack_frame* currentFrame;
    setjmp(jb);
    currentFrame = (stack_frame*)(jb[0].__jmpbuf[JB_BP]);
    currentFrame = currentFrame->next;
    return currentFrame;
}

static void* GetStackTop()
{
    stack_frame* frame;
    frame = GetStackFrame();
    while (frame != NULL)
    {
        ptrdiff_t pc = (ptrdiff_t)frame->pc;
        if ((pc < 0x08000000) || (pc > 0x7fffffff) || (frame->next < frame))
            return frame;
        frame = frame->next;
    }
    return NULL;
}
#endif 




static void _PR_InitializeStack(PRThreadStack *ts)
{
    if( ts && (ts->stackTop == 0) ) {
        ts->allocBase = (char *) &ts;
        ts->allocSize = ts->stackSize;

        


#ifdef HAVE_STACK_GROWING_UP
        ts->stackBottom = ts->allocBase + ts->stackSize;
        ts->stackTop = ts->allocBase;
#else
#ifdef GC_LEAK_DETECTOR
        ts->stackTop    = GetStackTop();
        ts->stackBottom = ts->stackTop - ts->stackSize;
#else
        ts->stackTop    = ts->allocBase;
        ts->stackBottom = ts->allocBase - ts->stackSize;
#endif
#endif
    }
}

static void *_pt_root(void *arg)
{
    PRIntn rv;
    PRThread *thred = (PRThread*)arg;
    PRBool detached = (thred->state & PT_THREAD_DETACHED) ? PR_TRUE : PR_FALSE;

    








    thred->id = pthread_self();

    




#if defined(_PR_DCETHREADS)
    if (detached)
    {
        
        pthread_t self = thred->id;
        rv = pthread_detach(&self);
        PR_ASSERT(0 == rv);
    }
#endif 

    
    _PR_InitializeStack(thred->stack);

    




    rv = pthread_setspecific(pt_book.key, thred);
    PR_ASSERT(0 == rv);

    
    PR_Lock(pt_book.ml);

    
    if (thred->suspend & PT_THREAD_SETGCABLE)
	    thred->state |= PT_THREAD_GCABLE;
    thred->suspend = 0;

    thred->prev = pt_book.last;
    if (pt_book.last)
        pt_book.last->next = thred;
    else
        pt_book.first = thred;
    thred->next = NULL;
    pt_book.last = thred;
    PR_Unlock(pt_book.ml);

    thred->startFunc(thred->arg);  

    
    PR_Lock(pt_book.ml);
    




    if (detached)
    {
        while (!thred->okToDelete)
            PR_WaitCondVar(pt_book.cv, PR_INTERVAL_NO_TIMEOUT);
    }

    if (thred->state & PT_THREAD_SYSTEM)
        pt_book.system -= 1;
    else if (--pt_book.user == pt_book.this_many)
        PR_NotifyAllCondVar(pt_book.cv);
    if (NULL == thred->prev)
        pt_book.first = thred->next;
    else
        thred->prev->next = thred->next;
    if (NULL == thred->next)
        pt_book.last = thred->prev;
    else
        thred->next->prev = thred->prev;
    PR_Unlock(pt_book.ml);

    







    if (PR_FALSE == detached)
    {
        
        _PR_DestroyThreadPrivate(thred);
        rv = pthread_setspecific(pt_book.key, NULL);
        PR_ASSERT(0 == rv);
    }

    return NULL;
}  

static PRThread* pt_AttachThread(void)
{
    PRThread *thred = NULL;

    








    if (!_pr_initialized) return NULL;

    
    thred = PR_NEWZAP(PRThread);
    if (NULL != thred)
    {
        int rv;

        thred->priority = PR_PRIORITY_NORMAL;
        thred->id = pthread_self();
        rv = pthread_setspecific(pt_book.key, thred);
        PR_ASSERT(0 == rv);

        thred->state = PT_THREAD_GLOBAL | PT_THREAD_FOREIGN;
        PR_Lock(pt_book.ml);

        
        thred->prev = pt_book.last;
        if (pt_book.last)
            pt_book.last->next = thred;
        else
            pt_book.first = thred;
        thred->next = NULL;
        pt_book.last = thred;
        PR_Unlock(pt_book.ml);

    }
    return thred;  
}  

static PRThread* _PR_CreateThread(
    PRThreadType type, void (*start)(void *arg),
    void *arg, PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize, PRBool isGCAble)
{
    int rv;
    PRThread *thred;
    pthread_attr_t tattr;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if ((PRIntn)PR_PRIORITY_FIRST > (PRIntn)priority)
        priority = PR_PRIORITY_FIRST;
    else if ((PRIntn)PR_PRIORITY_LAST < (PRIntn)priority)
        priority = PR_PRIORITY_LAST;

    rv = _PT_PTHREAD_ATTR_INIT(&tattr);
    PR_ASSERT(0 == rv);

    if (EPERM != pt_schedpriv)
    {
#if !defined(_PR_DCETHREADS) && defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
        struct sched_param schedule;
#endif

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
        rv = pthread_attr_setinheritsched(&tattr, PTHREAD_EXPLICIT_SCHED);
        PR_ASSERT(0 == rv);
#endif

        

#if defined(_PR_DCETHREADS)
        rv = pthread_attr_setprio(&tattr, pt_PriorityMap(priority));
        PR_ASSERT(0 == rv);
#elif defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
        rv = pthread_attr_getschedparam(&tattr, &schedule);
        PR_ASSERT(0 == rv);
        schedule.sched_priority = pt_PriorityMap(priority);
        rv = pthread_attr_setschedparam(&tattr, &schedule);
        PR_ASSERT(0 == rv);
#ifdef NTO
        rv = pthread_attr_setschedpolicy(&tattr, SCHED_RR); 
        PR_ASSERT(0 == rv);
#endif
#endif 
    }

    



#if !defined(_PR_DCETHREADS)
    rv = pthread_attr_setdetachstate(&tattr,
        ((PR_JOINABLE_THREAD == state) ?
            PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED));
    PR_ASSERT(0 == rv);
#endif 

    


    if (stackSize)
    {
#ifdef _MD_MINIMUM_STACK_SIZE
        if (stackSize < _MD_MINIMUM_STACK_SIZE)
            stackSize = _MD_MINIMUM_STACK_SIZE;
#endif
        rv = pthread_attr_setstacksize(&tattr, stackSize);
        PR_ASSERT(0 == rv);
    }

    thred = PR_NEWZAP(PRThread);
    if (NULL == thred)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, errno);
        goto done;
    }
    else
    {
        pthread_t id;

        thred->arg = arg;
        thred->startFunc = start;
        thred->priority = priority;
        if (PR_UNJOINABLE_THREAD == state)
            thred->state |= PT_THREAD_DETACHED;

        if (PR_LOCAL_THREAD == scope)
        	scope = PR_GLOBAL_THREAD;
			
        if (PR_GLOBAL_BOUND_THREAD == scope) {
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    		rv = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM);
			if (rv) {
				


        		scope = PR_GLOBAL_THREAD;
				


    			rv = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_PROCESS);
    			PR_ASSERT(0 == rv);
			}
#endif
		}
        if (PR_GLOBAL_THREAD == scope)
            thred->state |= PT_THREAD_GLOBAL;
        else if (PR_GLOBAL_BOUND_THREAD == scope)
            thred->state |= (PT_THREAD_GLOBAL | PT_THREAD_BOUND);
		else	
            thred->state |= PT_THREAD_GLOBAL;
        if (PR_SYSTEM_THREAD == type)
            thred->state |= PT_THREAD_SYSTEM;

        thred->suspend =(isGCAble) ? PT_THREAD_SETGCABLE : 0;

        thred->stack = PR_NEWZAP(PRThreadStack);
        if (thred->stack == NULL) {
            PRIntn oserr = errno;
            PR_Free(thred);  
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, oserr);
            thred = NULL;  
            goto done;
        }
        thred->stack->stackSize = stackSize;
        thred->stack->thr = thred;

#ifdef PT_NO_SIGTIMEDWAIT
        pthread_mutex_init(&thred->suspendResumeMutex,NULL);
        pthread_cond_init(&thred->suspendResumeCV,NULL);
#endif

        
        PR_Lock(pt_book.ml);
        if (PR_SYSTEM_THREAD == type)
            pt_book.system += 1;
        else pt_book.user += 1;
        PR_Unlock(pt_book.ml);

        




        rv = _PT_PTHREAD_CREATE(&id, tattr, _pt_root, thred);

#if !defined(_PR_DCETHREADS)
        if (EPERM == rv)
        {
#if defined(IRIX)
        	if (PR_GLOBAL_BOUND_THREAD == scope) {
				



    			rv = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_PROCESS);
    			PR_ASSERT(0 == rv);
            	thred->state &= ~PT_THREAD_BOUND;
			}
#else
            
            pt_schedpriv = EPERM;
            PR_LOG(_pr_thread_lm, PR_LOG_MIN,
                ("_PR_CreateThread: no thread scheduling privilege"));
            
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
            rv = pthread_attr_setinheritsched(&tattr, PTHREAD_INHERIT_SCHED);
            PR_ASSERT(0 == rv);
#endif
#endif	
            rv = _PT_PTHREAD_CREATE(&id, tattr, _pt_root, thred);
        }
#endif

        if (0 != rv)
        {
#if defined(_PR_DCETHREADS)
            PRIntn oserr = errno;
#else
            PRIntn oserr = rv;
#endif
            PR_Lock(pt_book.ml);
            if (thred->state & PT_THREAD_SYSTEM)
                pt_book.system -= 1;
            else if (--pt_book.user == pt_book.this_many)
                PR_NotifyAllCondVar(pt_book.cv);
            PR_Unlock(pt_book.ml);

            PR_Free(thred->stack);
            PR_Free(thred);  
            PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, oserr);
            thred = NULL;  
            goto done;
        }

        




        thred->id = id;

        



        if (PR_UNJOINABLE_THREAD == state)
        {
            PR_Lock(pt_book.ml);
            thred->okToDelete = PR_TRUE;
            PR_NotifyAllCondVar(pt_book.cv);
            PR_Unlock(pt_book.ml);
        }
    }

done:
    rv = _PT_PTHREAD_ATTR_DESTROY(&tattr);
    PR_ASSERT(0 == rv);

    return thred;
}  

PR_IMPLEMENT(PRThread*) PR_CreateThread(
    PRThreadType type, void (*start)(void *arg), void *arg,
    PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize)
{
    return _PR_CreateThread(
        type, start, arg, priority, scope, state, stackSize, PR_FALSE);
} 

PR_IMPLEMENT(PRThread*) PR_CreateThreadGCAble(
    PRThreadType type, void (*start)(void *arg), void *arg, 
    PRThreadPriority priority, PRThreadScope scope,
    PRThreadState state, PRUint32 stackSize)
{
    return _PR_CreateThread(
        type, start, arg, priority, scope, state, stackSize, PR_TRUE);
}  

PR_IMPLEMENT(void*) GetExecutionEnvironment(PRThread *thred)
{
    return thred->environment;
}  
 
PR_IMPLEMENT(void) SetExecutionEnvironment(PRThread *thred, void *env)
{
    thred->environment = env;
}  

PR_IMPLEMENT(PRThread*) PR_AttachThread(
    PRThreadType type, PRThreadPriority priority, PRThreadStack *stack)
{
    return PR_GetCurrentThread();
}  


PR_IMPLEMENT(PRStatus) PR_JoinThread(PRThread *thred)
{
    int rv = -1;
    void *result = NULL;
    PR_ASSERT(thred != NULL);

    if ((0xafafafaf == thred->state)
    || (PT_THREAD_DETACHED == (PT_THREAD_DETACHED & thred->state))
    || (PT_THREAD_FOREIGN == (PT_THREAD_FOREIGN & thred->state)))
    {
        





        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        PR_LogPrint(
            "PR_JoinThread: %p not joinable | already smashed\n", thred);
    }
    else
    {
        pthread_t id = thred->id;
        rv = pthread_join(id, &result);
        PR_ASSERT(rv == 0 && result == NULL);
        if (0 == rv)
        {
#ifdef _PR_DCETHREADS
            rv = pthread_detach(&id);
            PR_ASSERT(0 == rv);
#endif
            



            _pt_thread_death_internal(thred, PR_FALSE);
        }
        else
        {
            PRErrorCode prerror;
            switch (rv)
            {
                case EINVAL:  
                case ESRCH:   
                    prerror = PR_INVALID_ARGUMENT_ERROR;
                    break;
                case EDEADLK: 
                    prerror = PR_DEADLOCK_ERROR;
                    break;
                default:
                    prerror = PR_UNKNOWN_ERROR;
                    break;
            }
            PR_SetError(prerror, rv);
        }
    }
    return (0 == rv) ? PR_SUCCESS : PR_FAILURE;
}  

PR_IMPLEMENT(void) PR_DetachThread(void)
{
    void *thred;
    int rv;

    _PT_PTHREAD_GETSPECIFIC(pt_book.key, thred);
    if (NULL == thred) return;
    _pt_thread_death(thred);
    rv = pthread_setspecific(pt_book.key, NULL);
    PR_ASSERT(0 == rv);
}  

PR_IMPLEMENT(PRThread*) PR_GetCurrentThread(void)
{
    void *thred;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    _PT_PTHREAD_GETSPECIFIC(pt_book.key, thred);
    if (NULL == thred) thred = pt_AttachThread();
    PR_ASSERT(NULL != thred);
    return (PRThread*)thred;
}  

PR_IMPLEMENT(PRThreadScope) PR_GetThreadScope(const PRThread *thred)
{
    return (thred->state & PT_THREAD_BOUND) ?
        PR_GLOBAL_BOUND_THREAD : PR_GLOBAL_THREAD;
}  

PR_IMPLEMENT(PRThreadType) PR_GetThreadType(const PRThread *thred)
{
    return (thred->state & PT_THREAD_SYSTEM) ?
        PR_SYSTEM_THREAD : PR_USER_THREAD;
}

PR_IMPLEMENT(PRThreadState) PR_GetThreadState(const PRThread *thred)
{
    return (thred->state & PT_THREAD_DETACHED) ?
        PR_UNJOINABLE_THREAD : PR_JOINABLE_THREAD;
}  

PR_IMPLEMENT(PRThreadPriority) PR_GetThreadPriority(const PRThread *thred)
{
    PR_ASSERT(thred != NULL);
    return thred->priority;
}  

PR_IMPLEMENT(void) PR_SetThreadPriority(PRThread *thred, PRThreadPriority newPri)
{
    PRIntn rv = -1;

    PR_ASSERT(NULL != thred);

    if ((PRIntn)PR_PRIORITY_FIRST > (PRIntn)newPri)
        newPri = PR_PRIORITY_FIRST;
    else if ((PRIntn)PR_PRIORITY_LAST < (PRIntn)newPri)
        newPri = PR_PRIORITY_LAST;

#if defined(_PR_DCETHREADS)
    rv = pthread_setprio(thred->id, pt_PriorityMap(newPri));
    
#elif defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
    if (EPERM != pt_schedpriv)
    {
        int policy;
        struct sched_param schedule;

        rv = pthread_getschedparam(thred->id, &policy, &schedule);
        if(0 == rv) {
			schedule.sched_priority = pt_PriorityMap(newPri);
			rv = pthread_setschedparam(thred->id, policy, &schedule);
			if (EPERM == rv)
			{
				pt_schedpriv = EPERM;
				PR_LOG(_pr_thread_lm, PR_LOG_MIN,
					("PR_SetThreadPriority: no thread scheduling privilege"));
			}
		}
		if (rv != 0)
			rv = -1;
    }
#endif

    thred->priority = newPri;
}  

PR_IMPLEMENT(PRStatus) PR_Interrupt(PRThread *thred)
{
    


















    PRCondVar *cv;
    PR_ASSERT(NULL != thred);
    if (NULL == thred) return PR_FAILURE;

    thred->state |= PT_THREAD_ABORTED;

    cv = thred->waiting;
    if ((NULL != cv) && !thred->interrupt_blocked)
    {
        PRIntn rv;
        (void)PR_AtomicIncrement(&cv->notify_pending);
        rv = pthread_cond_broadcast(&cv->cv);
        PR_ASSERT(0 == rv);
        if (0 > PR_AtomicDecrement(&cv->notify_pending))
            PR_DestroyCondVar(cv);
    }
    return PR_SUCCESS;
}  

PR_IMPLEMENT(void) PR_ClearInterrupt(void)
{
    PRThread *me = PR_GetCurrentThread();
    me->state &= ~PT_THREAD_ABORTED;
}  

PR_IMPLEMENT(void) PR_BlockInterrupt(void)
{
    PRThread *me = PR_GetCurrentThread();
    _PT_THREAD_BLOCK_INTERRUPT(me);
}  

PR_IMPLEMENT(void) PR_UnblockInterrupt(void)
{
    PRThread *me = PR_GetCurrentThread();
    _PT_THREAD_UNBLOCK_INTERRUPT(me);
}  

PR_IMPLEMENT(PRStatus) PR_Yield(void)
{
    static PRBool warning = PR_TRUE;
    if (warning) warning = _PR_Obsolete(
        "PR_Yield()", "PR_Sleep(PR_INTERVAL_NO_WAIT)");
    return PR_Sleep(PR_INTERVAL_NO_WAIT);
}

PR_IMPLEMENT(PRStatus) PR_Sleep(PRIntervalTime ticks)
{
    PRStatus rv = PR_SUCCESS;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (PR_INTERVAL_NO_WAIT == ticks)
    {
        _PT_PTHREAD_YIELD();
    }
    else
    {
        PRCondVar *cv;
        PRIntervalTime timein;

        timein = PR_IntervalNow();
        cv = PR_NewCondVar(_pr_sleeplock);
        PR_ASSERT(cv != NULL);
        PR_Lock(_pr_sleeplock);
        do
        {
            PRIntervalTime now = PR_IntervalNow();
            PRIntervalTime delta = now - timein;
            if (delta > ticks) break;
            rv = PR_WaitCondVar(cv, ticks - delta);
        } while (PR_SUCCESS == rv);
        PR_Unlock(_pr_sleeplock);
        PR_DestroyCondVar(cv);
    }
    return rv;
}  

static void _pt_thread_death(void *arg)
{
     
    _pt_thread_death_internal(arg, PR_TRUE);
}

static void _pt_thread_death_internal(void *arg, PRBool callDestructors)
{
    PRThread *thred = (PRThread*)arg;

    if (thred->state & (PT_THREAD_FOREIGN|PT_THREAD_PRIMORD))
    {
        PR_Lock(pt_book.ml);
        if (NULL == thred->prev)
            pt_book.first = thred->next;
        else
            thred->prev->next = thred->next;
        if (NULL == thred->next)
            pt_book.last = thred->prev;
        else
            thred->next->prev = thred->prev;
        PR_Unlock(pt_book.ml);
    }
    if (callDestructors)
        _PR_DestroyThreadPrivate(thred);
    PR_Free(thred->privateData);
    if (NULL != thred->errorString)
        PR_Free(thred->errorString);
    PR_Free(thred->stack);
    if (NULL != thred->syspoll_list)
        PR_Free(thred->syspoll_list);
#if defined(_PR_POLL_WITH_SELECT)
    if (NULL != thred->selectfd_list)
        PR_Free(thred->selectfd_list);
#endif
#if defined(DEBUG)
    memset(thred, 0xaf, sizeof(PRThread));
#endif 
    PR_Free(thred);
}  

void _PR_InitThreads(
    PRThreadType type, PRThreadPriority priority, PRUintn maxPTDs)
{
    int rv;
    PRThread *thred;

#ifdef _PR_NEED_PTHREAD_INIT
    






    pthread_init();
#endif

#if defined(_PR_DCETHREADS) || defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
#if defined(FREEBSD)
    {
    pthread_attr_t attr;
    int policy;
    
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_getschedpolicy(&attr, &policy);
    pt_book.minPrio = sched_get_priority_min(policy);
    PR_ASSERT(-1 != pt_book.minPrio);
    pt_book.maxPrio = sched_get_priority_max(policy);
    PR_ASSERT(-1 != pt_book.maxPrio);
    pthread_attr_destroy(&attr);
    }
#else
    


    pt_book.minPrio = PT_PRIO_MIN;
    pt_book.maxPrio = PT_PRIO_MAX;
#endif
#endif
    
    PR_ASSERT(NULL == pt_book.ml);
    pt_book.ml = PR_NewLock();
    PR_ASSERT(NULL != pt_book.ml);
    pt_book.cv = PR_NewCondVar(pt_book.ml);
    PR_ASSERT(NULL != pt_book.cv);
    thred = PR_NEWZAP(PRThread);
    PR_ASSERT(NULL != thred);
    thred->arg = NULL;
    thred->startFunc = NULL;
    thred->priority = priority;
    thred->id = pthread_self();

    thred->state = (PT_THREAD_DETACHED | PT_THREAD_PRIMORD);
    if (PR_SYSTEM_THREAD == type)
    {
        thred->state |= PT_THREAD_SYSTEM;
        pt_book.system += 1;
	    pt_book.this_many = 0;
    }
    else
    {
	    pt_book.user += 1;
	    pt_book.this_many = 1;
    }
    thred->next = thred->prev = NULL;
    pt_book.first = pt_book.last = thred;

    thred->stack = PR_NEWZAP(PRThreadStack);
    PR_ASSERT(thred->stack != NULL);
    thred->stack->stackSize = 0;
    thred->stack->thr = thred;
	_PR_InitializeStack(thred->stack);

    













    rv = _PT_PTHREAD_KEY_CREATE(&pt_book.key, _pt_thread_death);
    PR_ASSERT(0 == rv);
    rv = pthread_setspecific(pt_book.key, thred);
    PR_ASSERT(0 == rv);    
    PR_SetThreadPriority(thred, priority);
}  

#ifdef __GNUC__




static void _PR_Fini(void) __attribute__ ((destructor));
#elif defined(__SUNPRO_C)



#pragma fini(_PR_Fini)
static void _PR_Fini(void);
#elif defined(HPUX)




#if defined(__ia64) || defined(_LP64)
#pragma FINI "_PR_Fini"
static void _PR_Fini(void);
#else




#include <dl.h>

static void _PR_Fini(void);

void PR_HPUX10xInit(shl_t handle, int loading)
{
    









    if (loading) {
	
    } else {
	_PR_Fini();
    }
}
#endif
#elif defined(AIX)

#endif

void _PR_Fini(void)
{
    void *thred;
    int rv;

    if (!_pr_initialized) return;

    _PT_PTHREAD_GETSPECIFIC(pt_book.key, thred);
    if (NULL != thred)
    {
        



        _pt_thread_death_internal(thred, PR_FALSE);
        rv = pthread_setspecific(pt_book.key, NULL);
        PR_ASSERT(0 == rv);
    }
    
    
}  

PR_IMPLEMENT(PRStatus) PR_Cleanup(void)
{
    PRThread *me = PR_GetCurrentThread();
    int rv;
    PR_LOG(_pr_thread_lm, PR_LOG_MIN, ("PR_Cleanup: shutting down NSPR"));
    PR_ASSERT(me->state & PT_THREAD_PRIMORD);
    if (me->state & PT_THREAD_PRIMORD)
    {
        PR_Lock(pt_book.ml);
        while (pt_book.user > pt_book.this_many)
            PR_WaitCondVar(pt_book.cv, PR_INTERVAL_NO_TIMEOUT);
        PR_Unlock(pt_book.ml);

        _PR_CleanupMW();
        _PR_CleanupTime();
        _PR_CleanupDtoa();
        _PR_CleanupCallOnce();
        _PR_ShutdownLinker();
        _PR_LogCleanup();
        _PR_CleanupNet();
        
        _PR_CleanupIO();
        _PR_CleanupCMon();

        _pt_thread_death(me);
        rv = pthread_setspecific(pt_book.key, NULL);
        PR_ASSERT(0 == rv);
        





        if (0 == pt_book.system)
        {
            PR_DestroyCondVar(pt_book.cv); pt_book.cv = NULL;
            PR_DestroyLock(pt_book.ml); pt_book.ml = NULL;
        }
        PR_DestroyLock(_pr_sleeplock);
        _pr_sleeplock = NULL;
        _PR_CleanupLayerCache();
        _PR_CleanupEnv();
#ifdef _PR_ZONE_ALLOCATOR
        _PR_DestroyZones();
#endif
        _pr_initialized = PR_FALSE;
        return PR_SUCCESS;
    }
    return PR_FAILURE;
}  

PR_IMPLEMENT(void) PR_ProcessExit(PRIntn status)
{
    _exit(status);
}

PR_IMPLEMENT(PRUint32) PR_GetThreadID(PRThread *thred)
{
#if defined(_PR_DCETHREADS)
    return (PRUint32)&thred->id;  
#else
    return (PRUint32)thred->id;  
#endif
}









PR_IMPLEMENT(PRInt32) PR_GetThreadAffinityMask(PRThread *thread, PRUint32 *mask)
{
    return 0;  
}

PR_IMPLEMENT(PRInt32) PR_SetThreadAffinityMask(PRThread *thread, PRUint32 mask )
{
    return 0;  
}

PR_IMPLEMENT(void)
PR_SetThreadDumpProc(PRThread* thread, PRThreadDumpProc dump, void *arg)
{
    thread->dump = dump;
    thread->dumpArg = arg;
}





#if defined(_PR_DCETHREADS)






static sigset_t javagc_vtalarm_sigmask;
static sigset_t javagc_intsoff_sigmask;

#else 



static sigset_t sigwait_set;

static struct timespec onemillisec = {0, 1000000L};
#ifndef PT_NO_SIGTIMEDWAIT
static struct timespec hundredmillisec = {0, 100000000L};
#endif

static void suspend_signal_handler(PRIntn sig);

#ifdef PT_NO_SIGTIMEDWAIT
static void null_signal_handler(PRIntn sig);
#endif

#endif 






static void init_pthread_gc_support(void)
{
    PRIntn rv;

#if defined(_PR_DCETHREADS)
	rv = sigemptyset(&javagc_vtalarm_sigmask);
    PR_ASSERT(0 == rv);
	rv = sigaddset(&javagc_vtalarm_sigmask, SIGVTALRM);
    PR_ASSERT(0 == rv);
#else  
	{
	    struct sigaction sigact_usr2;

	    sigact_usr2.sa_handler = suspend_signal_handler;
	    sigact_usr2.sa_flags = SA_RESTART;
	    sigemptyset (&sigact_usr2.sa_mask);

        rv = sigaction (SIGUSR2, &sigact_usr2, NULL);
        PR_ASSERT(0 == rv);

        sigemptyset (&sigwait_set);
#if defined(PT_NO_SIGTIMEDWAIT)
        sigaddset (&sigwait_set, SIGUSR1);
#else
        sigaddset (&sigwait_set, SIGUSR2);
#endif  
	}
#if defined(PT_NO_SIGTIMEDWAIT)
	{
	    struct sigaction sigact_null;
	    sigact_null.sa_handler = null_signal_handler;
	    sigact_null.sa_flags = SA_RESTART;
	    sigemptyset (&sigact_null.sa_mask);
        rv = sigaction (SIGUSR1, &sigact_null, NULL);
	    PR_ASSERT(0 ==rv); 
    }
#endif  
#endif 
}

PR_IMPLEMENT(void) PR_SetThreadGCAble(void)
{
    PR_Lock(pt_book.ml);
	PR_GetCurrentThread()->state |= PT_THREAD_GCABLE;
    PR_Unlock(pt_book.ml);
}

PR_IMPLEMENT(void) PR_ClearThreadGCAble(void)
{
    PR_Lock(pt_book.ml);
	PR_GetCurrentThread()->state &= (~PT_THREAD_GCABLE);
    PR_Unlock(pt_book.ml);
}

#if defined(DEBUG)
static PRBool suspendAllOn = PR_FALSE;
#endif

static PRBool suspendAllSuspended = PR_FALSE;

PR_IMPLEMENT(PRStatus) PR_EnumerateThreads(PREnumerator func, void *arg)
{
    PRIntn count = 0;
    PRStatus rv = PR_SUCCESS;
    PRThread* thred = pt_book.first;

#if defined(DEBUG) || defined(FORCE_PR_ASSERT)
#if !defined(_PR_DCETHREADS)
    PRThread *me = PR_GetCurrentThread();
#endif
#endif

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_EnumerateThreads\n"));
    







    PR_ASSERT(suspendAllOn);

    while (thred != NULL)
    {
        








        PRThread* next = thred->next;

        if (_PT_IS_GCABLE_THREAD(thred))
        {
#if !defined(_PR_DCETHREADS)
            PR_ASSERT((thred == me) || (thred->suspend & PT_THREAD_SUSPENDED));
#endif
            PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
                   ("In PR_EnumerateThreads callback thread %p thid = %X\n", 
                    thred, thred->id));

            rv = func(thred, count++, arg);
            if (rv != PR_SUCCESS)
                return rv;
        }
        thred = next;
    }
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("End PR_EnumerateThreads count = %d \n", count));
    return rv;
}  













#if !defined(_PR_DCETHREADS)










 








#if defined(PT_NO_SIGTIMEDWAIT)
static void null_signal_handler(PRIntn sig)
{
	return;
}
#endif

static void suspend_signal_handler(PRIntn sig)
{
	PRThread *me = PR_GetCurrentThread();

	PR_ASSERT(me != NULL);
	PR_ASSERT(_PT_IS_GCABLE_THREAD(me));
	PR_ASSERT((me->suspend & PT_THREAD_SUSPENDED) == 0);

	PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
        ("Begin suspend_signal_handler thred %p thread id = %X\n", 
		me, me->id));

	


	me->sp = &me;

	





	me->suspend |= PT_THREAD_SUSPENDED;

	


#if defined(PT_NO_SIGTIMEDWAIT)
	pthread_cond_signal(&me->suspendResumeCV);
	while (me->suspend & PT_THREAD_SUSPENDED)
	{
#if !defined(FREEBSD) && !defined(NETBSD) && !defined(OPENBSD) \
    && !defined(BSDI) && !defined(VMS) && !defined(UNIXWARE) \
    && !defined(DARWIN) && !defined(RISCOS) 
        PRIntn rv;
	    sigwait(&sigwait_set, &rv);
#endif
	}
	me->suspend |= PT_THREAD_RESUMED;
	pthread_cond_signal(&me->suspendResumeCV);
#else 
	while (me->suspend & PT_THREAD_SUSPENDED)
	{
		PRIntn rv = sigtimedwait(&sigwait_set, NULL, &hundredmillisec);
    	PR_ASSERT(-1 == rv);
	}
	me->suspend |= PT_THREAD_RESUMED;
#endif

    






    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
        ("End suspend_signal_handler thred = %p tid = %X\n", me, me->id));
}  

static void pt_SuspendSet(PRThread *thred)
{
    PRIntn rv;

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("pt_SuspendSet thred %p thread id = %X\n", thred, thred->id));


    



    PR_ASSERT((thred->suspend & PT_THREAD_SUSPENDED) == 0);

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("doing pthread_kill in pt_SuspendSet thred %p tid = %X\n",
	   thred, thred->id));
#if defined(VMS)
    rv = thread_suspend(thred);
#else
    rv = pthread_kill (thred->id, SIGUSR2);
#endif
    PR_ASSERT(0 == rv);
}

static void pt_SuspendTest(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("Begin pt_SuspendTest thred %p thread id = %X\n", thred, thred->id));


    





#if defined(PT_NO_SIGTIMEDWAIT)
    pthread_mutex_lock(&thred->suspendResumeMutex);
    while ((thred->suspend & PT_THREAD_SUSPENDED) == 0)
    {
	    pthread_cond_timedwait(
	        &thred->suspendResumeCV, &thred->suspendResumeMutex, &onemillisec);
	}
	pthread_mutex_unlock(&thred->suspendResumeMutex);
#else
    while ((thred->suspend & PT_THREAD_SUSPENDED) == 0)
    {
		PRIntn rv = sigtimedwait(&sigwait_set, NULL, &onemillisec);
    	PR_ASSERT(-1 == rv);
	}
#endif

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS,
        ("End pt_SuspendTest thred %p tid %X\n", thred, thred->id));
}  

static void pt_ResumeSet(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("pt_ResumeSet thred %p thread id = %X\n", thred, thred->id));

    




    PR_ASSERT(thred->suspend & PT_THREAD_SUSPENDED);


    thred->suspend &= ~PT_THREAD_SUSPENDED;

#if defined(PT_NO_SIGTIMEDWAIT)
#if defined(VMS)
	thread_resume(thred);
#else
	pthread_kill(thred->id, SIGUSR1);
#endif
#endif

}  

static void pt_ResumeTest(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	   ("Begin pt_ResumeTest thred %p thread id = %X\n", thred, thred->id));

    



#if defined(PT_NO_SIGTIMEDWAIT)
    pthread_mutex_lock(&thred->suspendResumeMutex);
    while ((thred->suspend & PT_THREAD_RESUMED) == 0)
    {
	    pthread_cond_timedwait(
	        &thred->suspendResumeCV, &thred->suspendResumeMutex, &onemillisec);
    }
    pthread_mutex_unlock(&thred->suspendResumeMutex);
#else
    while ((thred->suspend & PT_THREAD_RESUMED) == 0) {
		PRIntn rv = sigtimedwait(&sigwait_set, NULL, &onemillisec);
    	PR_ASSERT(-1 == rv);
	}
#endif

    thred->suspend &= ~PT_THREAD_RESUMED;

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, (
        "End pt_ResumeTest thred %p tid %X\n", thred, thred->id));
}  

static pthread_once_t pt_gc_support_control = PTHREAD_ONCE_INIT;

PR_IMPLEMENT(void) PR_SuspendAll(void)
{
#ifdef DEBUG
    PRIntervalTime stime, etime;
#endif
    PRThread* thred = pt_book.first;
    PRThread *me = PR_GetCurrentThread();
    int rv;

    rv = pthread_once(&pt_gc_support_control, init_pthread_gc_support);
    PR_ASSERT(0 == rv);
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_SuspendAll\n"));
    


    PR_Lock(pt_book.ml);
#ifdef DEBUG
    suspendAllOn = PR_TRUE;
    stime = PR_IntervalNow();
#endif
    while (thred != NULL)
    {
	    if ((thred != me) && _PT_IS_GCABLE_THREAD(thred))
    		pt_SuspendSet(thred);
        thred = thred->next;
    }

    
    thred = pt_book.first;
    while (thred != NULL)
    {
	    if ((thred != me) && _PT_IS_GCABLE_THREAD(thred))
            pt_SuspendTest(thred);
        thred = thred->next;
    }

    suspendAllSuspended = PR_TRUE;

#ifdef DEBUG
    etime = PR_IntervalNow();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS,\
        ("End PR_SuspendAll (time %dms)\n",
        PR_IntervalToMilliseconds(etime - stime)));
#endif
}  

PR_IMPLEMENT(void) PR_ResumeAll(void)
{
#ifdef DEBUG
    PRIntervalTime stime, etime;
#endif
    PRThread* thred = pt_book.first;
    PRThread *me = PR_GetCurrentThread();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_ResumeAll\n"));
    


    suspendAllSuspended = PR_FALSE;
#ifdef DEBUG
    stime = PR_IntervalNow();
#endif

    while (thred != NULL)
    {
	    if ((thred != me) && _PT_IS_GCABLE_THREAD(thred))
    	    pt_ResumeSet(thred);
        thred = thred->next;
    }

    thred = pt_book.first;
    while (thred != NULL)
    {
	    if ((thred != me) && _PT_IS_GCABLE_THREAD(thred))
    	    pt_ResumeTest(thred);
        thred = thred->next;
    }

    PR_Unlock(pt_book.ml);
#ifdef DEBUG
    suspendAllOn = PR_FALSE;
    etime = PR_IntervalNow();
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS,
        ("End PR_ResumeAll (time %dms)\n",
        PR_IntervalToMilliseconds(etime - stime)));
#endif
}  


PR_IMPLEMENT(void *)PR_GetSP(PRThread *thred)
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, 
	    ("in PR_GetSP thred %p thid = %X, sp = %p\n", 
	    thred, thred->id, thred->sp));
    return thred->sp;
}  

#else 

static pthread_once_t pt_gc_support_control = pthread_once_init;







PR_IMPLEMENT(void) PR_SuspendAll()
{
    PRIntn rv;

    rv = pthread_once(&pt_gc_support_control, init_pthread_gc_support);
    PR_ASSERT(0 == rv);  
#ifdef DEBUG
    suspendAllOn = PR_TRUE;
#endif
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_SuspendAll\n"));
    



    rv = sigprocmask(
        SIG_BLOCK, &javagc_vtalarm_sigmask, &javagc_intsoff_sigmask);
    PR_ASSERT(0 == rv);
    suspendAllSuspended = PR_TRUE;
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("End PR_SuspendAll\n"));
}  

PR_IMPLEMENT(void) PR_ResumeAll()
{
    PRIntn rv;
    
    suspendAllSuspended = PR_FALSE;
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_ResumeAll\n"));
    

    rv = sigprocmask(SIG_SETMASK, &javagc_intsoff_sigmask, (sigset_t *)NULL);
    PR_ASSERT(0 == rv);
#ifdef DEBUG
    suspendAllOn = PR_FALSE;
#endif

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("End PR_ResumeAll\n"));
}  


PR_IMPLEMENT(void*)PR_GetSP(PRThread *thred)
{
	pthread_t tid = thred->id;
	char *thread_tcb, *top_sp;

	








	PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin PR_GetSP\n"));
	thread_tcb = (char*)tid.field1;
	top_sp = *(char**)(thread_tcb + 128);
	PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("End PR_GetSP %p \n", top_sp));
	return top_sp;
}  

#endif 

#endif  


