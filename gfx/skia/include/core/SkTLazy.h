









#ifndef SkTLazy_DEFINED
#define SkTLazy_DEFINED

#include "SkTypes.h"
#include <new>

template <typename T> class SkTLazy;
template <typename T> void* operator new(size_t, SkTLazy<T>* lazy);





template <typename T> class SkTLazy {
public:
    SkTLazy() : fPtr(NULL) {}

    explicit SkTLazy(const T* src) : fPtr(NULL) {
        if (src) {
            fPtr = new (fStorage) T(*src);
        }
    }

    SkTLazy(const SkTLazy<T>& src) : fPtr(NULL) {
        if (src.isValid()) {
            fPtr = new (fStorage) T(*src->get());
        } else {
            fPtr = NULL;
        }
    }

    ~SkTLazy() {
        if (this->isValid()) {
            fPtr->~T();
        }
    }

    





    T* init() {
        if (this->isValid()) {
            fPtr->~T();
        }
        fPtr = new (SkTCast<T*>(fStorage)) T;
        return fPtr;
    }

    





    T* set(const T& src) {
        if (this->isValid()) {
            *fPtr = src;
        } else {
            fPtr = new (SkTCast<T*>(fStorage)) T(src);
        }
        return fPtr;
    }

    



    bool isValid() const { return NULL != fPtr; }

    



    T* get() const { SkASSERT(this->isValid()); return fPtr; }

    



    T* getMaybeNull() const { return fPtr; }

private:
    friend void* operator new<T>(size_t, SkTLazy* lazy);

    T*   fPtr; 
    char fStorage[sizeof(T)];
};


template <typename T> void* operator new(size_t, SkTLazy<T>* lazy) {
    SkASSERT(!lazy->isValid());
    lazy->fPtr = reinterpret_cast<T*>(lazy->fStorage);
    return lazy->fPtr;
}




template <typename T> void operator delete(void*, SkTLazy<T>*) { SK_CRASH(); }


#define SkNEW_IN_TLAZY(tlazy_ptr, type_name, args) (new (tlazy_ptr) type_name args)
























template <typename T>
class SkTCopyOnFirstWrite {
public:
    SkTCopyOnFirstWrite(const T& initial) : fObj(&initial) {}

    
    SkTCopyOnFirstWrite() : fObj(NULL) {}

    
    void init(const T& initial) {
        SkASSERT(NULL == fObj);
        SkASSERT(!fLazy.isValid());
        fObj = &initial;
    }

    


    T* writable() {
        SkASSERT(NULL != fObj);
        if (!fLazy.isValid()) {
            fLazy.set(*fObj);
            fObj = fLazy.get();
        }
        return const_cast<T*>(fObj);
    }

    



    const T *operator->() const { return fObj; }

    operator const T*() const { return fObj; }

    const T& operator *() const { return *fObj; }

private:
    const T*    fObj;
    SkTLazy<T>  fLazy;
};

#endif
