







#ifndef GrBuiltInPathRenderer_DEFINED
#define GrBuiltInPathRenderer_DEFINED

#include "GrPathRenderer.h"

class GrContext;
class GrGpu;





class GrStencilAndCoverPathRenderer : public GrPathRenderer {
public:

    static GrPathRenderer* Create(GrContext* context);

    virtual ~GrStencilAndCoverPathRenderer();

    virtual bool canDrawPath(const SkPath& path,
                             GrPathFill fill,
                             const GrDrawTarget* target,
                             bool antiAlias) const SK_OVERRIDE;

    virtual bool requiresStencilPass(const SkPath& path,
                                     GrPathFill fill,
                                     const GrDrawTarget* target) const SK_OVERRIDE;

    virtual void drawPathToStencil(const SkPath& path,
                                   GrPathFill fill,
                                   GrDrawTarget* target) SK_OVERRIDE;

protected:
    virtual bool onDrawPath(const SkPath& path,
                            GrPathFill fill,
                            const GrVec* translate,
                            GrDrawTarget* target,
                            bool antiAlias) SK_OVERRIDE;

private:
    GrStencilAndCoverPathRenderer(GrGpu* gpu);

    GrGpu* fGpu;

    typedef GrPathRenderer INHERITED;
};

#endif
