





#include "TestHarness.h"



#define PASS()                                  \
    do {                                        \
        passed(__FUNCTION__);                   \
        return NS_OK;                           \
    } while (0)

#define FAIL(why)                               \
    do {                                        \
        fail("%s | %s - %s", __FILE__, __FUNCTION__, why); \
        return NS_ERROR_FAILURE;                \
    } while (0)

#ifdef OLD_API
#  include "nsAutoLock.h"
   typedef PRLock* moz_lock_t;
#  define NEWLOCK(n) nsAutoLock::NewLock(n)
#  define DELETELOCK(v) nsAutoLock::DestroyLock(v)
#  define AUTOLOCK(v, l) nsAutoLock v(l)
#else
#  include "mozilla/Mutex.h"
   typedef mozilla::Mutex* moz_lock_t;
#  define NEWLOCK(n) new mozilla::Mutex(n)
#  define DELETELOCK(v) delete (v)
#  define AUTOLOCK(v, l) mozilla::MutexAutoLock v(*l)
#endif


#undef DD_TEST1
#undef DD_TEST2
#undef DD_TEST3



#ifdef DD_TEST1

static void
AllocLockRecurseUnlockFree(int i)
{
    if (0 == i)
        return;

    moz_lock_t lock = NEWLOCK("deadlockDetector.scalability.t1");
    {
        AUTOLOCK(_, lock);
        AllocLockRecurseUnlockFree(i - 1);
    }
    DELETELOCK(lock);
}



static nsresult
LengthNDepChain(int N)
{
    AllocLockRecurseUnlockFree(N);
    PASS();
}

#endif



#ifdef DD_TEST2



static nsresult
OneLockNDeps(const int N, const int K)
{
    moz_lock_t lock = NEWLOCK("deadlockDetector.scalability.t2.master");
    moz_lock_t* locks = new moz_lock_t[N];
    if (!locks)
        NS_RUNTIMEABORT("couldn't allocate lock array");

    for (int i = 0; i < N; ++i)
        locks[i] =
            NEWLOCK("deadlockDetector.scalability.t2.dep");

    
    {AUTOLOCK(m, lock);
        for (int i = 0; i < N; ++i)
            AUTOLOCK(s, locks[i]);
    }

    
    {AUTOLOCK(m, lock);
        for (int i = 0; i < K; ++i)
            for (int j = 0; j < N; ++j)
                AUTOLOCK(s, locks[i]);
    }

    for (int i = 0; i < N; ++i)
        DELETELOCK(locks[i]);
    delete[] locks;

    PASS();
}

#endif



#ifdef DD_TEST3








static nsresult
MaxDepsNsq(const int N, const int K)
{
    moz_lock_t* locks = new moz_lock_t[N];
    if (!locks)
        NS_RUNTIMEABORT("couldn't allocate lock array");

    for (int i = 0; i < N; ++i)
        locks[i] = NEWLOCK("deadlockDetector.scalability.t3");

    for (int i = 0; i < N; ++i) {
        AUTOLOCK(al1, locks[i]);
        for (int j = i+1; j < N; ++j)
            AUTOLOCK(al2, locks[j]);
    }

    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < N; ++j) {
            AUTOLOCK(al1, locks[j]);
            for (int k = j+1; k < N; ++k)
                AUTOLOCK(al2, locks[k]);
        }
    }

    for (int i = 0; i < N; ++i)
        DELETELOCK(locks[i]);
    delete[] locks;

    PASS();
}

#endif



int
main(int argc, char** argv)
{
    ScopedXPCOM xpcom("Deadlock detector scalability (" __FILE__ ")");
    if (xpcom.failed())
        return 1;

    int rv = 0;

    

#ifndef DD_TEST1
    puts("Skipping not-requested LengthNDepChain() test");
#else
    if (NS_FAILED(LengthNDepChain(1 << 14))) 
        rv = 1;
#endif

#ifndef DD_TEST2
    puts("Skipping not-requested OneLockNDeps() test");
#else
    
    if (NS_FAILED(OneLockNDeps(1 << 17, 100))) 
        rv = 1;
#endif

#ifndef DD_TEST3
    puts("Skipping not-requested MaxDepsNsq() test");
#else
    if (NS_FAILED(MaxDepsNsq(1 << 10, 10))) 
        rv = 1;
#endif

    return rv;
}
