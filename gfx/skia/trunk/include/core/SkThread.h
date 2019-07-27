






#ifndef SkThread_DEFINED
#define SkThread_DEFINED

#include "SkTypes.h"






static int32_t sk_atomic_inc(int32_t* addr);
static int64_t sk_atomic_inc(int64_t* addr);




static int32_t sk_atomic_add(int32_t* addr, int32_t inc);




static int32_t sk_atomic_dec(int32_t* addr);





static bool sk_atomic_cas(int32_t* addr, int32_t before, int32_t after);




static void sk_membar_acquire__after_atomic_dec();




static void sk_membar_acquire__after_atomic_conditional_inc();

#include SK_ATOMICS_PLATFORM_H





template<typename INT_TYPE> static inline INT_TYPE sk_atomic_conditional_inc(INT_TYPE* addr) {
    INT_TYPE prev;
    do {
        prev = *addr;
        if (0 == prev) {
            break;
        }
    } while (!sk_atomic_cas(addr, prev, prev+1));
    return prev;
}




static void sk_compiler_barrier();





template <typename T> T sk_acquire_load(T*);





template <typename T> void sk_release_store(T*, T);

#include SK_BARRIERS_PLATFORM_H



















#include SK_MUTEX_PLATFORM_H


class SkAutoMutexAcquire : SkNoncopyable {
public:
    explicit SkAutoMutexAcquire(SkBaseMutex& mutex) : fMutex(&mutex) {
        SkASSERT(fMutex != NULL);
        mutex.acquire();
    }

    explicit SkAutoMutexAcquire(SkBaseMutex* mutex) : fMutex(mutex) {
        if (mutex) {
            mutex->acquire();
        }
    }

    
    ~SkAutoMutexAcquire() {
        if (fMutex) {
            fMutex->release();
        }
    }

    
    void release() {
        if (fMutex) {
            fMutex->release();
            fMutex = NULL;
        }
    }

    
    void assertHeld() {
        SkASSERT(fMutex);
        fMutex->assertHeld();
    }

private:
    SkBaseMutex* fMutex;
};
#define SkAutoMutexAcquire(...) SK_REQUIRE_LOCAL_VAR(SkAutoMutexAcquire)

#endif
