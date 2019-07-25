








#ifndef GrPathRenderer_DEFINED
#define GrPathRenderer_DEFINED

#include "GrDrawTarget.h"
#include "GrPathRendererChain.h"

#include "SkTArray.h"

class SkPath;

struct GrPoint;








class GR_API GrPathRenderer : public GrRefCnt {
public:

    










    static void AddPathRenderers(GrContext* context,
                                 GrPathRendererChain::UsageFlags flags,
                                 GrPathRendererChain* prChain);


    GrPathRenderer();

    


















    virtual bool requiresStencilPass(const SkPath& path,
                                     GrPathFill fill,
                                     const GrDrawTarget* target) const {
        return false;
    }

    












    virtual bool canDrawPath(const SkPath& path,
                             GrPathFill fill,
                             const GrDrawTarget* target,
                             bool antiAlias) const = 0;
    
















    virtual bool drawPath(const SkPath& path,
                          GrPathFill fill,
                          const GrVec* translate,
                          GrDrawTarget* target,
                          GrDrawState::StageMask stageMask,
                          bool antiAlias) {
        GrAssert(this->canDrawPath(path, fill, target, antiAlias));
        return this->onDrawPath(path, fill, translate,
                                target, stageMask, antiAlias);
    }

    











    virtual void drawPathToStencil(const SkPath& path,
                                   GrPathFill fill,
                                   GrDrawTarget* target) {
        GrCrash("Unexpected call to drawPathToStencil.");
    }

protected:
    














    virtual bool onDrawPath(const SkPath& path,
                            GrPathFill fill,
                            const GrVec* translate,
                            GrDrawTarget* target,
                            GrDrawState::StageMask stageMask,
                            bool antiAlias) = 0;

private:

    typedef GrRefCnt INHERITED;
};

#endif

