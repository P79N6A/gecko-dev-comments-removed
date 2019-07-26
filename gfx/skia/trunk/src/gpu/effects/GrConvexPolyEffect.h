






#ifndef GrConvexPolyEffect_DEFINED
#define GrConvexPolyEffect_DEFINED

#include "GrDrawTargetCaps.h"
#include "GrEffect.h"
#include "GrVertexEffect.h"

class GrGLConvexPolyEffect;
class SkPath;






class GrConvexPolyEffect : public GrEffect {
public:
    

    enum EdgeType {
        kFillNoAA_EdgeType,
        kFillAA_EdgeType,

        kLastEdgeType = kFillAA_EdgeType,
    };

    enum {
        kEdgeTypeCnt = kLastEdgeType + 1,
        kMaxEdges = 8,
    };

    










    static GrEffectRef* Create(EdgeType edgeType, int n, const SkScalar edges[]) {
        if (n <= 0 || n > kMaxEdges) {
            return NULL;
        }
        return CreateEffectRef(AutoEffectUnref(SkNEW_ARGS(GrConvexPolyEffect,
                                                          (edgeType, n, edges))));
    }

    




    static GrEffectRef* Create(EdgeType, const SkPath&, const SkVector* offset= NULL);

    


    static GrEffectRef* CreateForAAFillRect(const SkRect&);

    virtual ~GrConvexPolyEffect();

    static const char* Name() { return "ConvexPoly"; }

    EdgeType getEdgeType() const { return fEdgeType; }

    int getEdgeCount() const { return fEdgeCount; }

    const SkScalar* getEdges() const { return fEdges; }

    typedef GrGLConvexPolyEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrConvexPolyEffect(EdgeType edgeType, int n, const SkScalar edges[]);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    EdgeType fEdgeType;
    int      fEdgeCount;
    SkScalar fEdges[3 * kMaxEdges];

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};


#endif
