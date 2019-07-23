










































#if defined(_PR_PTHREADS)

#include "primpl.h"
#include "obsolete/prsem.h"

#include <string.h>
#include <pthread.h>
#include <sys/time.h>

static pthread_mutexattr_t _pt_mattr;
static pthread_condattr_t _pt_cvar_attr;

#if defined(DEBUG)
extern PTDebug pt_debug;  

#if defined(_PR_DCETHREADS)
static pthread_t pt_zero_tid;  

#endif  
#endif  

#if defined(FREEBSD)




static int
pt_pthread_mutex_is_locked(pthread_mutex_t *m)
{
    int rv = pthread_mutex_trylock(m);
    return (EBUSY == rv || EDEADLK == rv);
}
#endif







void _PR_InitLocks(void)
{
    int rv;
    rv = _PT_PTHREAD_MUTEXATTR_INIT(&_pt_mattr); 
    PR_ASSERT(0 == rv);

#ifdef LINUX
#if (__GLIBC__ > 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 2)
    rv = pthread_mutexattr_settype(&_pt_mattr, PTHREAD_MUTEX_ADAPTIVE_NP);
    PR_ASSERT(0 == rv);
#endif
#endif

    rv = _PT_PTHREAD_CONDATTR_INIT(&_pt_cvar_attr);
    PR_ASSERT(0 == rv);
}

static void pt_PostNotifies(PRLock *lock, PRBool unlock)
{
    PRIntn index, rv;
    _PT_Notified post;
    _PT_Notified *notified, *prev = NULL;
    





    post = lock->notified;  

#if defined(DEBUG)
    memset(&lock->notified, 0, sizeof(_PT_Notified));  
#else
    lock->notified.length = 0;  
    lock->notified.link = NULL;
#endif

    
    if (unlock)
    {
        rv = pthread_mutex_unlock(&lock->mutex);
        PR_ASSERT(0 == rv);
    }

    notified = &post;  
    do
    {
        for (index = 0; index < notified->length; ++index)
        {
            PRCondVar *cv = notified->cv[index].cv;
            PR_ASSERT(NULL != cv);
            PR_ASSERT(0 != notified->cv[index].times);
            if (-1 == notified->cv[index].times)
            {
                rv = pthread_cond_broadcast(&cv->cv);
                PR_ASSERT(0 == rv);
            }
            else
            {
                while (notified->cv[index].times-- > 0)
                {
                    rv = pthread_cond_signal(&cv->cv);
                    PR_ASSERT(0 == rv);
                }
            }
#if defined(DEBUG)
            pt_debug.cvars_notified += 1;
            if (0 > PR_AtomicDecrement(&cv->notify_pending))
            {
                pt_debug.delayed_cv_deletes += 1;
                PR_DestroyCondVar(cv);
            }
#else  
            if (0 > PR_AtomicDecrement(&cv->notify_pending))
                PR_DestroyCondVar(cv);
#endif  
        }
        prev = notified;
        notified = notified->link;
        if (&post != prev) PR_DELETE(prev);
    } while (NULL != notified);
}  

PR_IMPLEMENT(PRLock*) PR_NewLock(void)
{
    PRIntn rv;
    PRLock *lock;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    lock = PR_NEWZAP(PRLock);
    if (lock != NULL)
    {
        rv = _PT_PTHREAD_MUTEX_INIT(lock->mutex, _pt_mattr); 
        PR_ASSERT(0 == rv);
    }
#if defined(DEBUG)
    pt_debug.locks_created += 1;
#endif
    return lock;
}  

PR_IMPLEMENT(void) PR_DestroyLock(PRLock *lock)
{
    PRIntn rv;
    PR_ASSERT(NULL != lock);
    PR_ASSERT(PR_FALSE == lock->locked);
    PR_ASSERT(0 == lock->notified.length);
    PR_ASSERT(NULL == lock->notified.link);
    rv = pthread_mutex_destroy(&lock->mutex);
    PR_ASSERT(0 == rv);
#if defined(DEBUG)
    memset(lock, 0xaf, sizeof(PRLock));
    pt_debug.locks_destroyed += 1;
#endif
    PR_DELETE(lock);
}  

PR_IMPLEMENT(void) PR_Lock(PRLock *lock)
{
    PRIntn rv;
    PR_ASSERT(lock != NULL);
    rv = pthread_mutex_lock(&lock->mutex);
    PR_ASSERT(0 == rv);
    PR_ASSERT(0 == lock->notified.length);
    PR_ASSERT(NULL == lock->notified.link);
    PR_ASSERT(PR_FALSE == lock->locked);
    



    lock->owner = pthread_self();
    lock->locked = PR_TRUE;
#if defined(DEBUG)
    pt_debug.locks_acquired += 1;
#endif
}  

PR_IMPLEMENT(PRStatus) PR_Unlock(PRLock *lock)
{
    PRIntn rv;

    PR_ASSERT(lock != NULL);
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(lock->mutex));
    PR_ASSERT(PR_TRUE == lock->locked);
    PR_ASSERT(pthread_equal(lock->owner, pthread_self()));

    if (!lock->locked || !pthread_equal(lock->owner, pthread_self()))
        return PR_FAILURE;

    lock->locked = PR_FALSE;
    if (0 == lock->notified.length)  
    {
        rv = pthread_mutex_unlock(&lock->mutex);
        PR_ASSERT(0 == rv);
    }
    else pt_PostNotifies(lock, PR_TRUE);

#if defined(DEBUG)
    pt_debug.locks_released += 1;
#endif
    return PR_SUCCESS;
}  

PR_IMPLEMENT(void) PR_AssertCurrentThreadOwnsLock(PRLock *lock)
{
    



    PR_ASSERT(lock->locked && pthread_equal(lock->owner, pthread_self()));
}













#define PT_NANOPERMICRO 1000UL
#define PT_BILLION 1000000000UL

static PRIntn pt_TimedWait(
    pthread_cond_t *cv, pthread_mutex_t *ml, PRIntervalTime timeout)
{
    int rv;
    struct timeval now;
    struct timespec tmo;
    PRUint32 ticks = PR_TicksPerSecond();

    tmo.tv_sec = (PRInt32)(timeout / ticks);
    tmo.tv_nsec = (PRInt32)(timeout - (tmo.tv_sec * ticks));
    tmo.tv_nsec = (PRInt32)PR_IntervalToMicroseconds(PT_NANOPERMICRO * tmo.tv_nsec);

    
    (void)GETTIMEOFDAY(&now);
    
    tmo.tv_sec += now.tv_sec;
    tmo.tv_nsec += (PT_NANOPERMICRO * now.tv_usec);
    tmo.tv_sec += tmo.tv_nsec / PT_BILLION;
    tmo.tv_nsec %= PT_BILLION;

    rv = pthread_cond_timedwait(cv, ml, &tmo);

    
#ifdef _PR_DCETHREADS
    if (rv == -1) return (errno == EAGAIN) ? 0 : errno;
    else return rv;
#else
    return (rv == ETIMEDOUT) ? 0 : rv;
#endif
}  







static void pt_PostNotifyToCvar(PRCondVar *cvar, PRBool broadcast)
{
    PRIntn index = 0;
    _PT_Notified *notified = &cvar->lock->notified;

    PR_ASSERT(PR_TRUE == cvar->lock->locked);
    PR_ASSERT(pthread_equal(cvar->lock->owner, pthread_self()));
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(cvar->lock->mutex));

    while (1)
    {
        for (index = 0; index < notified->length; ++index)
        {
            if (notified->cv[index].cv == cvar)
            {
                if (broadcast)
                    notified->cv[index].times = -1;
                else if (-1 != notified->cv[index].times)
                    notified->cv[index].times += 1;
                goto finished;  
            }
        }
        
        if (notified->length < PT_CV_NOTIFIED_LENGTH) break;

        
        if (NULL == notified->link)
            notified->link = PR_NEWZAP(_PT_Notified);
        notified = notified->link;
    }

    
    (void)PR_AtomicIncrement(&cvar->notify_pending);
    notified->cv[index].times = (broadcast) ? -1 : 1;
    notified->cv[index].cv = cvar;
    notified->length += 1;

finished:
    PR_ASSERT(PR_TRUE == cvar->lock->locked);
    PR_ASSERT(pthread_equal(cvar->lock->owner, pthread_self()));
}  

PR_IMPLEMENT(PRCondVar*) PR_NewCondVar(PRLock *lock)
{
    PRCondVar *cv = PR_NEW(PRCondVar);
    PR_ASSERT(lock != NULL);
    if (cv != NULL)
    {
        int rv = _PT_PTHREAD_COND_INIT(cv->cv, _pt_cvar_attr); 
        PR_ASSERT(0 == rv);
        cv->lock = lock;
        cv->notify_pending = 0;
#if defined(DEBUG)
        pt_debug.cvars_created += 1;
#endif
    }
    return cv;
}  

PR_IMPLEMENT(void) PR_DestroyCondVar(PRCondVar *cvar)
{
    if (0 > PR_AtomicDecrement(&cvar->notify_pending))
    {
        PRIntn rv = pthread_cond_destroy(&cvar->cv); PR_ASSERT(0 == rv);
#if defined(DEBUG)
        memset(cvar, 0xaf, sizeof(PRCondVar));
        pt_debug.cvars_destroyed += 1;
#endif
        PR_DELETE(cvar);
    }
}  

PR_IMPLEMENT(PRStatus) PR_WaitCondVar(PRCondVar *cvar, PRIntervalTime timeout)
{
    PRIntn rv;
    PRThread *thred = PR_GetCurrentThread();

    PR_ASSERT(cvar != NULL);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(cvar->lock->mutex));
    PR_ASSERT(PR_TRUE == cvar->lock->locked);
    
    PR_ASSERT(pthread_equal(cvar->lock->owner, pthread_self()));

    if (_PT_THREAD_INTERRUPTED(thred)) goto aborted;

    


    thred->waiting = cvar;  

    







    if (0 != cvar->lock->notified.length)
        pt_PostNotifies(cvar->lock, PR_FALSE);

    


    cvar->lock->locked = PR_FALSE;

    if (timeout == PR_INTERVAL_NO_TIMEOUT)
        rv = pthread_cond_wait(&cvar->cv, &cvar->lock->mutex);
    else
        rv = pt_TimedWait(&cvar->cv, &cvar->lock->mutex, timeout);

    
    PR_ASSERT(PR_FALSE == cvar->lock->locked);
    cvar->lock->locked = PR_TRUE;
    cvar->lock->owner = pthread_self();

    PR_ASSERT(0 == cvar->lock->notified.length);
    thred->waiting = NULL;  
    if (_PT_THREAD_INTERRUPTED(thred)) goto aborted;
    if (rv != 0)
    {
        _PR_MD_MAP_DEFAULT_ERROR(rv);
        return PR_FAILURE;
    }
    return PR_SUCCESS;

aborted:
    PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
    thred->state &= ~PT_THREAD_ABORTED;
    return PR_FAILURE;
}  

PR_IMPLEMENT(PRStatus) PR_NotifyCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);   
    pt_PostNotifyToCvar(cvar, PR_FALSE);
    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PR_NotifyAllCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar != NULL);
    pt_PostNotifyToCvar(cvar, PR_TRUE);
    return PR_SUCCESS;
}  







PR_IMPLEMENT(PRMonitor*) PR_NewMonitor(void)
{
    PRMonitor *mon;
    PRCondVar *cvar;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    cvar = PR_NEWZAP(PRCondVar);
    if (NULL == cvar)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }
    mon = PR_NEWZAP(PRMonitor);
    if (mon != NULL)
    {
        int rv;
        rv = _PT_PTHREAD_MUTEX_INIT(mon->lock.mutex, _pt_mattr); 
        PR_ASSERT(0 == rv);

        _PT_PTHREAD_INVALIDATE_THR_HANDLE(mon->owner);

        mon->cvar = cvar;
        rv = _PT_PTHREAD_COND_INIT(mon->cvar->cv, _pt_cvar_attr); 
        PR_ASSERT(0 == rv);
        mon->entryCount = 0;
        mon->cvar->lock = &mon->lock;
        if (0 != rv)
        {
            PR_DELETE(mon);
            PR_DELETE(cvar);
            mon = NULL;
        }
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
    int rv;
    PR_ASSERT(mon != NULL);
    PR_DestroyCondVar(mon->cvar);
    rv = pthread_mutex_destroy(&mon->lock.mutex); PR_ASSERT(0 == rv);
#if defined(DEBUG)
        memset(mon, 0xaf, sizeof(PRMonitor));
#endif
    PR_DELETE(mon);    
}  





PR_IMPLEMENT(PRIntn) PR_GetMonitorEntryCount(PRMonitor *mon)
{
    pthread_t self = pthread_self();
    if (pthread_equal(mon->owner, self))
        return mon->entryCount;
    return 0;
}

PR_IMPLEMENT(void) PR_AssertCurrentThreadInMonitor(PRMonitor *mon)
{
    PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(&mon->lock);
}

PR_IMPLEMENT(void) PR_EnterMonitor(PRMonitor *mon)
{
    pthread_t self = pthread_self();

    PR_ASSERT(mon != NULL);
    




    if (!pthread_equal(mon->owner, self))
    {
        PR_Lock(&mon->lock);
        
        PR_ASSERT(0 == mon->entryCount);
        PR_ASSERT(_PT_PTHREAD_THR_HANDLE_IS_INVALID(mon->owner));
        _PT_PTHREAD_COPY_THR_HANDLE(self, mon->owner);
    }
    mon->entryCount += 1;
}  

PR_IMPLEMENT(PRStatus) PR_ExitMonitor(PRMonitor *mon)
{
    pthread_t self = pthread_self();

    PR_ASSERT(mon != NULL);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    
    PR_ASSERT(pthread_equal(mon->owner, self));
    if (!pthread_equal(mon->owner, self))
        return PR_FAILURE;

    
    PR_ASSERT(mon->entryCount > 0);
    mon->entryCount -= 1;  
    if (mon->entryCount == 0)
    {
        
        _PT_PTHREAD_INVALIDATE_THR_HANDLE(mon->owner);  
        PR_Unlock(&mon->lock);
    }
    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime timeout)
{
    PRStatus rv;
    PRInt16 saved_entries;
    pthread_t saved_owner;

    PR_ASSERT(mon != NULL);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    
    PR_ASSERT(mon->entryCount > 0);
    
    PR_ASSERT(pthread_equal(mon->owner, pthread_self()));

    
    saved_entries = mon->entryCount; 
    mon->entryCount = 0;
    _PT_PTHREAD_COPY_THR_HANDLE(mon->owner, saved_owner);
    _PT_PTHREAD_INVALIDATE_THR_HANDLE(mon->owner);
    
    rv = PR_WaitCondVar(mon->cvar, timeout);

    
    mon->entryCount = saved_entries;
    _PT_PTHREAD_COPY_THR_HANDLE(saved_owner, mon->owner);

    return rv;
}  

PR_IMPLEMENT(PRStatus) PR_Notify(PRMonitor *mon)
{
    PR_ASSERT(NULL != mon);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    
    PR_ASSERT(mon->entryCount > 0);
    
    PR_ASSERT(pthread_equal(mon->owner, pthread_self()));

    pt_PostNotifyToCvar(mon->cvar, PR_FALSE);

    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PR_NotifyAll(PRMonitor *mon)
{
    PR_ASSERT(mon != NULL);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(mon->lock.mutex));
    
    PR_ASSERT(mon->entryCount > 0);
    
    PR_ASSERT(pthread_equal(mon->owner, pthread_self()));

    pt_PostNotifyToCvar(mon->cvar, PR_TRUE);

    return PR_SUCCESS;
}  






PR_IMPLEMENT(void) PR_PostSem(PRSemaphore *semaphore)
{
    static PRBool unwarned = PR_TRUE;
    if (unwarned) unwarned = _PR_Obsolete(
        "PR_PostSem", "locks & condition variables");
	PR_Lock(semaphore->cvar->lock);
	PR_NotifyCondVar(semaphore->cvar);
	semaphore->count += 1;
	PR_Unlock(semaphore->cvar->lock);
}  

PR_IMPLEMENT(PRStatus) PR_WaitSem(PRSemaphore *semaphore)
{
	PRStatus status = PR_SUCCESS;
    static PRBool unwarned = PR_TRUE;
    if (unwarned) unwarned = _PR_Obsolete(
        "PR_WaitSem", "locks & condition variables");
	PR_Lock(semaphore->cvar->lock);
	while ((semaphore->count == 0) && (PR_SUCCESS == status))
		status = PR_WaitCondVar(semaphore->cvar, PR_INTERVAL_NO_TIMEOUT);
	if (PR_SUCCESS == status) semaphore->count -= 1;
	PR_Unlock(semaphore->cvar->lock);
	return status;
}  

PR_IMPLEMENT(void) PR_DestroySem(PRSemaphore *semaphore)
{
    static PRBool unwarned = PR_TRUE;
    if (unwarned) unwarned = _PR_Obsolete(
        "PR_DestroySem", "locks & condition variables");
    PR_DestroyLock(semaphore->cvar->lock);
    PR_DestroyCondVar(semaphore->cvar);
    PR_DELETE(semaphore);
}  

PR_IMPLEMENT(PRSemaphore*) PR_NewSem(PRUintn value)
{
    PRSemaphore *semaphore;
    static PRBool unwarned = PR_TRUE;
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (unwarned) unwarned = _PR_Obsolete(
        "PR_NewSem", "locks & condition variables");

    semaphore = PR_NEWZAP(PRSemaphore);
    if (NULL != semaphore)
    {
        PRLock *lock = PR_NewLock();
        if (NULL != lock)
        {
            semaphore->cvar = PR_NewCondVar(lock);
            if (NULL != semaphore->cvar)
            {
                semaphore->count = value;
                return semaphore;
            }
            PR_DestroyLock(lock);
        }
        PR_DELETE(semaphore);
    }
    return NULL;
}









#ifdef _PR_HAVE_POSIX_SEMAPHORES
#include <fcntl.h>

PR_IMPLEMENT(PRSem *) PR_OpenSemaphore(
    const char *name,
    PRIntn flags,
    PRIntn mode,
    PRUintn value)
{
    PRSem *sem;
    char osname[PR_IPC_NAME_SIZE];

    if (_PR_MakeNativeIPCName(name, osname, sizeof(osname), _PRIPCSem)
        == PR_FAILURE)
    {
        return NULL;
    }

    sem = PR_NEW(PRSem);
    if (NULL == sem)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }

    if (flags & PR_SEM_CREATE)
    {
        int oflag = O_CREAT;

        if (flags & PR_SEM_EXCL) oflag |= O_EXCL;
        sem->sem = sem_open(osname, oflag, mode, value);
    }
    else
    {
#ifdef HPUX
        
        sem->sem = sem_open(osname, 0, 0, 0);
#else
        sem->sem = sem_open(osname, 0);
#endif
    }
    if ((sem_t *) -1 == sem->sem)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        PR_DELETE(sem);
        return NULL;
    }
    return sem;
}

PR_IMPLEMENT(PRStatus) PR_WaitSemaphore(PRSem *sem)
{
    int rv;
    rv = sem_wait(sem->sem);
    if (0 != rv)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_PostSemaphore(PRSem *sem)
{
    int rv;
    rv = sem_post(sem->sem);
    if (0 != rv)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_CloseSemaphore(PRSem *sem)
{
    int rv;
    rv = sem_close(sem->sem);
    if (0 != rv)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    PR_DELETE(sem);
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_DeleteSemaphore(const char *name)
{
    int rv;
    char osname[PR_IPC_NAME_SIZE];

    if (_PR_MakeNativeIPCName(name, osname, sizeof(osname), _PRIPCSem)
        == PR_FAILURE)
    {
        return PR_FAILURE;
    }
    rv = sem_unlink(osname);
    if (0 != rv)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}
    
#elif defined(_PR_HAVE_SYSV_SEMAPHORES)

#include <fcntl.h>
#include <sys/sem.h>




#if (defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)) \
    || defined(FREEBSD) || defined(OPENBSD) || defined(BSDI) \
    || defined(DARWIN) || defined(SYMBIAN)

#else

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short  *array;
};
#endif




#define NSPR_IPC_KEY_ID 'a'  /* the id argument for ftok() */

#define NSPR_SEM_MODE 0666

PR_IMPLEMENT(PRSem *) PR_OpenSemaphore(
    const char *name,
    PRIntn flags,
    PRIntn mode,
    PRUintn value)
{
    PRSem *sem;
    key_t key;
    union semun arg;
    struct sembuf sop;
    struct semid_ds seminfo;
#define MAX_TRIES 60
    PRIntn i;
    char osname[PR_IPC_NAME_SIZE];

    if (_PR_MakeNativeIPCName(name, osname, sizeof(osname), _PRIPCSem)
        == PR_FAILURE)
    {
        return NULL;
    }

    
    if (flags & PR_SEM_CREATE)
    {
        int osfd = open(osname, O_RDWR|O_CREAT, mode);
        if (-1 == osfd)
        {
            _PR_MD_MAP_OPEN_ERROR(errno);
            return NULL;
        }
        if (close(osfd) == -1)
        {
            _PR_MD_MAP_CLOSE_ERROR(errno);
            return NULL;
        }
    }
    key = ftok(osname, NSPR_IPC_KEY_ID);
    if ((key_t)-1 == key)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return NULL;
    }

    sem = PR_NEW(PRSem);
    if (NULL == sem)
    {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
        return NULL;
    }

    if (flags & PR_SEM_CREATE)
    {
        sem->semid = semget(key, 1, mode|IPC_CREAT|IPC_EXCL);
        if (sem->semid >= 0)
        {
            
            arg.val = 0;
            if (semctl(sem->semid, 0, SETVAL, arg) == -1)
            {
                _PR_MD_MAP_DEFAULT_ERROR(errno);
                PR_DELETE(sem);
                return NULL;
            }
            
            sop.sem_num = 0;
            sop.sem_op = value;
            sop.sem_flg = 0;
            if (semop(sem->semid, &sop, 1) == -1)
            {
                _PR_MD_MAP_DEFAULT_ERROR(errno);
                PR_DELETE(sem);
                return NULL;
            }
            return sem;
        }

        if (errno != EEXIST || flags & PR_SEM_EXCL)
        {
            _PR_MD_MAP_DEFAULT_ERROR(errno);
            PR_DELETE(sem);
            return NULL;
        }
    }

    sem->semid = semget(key, 1, NSPR_SEM_MODE);
    if (sem->semid == -1)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        PR_DELETE(sem);
        return NULL;
    }
    for (i = 0; i < MAX_TRIES; i++)
    {
        arg.buf = &seminfo;
        semctl(sem->semid, 0, IPC_STAT, arg);
        if (seminfo.sem_otime != 0) break;
        sleep(1);
    }
    if (i == MAX_TRIES)
    {
        PR_SetError(PR_IO_TIMEOUT_ERROR, 0);
        PR_DELETE(sem);
        return NULL;
    }
    return sem;
}

PR_IMPLEMENT(PRStatus) PR_WaitSemaphore(PRSem *sem)
{
    struct sembuf sop;

    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if (semop(sem->semid, &sop, 1) == -1)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_PostSemaphore(PRSem *sem)
{
    struct sembuf sop;

    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if (semop(sem->semid, &sop, 1) == -1)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_CloseSemaphore(PRSem *sem)
{
    PR_DELETE(sem);
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus) PR_DeleteSemaphore(const char *name)
{
    key_t key;
    int semid;
    
    union semun unused;
    char osname[PR_IPC_NAME_SIZE];

    if (_PR_MakeNativeIPCName(name, osname, sizeof(osname), _PRIPCSem)
        == PR_FAILURE)
    {
        return PR_FAILURE;
    }
    key = ftok(osname, NSPR_IPC_KEY_ID);
    if ((key_t) -1 == key)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    if (unlink(osname) == -1)
    {
        _PR_MD_MAP_UNLINK_ERROR(errno);
        return PR_FAILURE;
    }
    semid = semget(key, 1, NSPR_SEM_MODE);
    if (-1 == semid)
    {
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    unused.val = 0;
    if (semctl(semid, 0, IPC_RMID, unused) == -1)
    { 
        _PR_MD_MAP_DEFAULT_ERROR(errno);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}

#else 

PR_IMPLEMENT(PRSem *) PR_OpenSemaphore(
    const char *name,
    PRIntn flags,
    PRIntn mode,
    PRUintn value)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PR_IMPLEMENT(PRStatus) PR_WaitSemaphore(PRSem *sem)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PR_IMPLEMENT(PRStatus) PR_PostSemaphore(PRSem *sem)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PR_IMPLEMENT(PRStatus) PR_CloseSemaphore(PRSem *sem)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

PR_IMPLEMENT(PRStatus) PR_DeleteSemaphore(const char *name)
{
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return PR_FAILURE;
}

#endif 







#include "prpdce.h"

PR_IMPLEMENT(PRStatus) PRP_TryLock(PRLock *lock)
{
    PRIntn rv = pthread_mutex_trylock(&lock->mutex);
    if (rv == PT_TRYLOCK_SUCCESS)
    {
        PR_ASSERT(PR_FALSE == lock->locked);
        lock->locked = PR_TRUE;
        lock->owner = pthread_self();
    }
    
    return (PT_TRYLOCK_SUCCESS == rv) ? PR_SUCCESS : PR_FAILURE;
}  

PR_IMPLEMENT(PRCondVar*) PRP_NewNakedCondVar(void)
{
    PRCondVar *cv;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    cv = PR_NEW(PRCondVar);
    if (cv != NULL)
    {
        int rv;
        rv = _PT_PTHREAD_COND_INIT(cv->cv, _pt_cvar_attr); 
        PR_ASSERT(0 == rv);
        cv->lock = _PR_NAKED_CV_LOCK;
    }
    return cv;
}  

PR_IMPLEMENT(void) PRP_DestroyNakedCondVar(PRCondVar *cvar)
{
    int rv;
    rv = pthread_cond_destroy(&cvar->cv); PR_ASSERT(0 == rv);
#if defined(DEBUG)
        memset(cvar, 0xaf, sizeof(PRCondVar));
#endif
    PR_DELETE(cvar);
}  

PR_IMPLEMENT(PRStatus) PRP_NakedWait(
    PRCondVar *cvar, PRLock *ml, PRIntervalTime timeout)
{
    PRIntn rv;
    PR_ASSERT(cvar != NULL);
    
    PR_ASSERT(_PT_PTHREAD_MUTEX_IS_LOCKED(ml->mutex));
    if (timeout == PR_INTERVAL_NO_TIMEOUT)
        rv = pthread_cond_wait(&cvar->cv, &ml->mutex);
    else
        rv = pt_TimedWait(&cvar->cv, &ml->mutex, timeout);
    if (rv != 0)
    {
        _PR_MD_MAP_DEFAULT_ERROR(rv);
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PRP_NakedNotify(PRCondVar *cvar)
{
    int rv;
    PR_ASSERT(cvar != NULL);
    rv = pthread_cond_signal(&cvar->cv);
    PR_ASSERT(0 == rv);
    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PRP_NakedBroadcast(PRCondVar *cvar)
{
    int rv;
    PR_ASSERT(cvar != NULL);
    rv = pthread_cond_broadcast(&cvar->cv);
    PR_ASSERT(0 == rv);
    return PR_SUCCESS;
}  

#endif  


