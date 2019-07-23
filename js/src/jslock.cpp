






































#ifdef JS_THREADSAFE




#include <stdlib.h>
#include <string.h>
#include "jspubtd.h"
#include "jsutil.h" 
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jscntxt.h"
#include "jsdtoa.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsscope.h"
#include "jsstr.h"

#define ReadWord(W) (W)

#if !defined(__GNUC__)
# define __asm__ asm
# define __volatile__ volatile
#endif



#if defined(_WIN32) && defined(_M_IX86)
#pragma warning( disable : 4035 )
JS_BEGIN_EXTERN_C
extern long __cdecl
_InterlockedCompareExchange(long *volatile dest, long exchange, long comp);
JS_END_EXTERN_C
#pragma intrinsic(_InterlockedCompareExchange)

JS_STATIC_ASSERT(sizeof(jsword) == sizeof(long));

static JS_ALWAYS_INLINE int
NativeCompareAndSwapHelper(jsword *w, jsword ov, jsword nv)
{
    _InterlockedCompareExchange((long*) w, nv, ov);
    __asm {
        sete al
    }
}

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    return (NativeCompareAndSwapHelper(w, ov, nv) & 1);
}

#elif defined(_MSC_VER) && (defined(_M_AMD64) || defined(_M_X64))
JS_BEGIN_EXTERN_C
extern long long __cdecl
_InterlockedCompareExchange64(long long *volatile dest, long long exchange, long long comp);
JS_END_EXTERN_C
#pragma intrinsic(_InterlockedCompareExchange64)

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    return _InterlockedCompareExchange64(w, nv, ov) == ov;
}

#elif defined(XP_MACOSX) || defined(DARWIN)

#include <libkern/OSAtomic.h>

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    
    return OSAtomicCompareAndSwapPtrBarrier(ov, nv, w);
}

#elif defined(__i386) && (defined(__GNUC__) || defined(__SUNPRO_CC))


static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    unsigned int res;

    __asm__ __volatile__ (
                          "lock\n"
                          "cmpxchgl %2, (%1)\n"
                          "sete %%al\n"
                          "andl $1, %%eax\n"
                          : "=a" (res)
                          : "r" (w), "r" (nv), "a" (ov)
                          : "cc", "memory");
    return (int)res;
}

#elif defined(__x86_64) && (defined(__GNUC__) || defined(__SUNPRO_CC))

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    unsigned int res;

    __asm__ __volatile__ (
                          "lock\n"
                          "cmpxchgq %2, (%1)\n"
                          "sete %%al\n"
                          "movzbl %%al, %%eax\n"
                          : "=a" (res)
                          : "r" (w), "r" (nv), "a" (ov)
                          : "cc", "memory");
    return (int)res;
}

#elif defined(__sparc)
#if defined(__GNUC__)

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    unsigned int res;

    __asm__ __volatile__ (
                  "stbar\n"
                  "cas [%1],%2,%3\n"
                  "cmp %2,%3\n"
                  "be,a 1f\n"
                  "mov 1,%0\n"
                  "mov 0,%0\n"
                  "1:"
                  : "=r" (res)
                  : "r" (w), "r" (ov), "r" (nv));
    return (int)res;
}

#elif defined(__SUNPRO_CC)


extern "C" int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv);

#endif

#elif defined(AIX)

#include <sys/atomic_op.h>

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    return !_check_lock((atomic_p)w, ov, nv);
}

#elif defined(USE_ARM_KUSER)





typedef int (__kernel_cmpxchg_t)(int oldval, int newval, volatile int *ptr);
#define __kernel_cmpxchg (*(__kernel_cmpxchg_t *)0xffff0fc0)

JS_STATIC_ASSERT(sizeof(jsword) == sizeof(int));

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    volatile int *vp = (volatile int *) w;
    PRInt32 failed = 1;

    
    do {
        failed = __kernel_cmpxchg(ov, nv, vp);
    } while (failed && *vp == ov);
    return !failed;
}

#elif JS_HAS_NATIVE_COMPARE_AND_SWAP

#error "JS_HAS_NATIVE_COMPARE_AND_SWAP should be 0 if your platform lacks a compare-and-swap instruction."

#endif 

#if JS_HAS_NATIVE_COMPARE_AND_SWAP

JSBool
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    return !!NativeCompareAndSwap(w, ov, nv);
}

#elif defined(NSPR_LOCK)

# ifdef __GNUC__
# warning "js_CompareAndSwap is implemented using NSPR lock"
# endif

JSBool
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    int result;
    static PRLock *CompareAndSwapLock = JS_NEW_LOCK();

    JS_ACQUIRE_LOCK(CompareAndSwapLock);
    result = (*w == ov);
    if (result)
        *w = nv;
    JS_RELEASE_LOCK(CompareAndSwapLock);
    return result;
}

#else 

#error "NSPR_LOCK should be on when the platform lacks native compare-and-swap."

#endif

void
js_AtomicSetMask(jsword *w, jsword mask)
{
    jsword ov, nv;

    do {
        ov = *w;
        nv = ov | mask;
    } while (!js_CompareAndSwap(w, ov, nv));
}

#ifndef NSPR_LOCK

struct JSFatLock {
    int         susp;
    PRLock      *slock;
    PRCondVar   *svar;
    JSFatLock   *next;
    JSFatLock   **prevp;
};

typedef struct JSFatLockTable {
    JSFatLock   *free;
    JSFatLock   *taken;
} JSFatLockTable;

#define GLOBAL_LOCK_INDEX(id)   (((uint32)(jsuword)(id)>>2) & global_locks_mask)

static void
js_Dequeue(JSThinLock *);

static PRLock **global_locks;
static uint32 global_lock_count = 1;
static uint32 global_locks_log2 = 0;
static uint32 global_locks_mask = 0;

static void
js_LockGlobal(void *id)
{
    uint32 i = GLOBAL_LOCK_INDEX(id);
    PR_Lock(global_locks[i]);
}

static void
js_UnlockGlobal(void *id)
{
    uint32 i = GLOBAL_LOCK_INDEX(id);
    PR_Unlock(global_locks[i]);
}

#endif 

void
js_InitLock(JSThinLock *tl)
{
#ifdef NSPR_LOCK
    tl->owner = 0;
    tl->fat = (JSFatLock*)JS_NEW_LOCK();
#else
    memset(tl, 0, sizeof(JSThinLock));
#endif
}

void
js_FinishLock(JSThinLock *tl)
{
#ifdef NSPR_LOCK
    tl->owner = 0xdeadbeef;
    if (tl->fat)
        JS_DESTROY_LOCK(((JSLock*)tl->fat));
#else
    JS_ASSERT(tl->owner == 0);
    JS_ASSERT(tl->fat == NULL);
#endif
}

#ifdef DEBUG_SCOPE_COUNT

#include <stdio.h>
#include "jsdhash.h"

static FILE *logfp = NULL;
static JSDHashTable logtbl;

typedef struct logentry {
    JSDHashEntryStub stub;
    char             op;
    const char       *file;
    int              line;
} logentry;

static void
logit(JSTitle *title, char op, const char *file, int line)
{
    logentry *entry;

    if (!logfp) {
        logfp = fopen("/tmp/scope.log", "w");
        if (!logfp)
            return;
        setvbuf(logfp, NULL, _IONBF, 0);
    }
    fprintf(logfp, "%p %d %c %s %d\n", title, title->u.count, op, file, line);

    if (!logtbl.entryStore &&
        !JS_DHashTableInit(&logtbl, JS_DHashGetStubOps(), NULL,
                           sizeof(logentry), 100)) {
        return;
    }
    entry = (logentry *) JS_DHashTableOperate(&logtbl, title, JS_DHASH_ADD);
    if (!entry)
        return;
    entry->stub.key = title;
    entry->op = op;
    entry->file = file;
    entry->line = line;
}

void
js_unlog_title(JSTitle *title)
{
    if (!logtbl.entryStore)
        return;
    (void) JS_DHashTableOperate(&logtbl, title, JS_DHASH_REMOVE);
}

# define LOGIT(title,op) logit(title, op, __FILE__, __LINE__)

#else

# define LOGIT(title, op)

#endif 







static bool
WillDeadlock(JSContext *ownercx, JSThread *thread)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(thread));
    JS_ASSERT(ownercx->thread != thread);

     for (;;) {
        JS_ASSERT(ownercx->thread);
        JS_ASSERT(ownercx->requestDepth > 0);
        JSTitle *title = ownercx->thread->titleToShare;
        if (!title || !title->ownercx) {
            



            return false;
        }

        





        if (title->ownercx->thread == thread) {
            JS_RUNTIME_METER(ownercx->runtime, deadlocksAvoided);
            return true;
        }
        ownercx = title->ownercx;
     }
}

static void
FinishSharingTitle(JSContext *cx, JSTitle *title);










static void
ShareTitle(JSContext *cx, JSTitle *title)
{
    JSRuntime *rt;
    JSTitle **todop;

    rt = cx->runtime;
    if (title->u.link) {
        for (todop = &rt->titleSharingTodo; *todop != title;
             todop = &(*todop)->u.link) {
            JS_ASSERT(*todop != NO_TITLE_SHARING_TODO);
        }
        *todop = title->u.link;
        title->u.link = NULL;       
        JS_NOTIFY_ALL_CONDVAR(rt->titleSharingDone);
    }
    FinishSharingTitle(cx, title);
}













static void
FinishSharingTitle(JSContext *cx, JSTitle *title)
{
    js_InitLock(&title->lock);
    title->u.count = 0;     

    JSScope *scope = TITLE_TO_SCOPE(title);
    JSObject *obj = scope->object;
    if (obj) {
        uint32 nslots = scope->freeslot;
        JS_ASSERT(nslots >= JSSLOT_START(obj->getClass()));
        for (uint32 i = JSSLOT_START(obj->getClass()); i != nslots; ++i) {
            jsval v = STOBJ_GET_SLOT(obj, i);
            if (JSVAL_IS_STRING(v) &&
                !js_MakeStringImmutable(cx, JSVAL_TO_STRING(v))) {
                





                STOBJ_SET_SLOT(obj, i, JSVAL_VOID);
            }
        }
    }

    title->ownercx = NULL;  
    JS_RUNTIME_METER(cx->runtime, sharedTitles);
}





void
js_NudgeOtherContexts(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;
    JSContext *acx = NULL;

    while ((acx = js_NextActiveContext(rt, acx)) != NULL) {
        if (cx != acx)
            JS_TriggerOperationCallback(acx);
    }
}





static void
NudgeThread(JSThread *thread)
{
    JSCList *link;
    JSContext *acx;

    link = &thread->contextList;
    while ((link = link->next) != &thread->contextList) {
        acx = CX_FROM_THREAD_LINKS(link);
        JS_ASSERT(acx->thread == thread);
        if (acx->requestDepth)
            JS_TriggerOperationCallback(acx);
    }
}









static JSBool
ClaimTitle(JSTitle *title, JSContext *cx)
{
    JSRuntime *rt;
    JSContext *ownercx;
    uint32 requestDebit;

    rt = cx->runtime;
    JS_RUNTIME_METER(rt, claimAttempts);
    JS_LOCK_GC(rt);

    
    while ((ownercx = title->ownercx) != NULL) {
        




















        bool canClaim;
        if (title->u.link) {
            JS_ASSERT(js_ValidContextPointer(rt, ownercx));
            JS_ASSERT(ownercx->requestDepth > 0);
            JS_ASSERT_IF(cx->requestDepth == 0, cx->thread == rt->gcThread);
            canClaim = (ownercx->thread == cx->thread &&
                        cx->requestDepth > 0);
        } else {
            canClaim = (!js_ValidContextPointer(rt, ownercx) ||
                        !ownercx->requestDepth ||
                        ownercx->thread == cx->thread);
        }
        if (canClaim) {
            title->ownercx = cx;
            JS_UNLOCK_GC(rt);
            JS_RUNTIME_METER(rt, claimedTitles);
            return JS_TRUE;
        }

        


















        if (rt->gcThread == cx->thread || WillDeadlock(ownercx, cx->thread)) {
            ShareTitle(cx, title);
            break;
        }

        




        if (!title->u.link) {
            TITLE_TO_SCOPE(title)->hold();
            title->u.link = rt->titleSharingTodo;
            rt->titleSharingTodo = title;
        }

        




        requestDebit = js_DiscountRequestsForGC(cx);
        if (title->ownercx != ownercx) {
            



            js_RecountRequestsAfterGC(rt, requestDebit);
            continue;
        }

        






        NudgeThread(ownercx->thread);

        JS_ASSERT(!cx->thread->titleToShare);
        cx->thread->titleToShare = title;
#ifdef DEBUG
        PRStatus stat =
#endif
            PR_WaitCondVar(rt->titleSharingDone, PR_INTERVAL_NO_TIMEOUT);
        JS_ASSERT(stat != PR_FAILURE);

        js_RecountRequestsAfterGC(rt, requestDebit);

        











        cx->thread->titleToShare = NULL;
    }

    JS_UNLOCK_GC(rt);
    return JS_FALSE;
}

void
js_ShareWaitingTitles(JSContext *cx)
{
    JSTitle *title, **todop;
    bool shared;

    
    todop = &cx->runtime->titleSharingTodo;
    shared = false;
    while ((title = *todop) != NO_TITLE_SHARING_TODO) {
        if (title->ownercx != cx) {
            todop = &title->u.link;
            continue;
        }
        *todop = title->u.link;
        title->u.link = NULL;       

        






        if (TITLE_TO_SCOPE(title)->drop(cx, NULL)) {
            FinishSharingTitle(cx, title); 
            shared = true;
        }
    }
    if (shared)
        JS_NOTIFY_ALL_CONDVAR(cx->runtime->titleSharingDone);
}


JS_FRIEND_API(jsval)
js_GetSlotThreadSafe(JSContext *cx, JSObject *obj, uint32 slot)
{
    jsval v;
    JSScope *scope;
    JSTitle *title;
#ifndef NSPR_LOCK
    JSThinLock *tl;
    jsword me;
#endif

    OBJ_CHECK_SLOT(obj, slot);

    



    scope = OBJ_SCOPE(obj);
    title = &scope->title;
    JS_ASSERT(title->ownercx != cx);
    JS_ASSERT(slot < scope->freeslot);

    





    if (CX_THREAD_IS_RUNNING_GC(cx) ||
        scope->sealed() ||
        (title->ownercx && ClaimTitle(title, cx))) {
        return STOBJ_GET_SLOT(obj, slot);
    }

#ifndef NSPR_LOCK
    tl = &title->lock;
    me = CX_THINLOCK_ID(cx);
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (NativeCompareAndSwap(&tl->owner, 0, me)) {
        





        if (scope == OBJ_SCOPE(obj)) {
            v = STOBJ_GET_SLOT(obj, slot);
            if (!NativeCompareAndSwap(&tl->owner, me, 0)) {
                
                JS_ASSERT(title->ownercx != cx);
                LOGIT(title, '1');
                title->u.count = 1;
                js_UnlockObj(cx, obj);
            }
            return v;
        }
        if (!NativeCompareAndSwap(&tl->owner, me, 0))
            js_Dequeue(tl);
    }
    else if (Thin_RemoveWait(ReadWord(tl->owner)) == me) {
        return STOBJ_GET_SLOT(obj, slot);
    }
#endif

    js_LockObj(cx, obj);
    v = STOBJ_GET_SLOT(obj, slot);

    








    title = &OBJ_SCOPE(obj)->title;
    if (title->ownercx != cx)
        js_UnlockTitle(cx, title);
    return v;
}

void
js_SetSlotThreadSafe(JSContext *cx, JSObject *obj, uint32 slot, jsval v)
{
    JSTitle *title;
    JSScope *scope;
#ifndef NSPR_LOCK
    JSThinLock *tl;
    jsword me;
#endif

    OBJ_CHECK_SLOT(obj, slot);

    
    if (JSVAL_IS_STRING(v) &&
        !js_MakeStringImmutable(cx, JSVAL_TO_STRING(v))) {
        
        v = JSVAL_NULL;
    }

    



    scope = OBJ_SCOPE(obj);
    title = &scope->title;
    JS_ASSERT(title->ownercx != cx);
    JS_ASSERT(slot < scope->freeslot);

    





    if (CX_THREAD_IS_RUNNING_GC(cx) ||
        scope->sealed() ||
        (title->ownercx && ClaimTitle(title, cx))) {
        LOCKED_OBJ_SET_SLOT(obj, slot, v);
        return;
    }

#ifndef NSPR_LOCK
    tl = &title->lock;
    me = CX_THINLOCK_ID(cx);
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (NativeCompareAndSwap(&tl->owner, 0, me)) {
        if (scope == OBJ_SCOPE(obj)) {
            LOCKED_OBJ_SET_SLOT(obj, slot, v);
            if (!NativeCompareAndSwap(&tl->owner, me, 0)) {
                
                JS_ASSERT(title->ownercx != cx);
                LOGIT(title, '1');
                title->u.count = 1;
                js_UnlockObj(cx, obj);
            }
            return;
        }
        if (!NativeCompareAndSwap(&tl->owner, me, 0))
            js_Dequeue(tl);
    } else if (Thin_RemoveWait(ReadWord(tl->owner)) == me) {
        LOCKED_OBJ_SET_SLOT(obj, slot, v);
        return;
    }
#endif

    js_LockObj(cx, obj);
    LOCKED_OBJ_SET_SLOT(obj, slot, v);

    


    title = &OBJ_SCOPE(obj)->title;
    if (title->ownercx != cx)
        js_UnlockTitle(cx, title);
}

#ifndef NSPR_LOCK

static JSFatLock *
NewFatlock()
{
    JSFatLock *fl = (JSFatLock *)malloc(sizeof(JSFatLock)); 
    if (!fl) return NULL;
    fl->susp = 0;
    fl->next = NULL;
    fl->prevp = NULL;
    fl->slock = PR_NewLock();
    fl->svar = PR_NewCondVar(fl->slock);
    return fl;
}

static void
DestroyFatlock(JSFatLock *fl)
{
    PR_DestroyLock(fl->slock);
    PR_DestroyCondVar(fl->svar);
    js_free(fl);
}

static JSFatLock *
ListOfFatlocks(int listc)
{
    JSFatLock *m;
    JSFatLock *m0;
    int i;

    JS_ASSERT(listc>0);
    m0 = m = NewFatlock();
    for (i=1; i<listc; i++) {
        m->next = NewFatlock();
        m = m->next;
    }
    return m0;
}

static void
DeleteListOfFatlocks(JSFatLock *m)
{
    JSFatLock *m0;
    for (; m; m=m0) {
        m0 = m->next;
        DestroyFatlock(m);
    }
}

static JSFatLockTable *fl_list_table = NULL;
static uint32          fl_list_table_len = 0;
static uint32          fl_list_chunk_len = 0;

static JSFatLock *
GetFatlock(void *id)
{
    JSFatLock *m;

    uint32 i = GLOBAL_LOCK_INDEX(id);
    if (fl_list_table[i].free == NULL) {
#ifdef DEBUG
        if (fl_list_table[i].taken)
            printf("Ran out of fat locks!\n");
#endif
        fl_list_table[i].free = ListOfFatlocks(fl_list_chunk_len);
    }
    m = fl_list_table[i].free;
    fl_list_table[i].free = m->next;
    m->susp = 0;
    m->next = fl_list_table[i].taken;
    m->prevp = &fl_list_table[i].taken;
    if (fl_list_table[i].taken)
        fl_list_table[i].taken->prevp = &m->next;
    fl_list_table[i].taken = m;
    return m;
}

static void
PutFatlock(JSFatLock *m, void *id)
{
    uint32 i;
    if (m == NULL)
        return;

    
    *m->prevp = m->next;
    if (m->next)
        m->next->prevp = m->prevp;

    
    i = GLOBAL_LOCK_INDEX(id);
    m->next = fl_list_table[i].free;
    fl_list_table[i].free = m;
}

#endif 

JSBool
js_SetupLocks(int listc, int globc)
{
#ifndef NSPR_LOCK
    uint32 i;

    if (global_locks)
        return JS_TRUE;
#ifdef DEBUG
    if (listc > 10000 || listc < 0) 
        printf("Bad number %d in js_SetupLocks()!\n", listc);
    if (globc > 100 || globc < 0)   
        printf("Bad number %d in js_SetupLocks()!\n", listc);
#endif
    global_locks_log2 = JS_CeilingLog2(globc);
    global_locks_mask = JS_BITMASK(global_locks_log2);
    global_lock_count = JS_BIT(global_locks_log2);
    global_locks = (PRLock **) js_malloc(global_lock_count * sizeof(PRLock*));
    if (!global_locks)
        return JS_FALSE;
    for (i = 0; i < global_lock_count; i++) {
        global_locks[i] = PR_NewLock();
        if (!global_locks[i]) {
            global_lock_count = i;
            js_CleanupLocks();
            return JS_FALSE;
        }
    }
    fl_list_table = (JSFatLockTable *) js_malloc(i * sizeof(JSFatLockTable));
    if (!fl_list_table) {
        js_CleanupLocks();
        return JS_FALSE;
    }
    fl_list_table_len = global_lock_count;
    for (i = 0; i < global_lock_count; i++)
        fl_list_table[i].free = fl_list_table[i].taken = NULL;
    fl_list_chunk_len = listc;
#endif 
    return JS_TRUE;
}

void
js_CleanupLocks()
{
#ifndef NSPR_LOCK
    uint32 i;

    if (global_locks) {
        for (i = 0; i < global_lock_count; i++)
            PR_DestroyLock(global_locks[i]);
        js_free(global_locks);
        global_locks = NULL;
        global_lock_count = 1;
        global_locks_log2 = 0;
        global_locks_mask = 0;
    }
    if (fl_list_table) {
        for (i = 0; i < fl_list_table_len; i++) {
            DeleteListOfFatlocks(fl_list_table[i].free);
            fl_list_table[i].free = NULL;
            DeleteListOfFatlocks(fl_list_table[i].taken);
            fl_list_table[i].taken = NULL;
        }
        js_free(fl_list_table);
        fl_list_table = NULL;
        fl_list_table_len = 0;
    }
#endif 
}

#ifdef NSPR_LOCK

static JS_ALWAYS_INLINE void
ThinLock(JSThinLock *tl, jsword me)
{
    JS_ACQUIRE_LOCK((JSLock *) tl->fat);
    tl->owner = me;
}

static JS_ALWAYS_INLINE void
ThinUnlock(JSThinLock *tl, jsword )
{
    tl->owner = 0;
    JS_RELEASE_LOCK((JSLock *) tl->fat);
}

#else



































static int
js_SuspendThread(JSThinLock *tl)
{
    JSFatLock *fl;
    PRStatus stat;

    if (tl->fat == NULL)
        fl = tl->fat = GetFatlock(tl);
    else
        fl = tl->fat;
    JS_ASSERT(fl->susp >= 0);
    fl->susp++;
    PR_Lock(fl->slock);
    js_UnlockGlobal(tl);
    stat = PR_WaitCondVar(fl->svar, PR_INTERVAL_NO_TIMEOUT);
    JS_ASSERT(stat != PR_FAILURE);
    PR_Unlock(fl->slock);
    js_LockGlobal(tl);
    fl->susp--;
    if (fl->susp == 0) {
        PutFatlock(fl, tl);
        tl->fat = NULL;
    }
    return tl->fat == NULL;
}





static void
js_ResumeThread(JSThinLock *tl)
{
    JSFatLock *fl = tl->fat;
    PRStatus stat;

    JS_ASSERT(fl != NULL);
    JS_ASSERT(fl->susp > 0);
    PR_Lock(fl->slock);
    js_UnlockGlobal(tl);
    stat = PR_NotifyCondVar(fl->svar);
    JS_ASSERT(stat != PR_FAILURE);
    PR_Unlock(fl->slock);
}

static void
js_Enqueue(JSThinLock *tl, jsword me)
{
    jsword o, n;

    js_LockGlobal(tl);
    for (;;) {
        o = ReadWord(tl->owner);
        n = Thin_SetWait(o);
        if (o != 0 && NativeCompareAndSwap(&tl->owner, o, n)) {
            if (js_SuspendThread(tl))
                me = Thin_RemoveWait(me);
            else
                me = Thin_SetWait(me);
        }
        else if (NativeCompareAndSwap(&tl->owner, 0, me)) {
            js_UnlockGlobal(tl);
            return;
        }
    }
}

static void
js_Dequeue(JSThinLock *tl)
{
    jsword o;

    js_LockGlobal(tl);
    o = ReadWord(tl->owner);
    JS_ASSERT(Thin_GetWait(o) != 0);
    JS_ASSERT(tl->fat != NULL);
    if (!NativeCompareAndSwap(&tl->owner, o, 0)) 
        JS_ASSERT(0);
    js_ResumeThread(tl);
}

static JS_ALWAYS_INLINE void
ThinLock(JSThinLock *tl, jsword me)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (NativeCompareAndSwap(&tl->owner, 0, me))
        return;
    if (Thin_RemoveWait(ReadWord(tl->owner)) != me)
        js_Enqueue(tl, me);
#ifdef DEBUG
    else
        JS_ASSERT(0);
#endif
}

static JS_ALWAYS_INLINE void
ThinUnlock(JSThinLock *tl, jsword me)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));

    



    if (NativeCompareAndSwap(&tl->owner, me, 0))
        return;

    JS_ASSERT(Thin_GetWait(tl->owner));
    if (Thin_RemoveWait(ReadWord(tl->owner)) == me)
        js_Dequeue(tl);
#ifdef DEBUG
    else
        JS_ASSERT(0);   
#endif
}

#endif 

void
js_Lock(JSContext *cx, JSThinLock *tl)
{
    ThinLock(tl, CX_THINLOCK_ID(cx));
}

void
js_Unlock(JSContext *cx, JSThinLock *tl)
{
    ThinUnlock(tl, CX_THINLOCK_ID(cx));
}

void
js_LockRuntime(JSRuntime *rt)
{
    PR_Lock(rt->rtLock);
#ifdef DEBUG
    rt->rtLockOwner = js_CurrentThreadId();
#endif
}

void
js_UnlockRuntime(JSRuntime *rt)
{
#ifdef DEBUG
    rt->rtLockOwner = 0;
#endif
    PR_Unlock(rt->rtLock);
}

void
js_LockTitle(JSContext *cx, JSTitle *title)
{
    jsword me = CX_THINLOCK_ID(cx);

    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    JS_ASSERT(title->ownercx != cx);
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;
    if (title->ownercx && ClaimTitle(title, cx))
        return;

    if (Thin_RemoveWait(ReadWord(title->lock.owner)) == me) {
        JS_ASSERT(title->u.count > 0);
        LOGIT(scope, '+');
        title->u.count++;
    } else {
        ThinLock(&title->lock, me);
        JS_ASSERT(title->u.count == 0);
        LOGIT(scope, '1');
        title->u.count = 1;
    }
}

void
js_UnlockTitle(JSContext *cx, JSTitle *title)
{
    jsword me = CX_THINLOCK_ID(cx);

    
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;
    if (cx->lockedSealedTitle == title) {
        cx->lockedSealedTitle = NULL;
        return;
    }

    













    if (title->ownercx) {
        JS_ASSERT(title->u.count == 0);
        JS_ASSERT(title->lock.owner == 0);
        title->ownercx = cx;
        return;
    }

    JS_ASSERT(title->u.count > 0);
    if (Thin_RemoveWait(ReadWord(title->lock.owner)) != me) {
        JS_ASSERT(0);   
        return;
    }
    LOGIT(title, '-');
    if (--title->u.count == 0)
        ThinUnlock(&title->lock, me);
}





void
js_TransferTitle(JSContext *cx, JSTitle *oldtitle, JSTitle *newtitle)
{
    JS_ASSERT(JS_IS_TITLE_LOCKED(cx, newtitle));

    



    if (!oldtitle)
        return;
    JS_ASSERT(JS_IS_TITLE_LOCKED(cx, oldtitle));

    





    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;

    




    JS_ASSERT(cx->lockedSealedTitle != newtitle);
    if (cx->lockedSealedTitle == oldtitle) {
        JS_ASSERT(newtitle->ownercx == cx ||
                  (!newtitle->ownercx && newtitle->u.count == 1));
        cx->lockedSealedTitle = NULL;
        return;
    }

    


    if (oldtitle->ownercx) {
        JS_ASSERT(oldtitle->ownercx == cx);
        JS_ASSERT(newtitle->ownercx == cx ||
                  (!newtitle->ownercx && newtitle->u.count == 1));
        return;
    }

    





    if (newtitle->ownercx != cx) {
        JS_ASSERT(!newtitle->ownercx);
        newtitle->u.count = oldtitle->u.count;
    }

    


    LOGIT(oldtitle, '0');
    oldtitle->u.count = 0;
    ThinUnlock(&oldtitle->lock, CX_THINLOCK_ID(cx));
}

void
js_LockObj(JSContext *cx, JSObject *obj)
{
    JSScope *scope;
    JSTitle *title;

    JS_ASSERT(OBJ_IS_NATIVE(obj));

    




    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;

    for (;;) {
        scope = OBJ_SCOPE(obj);
        title = &scope->title;
        if (scope->sealed() && !cx->lockedSealedTitle) {
            cx->lockedSealedTitle = title;
            return;
        }

        js_LockTitle(cx, title);

        
        if (scope == OBJ_SCOPE(obj))
            return;

        
        js_UnlockTitle(cx, title);
    }
}

void
js_UnlockObj(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    js_UnlockTitle(cx, &OBJ_SCOPE(obj)->title);
}

bool
js_LockObjIfShape(JSContext *cx, JSObject *obj, uint32 shape)
{
    JS_ASSERT(OBJ_SCOPE(obj)->title.ownercx != cx);
    js_LockObj(cx, obj);
    if (OBJ_SHAPE(obj) == shape)
        return true;
    js_UnlockObj(cx, obj);
    return false;
}

void
js_InitTitle(JSContext *cx, JSTitle *title)
{
#ifdef JS_THREADSAFE
    title->ownercx = cx;
    memset(&title->lock, 0, sizeof title->lock);

    



    title->u.link = NULL;

#ifdef JS_DEBUG_TITLE_LOCKS
    title->file[0] = title->file[1] = title->file[2] = title->file[3] = NULL;
    title->line[0] = title->line[1] = title->line[2] = title->line[3] = 0;
#endif
#endif
}

void
js_FinishTitle(JSContext *cx, JSTitle *title)
{
#ifdef DEBUG_SCOPE_COUNT
    js_unlog_title(title);
#endif

#ifdef JS_THREADSAFE
    
    JS_ASSERT(title->u.count == 0);
    title->ownercx = cx;
    js_FinishLock(&title->lock);
#endif
}

#ifdef DEBUG

JSBool
js_IsRuntimeLocked(JSRuntime *rt)
{
    return js_CurrentThreadId() == rt->rtLockOwner;
}

JSBool
js_IsObjLocked(JSContext *cx, JSObject *obj)
{
    return js_IsTitleLocked(cx, &OBJ_SCOPE(obj)->title);
}

JSBool
js_IsTitleLocked(JSContext *cx, JSTitle *title)
{
    
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return JS_TRUE;

    
    if (cx->lockedSealedTitle == title)
        return JS_TRUE;

    



    if (title->ownercx) {
        JS_ASSERT(title->ownercx == cx || title->ownercx->thread == cx->thread);
        return JS_TRUE;
    }
    return js_CurrentThreadId() ==
           ((JSThread *)Thin_RemoveWait(ReadWord(title->lock.owner)))->id;
}

#ifdef JS_DEBUG_TITLE_LOCKS
void
js_SetScopeInfo(JSScope *scope, const char *file, int line)
{
    JSTitle *title = &scope->title;
    if (!title->ownercx) {
        jsrefcount count = title->u.count;
        JS_ASSERT_IF(!scope->sealed(), count > 0);
        JS_ASSERT(count <= 4);
        title->file[count - 1] = file;
        title->line[count - 1] = line;
    }
}
#endif 
#endif 
#endif 
