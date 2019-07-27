






#ifndef GrTemplates_DEFINED
#define GrTemplates_DEFINED

#include "SkTypes.h"




template <typename Dst, typename Src> Dst GrTCast(Src src) {
    union {
        Src src;
        Dst dst;
    } data;
    data.src = src;
    return data.dst;
}














template <typename T> class GrAutoTRestore : SkNoncopyable {
public:
    GrAutoTRestore() : fPtr(NULL), fVal() {}

    GrAutoTRestore(T* ptr) {
        fPtr = ptr;
        if (NULL != ptr) {
            fVal = *ptr;
        }
    }

    ~GrAutoTRestore() {
        if (NULL != fPtr) {
            *fPtr = fVal;
        }
    }

    
    void reset(T* ptr) {
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
