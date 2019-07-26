







#include "GrClipMaskManager.h"
#include "GrGpu.h"
#include "GrRenderTarget.h"
#include "GrStencilBuffer.h"
#include "GrPathRenderer.h"
#include "GrPaint.h"
#include "SkRasterClip.h"
#include "GrAAConvexPathRenderer.h"
#include "GrAAHairLinePathRenderer.h"
#include "GrSWMaskHelper.h"
#include "GrCacheID.h"

GR_DEFINE_RESOURCE_CACHE_DOMAIN(GrClipMaskManager, GetAlphaMaskDomain)





namespace {


void setup_drawstate_aaclip(GrGpu* gpu,
                            GrTexture* result,
                            const GrIRect &devBound) {
    GrDrawState* drawState = gpu->drawState();
    GrAssert(drawState);

    static const int maskStage = GrPaint::kTotalStages+1;

    GrMatrix mat;
    mat.setIDiv(result->width(), result->height());
    mat.preTranslate(SkIntToScalar(-devBound.fLeft),
                     SkIntToScalar(-devBound.fTop));
    mat.preConcat(drawState->getViewMatrix());

    drawState->sampler(maskStage)->reset(mat);

    drawState->createTextureEffect(maskStage, result);
}

bool path_needs_SW_renderer(GrContext* context,
                            GrGpu* gpu,
                            const SkPath& path,
                            GrPathFill fill,
                            bool doAA) {
    
    return NULL == context->getPathRenderer(path, fill, gpu, doAA, false);
}

GrPathFill get_path_fill(const SkPath& path) {
    switch (path.getFillType()) {
        case SkPath::kWinding_FillType:
            return kWinding_GrPathFill;
        case SkPath::kEvenOdd_FillType:
            return  kEvenOdd_GrPathFill;
        case SkPath::kInverseWinding_FillType:
            return kInverseWinding_GrPathFill;
        case SkPath::kInverseEvenOdd_FillType:
            return kInverseEvenOdd_GrPathFill;
        default:
            GrCrash("Unsupported path fill in clip.");
            return kWinding_GrPathFill; 
    }
}




bool requires_AA(const SkClipStack& clipIn) {

    SkClipStack::Iter iter;
    iter.reset(clipIn, SkClipStack::Iter::kBottom_IterStart);

    const SkClipStack::Iter::Clip* clip = NULL;
    for (clip = iter.skipToTopmost(SkRegion::kReplace_Op);
         NULL != clip;
         clip = iter.next()) {

        if (clip->fDoAA) {
            return true;
        }
    }

    return false;
}

}






bool GrClipMaskManager::useSWOnlyPath(const SkClipStack& clipIn) {

    
    
    
    bool useSW = false;

    SkClipStack::Iter iter(clipIn, SkClipStack::Iter::kBottom_IterStart);
    const SkClipStack::Iter::Clip* clip = NULL;

    for (clip = iter.skipToTopmost(SkRegion::kReplace_Op);
         NULL != clip;
         clip = iter.next()) {

        if (SkRegion::kReplace_Op == clip->fOp) {
            
            
            useSW = false;
        }

        
        
        if (NULL != clip->fPath &&
            path_needs_SW_renderer(this->getContext(), fGpu,
                                   *clip->fPath,
                                   get_path_fill(*clip->fPath),
                                   clip->fDoAA)) {
            useSW = true;
        }
    }

    return useSW;
}




bool GrClipMaskManager::setupClipping(const GrClipData* clipDataIn) {
    fCurrClipMaskType = kNone_ClipMaskType;

    GrDrawState* drawState = fGpu->drawState();
    if (!drawState->isClipState() || clipDataIn->fClipStack->isWideOpen()) {
        fGpu->disableScissor();
        this->setGpuStencil();
        return true;
    }

    GrRenderTarget* rt = drawState->getRenderTarget();
    
    GrAssert(NULL != rt);

    GrIRect devClipBounds;
    bool isIntersectionOfRects = false;

    clipDataIn->getConservativeBounds(rt, &devClipBounds,
                                      &isIntersectionOfRects);
    if (devClipBounds.isEmpty()) {
        return false;
    }

#if GR_SW_CLIP
    bool requiresAA = requires_AA(*clipDataIn->fClipStack);

    
    
    
    
    if (0 == rt->numSamples() &&
        requiresAA &&
        this->useSWOnlyPath(*clipDataIn->fClipStack)) {
        
        
        GrTexture* result = NULL;
        GrIRect devBound;
        if (this->createSoftwareClipMask(*clipDataIn, &result, &devBound)) {
            setup_drawstate_aaclip(fGpu, result, devBound);
            fGpu->disableScissor();
            this->setGpuStencil();
            return true;
        }

        
        
    }
#endif 

#if GR_AA_CLIP
    
    
    if (0 == rt->numSamples() && requiresAA) {
        
        
        
        
        GrTexture* result = NULL;
        GrIRect devBound;
        if (this->createAlphaClipMask(*clipDataIn, &result, &devBound)) {
            setup_drawstate_aaclip(fGpu, result, devBound);
            fGpu->disableScissor();
            this->setGpuStencil();
            return true;
        }

        
        
    }
#endif 

    
    
    
    
    
    
    
    fAACache.reset();

    
    
    if (isIntersectionOfRects) {
        fGpu->enableScissor(devClipBounds);
        this->setGpuStencil();
        return true;
    }

    
    bool useStencil = !clipDataIn->fClipStack->isWideOpen() &&
                      !devClipBounds.isEmpty();

    if (useStencil) {
        this->createStencilClipMask(*clipDataIn, devClipBounds);
    }
    
    
    
    
    fGpu->enableScissor(devClipBounds);
    this->setGpuStencil();
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






bool contains(const SkRect& canvContainer,
              const SkIRect& devContainee,
              const SkIPoint& origin) {
    return  !devContainee.isEmpty() && !canvContainer.isEmpty() &&
            canvContainer.fLeft <= SkIntToScalar(devContainee.fLeft+origin.fX) &&
            canvContainer.fTop <= SkIntToScalar(devContainee.fTop+origin.fY) &&
            canvContainer.fRight >= SkIntToScalar(devContainee.fRight+origin.fX) &&
            canvContainer.fBottom >= SkIntToScalar(devContainee.fBottom+origin.fY);
}





const SkClipStack::Iter::Clip* process_initial_clip_elements(
                                  SkClipStack::Iter* iter,
                                  const GrIRect& devBounds,
                                  bool* clearToInside,
                                  SkRegion::Op* firstOp,
                                  const GrClipData& clipData) {

    GrAssert(NULL != iter && NULL != clearToInside && NULL != firstOp);

    
    
    
    
    
    bool done = false;
    *clearToInside = true;

    const SkClipStack::Iter::Clip* clip = NULL;

    for (clip = iter->skipToTopmost(SkRegion::kReplace_Op);
         NULL != clip && !done;
         clip = iter->next()) {
        switch (clip->fOp) {
            case SkRegion::kReplace_Op:
                
                *firstOp = SkRegion::kReplace_Op;
                *clearToInside = false;
                done = true;
                break;
            case SkRegion::kIntersect_Op:
                
                
                if (NULL != clip->fRect &&
                    contains(*clip->fRect, devBounds, clipData.fOrigin)) {
                    break;
                }
                
                
                
                if (*clearToInside) {
                    *firstOp = SkRegion::kReplace_Op;
                    *clearToInside = false;
                    done = true;
                }
                break;
                
            case SkRegion::kUnion_Op:
                
                
                
                if (!*clearToInside) {
                    *firstOp = SkRegion::kReplace_Op;
                    done = true;
                }
                break;
            case SkRegion::kXOR_Op:
                
                
                if (*clearToInside) {
                    *firstOp = SkRegion::kDifference_Op;
                } else {
                    *firstOp = SkRegion::kReplace_Op;
                }
                done = true;
                break;
            case SkRegion::kDifference_Op:
                
                
                
                if (*clearToInside) {
                    *firstOp = SkRegion::kDifference_Op;
                    done = true;
                }
                break;
            case SkRegion::kReverseDifference_Op:
                
                
                if (*clearToInside) {
                    *clearToInside = false;
                } else {
                    *firstOp = SkRegion::kReplace_Op;
                    done = true;
                }
                break;
            default:
                GrCrash("Unknown set op.");
        }

        if (done) {
            
            
            break;
        }
    }
    return clip;
}

}

namespace {




void setup_boolean_blendcoeffs(GrDrawState* drawState, SkRegion::Op op) {

    switch (op) {
        case SkRegion::kReplace_Op:
            drawState->setBlendFunc(kOne_GrBlendCoeff, kZero_GrBlendCoeff);
            break;
        case SkRegion::kIntersect_Op:
            drawState->setBlendFunc(kDC_GrBlendCoeff, kZero_GrBlendCoeff);
            break;
        case SkRegion::kUnion_Op:
            drawState->setBlendFunc(kOne_GrBlendCoeff, kISC_GrBlendCoeff);
            break;
        case SkRegion::kXOR_Op:
            drawState->setBlendFunc(kIDC_GrBlendCoeff, kISC_GrBlendCoeff);
            break;
        case SkRegion::kDifference_Op:
            drawState->setBlendFunc(kZero_GrBlendCoeff, kISC_GrBlendCoeff);
            break;
        case SkRegion::kReverseDifference_Op:
            drawState->setBlendFunc(kIDC_GrBlendCoeff, kZero_GrBlendCoeff);
            break;
        default:
            GrAssert(false);
            break;
    }
}


bool draw_path_in_software(GrContext* context,
                           GrGpu* gpu,
                           const SkPath& path,
                           GrPathFill fill,
                           bool doAA,
                           const GrIRect& resultBounds) {

    SkAutoTUnref<GrTexture> texture(
                GrSWMaskHelper::DrawPathMaskToTexture(context, path,
                                                      resultBounds, fill,
                                                      doAA, NULL));
    if (NULL == texture) {
        return false;
    }

    
    GrIRect rect = GrIRect::MakeWH(resultBounds.width(), resultBounds.height());

    GrSWMaskHelper::DrawToTargetWithPathMask(texture, gpu, rect);

    GrAssert(!GrIsFillInverted(fill));
    return true;
}



bool draw_path(GrContext* context,
               GrGpu* gpu,
               const SkPath& path,
               GrPathFill fill,
               bool doAA,
               const GrIRect& resultBounds) {

    GrPathRenderer* pr = context->getPathRenderer(path, fill, gpu, doAA, false);
    if (NULL == pr) {
        return draw_path_in_software(context, gpu, path, fill, doAA, resultBounds);
    }

    pr->drawPath(path, fill, NULL, gpu, doAA);
    return true;
}


void device_to_canvas(SkRect* rect, const SkIPoint& origin) {
    GrAssert(NULL != rect);

    rect->fLeft   += SkIntToScalar(origin.fX);
    rect->fTop    += SkIntToScalar(origin.fY);
    rect->fRight  += SkIntToScalar(origin.fX);
    rect->fBottom += SkIntToScalar(origin.fY);
}

}


bool GrClipMaskManager::drawClipShape(GrTexture* target,
                                      const SkClipStack::Iter::Clip* clip,
                                      const GrIRect& resultBounds) {
    GrDrawState* drawState = fGpu->drawState();
    GrAssert(NULL != drawState);

    drawState->setRenderTarget(target->asRenderTarget());

    if (NULL != clip->fRect) {
        if (clip->fDoAA) {
            getContext()->getAARectRenderer()->fillAARect(fGpu, fGpu,
                                                          *clip->fRect,
                                                          true);
        } else {
            fGpu->drawSimpleRect(*clip->fRect, NULL);
        }
    } else if (NULL != clip->fPath) {
        return draw_path(this->getContext(), fGpu,
                         *clip->fPath,
                         get_path_fill(*clip->fPath),
                         clip->fDoAA,
                         resultBounds);
    }
    return true;
}

void GrClipMaskManager::drawTexture(GrTexture* target,
                                    GrTexture* texture) {
    GrDrawState* drawState = fGpu->drawState();
    GrAssert(NULL != drawState);

    
    drawState->setRenderTarget(target->asRenderTarget());

    GrMatrix sampleM;
    sampleM.setIDiv(texture->width(), texture->height());

    drawState->sampler(0)->reset(sampleM);
    drawState->createTextureEffect(0, texture);

    GrRect rect = GrRect::MakeWH(SkIntToScalar(target->width()),
                                 SkIntToScalar(target->height()));

    fGpu->drawSimpleRect(rect, NULL);

    drawState->disableStage(0);
}



void GrClipMaskManager::getTemp(const GrIRect& bounds,
                                GrAutoScratchTexture* temp) {
    if (NULL != temp->texture()) {
        
        return;
    }

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit;
    desc.fWidth = bounds.width();
    desc.fHeight = bounds.height();
    desc.fConfig = kAlpha_8_GrPixelConfig;

    temp->set(this->getContext(), desc);
}


void GrClipMaskManager::setupCache(const SkClipStack& clipIn,
                                   const GrIRect& bounds) {
    
    
    fAACache.reset();

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit;
    desc.fWidth = bounds.width();
    desc.fHeight = bounds.height();
    desc.fConfig = kAlpha_8_GrPixelConfig;

    fAACache.acquireMask(clipIn, desc, bounds);
}






bool GrClipMaskManager::clipMaskPreamble(const GrClipData& clipDataIn,
                                         GrTexture** result,
                                         GrIRect* devResultBounds) {
    GrDrawState* origDrawState = fGpu->drawState();
    GrAssert(origDrawState->isClipState());

    GrRenderTarget* rt = origDrawState->getRenderTarget();
    GrAssert(NULL != rt);

    
    
    
    clipDataIn.getConservativeBounds(rt, devResultBounds);

    
    
    devResultBounds->outset(1, 1);

    

    if (fAACache.canReuse(*clipDataIn.fClipStack,
                          devResultBounds->width(),
                          devResultBounds->height())) {
        *result = fAACache.getLastMask();
        fAACache.getLastBound(devResultBounds);
        return true;
    }

    this->setupCache(*clipDataIn.fClipStack, *devResultBounds);
    return false;
}



bool GrClipMaskManager::createAlphaClipMask(const GrClipData& clipDataIn,
                                            GrTexture** result,
                                            GrIRect *devResultBounds) {
    GrAssert(NULL != devResultBounds);
    GrAssert(kNone_ClipMaskType == fCurrClipMaskType);

    if (this->clipMaskPreamble(clipDataIn, result, devResultBounds)) {
        fCurrClipMaskType = kAlpha_ClipMaskType;
        return true;
    }

    

    GrTexture* accum = fAACache.getLastMask();
    if (NULL == accum) {
        fAACache.reset();
        return false;
    }

    GrDrawTarget::AutoStateRestore asr(fGpu, GrDrawTarget::kReset_ASRInit);
    GrDrawState* drawState = fGpu->drawState();

    GrDrawTarget::AutoGeometryPush agp(fGpu);

    if (0 != devResultBounds->fTop || 0 != devResultBounds->fLeft ||
        0 != clipDataIn.fOrigin.fX || 0 != clipDataIn.fOrigin.fY) {
        
        
        drawState->viewMatrix()->setTranslate(
                SkIntToScalar(-devResultBounds->fLeft-clipDataIn.fOrigin.fX),
                SkIntToScalar(-devResultBounds->fTop-clipDataIn.fOrigin.fY));
    }

    bool clearToInside;
    SkRegion::Op firstOp = SkRegion::kReplace_Op; 

    SkClipStack::Iter iter(*clipDataIn.fClipStack,
                           SkClipStack::Iter::kBottom_IterStart);
    const SkClipStack::Iter::Clip* clip = process_initial_clip_elements(&iter,
                                                              *devResultBounds,
                                                              &clearToInside,
                                                              &firstOp,
                                                              clipDataIn);

    fGpu->clear(NULL,
                clearToInside ? 0xffffffff : 0x00000000,
                accum->asRenderTarget());

    GrAutoScratchTexture temp;
    bool first = true;
    
    for ( ; NULL != clip; clip = iter.next()) {

        SkRegion::Op op = clip->fOp;
        if (first) {
            first = false;
            op = firstOp;
        }

        if (SkRegion::kReplace_Op == op) {
            
            fGpu->clear(NULL, 0x00000000, accum->asRenderTarget());

            setup_boolean_blendcoeffs(drawState, op);
            this->drawClipShape(accum, clip, *devResultBounds);

        } else if (SkRegion::kReverseDifference_Op == op ||
                   SkRegion::kIntersect_Op == op) {
            
            if (SkRegion::kIntersect_Op == op && NULL != clip->fRect &&
                contains(*clip->fRect, *devResultBounds, clipDataIn.fOrigin)) {
                continue;
            }

            getTemp(*devResultBounds, &temp);
            if (NULL == temp.texture()) {
                fAACache.reset();
                return false;
            }

            
            fGpu->clear(NULL, 0x00000000, temp.texture()->asRenderTarget());

            setup_boolean_blendcoeffs(drawState, SkRegion::kReplace_Op);
            this->drawClipShape(temp.texture(), clip, *devResultBounds);

            
            
            
            if (0 != devResultBounds->fTop || 0 != devResultBounds->fLeft ||
                0 != clipDataIn.fOrigin.fX || 0 != clipDataIn.fOrigin.fY) {
                
                
                drawState->viewMatrix()->reset();
            }

            
            
            setup_boolean_blendcoeffs(drawState, op);
            this->drawTexture(accum, temp.texture());

            if (0 != devResultBounds->fTop || 0 != devResultBounds->fLeft ||
                0 != clipDataIn.fOrigin.fX || 0 != clipDataIn.fOrigin.fY) {
                drawState->viewMatrix()->setTranslate(
                  SkIntToScalar(-devResultBounds->fLeft-clipDataIn.fOrigin.fX),
                  SkIntToScalar(-devResultBounds->fTop-clipDataIn.fOrigin.fY));
            }

        } else {
            
            
            setup_boolean_blendcoeffs(drawState, op);
            this->drawClipShape(accum, clip, *devResultBounds);
        }
    }

    *result = accum;
    fCurrClipMaskType = kAlpha_ClipMaskType;
    return true;
}




bool GrClipMaskManager::createStencilClipMask(const GrClipData& clipDataIn,
                                              const GrIRect& devClipBounds) {

    GrAssert(kNone_ClipMaskType == fCurrClipMaskType);

    GrDrawState* drawState = fGpu->drawState();
    GrAssert(drawState->isClipState());

    GrRenderTarget* rt = drawState->getRenderTarget();
    GrAssert(NULL != rt);

    
    GrStencilBuffer* stencilBuffer = rt->getStencilBuffer();
    if (NULL == stencilBuffer) {
        return false;
    }

    if (stencilBuffer->mustRenderClip(clipDataIn, rt->width(), rt->height())) {

        stencilBuffer->setLastClip(clipDataIn, rt->width(), rt->height());

        
        
        
        
        const GrClipData* oldClipData = fGpu->getClip();

        
        
        SkClipStack newClipStack(devClipBounds);
        GrClipData newClipData;
        newClipData.fClipStack = &newClipStack;

        fGpu->setClip(&newClipData);

        GrDrawTarget::AutoStateRestore asr(fGpu, GrDrawTarget::kReset_ASRInit);
        drawState = fGpu->drawState();
        drawState->setRenderTarget(rt);
        GrDrawTarget::AutoGeometryPush agp(fGpu);

        if (0 != clipDataIn.fOrigin.fX || 0 != clipDataIn.fOrigin.fY) {
            
            
            drawState->viewMatrix()->setTranslate(
                           SkIntToScalar(-clipDataIn.fOrigin.fX),
                           SkIntToScalar(-clipDataIn.fOrigin.fY));
        }

#if !VISUALIZE_COMPLEX_CLIP
        drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
#endif

        int clipBit = stencilBuffer->bits();
        SkASSERT((clipBit <= 16) &&
                    "Ganesh only handles 16b or smaller stencil buffers");
        clipBit = (1 << (clipBit-1));

        GrIRect devRTRect = GrIRect::MakeWH(rt->width(), rt->height());

        bool clearToInside;
        SkRegion::Op firstOp = SkRegion::kReplace_Op; 

        SkClipStack::Iter iter(*oldClipData->fClipStack,
                               SkClipStack::Iter::kBottom_IterStart);
        const SkClipStack::Iter::Clip* clip = process_initial_clip_elements(&iter,
                                                  devRTRect,
                                                  &clearToInside,
                                                  &firstOp,
                                                  clipDataIn);

        fGpu->clearStencilClip(devClipBounds, clearToInside);
        bool first = true;

        
        
        for ( ; NULL != clip; clip = iter.next()) {
            GrPathFill fill;
            bool fillInverted = false;
            
            drawState->disableState(GrGpu::kModifyStencilClip_StateBit);
            
            if (rt->isMultisampled()) {
                if (clip->fDoAA) {
                    drawState->enableState(GrDrawState::kHWAntialias_StateBit);
                } else {
                    drawState->disableState(GrDrawState::kHWAntialias_StateBit);
                }
            }

            
            
            
            bool canRenderDirectToStencil = false;

            SkRegion::Op op = clip->fOp;
            if (first) {
                first = false;
                op = firstOp;
            }

            GrPathRenderer* pr = NULL;
            const SkPath* clipPath = NULL;
            if (NULL != clip->fRect) {
                canRenderDirectToStencil = true;
                fill = kEvenOdd_GrPathFill;
                fillInverted = false;
                
                
                if (SkRegion::kIntersect_Op == op &&
                    contains(*clip->fRect, devRTRect, oldClipData->fOrigin)) {
                    continue;
                }
            } else {
                GrAssert(NULL != clip->fPath);
                fill = get_path_fill(*clip->fPath);
                fillInverted = GrIsFillInverted(fill);
                fill = GrNonInvertedFill(fill);
                clipPath = clip->fPath;
                pr = this->getContext()->getPathRenderer(*clipPath, fill, fGpu, false, true);
                if (NULL == pr) {
                    fGpu->setClip(oldClipData);
                    return false;
                }
                canRenderDirectToStencil =
                    !pr->requiresStencilPass(*clipPath, fill, fGpu);
            }

            int passes;
            GrStencilSettings stencilSettings[GrStencilSettings::kMaxStencilClipPasses];

            bool canDrawDirectToClip; 
                                        
                                        
                                        
            canDrawDirectToClip =
                GrStencilSettings::GetClipPasses(op,
                                                 canRenderDirectToStencil,
                                                 clipBit,
                                                 fillInverted,
                                                 &passes,
                                                 stencilSettings);

            
            if (!canDrawDirectToClip) {
                GR_STATIC_CONST_SAME_STENCIL(gDrawToStencil,
                    kIncClamp_StencilOp,
                    kIncClamp_StencilOp,
                    kAlways_StencilFunc,
                    0xffff,
                    0x0000,
                    0xffff);
                SET_RANDOM_COLOR
                if (NULL != clip->fRect) {
                    *drawState->stencil() = gDrawToStencil;
                    fGpu->drawSimpleRect(*clip->fRect, NULL);
                } else {
                    if (canRenderDirectToStencil) {
                        *drawState->stencil() = gDrawToStencil;
                        pr->drawPath(*clipPath, fill, NULL, fGpu, false);
                    } else {
                        pr->drawPathToStencil(*clipPath, fill, fGpu);
                    }
                }
            }

            
            
            drawState->enableState(GrGpu::kModifyStencilClip_StateBit);
            for (int p = 0; p < passes; ++p) {
                *drawState->stencil() = stencilSettings[p];
                if (canDrawDirectToClip) {
                    if (NULL != clip->fRect) {
                        SET_RANDOM_COLOR
                        fGpu->drawSimpleRect(*clip->fRect, NULL);
                    } else {
                        SET_RANDOM_COLOR
                        pr->drawPath(*clipPath, fill, NULL, fGpu, false);
                    }
                } else {
                    SET_RANDOM_COLOR
                    
                    
                    
                    
                    GrRect canvClipBounds;
                    canvClipBounds.set(devClipBounds);
                    device_to_canvas(&canvClipBounds, clipDataIn.fOrigin);
                    fGpu->drawSimpleRect(canvClipBounds, NULL);
                }
            }
        }
        
        fGpu->setClip(oldClipData);
    }
    
    GrAssert(kNone_ClipMaskType == fCurrClipMaskType);
    fCurrClipMaskType = kStencil_ClipMaskType;
    return true;
}




static const GrStencilFunc
    gSpecialToBasicStencilFunc[2][kClipStencilFuncCount] = {
    {
        
        kAlways_StencilFunc,          
        kEqual_StencilFunc,           
        kLess_StencilFunc,            
        kLEqual_StencilFunc,          
        
        kNotEqual_StencilFunc,        
                                      
    },
    {
        
        kEqual_StencilFunc,           
                                      
                                      

        kEqual_StencilFunc,           
                                      

        kLess_StencilFunc,            
        kLEqual_StencilFunc,          
                                      
                                      
                                      
        
        kLess_StencilFunc,            
                                      
                                      
                                      
    }
};

namespace {


const GrStencilSettings& basic_apply_stencil_clip_settings() {
    
    GR_STATIC_CONST_SAME_STENCIL_STRUCT(gSettings,
        kKeep_StencilOp,
        kKeep_StencilOp,
        kAlwaysIfInClip_StencilFunc,
        0x0000,
        0x0000,
        0x0000);
    return *GR_CONST_STENCIL_SETTINGS_PTR_FROM_STRUCT_PTR(&gSettings);
}
}

void GrClipMaskManager::setGpuStencil() {
    
    
    
    

    const GrDrawState& drawState = fGpu->getDrawState();

    
    
    GrClipMaskManager::StencilClipMode clipMode;
    if (this->isClipInStencil() && drawState.isClipState()) {
        clipMode = GrClipMaskManager::kRespectClip_StencilClipMode;
        
        GrAssert(!drawState.isStateFlagEnabled(
                    GrGpu::kModifyStencilClip_StateBit));
    } else if (drawState.isStateFlagEnabled(
                    GrGpu::kModifyStencilClip_StateBit)) {
        clipMode = GrClipMaskManager::kModifyClip_StencilClipMode;
    } else {
        clipMode = GrClipMaskManager::kIgnoreClip_StencilClipMode;
    }

    GrStencilSettings settings;
    
    
    if (drawState.getStencil().isDisabled()) {
        if (GrClipMaskManager::kRespectClip_StencilClipMode == clipMode) {
            settings = basic_apply_stencil_clip_settings();
        } else {
            fGpu->disableStencil();
            return;
        }
    } else {
        settings = drawState.getStencil();
    }

    
    int stencilBits = 0;
    GrStencilBuffer* stencilBuffer =
        drawState.getRenderTarget()->getStencilBuffer();
    if (NULL != stencilBuffer) {
        stencilBits = stencilBuffer->bits();
    }

    GrAssert(fGpu->getCaps().stencilWrapOpsSupport() ||
             !settings.usesWrapOp());
    GrAssert(fGpu->getCaps().twoSidedStencilSupport() || !settings.isTwoSided());
    this->adjustStencilParams(&settings, clipMode, stencilBits);
    fGpu->setStencilSettings(settings);
}

void GrClipMaskManager::adjustStencilParams(GrStencilSettings* settings,
                                            StencilClipMode mode,
                                            int stencilBitCnt) {
    GrAssert(stencilBitCnt > 0);

    if (kModifyClip_StencilClipMode == mode) {
        
        
        return;
    }

    unsigned int clipBit = (1 << (stencilBitCnt - 1));
    unsigned int userBits = clipBit - 1;

    GrStencilSettings::Face face = GrStencilSettings::kFront_Face;
    bool twoSided = fGpu->getCaps().twoSidedStencilSupport();

    bool finished = false;
    while (!finished) {
        GrStencilFunc func = settings->func(face);
        uint16_t writeMask = settings->writeMask(face);
        uint16_t funcMask = settings->funcMask(face);
        uint16_t funcRef = settings->funcRef(face);

        GrAssert((unsigned) func < kStencilFuncCount);

        writeMask &= userBits;

        if (func >= kBasicStencilFuncCount) {
            int respectClip = kRespectClip_StencilClipMode == mode;
            if (respectClip) {
                
                GrAssert(this->isClipInStencil());
                switch (func) {
                    case kAlwaysIfInClip_StencilFunc:
                        funcMask = clipBit;
                        funcRef = clipBit;
                        break;
                    case kEqualIfInClip_StencilFunc:
                    case kLessIfInClip_StencilFunc:
                    case kLEqualIfInClip_StencilFunc:
                        funcMask = (funcMask & userBits) | clipBit;
                        funcRef  = (funcRef  & userBits) | clipBit;
                        break;
                    case kNonZeroIfInClip_StencilFunc:
                        funcMask = (funcMask & userBits) | clipBit;
                        funcRef = clipBit;
                        break;
                    default:
                        GrCrash("Unknown stencil func");
                }
            } else {
                funcMask &= userBits;
                funcRef &= userBits;
            }
            const GrStencilFunc* table =
                gSpecialToBasicStencilFunc[respectClip];
            func = table[func - kBasicStencilFuncCount];
            GrAssert(func >= 0 && func < kBasicStencilFuncCount);
        } else {
            funcMask &= userBits;
            funcRef &= userBits;
        }

        settings->setFunc(face, func);
        settings->setWriteMask(face, writeMask);
        settings->setFuncMask(face, funcMask);
        settings->setFuncRef(face, funcRef);

        if (GrStencilSettings::kFront_Face == face) {
            face = GrStencilSettings::kBack_Face;
            finished = !twoSided;
        } else {
            finished = true;
        }
    }
    if (!twoSided) {
        settings->copyFrontSettingsToBack();
    }
}



namespace {

GrPathFill invert_fill(GrPathFill fill) {
    static const GrPathFill gInvertedFillTable[] = {
        kInverseWinding_GrPathFill, 
        kInverseEvenOdd_GrPathFill, 
        kWinding_GrPathFill,        
        kEvenOdd_GrPathFill,        
        kHairLine_GrPathFill,       
    };
    GR_STATIC_ASSERT(0 == kWinding_GrPathFill);
    GR_STATIC_ASSERT(1 == kEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(2 == kInverseWinding_GrPathFill);
    GR_STATIC_ASSERT(3 == kInverseEvenOdd_GrPathFill);
    GR_STATIC_ASSERT(4 == kHairLine_GrPathFill);
    GR_STATIC_ASSERT(5 == kGrPathFillCount);
    return gInvertedFillTable[fill];
}

}

bool GrClipMaskManager::createSoftwareClipMask(const GrClipData& clipDataIn,
                                               GrTexture** result,
                                               GrIRect* devResultBounds) {
    GrAssert(kNone_ClipMaskType == fCurrClipMaskType);

    if (this->clipMaskPreamble(clipDataIn, result, devResultBounds)) {
        return true;
    }

    GrTexture* accum = fAACache.getLastMask();
    if (NULL == accum) {
        fAACache.reset();
        return false;
    }

    GrSWMaskHelper helper(this->getContext());

    GrMatrix matrix;
    matrix.setTranslate(SkIntToScalar(-clipDataIn.fOrigin.fX),
                        SkIntToScalar(-clipDataIn.fOrigin.fY));
    helper.init(*devResultBounds, &matrix);

    bool clearToInside;
    SkRegion::Op firstOp = SkRegion::kReplace_Op; 

    SkClipStack::Iter iter(*clipDataIn.fClipStack,
                           SkClipStack::Iter::kBottom_IterStart);
    const SkClipStack::Iter::Clip* clip = process_initial_clip_elements(&iter,
                                              *devResultBounds,
                                              &clearToInside,
                                              &firstOp,
                                              clipDataIn);

    helper.clear(clearToInside ? 0xFF : 0x00);

    bool first = true;
    for ( ; NULL != clip; clip = iter.next()) {

        SkRegion::Op op = clip->fOp;
        if (first) {
            first = false;
            op = firstOp;
        }

        if (SkRegion::kIntersect_Op == op ||
            SkRegion::kReverseDifference_Op == op) {
            
            
            
            
            
            
            if (SkRegion::kReverseDifference_Op == op) {
                SkRect temp;
                temp.set(*devResultBounds);

                
                helper.draw(temp, SkRegion::kXOR_Op, false, 0xFF);
            }

            if (NULL != clip->fRect) {

                
                SkPath temp;
                temp.addRect(*clip->fRect);

                helper.draw(temp, SkRegion::kReplace_Op,
                            kInverseEvenOdd_GrPathFill, clip->fDoAA,
                            0x00);
            } else if (NULL != clip->fPath) {
                helper.draw(*clip->fPath,
                            SkRegion::kReplace_Op,
                            invert_fill(get_path_fill(*clip->fPath)),
                            clip->fDoAA,
                            0x00);
            }

            continue;
        }

        
        
        if (NULL != clip->fRect) {

            helper.draw(*clip->fRect,
                        op,
                        clip->fDoAA, 0xFF);

        } else if (NULL != clip->fPath) {
            helper.draw(*clip->fPath,
                        op,
                        get_path_fill(*clip->fPath),
                        clip->fDoAA, 0xFF);
        }
    }

    
    
    

    
    
    GrDrawState* drawState = fGpu->drawState();
    GrAssert(NULL != drawState);
    GrRenderTarget* temp = drawState->getRenderTarget();
    fGpu->clear(NULL, 0x00000000, accum->asRenderTarget());
    
    drawState->setRenderTarget(temp);

    helper.toTexture(accum, clearToInside ? 0xFF : 0x00);

    *result = accum;

    fCurrClipMaskType = kAlpha_ClipMaskType;
    return true;
}


void GrClipMaskManager::releaseResources() {
    fAACache.releaseResources();
}
