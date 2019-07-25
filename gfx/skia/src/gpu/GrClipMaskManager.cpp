







#include "GrClipMaskManager.h"
#include "GrGpu.h"
#include "GrRenderTarget.h"
#include "GrStencilBuffer.h"
#include "GrPathRenderer.h"
#include "GrPaint.h"
#include "SkRasterClip.h"
#include "GrAAConvexPathRenderer.h"
#include "GrAAHairLinePathRenderer.h"


#include "GrSoftwarePathRenderer.h"





void ScissoringSettings::setupScissoring(GrGpu* gpu) {
    if (!fEnableScissoring) {
        gpu->disableScissor();
        return;
    }

    gpu->enableScissoring(fScissorRect);
}

namespace {


void setup_drawstate_aaclip(GrGpu* gpu, 
                            GrTexture* result, 
                            const GrIRect &bound) {
    GrDrawState* drawState = gpu->drawState();
    GrAssert(drawState);

    static const int maskStage = GrPaint::kTotalStages+1;

    GrMatrix mat;
    mat.setIDiv(result->width(), result->height());
    mat.preTranslate(SkIntToScalar(-bound.fLeft), SkIntToScalar(-bound.fTop));
    mat.preConcat(drawState->getViewMatrix());

    drawState->sampler(maskStage)->reset(GrSamplerState::kClamp_WrapMode,
                                         GrSamplerState::kNearest_Filter,
                                         mat);

    drawState->setTexture(maskStage, result);

    
    
    gpu->addToVertexLayout(
                GrDrawTarget::StagePosAsTexCoordVertexLayoutBit(maskStage));
}

}






bool GrClipMaskManager::useSWOnlyPath(GrGpu* gpu, const GrClip& clipIn) {

    if (!clipIn.requiresAA()) {
        
        return false;
    }

    
    
    
    bool useSW = false;

    for (int i = 0; i < clipIn.getElementCount(); ++i) {

        if (SkRegion::kReplace_Op == clipIn.getOp(i)) {
            
            
            useSW = false;
        }

        if (!clipIn.getDoAA(i)) {
            
            
            continue;
        }

        if (kRect_ClipType == clipIn.getElementType(i)) {
            
            
            if (!GrAAConvexPathRenderer::staticCanDrawPath(
                                                    true,     
                                                    kEvenOdd_PathFill,
                                                    gpu, 
                                                    true)) {  
                
                
                
                useSW = true;
            }

            continue;
        }

        

        if (GrAAHairLinePathRenderer::staticCanDrawPath(clipIn.getPath(i),
                                                        clipIn.getPathFill(i),
                                                        gpu,
                                                        clipIn.getDoAA(i))) {
            
            continue;
        }

        if (GrAAConvexPathRenderer::staticCanDrawPath(
                                                clipIn.getPath(i).isConvex(),
                                                clipIn.getPathFill(i),
                                                gpu,
                                                clipIn.getDoAA(i))) {
            
            continue;
        }

        
        useSW = true;
    }

    return useSW;
}




bool GrClipMaskManager::createClipMask(GrGpu* gpu, 
                                       const GrClip& clipIn,
                                       ScissoringSettings* scissorSettings) {

    GrAssert(scissorSettings);

    scissorSettings->fEnableScissoring = false;
    fClipMaskInStencil = false;
    fClipMaskInAlpha = false;

    GrDrawState* drawState = gpu->drawState();
    if (!drawState->isClipState()) {
        return true;
    }

    GrRenderTarget* rt = drawState->getRenderTarget();

    
    GrAssert(NULL != rt);

#if GR_SW_CLIP
    
    
    
    
    if (0 == rt->numSamples() && useSWOnlyPath(gpu, clipIn)) {
        
        
        GrTexture* result = NULL;
        GrIRect bound;
        if (this->createSoftwareClipMask(gpu, clipIn, &result, &bound)) {
            fClipMaskInAlpha = true;

            setup_drawstate_aaclip(gpu, result, bound);
            return true;
        }

        
        
    }
#endif 

#if GR_AA_CLIP
    
    
    if (0 == rt->numSamples() && clipIn.requiresAA()) {
        
        
        
        
        GrTexture* result = NULL;
        GrIRect bound;
        if (this->createAlphaClipMask(gpu, clipIn, &result, &bound)) {
            fClipMaskInAlpha = true;

            setup_drawstate_aaclip(gpu, result, bound);
            return true;
        }

        
        
    }
#endif 

    
    
    
    
    
    
    
    fAACache.reset();

    GrRect bounds;
    GrRect rtRect;
    rtRect.setLTRB(0, 0,
                   GrIntToScalar(rt->width()), GrIntToScalar(rt->height()));
    if (clipIn.hasConservativeBounds()) {
        bounds = clipIn.getConservativeBounds();
        if (!bounds.intersect(rtRect)) {
            bounds.setEmpty();
        }
    } else {
        bounds = rtRect;
    }

    bounds.roundOut(&scissorSettings->fScissorRect);
    if  (scissorSettings->fScissorRect.isEmpty()) {
        scissorSettings->fScissorRect.setLTRB(0,0,0,0);
        
        
        
    }
    scissorSettings->fEnableScissoring = true;

    
    fClipMaskInStencil = !clipIn.isRect() && !clipIn.isEmpty() &&
                         !bounds.isEmpty();

    if (fClipMaskInStencil) {
        return this->createStencilClipMask(gpu, clipIn, bounds, scissorSettings);
    }

    return true;
}

#define VISUALIZE_COMPLEX_CLIP 0

#if VISUALIZE_COMPLEX_CLIP
    #include "GrRandom.h"
    GrRandom gRandom;
    #define SET_RANDOM_COLOR drawState->setColor(0xff000000 | gRandom.nextU());
#else
    #define SET_RANDOM_COLOR
#endif

namespace {




bool contains(const SkRect& container, const SkIRect& containee) {
    return  !containee.isEmpty() && !container.isEmpty() &&
            container.fLeft <= SkIntToScalar(containee.fLeft) && 
            container.fTop <= SkIntToScalar(containee.fTop) &&
            container.fRight >= SkIntToScalar(containee.fRight) && 
            container.fBottom >= SkIntToScalar(containee.fBottom);
}






int process_initial_clip_elements(const GrClip& clip,
                                  const GrIRect& bounds,
                                  bool* clearToInside,
                                  SkRegion::Op* startOp) {

    
    
    
    
    
    int curr;
    bool done = false;
    *clearToInside = true;
    int count = clip.getElementCount();

    for (curr = 0; curr < count && !done; ++curr) {
        switch (clip.getOp(curr)) {
            case SkRegion::kReplace_Op:
                
                *startOp = SkRegion::kReplace_Op;
                *clearToInside = false;
                done = true;
                break;
            case SkRegion::kIntersect_Op:
                
                
                if (kRect_ClipType == clip.getElementType(curr)
                    && contains(clip.getRect(curr), bounds)) {
                    break;
                }
                
                
                
                if (*clearToInside) {
                    *startOp = SkRegion::kReplace_Op;
                    *clearToInside = false;
                    done = true;
                }
                break;
                
            case SkRegion::kUnion_Op:
                
                
                
                if (!*clearToInside) {
                    *startOp = SkRegion::kReplace_Op;
                    done = true;
                }
                break;
            case SkRegion::kXOR_Op:
                
                
                if (*clearToInside) {
                    *startOp = SkRegion::kDifference_Op;
                } else {
                    *startOp = SkRegion::kReplace_Op;
                }
                done = true;
                break;
            case SkRegion::kDifference_Op:
                
                
                
                if (*clearToInside) {
                    *startOp = SkRegion::kDifference_Op;
                    done = true;
                }
                break;
            case SkRegion::kReverseDifference_Op:
                
                
                if (*clearToInside) {
                    *clearToInside = false;
                } else {
                    *startOp = SkRegion::kReplace_Op;
                    done = true;
                }
                break;
            default:
                GrCrash("Unknown set op.");
        }
    }
    return done ? curr-1 : count;
}

}


namespace {




void setup_boolean_blendcoeffs(GrDrawState* drawState, SkRegion::Op op) {

    switch (op) {
        case SkRegion::kReplace_Op:
            drawState->setBlendFunc(kOne_BlendCoeff, kZero_BlendCoeff);
            break;
        case SkRegion::kIntersect_Op:
            drawState->setBlendFunc(kDC_BlendCoeff, kZero_BlendCoeff);
            break;
        case SkRegion::kUnion_Op:
            drawState->setBlendFunc(kOne_BlendCoeff, kISC_BlendCoeff);
            break;
        case SkRegion::kXOR_Op:
            drawState->setBlendFunc(kIDC_BlendCoeff, kISC_BlendCoeff);
            break;
        case SkRegion::kDifference_Op:
            drawState->setBlendFunc(kZero_BlendCoeff, kISC_BlendCoeff);
            break;
        case SkRegion::kReverseDifference_Op:
            drawState->setBlendFunc(kIDC_BlendCoeff, kZero_BlendCoeff);
            break;
        default:
            GrAssert(false);
            break;
    }
}


bool draw_path(GrContext* context,
               GrGpu* gpu,
               const SkPath& path,
               GrPathFill fill,
               bool doAA) {

    GrPathRenderer* pr = context->getPathRenderer(path, fill, gpu, doAA, true);
    if (NULL == pr) {
        return false;
    }

    pr->drawPath(path, fill, NULL, gpu, 0, doAA);
    return true;
}

}


bool GrClipMaskManager::drawClipShape(GrGpu* gpu,
                                      GrTexture* target,
                                      const GrClip& clipIn,
                                      int index) {
    GrDrawState* drawState = gpu->drawState();
    GrAssert(NULL != drawState);

    drawState->setRenderTarget(target->asRenderTarget());

    if (kRect_ClipType == clipIn.getElementType(index)) {
        if (clipIn.getDoAA(index)) {
            
            SkPath temp;
            temp.addRect(clipIn.getRect(index));

            return draw_path(this->getContext(), gpu, temp,
                             kEvenOdd_PathFill, clipIn.getDoAA(index));
        } else {
            gpu->drawSimpleRect(clipIn.getRect(index), NULL, 0);
        }
    } else {
        return draw_path(this->getContext(), gpu,
                         clipIn.getPath(index),
                         clipIn.getPathFill(index),
                         clipIn.getDoAA(index));
    }
    return true;
}

void GrClipMaskManager::drawTexture(GrGpu* gpu,
                                    GrTexture* target,
                                    GrTexture* texture) {
    GrDrawState* drawState = gpu->drawState();
    GrAssert(NULL != drawState);

    
    drawState->setRenderTarget(target->asRenderTarget());

    GrMatrix sampleM;
    sampleM.setIDiv(texture->width(), texture->height());
    drawState->setTexture(0, texture);

    drawState->sampler(0)->reset(GrSamplerState::kClamp_WrapMode,
                                 GrSamplerState::kNearest_Filter,
                                 sampleM);

    GrRect rect = GrRect::MakeWH(SkIntToScalar(target->width()), 
                                 SkIntToScalar(target->height()));

    gpu->drawSimpleRect(rect, NULL, 1 << 0);

    drawState->setTexture(0, NULL);
}

namespace {

void clear(GrGpu* gpu,
           GrTexture* target,
           GrColor color) {
    GrDrawState* drawState = gpu->drawState();
    GrAssert(NULL != drawState);

    
    drawState->setRenderTarget(target->asRenderTarget());
    gpu->clear(NULL, color);
}

}



void GrClipMaskManager::getTemp(const GrIRect& bounds, 
                                GrAutoScratchTexture* temp) {
    if (NULL != temp->texture()) {
        
        return;
    }

    const GrTextureDesc desc = {
        kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit,
        bounds.width(),
        bounds.height(),
        kAlpha_8_GrPixelConfig,
        0           
    };

    temp->set(this->getContext(), desc);
}


void GrClipMaskManager::setupCache(const GrClip& clipIn,
                                   const GrIRect& bounds) {
    
    
    fAACache.reset();

    const GrTextureDesc desc = {
        kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit,
        bounds.width(),
        bounds.height(),
        kAlpha_8_GrPixelConfig,
        0           
    };

    fAACache.acquireMask(clipIn, desc, bounds);
}






bool GrClipMaskManager::clipMaskPreamble(GrGpu* gpu,
                                         const GrClip& clipIn,
                                         GrTexture** result,
                                         GrIRect *resultBounds) {
    GrDrawState* origDrawState = gpu->drawState();
    GrAssert(origDrawState->isClipState());

    GrRenderTarget* rt = origDrawState->getRenderTarget();
    GrAssert(NULL != rt);

    GrRect rtRect;
    rtRect.setLTRB(0, 0,
                    GrIntToScalar(rt->width()), GrIntToScalar(rt->height()));

    
    
    GrRect bounds;

    if (clipIn.hasConservativeBounds()) {
        bounds = clipIn.getConservativeBounds();
        if (!bounds.intersect(rtRect)) {
            
            GrAssert(false);
            bounds.setEmpty();
        }
    } else {
        
        bounds = rtRect;
    }

    GrIRect intBounds;
    bounds.roundOut(&intBounds);

    
    
    intBounds.outset(1, 1);

    

    if (fAACache.canReuse(clipIn, 
                          intBounds.width(),
                          intBounds.height())) {
        *result = fAACache.getLastMask();
        fAACache.getLastBound(resultBounds);
        return true;
    }

    this->setupCache(clipIn, intBounds);

    *resultBounds = intBounds;
    return false;
}



bool GrClipMaskManager::createAlphaClipMask(GrGpu* gpu,
                                            const GrClip& clipIn,
                                            GrTexture** result,
                                            GrIRect *resultBounds) {

    if (this->clipMaskPreamble(gpu, clipIn, result, resultBounds)) {
        return true;
    }

    GrTexture* accum = fAACache.getLastMask();
    if (NULL == accum) {
        fClipMaskInAlpha = false;
        fAACache.reset();
        return false;
    }

    GrDrawTarget::AutoStateRestore asr(gpu, GrDrawTarget::kReset_ASRInit);
    GrDrawState* drawState = gpu->drawState();

    GrDrawTarget::AutoGeometryPush agp(gpu);

    int count = clipIn.getElementCount();

    if (0 != resultBounds->fTop || 0 != resultBounds->fLeft) {
        
        
        GrMatrix m;

        m.setTranslate(SkIntToScalar(-resultBounds->fLeft), 
                       SkIntToScalar(-resultBounds->fTop));

        drawState->setViewMatrix(m);
    }

    bool clearToInside;
    SkRegion::Op startOp = SkRegion::kReplace_Op; 
    int start = process_initial_clip_elements(clipIn,
                                              *resultBounds,
                                              &clearToInside,
                                              &startOp);

    clear(gpu, accum, clearToInside ? 0xffffffff : 0x00000000);

    GrAutoScratchTexture temp;

    
    for (int c = start; c < count; ++c) {

        SkRegion::Op op = (c == start) ? startOp : clipIn.getOp(c);

        if (SkRegion::kReplace_Op == op) {
            
            
            

            
            clear(gpu, accum, 0x00000000);

            setup_boolean_blendcoeffs(drawState, op);
            this->drawClipShape(gpu, accum, clipIn, c);

        } else if (SkRegion::kReverseDifference_Op == op ||
                   SkRegion::kIntersect_Op == op) {
            
            if (SkRegion::kIntersect_Op == op &&
                kRect_ClipType == clipIn.getElementType(c) &&
                contains(clipIn.getRect(c), *resultBounds)) {
                continue;
            }

            getTemp(*resultBounds, &temp);
            if (NULL == temp.texture()) {
                fClipMaskInAlpha = false;
                fAACache.reset();
                return false;
            }

            
            clear(gpu, temp.texture(), 0x00000000);

            setup_boolean_blendcoeffs(drawState, SkRegion::kReplace_Op);
            this->drawClipShape(gpu, temp.texture(), clipIn, c);

            
            
            
            if (0 != resultBounds->fTop || 0 != resultBounds->fLeft) {
                GrMatrix m;

                m.setTranslate(SkIntToScalar(resultBounds->fLeft), 
                               SkIntToScalar(resultBounds->fTop));

                drawState->preConcatViewMatrix(m);
            }

            
            
            setup_boolean_blendcoeffs(drawState, op);
            this->drawTexture(gpu, accum, temp.texture());

            if (0 != resultBounds->fTop || 0 != resultBounds->fLeft) {
                GrMatrix m;

                m.setTranslate(SkIntToScalar(-resultBounds->fLeft), 
                               SkIntToScalar(-resultBounds->fTop));

                drawState->preConcatViewMatrix(m);
            }

        } else {
            
            
            setup_boolean_blendcoeffs(drawState, op);
            this->drawClipShape(gpu, accum, clipIn, c);
        }
    }

    *result = accum;

    return true;
}



bool GrClipMaskManager::createStencilClipMask(GrGpu* gpu, 
                                              const GrClip& clipIn,
                                              const GrRect& bounds,
                                              ScissoringSettings* scissorSettings) {

    GrAssert(fClipMaskInStencil);

    GrDrawState* drawState = gpu->drawState();
    GrAssert(drawState->isClipState());

    GrRenderTarget* rt = drawState->getRenderTarget();
    GrAssert(NULL != rt);

    
    GrStencilBuffer* stencilBuffer = rt->getStencilBuffer();
    if (NULL == stencilBuffer) {
        return false;
    }

    if (stencilBuffer->mustRenderClip(clipIn, rt->width(), rt->height())) {

        stencilBuffer->setLastClip(clipIn, rt->width(), rt->height());

        
        
        
        
        const GrClip& clipCopy = stencilBuffer->getLastClip();
        gpu->setClip(GrClip(bounds));

        GrDrawTarget::AutoStateRestore asr(gpu, GrDrawTarget::kReset_ASRInit);
        drawState = gpu->drawState();
        drawState->setRenderTarget(rt);
        GrDrawTarget::AutoGeometryPush agp(gpu);

        gpu->disableScissor();
#if !VISUALIZE_COMPLEX_CLIP
        drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
#endif

        int count = clipCopy.getElementCount();
        int clipBit = stencilBuffer->bits();
        SkASSERT((clipBit <= 16) &&
                    "Ganesh only handles 16b or smaller stencil buffers");
        clipBit = (1 << (clipBit-1));

        GrIRect rtRect = GrIRect::MakeWH(rt->width(), rt->height());

        bool clearToInside;
        SkRegion::Op startOp = SkRegion::kReplace_Op; 
        int start = process_initial_clip_elements(clipCopy,
                                                    rtRect,
                                                    &clearToInside,
                                                    &startOp);

        gpu->clearStencilClip(scissorSettings->fScissorRect, clearToInside);

        
        
        for (int c = start; c < count; ++c) {
            GrPathFill fill;
            bool fillInverted;
            
            drawState->disableState(GrGpu::kModifyStencilClip_StateBit);

            bool canRenderDirectToStencil; 
                                           
                                           
                                           
                                           

            SkRegion::Op op = (c == start) ? startOp : clipCopy.getOp(c);

            GrPathRenderer* pr = NULL;
            const SkPath* clipPath = NULL;
            if (kRect_ClipType == clipCopy.getElementType(c)) {
                canRenderDirectToStencil = true;
                fill = kEvenOdd_PathFill;
                fillInverted = false;
                
                
                if (SkRegion::kIntersect_Op == op &&
                    contains(clipCopy.getRect(c), rtRect)) {
                    continue;
                }
            } else {
                fill = clipCopy.getPathFill(c);
                fillInverted = GrIsFillInverted(fill);
                fill = GrNonInvertedFill(fill);
                clipPath = &clipCopy.getPath(c);
                pr = this->getContext()->getPathRenderer(*clipPath,
                                                         fill, gpu, false,
                                                         true);
                if (NULL == pr) {
                    fClipMaskInStencil = false;
                    gpu->setClip(clipCopy);     
                    return false;
                }
                canRenderDirectToStencil =
                    !pr->requiresStencilPass(*clipPath, fill, gpu);
            }

            int passes;
            GrStencilSettings stencilSettings[GrStencilSettings::kMaxStencilClipPasses];

            bool canDrawDirectToClip; 
                                        
                                        
                                        
            canDrawDirectToClip =
                GrStencilSettings::GetClipPasses(op,
                                                    canRenderDirectToStencil,
                                                    clipBit,
                                                    fillInverted,
                                                    &passes, stencilSettings);

            
            if (!canDrawDirectToClip) {
                GR_STATIC_CONST_SAME_STENCIL(gDrawToStencil,
                    kIncClamp_StencilOp,
                    kIncClamp_StencilOp,
                    kAlways_StencilFunc,
                    0xffff,
                    0x0000,
                    0xffff);
                SET_RANDOM_COLOR
                if (kRect_ClipType == clipCopy.getElementType(c)) {
                    *drawState->stencil() = gDrawToStencil;
                    gpu->drawSimpleRect(clipCopy.getRect(c), NULL, 0);
                } else {
                    if (canRenderDirectToStencil) {
                        *drawState->stencil() = gDrawToStencil;
                        pr->drawPath(*clipPath, fill, NULL, gpu, 0, false);
                    } else {
                        pr->drawPathToStencil(*clipPath, fill, gpu);
                    }
                }
            }

            
            
            drawState->enableState(GrGpu::kModifyStencilClip_StateBit);
            for (int p = 0; p < passes; ++p) {
                *drawState->stencil() = stencilSettings[p];
                if (canDrawDirectToClip) {
                    if (kRect_ClipType == clipCopy.getElementType(c)) {
                        SET_RANDOM_COLOR
                        gpu->drawSimpleRect(clipCopy.getRect(c), NULL, 0);
                    } else {
                        SET_RANDOM_COLOR
                        pr->drawPath(*clipPath, fill, NULL, gpu, 0, false);
                    }
                } else {
                    SET_RANDOM_COLOR
                    gpu->drawSimpleRect(bounds, NULL, 0);
                }
            }
        }
        
        gpu->setClip(clipCopy);
        
        
        fClipMaskInStencil = true;
    }

    return true;
}

namespace {

GrPathFill invert_fill(GrPathFill fill) {
    static const GrPathFill gInvertedFillTable[] = {
        kInverseWinding_PathFill, 
        kInverseEvenOdd_PathFill, 
        kWinding_PathFill,        
        kEvenOdd_PathFill,        
        kHairLine_PathFill,       
    };
    GR_STATIC_ASSERT(0 == kWinding_PathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_PathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_PathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_PathFill);
    GR_STATIC_ASSERT(4 == kHairLine_PathFill);
    GR_STATIC_ASSERT(5 == kPathFillCount);
    return gInvertedFillTable[fill];
}

}


bool GrClipMaskManager::createSoftwareClipMask(GrGpu* gpu,
                                               const GrClip& clipIn,
                                               GrTexture** result,
                                               GrIRect *resultBounds) {

    if (this->clipMaskPreamble(gpu, clipIn, result, resultBounds)) {
        return true;
    }

    GrTexture* accum = fAACache.getLastMask();
    if (NULL == accum) {
        fClipMaskInAlpha = false;
        fAACache.reset();
        return false;
    }

    GrSWMaskHelper helper(this->getContext());

    helper.init(*resultBounds, NULL, false);

    int count = clipIn.getElementCount();

    bool clearToInside;
    SkRegion::Op startOp = SkRegion::kReplace_Op; 
    int start = process_initial_clip_elements(clipIn,
                                              *resultBounds,
                                              &clearToInside,
                                              &startOp);

    helper.clear(clearToInside ? SK_ColorWHITE : 0x00000000);

    for (int i = start; i < count; ++i) {

        SkRegion::Op op = (i == start) ? startOp : clipIn.getOp(i);

        if (SkRegion::kIntersect_Op == op ||
            SkRegion::kReverseDifference_Op == op) {
            
            
            
            
            
            
            if (SkRegion::kReverseDifference_Op == op) {
                SkRect temp = SkRect::MakeLTRB(
                                       SkIntToScalar(resultBounds->left()),
                                       SkIntToScalar(resultBounds->top()),
                                       SkIntToScalar(resultBounds->right()),
                                       SkIntToScalar(resultBounds->bottom()));

                
                helper.draw(temp, SkRegion::kXOR_Op, false, SK_ColorWHITE);
            }

            if (kRect_ClipType == clipIn.getElementType(i)) {

                
                SkPath temp;
                temp.addRect(clipIn.getRect(i));

                helper.draw(temp, SkRegion::kReplace_Op, 
                            kInverseEvenOdd_PathFill, clipIn.getDoAA(i),
                            0x00000000);
            } else {
                GrAssert(kPath_ClipType == clipIn.getElementType(i));

                helper.draw(clipIn.getPath(i),
                            SkRegion::kReplace_Op,
                            invert_fill(clipIn.getPathFill(i)),
                            clipIn.getDoAA(i),
                            0x00000000);
            }

            continue;
        }

        
        
        if (kRect_ClipType == clipIn.getElementType(i)) {

            helper.draw(clipIn.getRect(i),
                        op,
                        clipIn.getDoAA(i), SK_ColorWHITE);

        } else {
            GrAssert(kPath_ClipType == clipIn.getElementType(i));

            helper.draw(clipIn.getPath(i), 
                        op,
                        clipIn.getPathFill(i), 
                        clipIn.getDoAA(i), SK_ColorWHITE);
        }
    }

    
    
    

    
    
    GrDrawState* drawState = gpu->drawState();
    GrAssert(NULL != drawState);
    GrRenderTarget* temp = drawState->getRenderTarget();
    clear(gpu, accum, 0x00000000);
    
    drawState->setRenderTarget(temp);

    helper.toTexture(accum);

    *result = accum;

    return true;
}


void GrClipMaskManager::releaseResources() {
    fAACache.releaseResources();
}
