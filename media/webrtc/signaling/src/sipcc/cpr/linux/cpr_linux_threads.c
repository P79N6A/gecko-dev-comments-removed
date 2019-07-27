



#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "prtypes.h"
#include "mozilla/Assertions.h"
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>

#define LINUX_MIN_THREAD_PRIORITY (-20)	/* tbd: check MV linux: current val from Larry port */
#define LINUX_MAX_THREAD_PRIORITY (+19)	/* tbd: check MV linux. current val from Larry port */

void CSFLogRegisterThread(const cprThread_t thread);























cprThread_t
cprCreateThread (const char *name,
                 cprThreadStartRoutine startRoutine,
                 uint16_t stackSize,
                 uint16_t priority,
                 void *data)
{
    static const char fname[] = "cprCreateThread";
    static uint16_t id = 0;
    cpr_thread_t *threadPtr;
    pthread_t threadId;
    pthread_attr_t attr;

    CPR_INFO("%s: creating '%s' thread\n", fname, name);

    
    threadPtr = (cpr_thread_t *)cpr_malloc(sizeof(cpr_thread_t));
    if (threadPtr != NULL) {
        if (pthread_attr_init(&attr) != 0) {

            CPR_ERROR("%s - Failed to init attribute for thread %s\n",
                      fname, name);
            cpr_free(threadPtr);
            return (cprThread_t)NULL;
        }

        if (pthread_attr_setstacksize(&attr, stackSize) != 0) {
            CPR_ERROR("%s - Invalid stacksize %d specified for thread %s\n",
                      fname, stackSize, name);
            cpr_free(threadPtr);
            return (cprThread_t)NULL;
        }

        if (pthread_create(&threadId, &attr, startRoutine, data) != 0) {
            CPR_ERROR("%s - Creation of thread %s failed: %d\n",
                      fname, name, errno);
            cpr_free(threadPtr);
            return (cprThread_t)NULL;
        }

        
        if (name != NULL) {
            threadPtr->name = name;
        }

        






        threadPtr->u.handleInt = threadId;
        threadPtr->threadId = ++id;
        CSFLogRegisterThread(threadPtr);
        return (cprThread_t)threadPtr;
    }

    
    CPR_ERROR("%s - Malloc for thread %s failed.\n", fname, name);
    errno = ENOMEM;
    return (cprThread_t)NULL;
}






void cprJoinThread(cprThread_t thread)
{
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t *) thread;
    MOZ_ASSERT(cprThreadPtr);
    pthread_join(cprThreadPtr->u.handleInt, NULL);
}
















cprRC_t
cprDestroyThread (cprThread_t thread)
{
    cpr_thread_t *cprThreadPtr;

    cprThreadPtr = (cpr_thread_t *) thread;
    if (cprThreadPtr) {
        


        if ((pthread_t) cprThreadPtr->u.handleInt == pthread_self()) {
            CPR_INFO("%s: Destroying Thread %d", __FUNCTION__, cprThreadPtr->threadId);
            pthread_exit(NULL);
            return CPR_SUCCESS;
        }

        CPR_ERROR("%s: Thread attempted to destroy another thread, not itself.",
                  __FUNCTION__);
        MOZ_ASSERT(PR_FALSE);
        errno = EINVAL;
        return CPR_FAILURE;
    }

    CPR_ERROR("%s - NULL pointer passed in.", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    errno = EINVAL;
    return CPR_FAILURE;
}













cprRC_t
cprAdjustRelativeThreadPriority (int relPri)
{
    const char *fname = "cprAdjustRelativeThreadPriority";

    if (setpriority(PRIO_PROCESS, 0, relPri) == -1) {
        CPR_ERROR("%s: could not set the nice..err=%d\n",
                  fname, errno);
        return CPR_FAILURE;
    }
    return CPR_SUCCESS;
}




















pthread_t
cprGetThreadId (cprThread_t thread)
{
    if (thread) {
        return ((cpr_thread_t *)thread)->u.handleInt;
    }
    return 0;
}



