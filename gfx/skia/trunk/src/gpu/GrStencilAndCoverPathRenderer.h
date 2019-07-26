







#ifndef GrBuiltInPathRenderer_DEFINED
#define GrBuiltInPathRenderer_DEFINED

#include "GrPathRenderer.h"

class GrContext;
class GrGpu;





class GrStencilAndCoverPathRenderer : public GrPathRenderer {
public:

    static GrPathRenderer* Create(GrContext*);

    virtual ~GrStencilAndCoverPathRenderer();

    virtual bool canDrawPath(const SkPath&,
                             const SkStrokeRec&,
                             const GrDrawTarget*,
                             bool antiAlias) const SK_OVERRIDE;

protected:
    virtual StencilSupport onGetStencilSupport(const SkPath&,
                                               const SkStrokeRec&,
                                               const GrDrawTarget*) const SK_OVERRIDE;

    virtual bool onDrawPath(const SkPath&,
                            const SkStrokeRec&,
                            GrDrawTarget*,
                            bool antiAlias) SK_OVERRIDE;

    virtual void onStencilPath(const SkPath&,
                               const SkStrokeRec&,
                               GrDrawTarget*) SK_OVERRIDE;

private:
    GrStencilAndCoverPathRenderer(GrGpu*);

    GrGpu* fGpu;

    typedef GrPathRenderer INHERITED;
};

#endif
