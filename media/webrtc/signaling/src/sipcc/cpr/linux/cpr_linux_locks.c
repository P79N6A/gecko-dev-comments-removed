



#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>






















cprMutex_t
cprCreateMutex (const char *name)
{
    static const char fname[] = "cprCreateMutex";
    static uint16_t id = 0;
    int32_t returnCode;
    cpr_mutex_t *cprMutexPtr;
    pthread_mutex_t *pthreadMutexPtr;

    




    cprMutexPtr = (cpr_mutex_t *) cpr_malloc(sizeof(cpr_mutex_t));
    pthreadMutexPtr = (pthread_mutex_t *) cpr_malloc(sizeof(pthread_mutex_t));
    if ((cprMutexPtr != NULL) && (pthreadMutexPtr != NULL)) {
        
        cprMutexPtr->name = name;

        



        returnCode = pthread_mutex_init(pthreadMutexPtr, NULL);
        if (returnCode != 0) {
            CPR_ERROR("%s - Failure trying to init Mutex %s: %d\n",
                      fname, name, returnCode);
            cpr_free(pthreadMutexPtr);
            cpr_free(cprMutexPtr);
            return (cprMutex_t)NULL;
        }

        






        cprMutexPtr->u.handlePtr = pthreadMutexPtr;
        cprMutexPtr->lockId = ++id;
        return (cprMutex_t)cprMutexPtr;
    }

    




    if (pthreadMutexPtr != NULL) {
        cpr_free(pthreadMutexPtr);
    } else if (cprMutexPtr != NULL) {
        cpr_free(cprMutexPtr);
    }

    
    CPR_ERROR("%s - Malloc for mutex %s failed.\n", fname, name);
    errno = ENOMEM;
    return (cprMutex_t)NULL;
}
















cprRC_t
cprDestroyMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprDestroyMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_destroy(cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Failure destroying Mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        cprMutexPtr->lockId = 0;
        cpr_free(cprMutexPtr->u.handlePtr);
        cpr_free(cprMutexPtr);
        return CPR_SUCCESS;
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}















cprRC_t
cprGetMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprGetMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_lock((pthread_mutex_t *) cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Error acquiring mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}












cprRC_t
cprReleaseMutex (cprMutex_t mutex)
{
    static const char fname[] = "cprReleaseMutex";
    cpr_mutex_t *cprMutexPtr;
    int32_t rc;

    cprMutexPtr = (cpr_mutex_t *) mutex;
    if (cprMutexPtr != NULL) {
        rc = pthread_mutex_unlock((pthread_mutex_t *) cprMutexPtr->u.handlePtr);
        if (rc != 0) {
            CPR_ERROR("%s - Error releasing mutex %s: %d\n",
                      fname, cprMutexPtr->name, rc);
            return CPR_FAILURE;
        }
        return CPR_SUCCESS;
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}

