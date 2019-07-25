








#ifndef SkTRelay_DEFINED
#define SkTRelay_DEFINED

#include "SkRefCnt.h"













template <template T> class SkTRelay : public SkRefCnt {
public:
    SkTRelay(T* ptr) : fPtr(ptr) {}

    
    T* get() const { return fPtr; }

    
    void set(T* ptr) { fPtr = ptr; }

    void clear() { this->set(NULL); }

private:
    T* fPtr;
};

#endif
