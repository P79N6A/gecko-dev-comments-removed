






#ifndef GrRRectEffect_DEFINED
#define GrRRectEffect_DEFINED

#include "GrTypes.h"
#include "GrTypesPriv.h"

class GrEffect;
class SkRRect;

namespace GrRRectEffect {
    



    GrEffect* Create(GrEffectEdgeType, const SkRRect&);
};

#endif
