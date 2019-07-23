






































#ifdef JS_THREADSAFE




#include "jsstddef.h"
#include <stdlib.h>
#include "jspubtd.h"
#include "jsutil.h" 
#include "jstypes.h"
#include "jsbit.h"
#include "jscntxt.h"
#include "jsdtoa.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsscope.h"
#include "jsstr.h"

#define ReadWord(W) (W)

#ifndef NSPR_LOCK

#include <memory.h>

static PRLock **global_locks;
static uint32 global_lock_count = 1;
static uint32 global_locks_log2 = 0;
static uint32 global_locks_mask = 0;

#define GLOBAL_LOCK_INDEX(id)   (((uint32)(id) >> 2) & global_locks_mask)

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


#if defined(_WIN32) && defined(_M_IX86)
#pragma warning( disable : 4035 )

static JS_INLINE int
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    __asm {
        mov eax, ov
        mov ecx, nv
        mov ebx, w
        lock cmpxchg [ebx], ecx
        sete al
        and eax, 1h
    }
}

#elif defined(__GNUC__) && defined(__i386__)


static JS_INLINE int
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
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

#elif (defined(__USLC__) || defined(_SCO_DS)) && defined(i386)



asm int
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
%ureg w, nv;
	movl	ov,%eax
	lock
	cmpxchgl nv,(w)
	sete	%al
	andl	$1,%eax
%ureg w;  mem ov, nv;
	movl	ov,%eax
	movl	nv,%ecx
	lock
	cmpxchgl %ecx,(w)
	sete	%al
	andl	$1,%eax
%ureg nv;
	movl	ov,%eax
	movl	w,%edx
	lock
	cmpxchgl nv,(%edx)
	sete	%al
	andl	$1,%eax
%mem w, ov, nv;
	movl	ov,%eax
	movl	nv,%ecx
	movl	w,%edx
	lock
	cmpxchgl %ecx,(%edx)
	sete	%al
	andl	$1,%eax
}
#pragma asm full_optimization js_CompareAndSwap

#elif defined(SOLARIS) && defined(sparc) && defined(ULTRA_SPARC)

static JS_INLINE int
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
#if defined(__GNUC__)
    unsigned int res;
    JS_ASSERT(ov != nv);
    asm volatile ("\
stbar\n\
cas [%1],%2,%3\n\
cmp %2,%3\n\
be,a 1f\n\
mov 1,%0\n\
mov 0,%0\n\
1:"
                  : "=r" (res)
                  : "r" (w), "r" (ov), "r" (nv));
    return (int)res;
#else 
    extern int compare_and_swap(jsword*, jsword, jsword);
    JS_ASSERT(ov != nv);
    return compare_and_swap(w, ov, nv);
#endif
}

#elif defined(AIX)

#include <sys/atomic_op.h>

static JS_INLINE int
js_CompareAndSwap(jsword *w, jsword ov, jsword nv)
{
    return !_check_lock((atomic_p)w, ov, nv);
}

#else

#error "Define NSPR_LOCK if your platform lacks a compare-and-swap instruction."

#endif 

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

#ifndef NSPR_LOCK
static void js_Dequeue(JSThinLock *);
#endif

#ifdef DEBUG_SCOPE_COUNT

#include <stdio.h>
#include "jsdhash.h"

static FILE *logfp;
static JSDHashTable logtbl;

typedef struct logentry {
    JSDHashEntryStub stub;
    char             op;
    const char       *file;
    int              line;
} logentry;

static void
logit(JSScope *scope, char op, const char *file, int line)
{
    logentry *entry;

    if (!logfp) {
        logfp = fopen("/tmp/scope.log", "w");
        if (!logfp)
            return;
        setvbuf(logfp, NULL, _IONBF, 0);
    }
    fprintf(logfp, "%p %c %s %d\n", scope, op, file, line);

    if (!logtbl.entryStore &&
        !JS_DHashTableInit(&logtbl, JS_DHashGetStubOps(), NULL,
                           sizeof(logentry), 100)) {
        return;
    }
    entry = (logentry *) JS_DHashTableOperate(&logtbl, scope, JS_DHASH_ADD);
    if (!entry)
        return;
    entry->stub.key = scope;
    entry->op = op;
    entry->file = file;
    entry->line = line;
}

void
js_unlog_scope(JSScope *scope)
{
    if (!logtbl.entryStore)
        return;
    (void) JS_DHashTableOperate(&logtbl, scope, JS_DHASH_REMOVE);
}

# define LOGIT(scope,op) logit(scope, op, __FILE__, __LINE__)

#else

# define LOGIT(scope,op)

#endif 









static JSBool
WillDeadlock(JSScope *scope, JSContext *cx)
{
    JSContext *ownercx;

    do {
        ownercx = scope->ownercx;
        if (ownercx == cx) {
            JS_RUNTIME_METER(cx->runtime, deadlocksAvoided);
            return JS_TRUE;
        }
    } while (ownercx && (scope = ownercx->scopeToShare) != NULL);
    return JS_FALSE;
}










static void
ShareScope(JSContext *cx, JSScope *scope)
{
    JSRuntime *rt;
    JSScope **todop;

    rt = cx->runtime;
    if (scope->u.link) {
        for (todop = &rt->scopeSharingTodo; *todop != scope;
             todop = &(*todop)->u.link) {
            JS_ASSERT(*todop != NO_SCOPE_SHARING_TODO);
        }
        *todop = scope->u.link;
        scope->u.link = NULL;       
        JS_NOTIFY_ALL_CONDVAR(rt->scopeSharingDone);
    }
    js_InitLock(&scope->lock);
    if (scope == rt->setSlotScope) {
        














        scope->lock.owner = CX_THINLOCK_ID(scope->ownercx);
#ifdef NSPR_LOCK
        JS_ACQUIRE_LOCK((JSLock*)scope->lock.fat);
#endif
        scope->u.count = 1;
    } else {
        scope->u.count = 0;
    }
    js_FinishSharingScope(cx, scope);
}














void
js_FinishSharingScope(JSContext *cx, JSScope *scope)
{
    JSObject *obj;
    uint32 nslots, i;
    jsval v;

    obj = scope->object;
    nslots = LOCKED_OBJ_NSLOTS(obj);
    for (i = 0; i != nslots; ++i) {
        v = STOBJ_GET_SLOT(obj, i);
        if (JSVAL_IS_STRING(v) &&
            !js_MakeStringImmutable(cx, JSVAL_TO_STRING(v))) {
            





            STOBJ_SET_SLOT(obj, i, JSVAL_VOID);
        }
    }

    scope->ownercx = NULL;  
    JS_RUNTIME_METER(cx->runtime, sharedScopes);
}









static JSBool
ClaimScope(JSScope *scope, JSContext *cx)
{
    JSRuntime *rt;
    JSContext *ownercx;
    jsrefcount saveDepth;
    PRStatus stat;

    rt = cx->runtime;
    JS_RUNTIME_METER(rt, claimAttempts);
    JS_LOCK_GC(rt);

    
    while ((ownercx = scope->ownercx) != NULL) {
        













        if (!scope->u.link &&
            (!js_ValidContextPointer(rt, ownercx) ||
             !ownercx->requestDepth ||
             ownercx->thread == cx->thread)) {
            JS_ASSERT(scope->u.count == 0);
            scope->ownercx = cx;
            JS_UNLOCK_GC(rt);
            JS_RUNTIME_METER(rt, claimedScopes);
            return JS_TRUE;
        }

        



























        if (rt->gcThread == cx->thread ||
            (ownercx->scopeToShare &&
             WillDeadlock(ownercx->scopeToShare, cx))) {
            ShareScope(cx, scope);
            break;
        }

        




        if (!scope->u.link) {
            scope->u.link = rt->scopeSharingTodo;
            rt->scopeSharingTodo = scope;
            js_HoldObjectMap(cx, &scope->map);
        }

        









        saveDepth = cx->requestDepth;
        if (saveDepth) {
            cx->requestDepth = 0;
            if (rt->gcThread != cx->thread) {
                JS_ASSERT(rt->requestCount > 0);
                rt->requestCount--;
                if (rt->requestCount == 0)
                    JS_NOTIFY_REQUEST_DONE(rt);
            }
        }

        




        cx->scopeToShare = scope;
        stat = PR_WaitCondVar(rt->scopeSharingDone, PR_INTERVAL_NO_TIMEOUT);
        JS_ASSERT(stat != PR_FAILURE);

        




        if (saveDepth) {
            if (rt->gcThread != cx->thread) {
                while (rt->gcLevel > 0)
                    JS_AWAIT_GC_DONE(rt);
                rt->requestCount++;
            }
            cx->requestDepth = saveDepth;
        }

        











        cx->scopeToShare = NULL;
    }

    JS_UNLOCK_GC(rt);
    return JS_FALSE;
}


JS_FRIEND_API(jsval)
js_GetSlotThreadSafe(JSContext *cx, JSObject *obj, uint32 slot)
{
    jsval v;
    JSScope *scope;
#ifndef NSPR_LOCK
    JSThinLock *tl;
    jsword me;
#endif

    









    if (!OBJ_IS_NATIVE(obj))
        return OBJ_GET_REQUIRED_SLOT(cx, obj, slot);

    



    scope = OBJ_SCOPE(obj);
    JS_ASSERT(scope->ownercx != cx);
    JS_ASSERT(slot < obj->map->freeslot);

    





    if (CX_THREAD_IS_RUNNING_GC(cx) ||
        (SCOPE_IS_SEALED(scope) && scope->object == obj) ||
        (scope->ownercx && ClaimScope(scope, cx))) {
        return STOBJ_GET_SLOT(obj, slot);
    }

#ifndef NSPR_LOCK
    tl = &scope->lock;
    me = CX_THINLOCK_ID(cx);
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (js_CompareAndSwap(&tl->owner, 0, me)) {
        





        if (scope == OBJ_SCOPE(obj)) {
            v = STOBJ_GET_SLOT(obj, slot);
            if (!js_CompareAndSwap(&tl->owner, me, 0)) {
                
                JS_ASSERT(scope->ownercx != cx);
                LOGIT(scope, '1');
                scope->u.count = 1;
                js_UnlockObj(cx, obj);
            }
            return v;
        }
        if (!js_CompareAndSwap(&tl->owner, me, 0))
            js_Dequeue(tl);
    }
    else if (Thin_RemoveWait(ReadWord(tl->owner)) == me) {
        return STOBJ_GET_SLOT(obj, slot);
    }
#endif

    js_LockObj(cx, obj);
    v = STOBJ_GET_SLOT(obj, slot);

    








    scope = OBJ_SCOPE(obj);
    if (scope->ownercx != cx)
        js_UnlockScope(cx, scope);
    return v;
}

void
js_SetSlotThreadSafe(JSContext *cx, JSObject *obj, uint32 slot, jsval v)
{
    JSScope *scope;
#ifndef NSPR_LOCK
    JSThinLock *tl;
    jsword me;
#endif

    
    if (JSVAL_IS_STRING(v) &&
        !js_MakeStringImmutable(cx, JSVAL_TO_STRING(v))) {
        
        v = JSVAL_NULL;
    }

    



    if (!OBJ_IS_NATIVE(obj)) {
        OBJ_SET_REQUIRED_SLOT(cx, obj, slot, v);
        return;
    }

    



    scope = OBJ_SCOPE(obj);
    JS_ASSERT(scope->ownercx != cx);
    JS_ASSERT(slot < obj->map->freeslot);

    





    if (CX_THREAD_IS_RUNNING_GC(cx) ||
        (SCOPE_IS_SEALED(scope) && scope->object == obj) ||
        (scope->ownercx && ClaimScope(scope, cx))) {
        STOBJ_SET_SLOT(obj, slot, v);
        return;
    }

#ifndef NSPR_LOCK
    tl = &scope->lock;
    me = CX_THINLOCK_ID(cx);
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (js_CompareAndSwap(&tl->owner, 0, me)) {
        if (scope == OBJ_SCOPE(obj)) {
            STOBJ_SET_SLOT(obj, slot, v);
            if (!js_CompareAndSwap(&tl->owner, me, 0)) {
                
                JS_ASSERT(scope->ownercx != cx);
                LOGIT(scope, '1');
                scope->u.count = 1;
                js_UnlockObj(cx, obj);
            }
            return;
        }
        if (!js_CompareAndSwap(&tl->owner, me, 0))
            js_Dequeue(tl);
    }
    else if (Thin_RemoveWait(ReadWord(tl->owner)) == me) {
        STOBJ_SET_SLOT(obj, slot, v);
        return;
    }
#endif

    js_LockObj(cx, obj);
    STOBJ_SET_SLOT(obj, slot, v);

    






    scope = OBJ_SCOPE(obj);
    if (scope->ownercx != cx)
        js_UnlockScope(cx, scope);
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
    free(fl);
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
    global_locks = (PRLock **) malloc(global_lock_count * sizeof(PRLock*));
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
    fl_list_table = (JSFatLockTable *) malloc(i * sizeof(JSFatLockTable));
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
        free(global_locks);
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
        free(fl_list_table);
        fl_list_table = NULL;
        fl_list_table_len = 0;
    }
#endif 
}

#ifndef NSPR_LOCK



































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
        if (o != 0 && js_CompareAndSwap(&tl->owner, o, n)) {
            if (js_SuspendThread(tl))
                me = Thin_RemoveWait(me);
            else
                me = Thin_SetWait(me);
        }
        else if (js_CompareAndSwap(&tl->owner, 0, me)) {
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
    if (!js_CompareAndSwap(&tl->owner, o, 0)) 
        JS_ASSERT(0);
    js_ResumeThread(tl);
}

JS_INLINE void
js_Lock(JSThinLock *tl, jsword me)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    if (js_CompareAndSwap(&tl->owner, 0, me))
        return;
    if (Thin_RemoveWait(ReadWord(tl->owner)) != me)
        js_Enqueue(tl, me);
#ifdef DEBUG
    else
        JS_ASSERT(0);
#endif
}

JS_INLINE void
js_Unlock(JSThinLock *tl, jsword me)
{
    JS_ASSERT(CURRENT_THREAD_IS_ME(me));

    



    if (tl->owner == me) {
        tl->owner = 0;
        return;
    }
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
js_LockScope(JSContext *cx, JSScope *scope)
{
    jsword me = CX_THINLOCK_ID(cx);

    JS_ASSERT(CURRENT_THREAD_IS_ME(me));
    JS_ASSERT(scope->ownercx != cx);
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;
    if (scope->ownercx && ClaimScope(scope, cx))
        return;

    if (Thin_RemoveWait(ReadWord(scope->lock.owner)) == me) {
        JS_ASSERT(scope->u.count > 0);
        LOGIT(scope, '+');
        scope->u.count++;
    } else {
        JSThinLock *tl = &scope->lock;
        JS_LOCK0(tl, me);
        JS_ASSERT(scope->u.count == 0);
        LOGIT(scope, '1');
        scope->u.count = 1;
    }
}

void
js_UnlockScope(JSContext *cx, JSScope *scope)
{
    jsword me = CX_THINLOCK_ID(cx);

    
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;
    if (cx->lockedSealedScope == scope) {
        cx->lockedSealedScope = NULL;
        return;
    }

    













    if (scope->ownercx) {
        JS_ASSERT(scope->u.count == 0);
        JS_ASSERT(scope->lock.owner == 0);
        scope->ownercx = cx;
        return;
    }

    JS_ASSERT(scope->u.count > 0);
    if (Thin_RemoveWait(ReadWord(scope->lock.owner)) != me) {
        JS_ASSERT(0);   
        return;
    }
    LOGIT(scope, '-');
    if (--scope->u.count == 0) {
        JSThinLock *tl = &scope->lock;
        JS_UNLOCK0(tl, me);
    }
}





void
js_TransferScopeLock(JSContext *cx, JSScope *oldscope, JSScope *newscope)
{
    jsword me;
    JSThinLock *tl;

    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, newscope));

    



    if (!oldscope)
        return;
    JS_ASSERT(JS_IS_SCOPE_LOCKED(cx, oldscope));

    





    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;

    




    JS_ASSERT(cx->lockedSealedScope != newscope);
    if (cx->lockedSealedScope == oldscope) {
        JS_ASSERT(newscope->ownercx == cx ||
                  (!newscope->ownercx && newscope->u.count == 1));
        cx->lockedSealedScope = NULL;
        return;
    }

    


    if (oldscope->ownercx) {
        JS_ASSERT(oldscope->ownercx == cx);
        JS_ASSERT(newscope->ownercx == cx ||
                  (!newscope->ownercx && newscope->u.count == 1));
        return;
    }

    





    if (newscope->ownercx != cx) {
        JS_ASSERT(!newscope->ownercx);
        newscope->u.count = oldscope->u.count;
    }

    


    LOGIT(oldscope, '0');
    oldscope->u.count = 0;
    tl = &oldscope->lock;
    me = CX_THINLOCK_ID(cx);
    JS_UNLOCK0(tl, me);
}

void
js_LockObj(JSContext *cx, JSObject *obj)
{
    JSScope *scope;

    JS_ASSERT(OBJ_IS_NATIVE(obj));

    




    if (CX_THREAD_IS_RUNNING_GC(cx))
        return;

    for (;;) {
        scope = OBJ_SCOPE(obj);
        if (SCOPE_IS_SEALED(scope) && scope->object == obj &&
            !cx->lockedSealedScope) {
            cx->lockedSealedScope = scope;
            return;
        }

        js_LockScope(cx, scope);

        
        if (scope == OBJ_SCOPE(obj))
            return;

        
        js_UnlockScope(cx, scope);
    }
}

void
js_UnlockObj(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(OBJ_IS_NATIVE(obj));
    js_UnlockScope(cx, OBJ_SCOPE(obj));
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
    JSScope *scope = OBJ_SCOPE(obj);

    return MAP_IS_NATIVE(&scope->map) && js_IsScopeLocked(cx, scope);
}

JSBool
js_IsScopeLocked(JSContext *cx, JSScope *scope)
{
    
    if (CX_THREAD_IS_RUNNING_GC(cx))
        return JS_TRUE;

    
    if (cx->lockedSealedScope == scope)
        return JS_TRUE;

    



    if (scope->ownercx) {
        JS_ASSERT(scope->ownercx == cx || scope->ownercx->thread == cx->thread);
        return JS_TRUE;
    }
    return js_CurrentThreadId() ==
           ((JSThread *)Thin_RemoveWait(ReadWord(scope->lock.owner)))->id;
}

#endif 
#endif 
