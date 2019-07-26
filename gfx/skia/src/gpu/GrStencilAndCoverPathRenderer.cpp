








#include "GrStencilAndCoverPathRenderer.h"
#include "GrContext.h"
#include "GrGpu.h"
#include "GrPath.h"

GrPathRenderer* GrStencilAndCoverPathRenderer::Create(GrContext* context) {
    GrAssert(NULL != context);
    GrAssert(NULL != context->getGpu());
    if (context->getGpu()->getCaps().pathStencilingSupport()) {
        return SkNEW_ARGS(GrStencilAndCoverPathRenderer, (context->getGpu()));
    } else {
        return NULL;
    }
}

GrStencilAndCoverPathRenderer::GrStencilAndCoverPathRenderer(GrGpu* gpu) {
    GrAssert(gpu->getCaps().pathStencilingSupport());
    fGpu = gpu;
    gpu->ref();
}

GrStencilAndCoverPathRenderer::~GrStencilAndCoverPathRenderer() {
    fGpu->unref();
}

bool GrStencilAndCoverPathRenderer::canDrawPath(const SkPath& path,
                                                GrPathFill fill,
                                                const GrDrawTarget* target,
                                                bool antiAlias) const {
    return kHairLine_GrPathFill != fill &&
           !antiAlias && 
           target->getDrawState().getStencil().isDisabled();
}

bool GrStencilAndCoverPathRenderer::requiresStencilPass(const SkPath& path,
                                                        GrPathFill fill,
                                                        const GrDrawTarget* target) const {
    return true;
}

void GrStencilAndCoverPathRenderer::drawPathToStencil(const SkPath& path,
                                                      GrPathFill fill,
                                                      GrDrawTarget* target) {
    GrAssert(kEvenOdd_GrPathFill == fill || kWinding_GrPathFill == fill);
    SkAutoTUnref<GrPath> p(fGpu->createPath(path));
    target->stencilPath(p, fill);
}

bool GrStencilAndCoverPathRenderer::onDrawPath(const SkPath& path,
                                               GrPathFill fill,
                                               const GrVec* translate,
                                               GrDrawTarget* target,
                                               bool antiAlias){
    GrAssert(!antiAlias);
    GrAssert(kHairLine_GrPathFill != fill);

    GrDrawState* drawState = target->drawState();
    GrAssert(drawState->getStencil().isDisabled());

    SkAutoTUnref<GrPath> p(fGpu->createPath(path));
    GrDrawState::AutoViewMatrixRestore avmr;
    if (translate) {
        avmr.set(drawState);
        drawState->viewMatrix()->postTranslate(translate->fX, translate->fY);
    }
    GrPathFill nonInvertedFill = GrNonInvertedFill(fill);
    target->stencilPath(p, nonInvertedFill);

    
    

    
    GrRect bounds = p->getBounds();
    GrScalar bloat = drawState->getViewMatrix().getMaxStretch() * GR_ScalarHalf;
    if (nonInvertedFill == fill) {
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
        GrMatrix vmi;
        bounds.setLTRB(0, 0,
                       GrIntToScalar(drawState->getRenderTarget()->width()),
                       GrIntToScalar(drawState->getRenderTarget()->height()));
        
        if (!drawState->getViewMatrix().hasPerspective() && drawState->getViewInverse(&vmi)) {
            vmi.mapRect(&bounds);
            
            
        } else {
            if (!drawState->preConcatSamplerMatricesWithInverse(drawState->getViewMatrix())) {
                GrPrintf("Could not invert matrix.\n");
                return false;
            }
            if (avmr.isSet()) {
                avmr.set(drawState);
            }
            drawState->viewMatrix()->reset();
            bloat = 0;
        }
        *drawState->stencil() = kInvertedStencilPass;
    }
    bounds.outset(bloat, bloat);
    target->drawSimpleRect(bounds, NULL);
    target->drawState()->stencil()->setDisabled();
    return true;
}
