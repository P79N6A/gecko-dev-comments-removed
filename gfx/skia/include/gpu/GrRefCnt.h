









#ifndef GrRefCnt_DEFINED
#define GrRefCnt_DEFINED

#include "GrTypes.h"
#include "SkRefCnt.h"

typedef SkRefCnt GrRefCnt;
typedef SkAutoRef GrAutoRef;
typedef SkAutoUnref GrAutoUnref;

#define GrSafeRef SkSafeRef
#define GrSafeUnref SkSafeUnref
#define GrSafeAssign(a, b)  SkRefCnt_SafeAssign(a, b)

template<typename T>
static inline void GrSafeSetNull(T*& obj) {
    if (NULL != obj) {
        obj->unref();
        obj = NULL;
    }
}

#endif

