









#include "pkix_pl_mutex.h"







static PKIX_Error *
pkix_pl_Mutex_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Mutex *mutex = NULL;

        PKIX_ENTER(MUTEX, "pkix_pl_Mutex_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType(object, PKIX_MUTEX_TYPE, plContext),
                    PKIX_OBJECTNOTMUTEX);

        mutex = (PKIX_PL_Mutex*) object;

        PKIX_MUTEX_DEBUG("\tCalling PR_DestroyLock).\n");
        PR_DestroyLock(mutex->lock);
        mutex->lock = NULL;

cleanup:

        PKIX_RETURN(MUTEX);
}












PKIX_Error *
pkix_pl_Mutex_RegisterSelf(
         void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(MUTEX, "pkix_pl_Mutex_RegisterSelf");

        entry.description = "Mutex";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_Mutex);
        entry.destructor = pkix_pl_Mutex_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_MUTEX_TYPE] = entry;

        PKIX_RETURN(MUTEX);
}






PKIX_Error *
PKIX_PL_Mutex_Create(
        PKIX_PL_Mutex **pNewLock,
        void *plContext)
{
        PKIX_PL_Mutex *mutex = NULL;

        PKIX_ENTER(MUTEX, "PKIX_PL_Mutex_Create");
        PKIX_NULLCHECK_ONE(pNewLock);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_MUTEX_TYPE,
                    sizeof (PKIX_PL_Mutex),
                    (PKIX_PL_Object **)&mutex,
                    plContext),
                    PKIX_COULDNOTCREATELOCKOBJECT);

        PKIX_MUTEX_DEBUG("\tCalling PR_NewLock).\n");
        mutex->lock = PR_NewLock();

        
        if (mutex->lock == NULL) {
                PKIX_DECREF(mutex);
                PKIX_ERROR_ALLOC_ERROR();
        }

        *pNewLock = mutex;

cleanup:

        PKIX_RETURN(MUTEX);
}




PKIX_Error *
PKIX_PL_Mutex_Lock(
        PKIX_PL_Mutex *mutex,
        void *plContext)
{
        PKIX_ENTER(MUTEX, "PKIX_PL_Mutex_Lock");
        PKIX_NULLCHECK_ONE(mutex);

        PKIX_MUTEX_DEBUG("\tCalling PR_Lock).\n");
        PR_Lock(mutex->lock);

        PKIX_MUTEX_DEBUG_ARG("(Thread %u just acquired the lock)\n",
                        (PKIX_UInt32)PR_GetCurrentThread());

        PKIX_RETURN(MUTEX);
}




PKIX_Error *
PKIX_PL_Mutex_Unlock(
        PKIX_PL_Mutex *mutex,
        void *plContext)
{
        PRStatus result;

        PKIX_ENTER(MUTEX, "PKIX_PL_Mutex_Unlock");
        PKIX_NULLCHECK_ONE(mutex);

        PKIX_MUTEX_DEBUG("\tCalling PR_Unlock).\n");
        result = PR_Unlock(mutex->lock);

        PKIX_MUTEX_DEBUG_ARG("(Thread %u just released the lock)\n",
                        (PKIX_UInt32)PR_GetCurrentThread());

        if (result == PR_FAILURE) {
                PKIX_ERROR_FATAL(PKIX_ERRORUNLOCKINGMUTEX);
        }

cleanup:
        PKIX_RETURN(MUTEX);
}
