






#ifndef GrOvalEffect_DEFINED
#define GrOvalEffect_DEFINED

#include "GrTypes.h"
#include "GrTypesPriv.h"

class GrEffect;
struct SkRect;

namespace GrOvalEffect {
    


    GrEffect* Create(GrEffectEdgeType, const SkRect&);
};

#endif
