








#ifndef GrPathRenderer_DEFINED
#define GrPathRenderer_DEFINED

#include "GrDrawTarget.h"
#include "GrPathRendererChain.h"
#include "GrStencil.h"

#include "SkStrokeRec.h"
#include "SkTArray.h"

class SkPath;

struct GrPoint;








class GR_API GrPathRenderer : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrPathRenderer)

    








    static void AddPathRenderers(GrContext* context, GrPathRendererChain* prChain);


    GrPathRenderer();

    


















    typedef GrPathRendererChain::StencilSupport StencilSupport;
    static const StencilSupport kNoSupport_StencilSupport =
        GrPathRendererChain::kNoSupport_StencilSupport;
    static const StencilSupport kStencilOnly_StencilSupport =
        GrPathRendererChain::kStencilOnly_StencilSupport;
    static const StencilSupport kNoRestriction_StencilSupport =
        GrPathRendererChain::kNoRestriction_StencilSupport;

    







    StencilSupport getStencilSupport(const SkPath& path,
                                     const SkStrokeRec& stroke,
                                     const GrDrawTarget* target) const {
        GrAssert(!path.isInverseFillType());
        return this->onGetStencilSupport(path, stroke, target);
    }

    











    virtual bool canDrawPath(const SkPath& path,
                             const SkStrokeRec& rec,
                             const GrDrawTarget* target,
                             bool antiAlias) const = 0;
    








    bool drawPath(const SkPath& path,
                  const SkStrokeRec& stroke,
                  GrDrawTarget* target,
                  bool antiAlias) {
        GrAssert(this->canDrawPath(path, stroke, target, antiAlias));
        GrAssert(target->drawState()->getStencil().isDisabled() ||
                 kNoRestriction_StencilSupport == this->getStencilSupport(path, stroke, target));
        return this->onDrawPath(path, stroke, target, antiAlias);
    }

    







    void stencilPath(const SkPath& path, const SkStrokeRec& stroke, GrDrawTarget* target) {
        GrAssert(kNoSupport_StencilSupport != this->getStencilSupport(path, stroke, target));
        this->onStencilPath(path, stroke, target);
    }

protected:
    


    virtual StencilSupport onGetStencilSupport(const SkPath&,
                                               const SkStrokeRec&,
                                               const GrDrawTarget*) const {
        return kNoRestriction_StencilSupport;
    }

    


    virtual bool onDrawPath(const SkPath& path,
                            const SkStrokeRec& stroke,
                            GrDrawTarget* target,
                            bool antiAlias) = 0;

    



    virtual void onStencilPath(const SkPath& path,  const SkStrokeRec& stroke, GrDrawTarget* target) {
        GrDrawTarget::AutoStateRestore asr(target, GrDrawTarget::kPreserve_ASRInit);
        GrDrawState* drawState = target->drawState();
        GR_STATIC_CONST_SAME_STENCIL(kIncrementStencil,
                                     kReplace_StencilOp,
                                     kReplace_StencilOp,
                                     kAlways_StencilFunc,
                                     0xffff,
                                     0xffff,
                                     0xffff);
        drawState->setStencil(kIncrementStencil);
        drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
        this->drawPath(path, stroke, target, false);
    }

private:

    typedef GrRefCnt INHERITED;
};

#endif
