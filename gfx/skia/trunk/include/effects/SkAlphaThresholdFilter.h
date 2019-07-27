






#ifndef SkAlphaThresholdFilter_DEFINED
#define SkAlphaThresholdFilter_DEFINED

#include "SkRegion.h"
#include "SkImageFilter.h"

class SK_API SkAlphaThresholdFilter {
public:
    






    static SkImageFilter* Create(const SkRegion& region, SkScalar innerThreshold,
                                 SkScalar outerThreshold, SkImageFilter* input = NULL);
};

#endif
