






#ifndef GrRRectEffect_DEFINED
#define GrRRectEffect_DEFINED

#include "GrTypes.h"
#include "GrTypesPriv.h"

class GrEffectRef;
class SkRRect;

namespace GrRRectEffect {
    



    GrEffectRef* Create(GrEffectEdgeType, const SkRRect&);
};

#endif
