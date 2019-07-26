






#ifndef GrConvexPolyEffect_DEFINED
#define GrConvexPolyEffect_DEFINED

#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrTypesPriv.h"

class GrGLConvexPolyEffect;
class SkPath;






class GrConvexPolyEffect : public GrEffect {
public:
    enum {
        kMaxEdges = 8,
    };

    










    static GrEffectRef* Create(GrEffectEdgeType edgeType, int n, const SkScalar edges[]) {
        if (n <= 0 || n > kMaxEdges || kHairlineAA_GrEffectEdgeType == edgeType) {
            return NULL;
        }
        return CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(GrConvexPolyEffect,
                                                          (edgeType, n, edges))));
    }

    




    static GrEffectRef* Create(GrEffectEdgeType, const SkPath&, const SkVector* offset = NULL);

    


    static GrEffectRef* Create(GrEffectEdgeType, const SkRect&);

    virtual ~GrConvexPolyEffect();

    static const char* Name() { return "ConvexPoly"; }

    GrEffectEdgeType getEdgeType() const { return fEdgeType; }

    int getEdgeCount() const { return fEdgeCount; }

    const SkScalar* getEdges() const { return fEdges; }

    typedef GrGLConvexPolyEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrConvexPolyEffect(GrEffectEdgeType edgeType, int n, const SkScalar edges[]);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrEffectEdgeType    fEdgeType;
    int                 fEdgeCount;
    SkScalar            fEdges[3 * kMaxEdges];

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};


#endif
