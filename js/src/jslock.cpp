






































#ifdef JS_THREADSAFE




#include <stdlib.h>
#include <string.h>
#include "jspubtd.h"
#include "jsutil.h"
#include "jstypes.h"
#include "jsstdint.h"
#include "jsbit.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jslock.h"
#include "jsscope.h"
#include "jsstr.h"

using namespace js;

#define ReadWord(W) (W)

#if !defined(__GNUC__)
# define __asm__ asm
# define __volatile__ volatile
#endif



#if defined(_MSC_VER) && defined(_M_IX86)
#pragma warning( disable : 4035 )
JS_BEGIN_EXTERN_C
extern long __cdecl
_InterlockedCompareExchange(long *volatile dest, long exchange, long comp);
JS_END_EXTERN_C
#pragma intrinsic(_InterlockedCompareExchange)

JS_STATIC_ASSERT(sizeof(jsword) == sizeof(long));

static JS_ALWAYS_INLINE int
NativeCompareAndSwapHelper(volatile jsword *w, jsword ov, jsword nv)
{
    _InterlockedCompareExchange((long*) w, nv, ov);
    __asm {
        sete al
    }
}

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
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
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    return _InterlockedCompareExchange64((long long *volatile)w, nv, ov) == ov;
}

#elif defined(XP_MACOSX) || defined(DARWIN)

#include <libkern/OSAtomic.h>

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    
    return OSAtomicCompareAndSwapPtrBarrier(reinterpret_cast<void *>(ov),
                                            reinterpret_cast<void *>(nv),
                                            reinterpret_cast<void * volatile *>(w));
}

#elif defined(__i386) && (defined(__GNUC__) || defined(__SUNPRO_CC))


static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    unsigned int res;

    __asm__ __volatile__ (
                          "lock\n"
                          "cmpxchgl %2, (%1)\n"
                          "sete %%al\n"
                          "andl $1, %%eax\n"
                          : "=a" (res)
#ifdef __SUNPRO_CC

                          : "c" (w), "d" (nv), "a" (ov)
#else
                          : "r" (w), "r" (nv), "a" (ov)
#endif
                          : "cc", "memory");
    return (int)res;
}
#elif defined(__x86_64) && (defined(__GNUC__) || defined(__SUNPRO_CC))

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
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
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    unsigned int res;

    __asm__ __volatile__ (
                  "membar #StoreLoad | #LoadLoad\n"
#if JS_BITS_PER_WORD == 32
                  "cas [%1],%2,%3\n"
#else
                  "casx [%1],%2,%3\n"
#endif
                  "membar #StoreLoad | #LoadLoad\n"
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
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv);

#endif

#elif defined(AIX)

#include <sys/atomic_op.h>

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    int res;
    JS_STATIC_ASSERT(sizeof(jsword) == sizeof(long));

    res = compare_and_swaplp((atomic_l)w, &ov, nv);
    if (res)
        __asm__("isync");
    return res;
}

#elif defined(__arm__) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

JS_STATIC_ASSERT(sizeof(jsword) == sizeof(int));

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
  return __sync_bool_compare_and_swap(w, ov, nv);
}

#elif defined(USE_ARM_KUSER)





typedef int (__kernel_cmpxchg_t)(int oldval, int newval, volatile int *ptr);
#define __kernel_cmpxchg (*(__kernel_cmpxchg_t *)0xffff0fc0)

JS_STATIC_ASSERT(sizeof(jsword) == sizeof(int));

static JS_ALWAYS_INLINE int
NativeCompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
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
js_CompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
{
    return !!NativeCompareAndSwap(w, ov, nv);
}

#elif defined(NSPR_LOCK)

# ifdef __GNUC__
# warning "js_CompareAndSwap is implemented using NSPR lock"
# endif

JSBool
js_CompareAndSwap(volatile jsword *w, jsword ov, jsword nv)
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
js_AtomicSetMask(volatile jsword *w, jsword mask)
{
    jsword ov, nv;

    do {
        ov = *w;
        nv = ov | mask;
    } while (!js_CompareAndSwap(w, ov, nv));
}

void
js_AtomicClearMask(volatile jsword *w, jsword mask)
{
    jsword ov, nv;

    do {
        ov = *w;
        nv = ov & ~mask;
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
    JSFatLock   *free_;
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
    PodZero(tl);
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

static JSFatLock *
NewFatlock()
{
    JSFatLock *fl = (JSFatLock *) OffTheBooks::malloc_(sizeof(JSFatLock)); 
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
    UnwantedForeground::free_(fl);
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
    if (fl_list_table[i].free_ == NULL) {
#ifdef DEBUG
        if (fl_list_table[i].taken)
            printf("Ran out of fat locks!\n");
#endif
        fl_list_table[i].free_ = ListOfFatlocks(fl_list_chunk_len);
    }
    m = fl_list_table[i].free_;
    fl_list_table[i].free_ = m->next;
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
    m->next = fl_list_table[i].free_;
    fl_list_table[i].free_ = m;
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
    global_locks = (PRLock **) OffTheBooks::malloc_(global_lock_count * sizeof(PRLock*));
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
    fl_list_table = (JSFatLockTable *) OffTheBooks::malloc_(i * sizeof(JSFatLockTable));
    if (!fl_list_table) {
        js_CleanupLocks();
        return JS_FALSE;
    }
    fl_list_table_len = global_lock_count;
    for (i = 0; i < global_lock_count; i++)
        fl_list_table[i].free_ = fl_list_table[i].taken = NULL;
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
        UnwantedForeground::free_(global_locks);
        global_locks = NULL;
        global_lock_count = 1;
        global_locks_log2 = 0;
        global_locks_mask = 0;
    }
    if (fl_list_table) {
        for (i = 0; i < fl_list_table_len; i++) {
            DeleteListOfFatlocks(fl_list_table[i].free_);
            fl_list_table[i].free_ = NULL;
            DeleteListOfFatlocks(fl_list_table[i].taken);
            fl_list_table[i].taken = NULL;
        }
        UnwantedForeground::free_(fl_list_table);
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
    rt->rtLockOwner = NULL;
#endif
    PR_Unlock(rt->rtLock);
}

#ifdef DEBUG
JSBool
js_IsRuntimeLocked(JSRuntime *rt)
{
    return js_CurrentThreadId() == rt->rtLockOwner;
}
#endif 
#endif 
