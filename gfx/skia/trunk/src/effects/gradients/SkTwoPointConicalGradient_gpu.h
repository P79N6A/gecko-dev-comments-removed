






#ifndef SkTwoPointConicalGradient_gpu_DEFINED
#define SkTwoPointConicalGradient_gpu_DEFINED

#include "SkGradientShaderPriv.h"

class GrEffect;
class SkTwoPointConicalGradient;

namespace Gr2PtConicalGradientEffect {
    



    GrEffect* Create(GrContext* ctx, const SkTwoPointConicalGradient& shader,
                     SkShader::TileMode tm, const SkMatrix* localMatrix);
};

#endif
