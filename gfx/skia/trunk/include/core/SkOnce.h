






#ifndef SkOnce_DEFINED
#define SkOnce_DEFINED




















#include "SkDynamicAnnotations.h"
#include "SkThread.h"
#include "SkTypes.h"


#define SK_DECLARE_STATIC_ONCE(name) static SkOnceFlag name

class SkOnceFlag;

inline void SkOnce(SkOnceFlag* once, void (*f)());

template <typename Arg>
inline void SkOnce(SkOnceFlag* once, void (*f)(Arg), Arg arg);


template <typename Lock>
inline void SkOnce(bool* done, Lock* lock, void (*f)());

template <typename Lock, typename Arg>
inline void SkOnce(bool* done, Lock* lock, void (*f)(Arg), Arg arg);




class SkOnceFlag {
public:
    bool* mutableDone() { return &fDone; }

    void acquire() {
        
        
        while (!sk_atomic_cas(&fSpinlock, 0, 1)) {
            
        }
        
        SkAssertResult(sk_acquire_load(&fSpinlock));
    }

    void release() {
        
        SkAssertResult(sk_atomic_cas(&fSpinlock, 1, 0));
    }

private:
    bool fDone;
    int32_t fSpinlock;
};








template <typename Lock, typename Arg>
static void sk_once_slow(bool* done, Lock* lock, void (*f)(Arg), Arg arg) {
    lock->acquire();
    if (!*done) {
        f(arg);
        
        
        
        
        
        
        
        
        
        sk_release_store(done, true);
    }
    lock->release();
}


template <typename Lock, typename Arg>
inline void SkOnce(bool* done, Lock* lock, void (*f)(Arg), Arg arg) {
    if (!SK_ANNOTATE_UNPROTECTED_READ(*done)) {
        sk_once_slow(done, lock, f, arg);
    }
    
    
    
    
    
    
    
    
    
    
    SkAssertResult(sk_acquire_load(done));
}

template <typename Arg>
inline void SkOnce(SkOnceFlag* once, void (*f)(Arg), Arg arg) {
    return SkOnce(once->mutableDone(), once, f, arg);
}




static void sk_once_no_arg_adaptor(void (*f)()) {
    f();
}

inline void SkOnce(SkOnceFlag* once, void (*func)()) {
    return SkOnce(once, sk_once_no_arg_adaptor, func);
}

template <typename Lock>
inline void SkOnce(bool* done, Lock* lock, void (*func)()) {
    return SkOnce(done, lock, sk_once_no_arg_adaptor, func);
}

#endif  
