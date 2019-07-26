






#ifndef SkOnce_DEFINED
#define SkOnce_DEFINED





















#include "SkDynamicAnnotations.h"
#include "SkThread.h"
#include "SkTypes.h"

#define SK_ONCE_INIT { false, { 0, SkDEBUGCODE(0) } }
#define SK_DECLARE_STATIC_ONCE(name) static SkOnceFlag name = SK_ONCE_INIT

struct SkOnceFlag;  

template <typename Func, typename Arg>
inline void SkOnce(SkOnceFlag* once, Func f, Arg arg, void(*atExit)() = NULL);


template <typename Lock, typename Func, typename Arg>
inline void SkOnce(bool* done, Lock* lock, Func f, Arg arg, void(*atExit)() = NULL);




struct SkSpinlock {
    void acquire() {
        SkASSERT(shouldBeZero == 0);
        
        while (!sk_atomic_cas(&thisIsPrivate, 0, 1)) {
            
        }
    }

    void release() {
        SkASSERT(shouldBeZero == 0);
        
        SkAssertResult(sk_atomic_cas(&thisIsPrivate, 1, 0));
    }

    int32_t thisIsPrivate;
    SkDEBUGCODE(int32_t shouldBeZero;)
};

struct SkOnceFlag {
    bool done;
    SkSpinlock lock;
};



#ifdef SK_BUILD_FOR_WIN
#  include <intrin.h>
inline static void compiler_barrier() {
    _ReadWriteBarrier();
}
#else
inline static void compiler_barrier() {
    asm volatile("" : : : "memory");
}
#endif

inline static void full_barrier_on_arm() {
#ifdef SK_CPU_ARM
#  if SK_ARM_ARCH >= 7
    asm volatile("dmb" : : : "memory");
#  else
    asm volatile("mcr p15, 0, %0, c7, c10, 5" : : "r" (0) : "memory");
#  endif
#endif
}








inline static void release_barrier() {
    compiler_barrier();
    full_barrier_on_arm();
}

inline static void acquire_barrier() {
    compiler_barrier();
    full_barrier_on_arm();
}


template <typename Lock>
class SkAutoLockAcquire {
public:
    explicit SkAutoLockAcquire(Lock* lock) : fLock(lock) { fLock->acquire(); }
    ~SkAutoLockAcquire() { fLock->release(); }
private:
    Lock* fLock;
};








template <typename Lock, typename Func, typename Arg>
static void sk_once_slow(bool* done, Lock* lock, Func f, Arg arg, void (*atExit)()) {
    const SkAutoLockAcquire<Lock> locked(lock);
    if (!*done) {
        f(arg);
        if (atExit != NULL) {
            atexit(atExit);
        }
        
        
        
        
        
        
        
        
        
        release_barrier();
        *done = true;
    }
}


template <typename Lock, typename Func, typename Arg>
inline void SkOnce(bool* done, Lock* lock, Func f, Arg arg, void(*atExit)()) {
    if (!SK_ANNOTATE_UNPROTECTED_READ(*done)) {
        sk_once_slow(done, lock, f, arg, atExit);
    }
    
    
    
    
    
    
    
    
    
    
    acquire_barrier();
}

template <typename Func, typename Arg>
inline void SkOnce(SkOnceFlag* once, Func f, Arg arg, void(*atExit)()) {
    return SkOnce(&once->done, &once->lock, f, arg, atExit);
}

#undef SK_ANNOTATE_BENIGN_RACE

#endif  
