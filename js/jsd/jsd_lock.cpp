














                                                                           

#include "jsd.h"

#include "js/Utility.h"

#ifdef JSD_THREADSAFE

#ifdef JSD_USE_NSPR_LOCKS

#include "prlock.h"
#include "prthread.h"

#ifdef JSD_ATTACH_THREAD_HACK
#include "pprthred.h"   
#endif

struct JSDStaticLock
{
    void*     owner;
    PRLock*   lock;
    int       count;
#ifdef DEBUG
    uint16_t  sig;
#endif
};










#undef _CURRENT_THREAD
#ifdef JSD_ATTACH_THREAD_HACK
#define _CURRENT_THREAD(out)                                                  \
JS_BEGIN_MACRO                                                                \
    out = (void*) PR_GetCurrentThread();                                      \
    if(!out)                                                                  \
        out = (void*) JS_AttachThread(PR_USER_THREAD,PR_PRIORITY_NORMAL,NULL);\
    JS_ASSERT(out);                                                           \
JS_END_MACRO
#else
#define _CURRENT_THREAD(out)             \
JS_BEGIN_MACRO                           \
    out = (void*) PR_GetCurrentThread(); \
    JS_ASSERT(out);                      \
JS_END_MACRO
#endif

#ifdef DEBUG
#define JSD_LOCK_SIG 0x10CC10CC
void ASSERT_VALID_LOCK(JSDStaticLock* lock)
{
    JS_ASSERT(lock);
    JS_ASSERT(lock->lock);
    JS_ASSERT(lock->count >= 0);
    JS_ASSERT(lock->sig == (uint16_t) JSD_LOCK_SIG);
}    
#else
#define ASSERT_VALID_LOCK(x) ((void)0)
#endif

JSDStaticLock*
jsd_CreateLock()
{
    JSDStaticLock* lock;

    if( ! (lock = js_pod_calloc<JSDStaticLock>(1)) ||
        ! (lock->lock = PR_NewLock()) )
    {
        if(lock)
        {
            free(lock);
            lock = NULL;
        }
    }
#ifdef DEBUG
    if(lock) lock->sig = (uint16_t) JSD_LOCK_SIG;
#endif
    return lock;
}    

void
jsd_Lock(JSDStaticLock* lock)
{
    void* me;
    ASSERT_VALID_LOCK(lock);
    _CURRENT_THREAD(me);

    if(lock->owner == me)
    {
        lock->count++;
        JS_ASSERT(lock->count > 1);
    }
    else
    {
        PR_Lock(lock->lock);            
        JS_ASSERT(lock->owner == 0);
        JS_ASSERT(lock->count == 0);
        lock->count = 1;
        lock->owner = me;
    }
}    

void
jsd_Unlock(JSDStaticLock* lock)
{
    void* me;
    ASSERT_VALID_LOCK(lock);
    _CURRENT_THREAD(me);

    
    JS_ASSERT(lock->owner == me);
    if(lock->owner != me)
        return;

    if(--lock->count == 0)
    {
        lock->owner = NULL;
        PR_Unlock(lock->lock);
    }
}    

#ifdef DEBUG
JSBool
jsd_IsLocked(JSDStaticLock* lock)
{
    void* me;
    ASSERT_VALID_LOCK(lock);
    _CURRENT_THREAD(me);
    if (lock->owner != me)
        return JS_FALSE;
    JS_ASSERT(lock->count > 0);
    return JS_TRUE;
}    
#endif 

void*
jsd_CurrentThread()
{
    void* me;
    _CURRENT_THREAD(me);
    return me;
}    


#else  

#ifdef WIN32    
#pragma message("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
#pragma message("!! you are compiling the stubbed version of jsd_lock.c !!")
#pragma message("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
#endif






void*
jsd_CreateLock()
{
    return (void*)1;
}    

void
jsd_Lock(void* lock)
{
}    

void
jsd_Unlock(void* lock)
{
}    

#ifdef DEBUG
JSBool
jsd_IsLocked(void* lock)
{
    return JS_TRUE;
}    
#endif 







#ifdef WIN32    

extern void* __stdcall GetCurrentThreadId(void);
#endif

void*
jsd_CurrentThread()
{
#ifdef WIN32    
    return GetCurrentThreadId();
#else
    return (void*)1;
#endif
}    

#endif 

#endif 
