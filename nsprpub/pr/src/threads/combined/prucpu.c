




































#include "primpl.h"

_PRCPU *_pr_primordialCPU = NULL;

PRInt32 _pr_md_idle_cpus;       







#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifndef _PR_HAVE_ATOMIC_OPS
static _MDLock _pr_md_idle_cpus_lock;
#endif
#endif
PRUintn _pr_numCPU;
PRInt32 _pr_cpus_exit;
PRInt32 _pr_cpu_affinity_mask = 0;

#if !defined (_PR_GLOBAL_THREADS_ONLY)

static PRUintn _pr_cpuID;

static void PR_CALLBACK _PR_CPU_Idle(void *);

static _PRCPU *_PR_CreateCPU(void);
static PRStatus _PR_StartCPU(_PRCPU *cpu, PRThread *thread);

#if !defined(_PR_LOCAL_THREADS_ONLY)
static void _PR_RunCPU(void *arg);
#endif

void  _PR_InitCPUs()
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (_native_threads_only)
        return;

    _pr_cpuID = 0;
    _MD_NEW_LOCK( &_pr_cpuLock);
#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifndef _PR_HAVE_ATOMIC_OPS
    _MD_NEW_LOCK(&_pr_md_idle_cpus_lock);
#endif
#endif

#ifdef _PR_LOCAL_THREADS_ONLY

#ifdef HAVE_CUSTOM_USER_THREADS
    _PR_MD_CREATE_PRIMORDIAL_USER_THREAD(me);
#endif

    
    _pr_primordialCPU = _PR_CreateCPU();
    _pr_numCPU = 1;
    _PR_StartCPU(_pr_primordialCPU, me);

    _PR_MD_SET_CURRENT_CPU(_pr_primordialCPU);

    
    _PR_MD_CURRENT_THREAD()->cpu = _pr_primordialCPU;

    _PR_MD_SET_LAST_THREAD(me);

#else 

    _pr_primordialCPU = _PR_CreateCPU();
    _pr_numCPU = 1;
    _PR_CreateThread(PR_SYSTEM_THREAD,
                     _PR_RunCPU,
                     _pr_primordialCPU,
                     PR_PRIORITY_NORMAL,
                     PR_GLOBAL_THREAD,
                     PR_UNJOINABLE_THREAD,
                     0,
                     _PR_IDLE_THREAD);

#endif 

    _PR_MD_INIT_CPUS();
}

#ifdef WINNT








void _PR_CleanupCPUs(void)
{
    PRUintn i;
    PRCList *qp;
    _PRCPU *cpu;

    _pr_cpus_exit = 1;
    for (i = 0; i < _pr_numCPU; i++) {
        _PR_MD_WAKEUP_WAITER(NULL);
    }
    for (qp = _PR_CPUQ().next; qp != &_PR_CPUQ(); qp = qp->next) {
        cpu = _PR_CPU_PTR(qp);
        _PR_MD_JOIN_THREAD(&cpu->thread->md);
    }
}
#endif

static _PRCPUQueue *_PR_CreateCPUQueue(void)
{
    PRInt32 index;
    _PRCPUQueue *cpuQueue;
    cpuQueue = PR_NEWZAP(_PRCPUQueue);
 
    _MD_NEW_LOCK( &cpuQueue->runQLock );
    _MD_NEW_LOCK( &cpuQueue->sleepQLock );
    _MD_NEW_LOCK( &cpuQueue->miscQLock );

    for (index = 0; index < PR_PRIORITY_LAST + 1; index++)
        PR_INIT_CLIST( &(cpuQueue->runQ[index]) );
    PR_INIT_CLIST( &(cpuQueue->sleepQ) );
    PR_INIT_CLIST( &(cpuQueue->pauseQ) );
    PR_INIT_CLIST( &(cpuQueue->suspendQ) );
    PR_INIT_CLIST( &(cpuQueue->waitingToJoinQ) );

    cpuQueue->numCPUs = 1;

    return cpuQueue;
}




















static _PRCPU *_PR_CreateCPU(void)
{
    _PRCPU *cpu;

    cpu = PR_NEWZAP(_PRCPU);
    if (cpu) {
        cpu->queue = _PR_CreateCPUQueue();
        if (!cpu->queue) {
            PR_DELETE(cpu);
            return NULL;
        }
    }
    return cpu;
}









static PRStatus _PR_StartCPU(_PRCPU *cpu, PRThread *thread)
{
    





    PR_ASSERT(!_native_threads_only);

    cpu->last_clock = PR_IntervalNow();

    


    _PR_MD_SET_CURRENT_CPU(cpu);
    _PR_MD_INIT_RUNNING_CPU(cpu);
    thread->cpu = cpu;

    cpu->idle_thread = _PR_CreateThread(PR_SYSTEM_THREAD,
                                        _PR_CPU_Idle,
                                        (void *)cpu,
                                        PR_PRIORITY_NORMAL,
                                        PR_LOCAL_THREAD,
                                        PR_UNJOINABLE_THREAD,
                                        0,
                                        _PR_IDLE_THREAD);

    if (!cpu->idle_thread) {
        
        PR_DELETE(cpu);
        return PR_FAILURE;
    } 
    PR_ASSERT(cpu->idle_thread->cpu == cpu);

    cpu->idle_thread->no_sched = 0;

    cpu->thread = thread;

    if (_pr_cpu_affinity_mask)
        PR_SetThreadAffinityMask(thread, _pr_cpu_affinity_mask);

    
    _PR_CPU_LIST_LOCK();
    cpu->id = _pr_cpuID++;
    PR_APPEND_LINK(&cpu->links, &_PR_CPUQ());
    _PR_CPU_LIST_UNLOCK();

    return PR_SUCCESS;
}

#if !defined(_PR_GLOBAL_THREADS_ONLY) && !defined(_PR_LOCAL_THREADS_ONLY)



static void _PR_RunCPU(void *arg)
{
    _PRCPU *cpu = (_PRCPU *)arg;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(NULL != me);

    















#ifdef HAVE_CUSTOM_USER_THREADS
    _PR_MD_CREATE_PRIMORDIAL_USER_THREAD(me);
#endif

    me->no_sched = 1;
    _PR_StartCPU(cpu, me);

#ifdef HAVE_CUSTOM_USER_THREADS
    me->flags &= (~_PR_GLOBAL_SCOPE);
#endif

    _PR_MD_SET_CURRENT_CPU(cpu);
    _PR_MD_SET_CURRENT_THREAD(cpu->thread);
    me->cpu = cpu;

    while(1) {
        PRInt32 is;
        if (!_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
	    _PR_MD_START_INTERRUPTS();
        _PR_MD_SWITCH_CONTEXT(me);
    }
}
#endif

static void PR_CALLBACK _PR_CPU_Idle(void *_cpu)
{
    _PRCPU *cpu = (_PRCPU *)_cpu;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(NULL != me);

    me->cpu = cpu;
    cpu->idle_thread = me;
    if (_MD_LAST_THREAD())
        _MD_LAST_THREAD()->no_sched = 0;
    if (!_PR_IS_NATIVE_THREAD(me)) _PR_MD_SET_INTSOFF(0);
    while(1) {
        PRInt32 is;
        PRIntervalTime timeout;
        if (!_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);

        _PR_RUNQ_LOCK(cpu);
#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifdef _PR_HAVE_ATOMIC_OPS
        _PR_MD_ATOMIC_INCREMENT(&_pr_md_idle_cpus);
#else
        _PR_MD_LOCK(&_pr_md_idle_cpus_lock);
        _pr_md_idle_cpus++;
        _PR_MD_UNLOCK(&_pr_md_idle_cpus_lock);
#endif 
#endif
        
        if (_PR_RUNQREADYMASK(me->cpu) != 0) {
            _PR_RUNQ_UNLOCK(cpu);
            timeout = PR_INTERVAL_NO_WAIT;
        } else {
            _PR_RUNQ_UNLOCK(cpu);

            _PR_SLEEPQ_LOCK(cpu);
            if (PR_CLIST_IS_EMPTY(&_PR_SLEEPQ(me->cpu))) {
                timeout = PR_INTERVAL_NO_TIMEOUT;
            } else {
                PRThread *wakeThread;
                wakeThread = _PR_THREAD_PTR(_PR_SLEEPQ(me->cpu).next);
                timeout = wakeThread->sleep;
            }
            _PR_SLEEPQ_UNLOCK(cpu);
        }

        
        (void)_PR_MD_PAUSE_CPU(timeout);

#ifdef WINNT
        if (_pr_cpus_exit) {
            
            _PR_MD_END_THREAD();
        }
#endif

#if !defined(_PR_LOCAL_THREADS_ONLY) && !defined(_PR_GLOBAL_THREADS_ONLY)
#ifdef _PR_HAVE_ATOMIC_OPS
        _PR_MD_ATOMIC_DECREMENT(&_pr_md_idle_cpus);
#else
        _PR_MD_LOCK(&_pr_md_idle_cpus_lock);
        _pr_md_idle_cpus--;
        _PR_MD_UNLOCK(&_pr_md_idle_cpus_lock);
#endif 
#endif

		_PR_ClockInterrupt();

		


		me->state = _PR_RUNNABLE;
		_PR_MD_SWITCH_CONTEXT(me);
		if (!_PR_IS_NATIVE_THREAD(me)) _PR_FAST_INTSON(is);
    }
}
#endif 

PR_IMPLEMENT(void) PR_SetConcurrency(PRUintn numCPUs)
{
#if defined(_PR_GLOBAL_THREADS_ONLY) || defined(_PR_LOCAL_THREADS_ONLY)

    

#else 

    PRUintn newCPU;
    _PRCPU *cpu;
    PRThread *thr;


    if (!_pr_initialized) _PR_ImplicitInitialization();

	if (_native_threads_only)
		return;
    
    _PR_CPU_LIST_LOCK();
    if (_pr_numCPU < numCPUs) {
        newCPU = numCPUs - _pr_numCPU;
        _pr_numCPU = numCPUs;
    } else newCPU = 0;
    _PR_CPU_LIST_UNLOCK();

    for (; newCPU; newCPU--) {
        cpu = _PR_CreateCPU();
        thr = _PR_CreateThread(PR_SYSTEM_THREAD,
                              _PR_RunCPU,
                              cpu,
                              PR_PRIORITY_NORMAL,
                              PR_GLOBAL_THREAD,
                              PR_UNJOINABLE_THREAD,
                              0,
                              _PR_IDLE_THREAD);
    }
#endif
}

PR_IMPLEMENT(_PRCPU *) _PR_GetPrimordialCPU(void)
{
    if (_pr_primordialCPU)
        return _pr_primordialCPU;
    else
        return _PR_MD_CURRENT_CPU();
}
