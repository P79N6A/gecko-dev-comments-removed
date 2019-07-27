







#ifndef GrDashingEffect_DEFINED
#define GrDashingEffect_DEFINED

#include "GrTypesPriv.h"
#include "SkPathEffect.h"

class GrGpu;
class GrDrawTarget;
class GrPaint;
class GrStrokeInfo;

class GrGLDashingEffect;
class SkPath;

namespace GrDashingEffect {
    bool DrawDashLine(const SkPoint pts[2], const GrPaint& paint, const GrStrokeInfo& strokeInfo,
                      GrGpu* gpu, GrDrawTarget* target, const SkMatrix& vm);

    enum DashCap {
        kRound_DashCap,
        kNonRound_DashCap,
    };

    





    GrEffect* Create(GrEffectEdgeType edgeType, const SkPathEffect::DashInfo& info,
                     SkScalar strokeWidth, DashCap cap);
}

#endif
