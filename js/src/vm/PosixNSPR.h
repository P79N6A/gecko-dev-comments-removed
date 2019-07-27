





#ifndef vm_PosixNSPR_h
#define vm_PosixNSPR_h

#ifdef JS_POSIX_NSPR

#include <pthread.h>
#include <stdint.h>

namespace nspr {
class Thread;
class Lock;
class CondVar;
};

typedef nspr::Thread PRThread;
typedef nspr::Lock PRLock;
typedef nspr::CondVar PRCondVar;

enum PRThreadType {
   PR_USER_THREAD,
   PR_SYSTEM_THREAD
};

enum PRThreadPriority
{
   PR_PRIORITY_FIRST   = 0,
   PR_PRIORITY_LOW     = 0,
   PR_PRIORITY_NORMAL  = 1,
   PR_PRIORITY_HIGH    = 2,
   PR_PRIORITY_URGENT  = 3,
   PR_PRIORITY_LAST    = 3
};

enum PRThreadScope {
   PR_LOCAL_THREAD,
   PR_GLOBAL_THREAD,
   PR_GLOBAL_BOUND_THREAD
};

enum PRThreadState {
   PR_JOINABLE_THREAD,
   PR_UNJOINABLE_THREAD
};

PRThread*
PR_CreateThread(PRThreadType type,
                void (*start)(void* arg),
                void* arg,
                PRThreadPriority priority,
                PRThreadScope scope,
                PRThreadState state,
                uint32_t stackSize);

typedef enum { PR_FAILURE = -1, PR_SUCCESS = 0 } PRStatus;

PRStatus
PR_JoinThread(PRThread* thread);

PRThread*
PR_GetCurrentThread();

PRStatus
PR_SetCurrentThreadName(const char* name);

typedef void (*PRThreadPrivateDTOR)(void* priv);

PRStatus
PR_NewThreadPrivateIndex(unsigned* newIndex, PRThreadPrivateDTOR destructor);

PRStatus
PR_SetThreadPrivate(unsigned index, void* priv);

void*
PR_GetThreadPrivate(unsigned index);

struct PRCallOnceType {
    int initialized;
    int32_t inProgress;
    PRStatus status;
};

typedef PRStatus (*PRCallOnceFN)();

PRStatus
PR_CallOnce(PRCallOnceType* once, PRCallOnceFN func);

typedef PRStatus (*PRCallOnceWithArgFN)(void*);

PRStatus
PR_CallOnceWithArg(PRCallOnceType* once, PRCallOnceWithArgFN func, void* arg);

PRLock*
PR_NewLock();

void
PR_DestroyLock(PRLock* lock);

void
PR_Lock(PRLock* lock);

PRStatus
PR_Unlock(PRLock* lock);

PRCondVar*
PR_NewCondVar(PRLock* lock);

void
PR_DestroyCondVar(PRCondVar* cvar);

PRStatus
PR_NotifyCondVar(PRCondVar* cvar);

PRStatus
PR_NotifyAllCondVar(PRCondVar* cvar);

#define PR_INTERVAL_MIN 1000UL
#define PR_INTERVAL_MAX 100000UL

#define PR_INTERVAL_NO_WAIT 0UL
#define PR_INTERVAL_NO_TIMEOUT 0xffffffffUL

uint32_t
PR_MillisecondsToInterval(uint32_t milli);

uint32_t
PR_MicrosecondsToInterval(uint32_t micro);

uint32_t
PR_TicksPerSecond();

PRStatus
PR_WaitCondVar(PRCondVar* cvar, uint32_t timeout);

#endif 

#endif 
