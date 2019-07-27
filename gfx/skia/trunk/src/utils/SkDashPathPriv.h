






#ifndef SkDashPathPriv_DEFINED
#define SkDashPathPriv_DEFINED

#include "SkPathEffect.h"

namespace SkDashPath {
    





    void CalcDashParameters(SkScalar phase, const SkScalar intervals[], int32_t count,
                            SkScalar* initialDashLength, int32_t* initialDashIndex,
                            SkScalar* intervalLength, SkScalar* adjustedPhase = NULL);

    bool FilterDashPath(SkPath* dst, const SkPath& src, SkStrokeRec*, const SkRect*,
                        const SkScalar aIntervals[], int32_t count, SkScalar initialDashLength,
                        int32_t initialDashIndex, SkScalar intervalLength);
    
    bool FilterDashPath(SkPath* dst, const SkPath& src, SkStrokeRec*, const SkRect*,
                        const SkPathEffect::DashInfo& info);
}

#endif
