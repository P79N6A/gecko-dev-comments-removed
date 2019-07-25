








#ifndef GrTemplates_DEFINED
#define GrTemplates_DEFINED

#include "GrNoncopyable.h"




template <typename Dst, typename Src> Dst GrTCast(Src src) {
    union {
        Src src;
        Dst dst;
    } data;
    data.src = src;
    return data.dst;
}













template <typename T> class GrAutoTPtrValueRestore : public GrNoncopyable {
public:
    GrAutoTPtrValueRestore() : fPtr(NULL), fVal() {}
    
    GrAutoTPtrValueRestore(T* ptr) {
        fPtr = ptr;
        if (NULL != ptr) {
            fVal = *ptr;
        }
    }
    
    ~GrAutoTPtrValueRestore() {
        if (NULL != fPtr) {
            *fPtr = fVal;
        }
    }
    
    
    void save(T* ptr) {
        if (NULL != fPtr) {
            *fPtr = fVal;
        }
        fPtr = ptr;
        fVal = *ptr;
    }
private:
    T* fPtr;
    T  fVal;
};

#endif
