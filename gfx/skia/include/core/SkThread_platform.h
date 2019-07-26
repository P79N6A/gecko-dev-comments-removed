








#ifndef SkThread_platform_DEFINED
#define SkThread_platform_DEFINED

#if defined(SK_BUILD_FOR_ANDROID)

#if defined(SK_BUILD_FOR_ANDROID_NDK)

#include <stdint.h>




static inline __attribute__((always_inline)) int32_t sk_atomic_inc(int32_t *addr) {
    return __sync_fetch_and_add(addr, 1);
}

static inline __attribute__((always_inline)) int32_t sk_atomic_add(int32_t *addr, int32_t inc) {
    return __sync_fetch_and_add(addr, inc);
}

static inline __attribute__((always_inline)) int32_t sk_atomic_dec(int32_t *addr) {
    return __sync_fetch_and_add(addr, -1);
}
static inline __attribute__((always_inline)) void sk_membar_aquire__after_atomic_dec() { }

static inline __attribute__((always_inline)) int32_t sk_atomic_conditional_inc(int32_t* addr) {
    int32_t value = *addr;

    while (true) {
        if (value == 0) {
            return 0;
        }

        int32_t before = __sync_val_compare_and_swap(addr, value, value + 1);

        if (before == value) {
            return value;
        } else {
            value = before;
        }
    }
}
static inline __attribute__((always_inline)) void sk_membar_aquire__after_atomic_conditional_inc() { }

#else 




#include <utils/Atomic.h>

#define sk_atomic_inc(addr)         android_atomic_inc(addr)
#define sk_atomic_add(addr, inc)    android_atomic_add(inc, addr)
#define sk_atomic_dec(addr)         android_atomic_dec(addr)
void sk_membar_aquire__after_atomic_dec() {
    
    
    
    
}
int32_t sk_atomic_conditional_inc(int32_t* addr) {
    while (true) {
        int32_t value = *addr;
        if (value == 0) {
            return 0;
        }
        if (0 == android_atomic_release_cas(value, value + 1, addr)) {
            return value;
        }
    }
}
void sk_membar_aquire__after_atomic_conditional_inc() {
    
    
    
    
}

#endif 

#else  







SK_API int32_t sk_atomic_inc(int32_t* addr);







SK_API int32_t sk_atomic_add(int32_t* addr, int32_t inc);






SK_API int32_t sk_atomic_dec(int32_t* addr);



SK_API void sk_membar_aquire__after_atomic_dec();







SK_API int32_t sk_atomic_conditional_inc(int32_t*);




SK_API void sk_membar_aquire__after_atomic_conditional_inc();

#endif 

#ifdef SK_USE_POSIX_THREADS

#include <pthread.h>






struct SkBaseMutex {
    void    acquire() { pthread_mutex_lock(&fMutex); }
    void    release() { pthread_mutex_unlock(&fMutex); }
    pthread_mutex_t  fMutex;
};



#define SK_DECLARE_STATIC_MUTEX(name)   static SkBaseMutex  name = { PTHREAD_MUTEX_INITIALIZER }


#define SK_DECLARE_GLOBAL_MUTEX(name)   SkBaseMutex  name = { PTHREAD_MUTEX_INITIALIZER }

#define SK_DECLARE_MUTEX_ARRAY(name, count)    SkBaseMutex name[count] = { { PTHREAD_MUTEX_INITIALIZER } }



class SkMutex : public SkBaseMutex, SkNoncopyable {
public:
    SkMutex();
    ~SkMutex();
};

#else 




class SkMutex : SkNoncopyable {
public:
    SkMutex();
    ~SkMutex();

    void    acquire();
    void    release();

private:
    bool fIsGlobal;
    enum {
        kStorageIntCount = 64
    };
    uint32_t    fStorage[kStorageIntCount];
};

typedef SkMutex SkBaseMutex;

#define SK_DECLARE_STATIC_MUTEX(name)           static SkBaseMutex  name
#define SK_DECLARE_GLOBAL_MUTEX(name)           SkBaseMutex  name
#define SK_DECLARE_MUTEX_ARRAY(name, count)     SkBaseMutex name[count]

#endif 


#endif
