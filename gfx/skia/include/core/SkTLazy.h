









#ifndef SkTLazy_DEFINED
#define SkTLazy_DEFINED

#include "SkTypes.h"
#include <new>





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
    
private:
    T*   fPtr; 
    char fStorage[sizeof(T)];
};

#endif

