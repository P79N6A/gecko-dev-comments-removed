








#include "GrStencilAndCoverPathRenderer.h"
#include "GrContext.h"
#include "GrDrawTargetCaps.h"
#include "GrGpu.h"
#include "GrPath.h"
#include "SkStrokeRec.h"

GrPathRenderer* GrStencilAndCoverPathRenderer::Create(GrContext* context) {
    GrAssert(NULL != context);
    GrAssert(NULL != context->getGpu());
    if (context->getGpu()->caps()->pathStencilingSupport()) {
        return SkNEW_ARGS(GrStencilAndCoverPathRenderer, (context->getGpu()));
    } else {
        return NULL;
    }
}

GrStencilAndCoverPathRenderer::GrStencilAndCoverPathRenderer(GrGpu* gpu) {
    GrAssert(gpu->caps()->pathStencilingSupport());
    fGpu = gpu;
    gpu->ref();
}

GrStencilAndCoverPathRenderer::~GrStencilAndCoverPathRenderer() {
    fGpu->unref();
}

bool GrStencilAndCoverPathRenderer::canDrawPath(const SkPath& path,
                                                const SkStrokeRec& stroke,
                                                const GrDrawTarget* target,
                                                bool antiAlias) const {
    return stroke.isFillStyle() &&
           !antiAlias && 
           target->getDrawState().getStencil().isDisabled();
}

GrPathRenderer::StencilSupport GrStencilAndCoverPathRenderer::onGetStencilSupport(
                                                        const SkPath&,
                                                        const SkStrokeRec& ,
                                                        const GrDrawTarget*) const {
    return GrPathRenderer::kStencilOnly_StencilSupport;
}

void GrStencilAndCoverPathRenderer::onStencilPath(const SkPath& path,
                                                  const SkStrokeRec& stroke,
                                                  GrDrawTarget* target) {
    GrAssert(!path.isInverseFillType());
    SkAutoTUnref<GrPath> p(fGpu->createPath(path));
    target->stencilPath(p, stroke, path.getFillType());
}

bool GrStencilAndCoverPathRenderer::onDrawPath(const SkPath& path,
                                               const SkStrokeRec& stroke,
                                               GrDrawTarget* target,
                                               bool antiAlias) {
    GrAssert(!antiAlias);
    GrAssert(!stroke.isHairlineStyle());

    GrDrawState* drawState = target->drawState();
    GrAssert(drawState->getStencil().isDisabled());

    SkAutoTUnref<GrPath> p(fGpu->createPath(path));

    SkPath::FillType nonInvertedFill = SkPath::ConvertToNonInverseFillType(path.getFillType());
    target->stencilPath(p, stroke, nonInvertedFill);

    
    

    
    GrRect bounds = p->getBounds();
    SkScalar bloat = drawState->getViewMatrix().getMaxStretch() * SK_ScalarHalf;
    GrDrawState::AutoDeviceCoordDraw adcd;

    if (nonInvertedFill == path.getFillType()) {
        GR_STATIC_CONST_SAME_STENCIL(kStencilPass,
            kZero_StencilOp,
            kZero_StencilOp,
            kNotEqual_StencilFunc,
            0xffff,
            0x0000,
            0xffff);
        *drawState->stencil() = kStencilPass;
    } else {
        GR_STATIC_CONST_SAME_STENCIL(kInvertedStencilPass,
            kZero_StencilOp,
            kZero_StencilOp,
            
            
            
            kEqualIfInClip_StencilFunc,
            0xffff,
            0x0000,
            0xffff);
        SkMatrix vmi;
        bounds.setLTRB(0, 0,
                       SkIntToScalar(drawState->getRenderTarget()->width()),
                       SkIntToScalar(drawState->getRenderTarget()->height()));
        
        if (!drawState->getViewMatrix().hasPerspective() && drawState->getViewInverse(&vmi)) {
            vmi.mapRect(&bounds);
            
            
        } else {
            adcd.set(drawState);
            bloat = 0;
        }
        *drawState->stencil() = kInvertedStencilPass;
    }
    bounds.outset(bloat, bloat);
    target->drawSimpleRect(bounds, NULL);
    target->drawState()->stencil()->setDisabled();
    return true;
}
