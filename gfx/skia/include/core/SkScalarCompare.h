








#ifndef SkScalarCompare_DEFINED
#define SkScalarCompare_DEFINED

#include "SkFloatBits.h"
#include "SkRect.h"













#ifdef SK_SCALAR_SLOW_COMPARES
    typedef int32_t SkScalarCompareType;
    typedef SkIRect SkRectCompareType;
    #define SkScalarToCompareType(x)    SkScalarAs2sCompliment(x)
#else
    typedef SkScalar SkScalarCompareType;
    typedef SkRect SkRectCompareType;
    #define SkScalarToCompareType(x)    (x)
#endif

#endif

