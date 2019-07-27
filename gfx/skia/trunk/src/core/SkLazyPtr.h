






#ifndef SkLazyPtr_DEFINED
#define SkLazyPtr_DEFINED











































#define SK_DECLARE_STATIC_LAZY_PTR(T, name, ...) \
    static Private::SkLazyPtr<T, ##__VA_ARGS__> name

#define SK_DECLARE_STATIC_LAZY_PTR_ARRAY(T, name, N, ...) \
    static Private::SkLazyPtrArray<T, N, ##__VA_ARGS__> name





#include "SkDynamicAnnotations.h"
#include "SkThread.h"
#include "SkThreadPriv.h"


class SkFontConfigInterfaceDirect;

namespace Private {



template <typename P, void (*Destroy)(P)>
static P try_cas(void** dst, P ptr) {
    P prev = (P)sk_atomic_cas(dst, NULL, ptr);

    if (prev) {
        
        Destroy(ptr);
        return prev;
    } else {
        
        return ptr;
    }
}

template <typename T> T* sk_new() { return SkNEW(T); }
template <typename T> void sk_delete(T* ptr) { SkDELETE(ptr); }














template <typename T, T* (*Create)() = sk_new<T>, void (*Destroy)(T*) = sk_delete<T> >
class SkLazyPtr {
public:
    T* get() {
        
        
        T* ptr = (T*)sk_consume_load(&fPtr);
        return ptr ? ptr : try_cas<T*, Destroy>(&fPtr, Create());
    }

#ifdef SK_DEVELOPER
    
    void cleanup(SkFontConfigInterfaceDirect*) {}
    template <typename U> void cleanup(U* ptr) { Destroy(ptr); }

    ~SkLazyPtr() {
        this->cleanup((T*)fPtr);
        fPtr = NULL;
    }
#endif

private:
    void* fPtr;
};

template <typename T> T* sk_new_arg(int i) { return SkNEW_ARGS(T, (i)); }


template <typename T, int N, T* (*Create)(int) = sk_new_arg<T>, void (*Destroy)(T*) = sk_delete<T> >
class SkLazyPtrArray {
public:
    T* operator[](int i) {
        SkASSERT(i >= 0 && i < N);
        
        
        T* ptr = (T*)sk_consume_load(&fArray[i]);
        return ptr ? ptr : try_cas<T*, Destroy>(&fArray[i], Create(i));
    }

#ifdef SK_DEVELOPER
    ~SkLazyPtrArray() {
        for (int i = 0; i < N; i++) {
            Destroy((T*)fArray[i]);
            fArray[i] = NULL;
        }
    }
#endif

private:
    void* fArray[N];
};

}  

#endif
