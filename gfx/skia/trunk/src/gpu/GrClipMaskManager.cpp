







#include "GrClipMaskManager.h"
#include "GrAAConvexPathRenderer.h"
#include "GrAAHairLinePathRenderer.h"
#include "GrAARectRenderer.h"
#include "GrDrawTargetCaps.h"
#include "GrGpu.h"
#include "GrPaint.h"
#include "GrPathRenderer.h"
#include "GrRenderTarget.h"
#include "GrStencilBuffer.h"
#include "GrSWMaskHelper.h"
#include "effects/GrTextureDomain.h"
#include "effects/GrConvexPolyEffect.h"
#include "SkRasterClip.h"
#include "SkStrokeRec.h"
#include "SkTLazy.h"

#define GR_AA_CLIP 1

typedef SkClipStack::Element Element;

using namespace GrReducedClip;


namespace {


void setup_drawstate_aaclip(GrGpu* gpu,
                            GrTexture* result,
                            const SkIRect &devBound) {
    GrDrawState* drawState = gpu->drawState();
    SkASSERT(drawState);

    SkMatrix mat;
    
    
    
    mat.setIDiv(result->width(), result->height());
    mat.preTranslate(SkIntToScalar(-devBound.fLeft),
                     SkIntToScalar(-devBound.fTop));
    mat.preConcat(drawState->getViewMatrix());

    SkIRect domainTexels = SkIRect::MakeWH(devBound.width(), devBound.height());
    
    drawState->addCoverageEffect(
        GrTextureDomainEffect::Create(result,
                                      mat,
                                      GrTextureDomain::MakeTexelDomain(result, domainTexels),
                                      GrTextureDomain::kDecal_Mode,
                                      GrTextureParams::kNone_FilterMode,
                                      kPosition_GrCoordSet))->unref();
}

bool path_needs_SW_renderer(GrContext* context,
                            GrGpu* gpu,
                            const SkPath& origPath,
                            const SkStrokeRec& stroke,
                            bool doAA) {
    
    SkTCopyOnFirstWrite<SkPath> path(origPath);
    if (path->isInverseFillType()) {
        path.writable()->toggleInverseFillType();
    }
    
    GrPathRendererChain::DrawType type = doAA ?
                                         GrPathRendererChain::kColorAntiAlias_DrawType :
                                         GrPathRendererChain::kColor_DrawType;

    return NULL == context->getPathRenderer(*path, stroke, gpu, false, type);
}

}






bool GrClipMaskManager::useSWOnlyPath(const ElementList& elements) {

    
    
    
    SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);

    for (ElementList::Iter iter(elements.headIter()); iter.get(); iter.next()) {
        const Element* element = iter.get();
        
        
        if (Element::kPath_Type == element->getType() &&
            path_needs_SW_renderer(this->getContext(), fGpu,
                                   element->getPath(),
                                   stroke,
                                   element->isAA())) {
            return true;
        }
    }
    return false;
}




bool GrClipMaskManager::setupClipping(const GrClipData* clipDataIn,
                                      GrDrawState::AutoRestoreEffects* are,
                                      const SkRect* devBounds) {
    fCurrClipMaskType = kNone_ClipMaskType;

    ElementList elements(16);
    int32_t genID;
    InitialState initialState;
    SkIRect clipSpaceIBounds;
    bool requiresAA;
    bool isRect = false;

    GrDrawState* drawState = fGpu->drawState();

    const GrRenderTarget* rt = drawState->getRenderTarget();
    
    SkASSERT(NULL != rt);

    bool ignoreClip = !drawState->isClipState() || clipDataIn->fClipStack->isWideOpen();

    if (!ignoreClip) {
        SkIRect clipSpaceRTIBounds = SkIRect::MakeWH(rt->width(), rt->height());
        clipSpaceRTIBounds.offset(clipDataIn->fOrigin);
        ReduceClipStack(*clipDataIn->fClipStack,
                        clipSpaceRTIBounds,
                        &elements,
                        &genID,
                        &initialState,
                        &clipSpaceIBounds,
                        &requiresAA);
        if (elements.isEmpty()) {
            if (kAllIn_InitialState == initialState) {
                ignoreClip = clipSpaceIBounds == clipSpaceRTIBounds;
                isRect = true;
            } else {
                return false;
            }
        }
    }

    if (ignoreClip) {
        fGpu->disableScissor();
        this->setGpuStencil();
        return true;
    }

    
    
    
    if (1 == elements.count() && SkRegion::kReplace_Op == elements.tail()->getOp()) {
        if (NULL != devBounds) {
            SkRect boundsInClipSpace = *devBounds;
            boundsInClipSpace.offset(SkIntToScalar(clipDataIn->fOrigin.fX),
                                     SkIntToScalar(clipDataIn->fOrigin.fY));
            if (elements.tail()->contains(boundsInClipSpace)) {
                fGpu->disableScissor();
                this->setGpuStencil();
                return true;
            }
        }
        SkAutoTUnref<GrEffectRef> effect;
        if (SkClipStack::Element::kPath_Type == elements.tail()->getType()) {
            const SkPath& path = elements.tail()->getPath();
            bool isAA = GR_AA_CLIP && elements.tail()->isAA();
            if (rt->isMultisampled()) {
                
                if (!isAA) {
                    SkVector offset = { SkIntToScalar(-clipDataIn->fOrigin.fX),
                                        SkIntToScalar(-clipDataIn->fOrigin.fY) };
                    effect.reset(GrConvexPolyEffect::Create(GrConvexPolyEffect::kFillNoAA_EdgeType,
                                                            path, &offset));
                }
            } else {
                SkVector offset = { SkIntToScalar(-clipDataIn->fOrigin.fX),
                                    SkIntToScalar(-clipDataIn->fOrigin.fY) };
                GrConvexPolyEffect::EdgeType type = isAA ? GrConvexPolyEffect::kFillAA_EdgeType :
                                                           GrConvexPolyEffect::kFillNoAA_EdgeType;
                effect.reset(GrConvexPolyEffect::Create(type, path, &offset));
            }
        } else if (GR_AA_CLIP && elements.tail()->isAA() && !rt->isMultisampled()) {
            
            
            SkASSERT(SkClipStack::Element::kRect_Type == elements.tail()->getType());
            SkRect rect = elements.tail()->getRect();
            SkVector offset = { SkIntToScalar(-clipDataIn->fOrigin.fX),
                SkIntToScalar(-clipDataIn->fOrigin.fY) };
            rect.offset(offset);
            effect.reset(GrConvexPolyEffect::CreateForAAFillRect(rect));
            
            SkASSERT(effect);
        }
        if (effect) {
            are->set(fGpu->drawState());
            fGpu->drawState()->addCoverageEffect(effect);
            SkIRect scissorSpaceIBounds(clipSpaceIBounds);
            scissorSpaceIBounds.offset(-clipDataIn->fOrigin);
            fGpu->enableScissor(scissorSpaceIBounds);
            this->setGpuStencil();
            return true;
        }
    }

#if GR_AA_CLIP
    
    if (0 == rt->numSamples() && requiresAA) {
        GrTexture* result = NULL;

        if (this->useSWOnlyPath(elements)) {
            
            
            result = this->createSoftwareClipMask(genID,
                                                  initialState,
                                                  elements,
                                                  clipSpaceIBounds);
        } else {
            result = this->createAlphaClipMask(genID,
                                               initialState,
                                               elements,
                                               clipSpaceIBounds);
        }

        if (NULL != result) {
            
            
            SkIRect rtSpaceMaskBounds = clipSpaceIBounds;
            rtSpaceMaskBounds.offset(-clipDataIn->fOrigin);
            are->set(fGpu->drawState());
            setup_drawstate_aaclip(fGpu, result, rtSpaceMaskBounds);
            fGpu->disableScissor();
            this->setGpuStencil();
            return true;
        }
        
    }
#endif 

    
    
    
    
    
    fAACache.reset();

    
    
    if (isRect) {
        SkIRect clipRect = clipSpaceIBounds;
        clipRect.offset(-clipDataIn->fOrigin);
        fGpu->enableScissor(clipRect);
        this->setGpuStencil();
        return true;
    }

    
    SkIPoint clipSpaceToStencilSpaceOffset = -clipDataIn->fOrigin;
    this->createStencilClipMask(genID,
                                initialState,
                                elements,
                                clipSpaceIBounds,
                                clipSpaceToStencilSpaceOffset);

    
    
    
    SkIRect scissorSpaceIBounds(clipSpaceIBounds);
    scissorSpaceIBounds.offset(clipSpaceToStencilSpaceOffset);
    fGpu->enableScissor(scissorSpaceIBounds);
    this->setGpuStencil();
    return true;
}

#define VISUALIZE_COMPLEX_CLIP 0

#if VISUALIZE_COMPLEX_CLIP
    #include "SkRandom.h"
    SkRandom gRandom;
    #define SET_RANDOM_COLOR drawState->setColor(0xff000000 | gRandom.nextU());
#else
    #define SET_RANDOM_COLOR
#endif

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
            SkASSERT(false);
            break;
    }
}

}


bool GrClipMaskManager::drawElement(GrTexture* target,
                                    const SkClipStack::Element* element,
                                    GrPathRenderer* pr) {
    GrDrawState* drawState = fGpu->drawState();

    drawState->setRenderTarget(target->asRenderTarget());

    switch (element->getType()) {
        case Element::kRect_Type:
            
            
            if (element->isAA()) {
                getContext()->getAARectRenderer()->fillAARect(fGpu,
                                                              fGpu,
                                                              element->getRect(),
                                                              SkMatrix::I(),
                                                              element->getRect(),
                                                              false);
            } else {
                fGpu->drawSimpleRect(element->getRect(), NULL);
            }
            return true;
        case Element::kPath_Type: {
            SkTCopyOnFirstWrite<SkPath> path(element->getPath());
            if (path->isInverseFillType()) {
                path.writable()->toggleInverseFillType();
            }
            SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);
            if (NULL == pr) {
                GrPathRendererChain::DrawType type;
                type = element->isAA() ? GrPathRendererChain::kColorAntiAlias_DrawType :
                                         GrPathRendererChain::kColor_DrawType;
                pr = this->getContext()->getPathRenderer(*path, stroke, fGpu, false, type);
            }
            if (NULL == pr) {
                return false;
            }
            pr->drawPath(element->getPath(), stroke, fGpu, element->isAA());
            break;
        }
        default:
            
            GrCrash("Unexpected element type");
            return false;
    }
    return true;
}

bool GrClipMaskManager::canStencilAndDrawElement(GrTexture* target,
                                                 const SkClipStack::Element* element,
                                                 GrPathRenderer** pr) {
    GrDrawState* drawState = fGpu->drawState();
    drawState->setRenderTarget(target->asRenderTarget());

    switch (element->getType()) {
        case Element::kRect_Type:
            return true;
        case Element::kPath_Type: {
            SkTCopyOnFirstWrite<SkPath> path(element->getPath());
            if (path->isInverseFillType()) {
                path.writable()->toggleInverseFillType();
            }
            SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);
            GrPathRendererChain::DrawType type = element->isAA() ?
                GrPathRendererChain::kStencilAndColorAntiAlias_DrawType :
                GrPathRendererChain::kStencilAndColor_DrawType;
            *pr = this->getContext()->getPathRenderer(*path, stroke, fGpu, false, type);
            return NULL != *pr;
        }
        default:
            
            GrCrash("Unexpected element type");
            return false;
    }
}

void GrClipMaskManager::mergeMask(GrTexture* dstMask,
                                  GrTexture* srcMask,
                                  SkRegion::Op op,
                                  const SkIRect& dstBound,
                                  const SkIRect& srcBound) {
    GrDrawState::AutoViewMatrixRestore avmr;
    GrDrawState* drawState = fGpu->drawState();
    SkAssertResult(avmr.setIdentity(drawState));
    GrDrawState::AutoRestoreEffects are(drawState);

    drawState->setRenderTarget(dstMask->asRenderTarget());

    setup_boolean_blendcoeffs(drawState, op);

    SkMatrix sampleM;
    sampleM.setIDiv(srcMask->width(), srcMask->height());

    drawState->addColorEffect(
        GrTextureDomainEffect::Create(srcMask,
                                      sampleM,
                                      GrTextureDomain::MakeTexelDomain(srcMask, srcBound),
                                      GrTextureDomain::kDecal_Mode,
                                      GrTextureParams::kNone_FilterMode))->unref();
    fGpu->drawSimpleRect(SkRect::Make(dstBound), NULL);
}



void GrClipMaskManager::getTemp(int width, int height, GrAutoScratchTexture* temp) {
    if (NULL != temp->texture()) {
        
        return;
    }

    GrTextureDesc desc;
    desc.fFlags = kRenderTarget_GrTextureFlagBit|kNoStencil_GrTextureFlagBit;
    desc.fWidth = width;
    desc.fHeight = height;
    desc.fConfig = kAlpha_8_GrPixelConfig;

    temp->set(this->getContext(), desc);
}





bool GrClipMaskManager::getMaskTexture(int32_t elementsGenID,
                                       const SkIRect& clipSpaceIBounds,
                                       GrTexture** result,
                                       bool willUpload) {
    bool cached = fAACache.canReuse(elementsGenID, clipSpaceIBounds);
    if (!cached) {

        
        
        
        fAACache.reset();

        GrTextureDesc desc;
        desc.fFlags = willUpload ? kNone_GrTextureFlags : kRenderTarget_GrTextureFlagBit;
        desc.fWidth = clipSpaceIBounds.width();
        desc.fHeight = clipSpaceIBounds.height();
        desc.fConfig = kRGBA_8888_GrPixelConfig;
        if (willUpload || this->getContext()->isConfigRenderable(kAlpha_8_GrPixelConfig, false)) {
            
            desc.fConfig = kAlpha_8_GrPixelConfig;
        }

        fAACache.acquireMask(elementsGenID, desc, clipSpaceIBounds);
    }

    *result = fAACache.getLastMask();
    return cached;
}



GrTexture* GrClipMaskManager::createAlphaClipMask(int32_t elementsGenID,
                                                  InitialState initialState,
                                                  const ElementList& elements,
                                                  const SkIRect& clipSpaceIBounds) {
    SkASSERT(kNone_ClipMaskType == fCurrClipMaskType);

    GrTexture* result;
    if (this->getMaskTexture(elementsGenID, clipSpaceIBounds, &result, false)) {
        fCurrClipMaskType = kAlpha_ClipMaskType;
        return result;
    }

    if (NULL == result) {
        fAACache.reset();
        return NULL;
    }

    
    SkVector clipToMaskOffset = {
        SkIntToScalar(-clipSpaceIBounds.fLeft),
        SkIntToScalar(-clipSpaceIBounds.fTop)
    };
    
    
    SkIRect maskSpaceIBounds = SkIRect::MakeWH(clipSpaceIBounds.width(), clipSpaceIBounds.height());

    
    SkMatrix translate;
    translate.setTranslate(clipToMaskOffset);
    GrDrawTarget::AutoGeometryAndStatePush agasp(fGpu, GrDrawTarget::kReset_ASRInit, &translate);

    GrDrawState* drawState = fGpu->drawState();

    
    drawState->enableState(GrDrawState::kCoverageDrawing_StateBit);

    
    
    fGpu->clear(&maskSpaceIBounds,
                kAllIn_InitialState == initialState ? 0xffffffff : 0x00000000,
                true,
                result->asRenderTarget());

    
    
    
    
    GrDrawTarget::AutoClipRestore acr(fGpu, maskSpaceIBounds);
    drawState->enableState(GrDrawState::kClip_StateBit);

    GrAutoScratchTexture temp;
    
    for (ElementList::Iter iter = elements.headIter(); iter.get(); iter.next()) {
        const Element* element = iter.get();
        SkRegion::Op op = element->getOp();
        bool invert = element->isInverseFilled();

        if (invert || SkRegion::kIntersect_Op == op || SkRegion::kReverseDifference_Op == op) {
            GrPathRenderer* pr = NULL;
            bool useTemp = !this->canStencilAndDrawElement(result, element, &pr);
            GrTexture* dst;
            
            
            
            
            SkIRect maskSpaceElementIBounds;

            if (useTemp) {
                if (invert) {
                    maskSpaceElementIBounds = maskSpaceIBounds;
                } else {
                    SkRect elementBounds = element->getBounds();
                    elementBounds.offset(clipToMaskOffset);
                    elementBounds.roundOut(&maskSpaceElementIBounds);
                }

                this->getTemp(maskSpaceIBounds.fRight, maskSpaceIBounds.fBottom, &temp);
                if (NULL == temp.texture()) {
                    fAACache.reset();
                    return NULL;
                }
                dst = temp.texture();
                
                fGpu->clear(&maskSpaceElementIBounds,
                            invert ? 0xffffffff : 0x00000000,
                            true,
                            dst->asRenderTarget());
                setup_boolean_blendcoeffs(drawState, SkRegion::kReplace_Op);

            } else {
                
                
                dst = result;
                GR_STATIC_CONST_SAME_STENCIL(kStencilInElement,
                                             kReplace_StencilOp,
                                             kReplace_StencilOp,
                                             kAlways_StencilFunc,
                                             0xffff,
                                             0xffff,
                                             0xffff);
                drawState->setStencil(kStencilInElement);
                setup_boolean_blendcoeffs(drawState, op);
            }

            drawState->setAlpha(invert ? 0x00 : 0xff);

            if (!this->drawElement(dst, element, pr)) {
                fAACache.reset();
                return NULL;
            }

            if (useTemp) {
                
                
                this->mergeMask(result,
                                temp.texture(),
                                op,
                                maskSpaceIBounds,
                                maskSpaceElementIBounds);
            } else {
                
                drawState->setAlpha(invert ? 0xff : 0x00);
                GR_STATIC_CONST_SAME_STENCIL(kDrawOutsideElement,
                                             kZero_StencilOp,
                                             kZero_StencilOp,
                                             kEqual_StencilFunc,
                                             0xffff,
                                             0x0000,
                                             0xffff);
                drawState->setStencil(kDrawOutsideElement);
                fGpu->drawSimpleRect(clipSpaceIBounds);
                drawState->disableStencil();
            }
        } else {
            
            drawState->setAlpha(0xff);
            setup_boolean_blendcoeffs(drawState, op);
            this->drawElement(result, element);
        }
    }

    fCurrClipMaskType = kAlpha_ClipMaskType;
    return result;
}




bool GrClipMaskManager::createStencilClipMask(int32_t elementsGenID,
                                              InitialState initialState,
                                              const ElementList& elements,
                                              const SkIRect& clipSpaceIBounds,
                                              const SkIPoint& clipSpaceToStencilOffset) {

    SkASSERT(kNone_ClipMaskType == fCurrClipMaskType);

    GrDrawState* drawState = fGpu->drawState();
    SkASSERT(drawState->isClipState());

    GrRenderTarget* rt = drawState->getRenderTarget();
    SkASSERT(NULL != rt);

    
    GrStencilBuffer* stencilBuffer = rt->getStencilBuffer();
    if (NULL == stencilBuffer) {
        return false;
    }

    if (stencilBuffer->mustRenderClip(elementsGenID, clipSpaceIBounds, clipSpaceToStencilOffset)) {

        stencilBuffer->setLastClip(elementsGenID, clipSpaceIBounds, clipSpaceToStencilOffset);

        
        SkVector translate = {
            SkIntToScalar(clipSpaceToStencilOffset.fX),
            SkIntToScalar(clipSpaceToStencilOffset.fY)
        };
        SkMatrix matrix;
        matrix.setTranslate(translate);
        GrDrawTarget::AutoGeometryAndStatePush agasp(fGpu, GrDrawTarget::kReset_ASRInit, &matrix);
        drawState = fGpu->drawState();

        drawState->setRenderTarget(rt);

        
        SkIRect stencilSpaceIBounds(clipSpaceIBounds);
        stencilSpaceIBounds.offset(clipSpaceToStencilOffset);
        GrDrawTarget::AutoClipRestore acr(fGpu, stencilSpaceIBounds);
        drawState->enableState(GrDrawState::kClip_StateBit);

#if !VISUALIZE_COMPLEX_CLIP
        drawState->enableState(GrDrawState::kNoColorWrites_StateBit);
#endif

        int clipBit = stencilBuffer->bits();
        SkASSERT((clipBit <= 16) && "Ganesh only handles 16b or smaller stencil buffers");
        clipBit = (1 << (clipBit-1));

        fGpu->clearStencilClip(stencilSpaceIBounds, kAllIn_InitialState == initialState);

        
        
        for (ElementList::Iter iter(elements.headIter()); NULL != iter.get(); iter.next()) {
            const Element* element = iter.get();
            bool fillInverted = false;
            
            drawState->disableState(GrGpu::kModifyStencilClip_StateBit);
            
            if (rt->isMultisampled()) {
                drawState->setState(GrDrawState::kHWAntialias_StateBit, element->isAA());
            }

            
            
            GrPathRenderer::StencilSupport stencilSupport;

            SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);

            SkRegion::Op op = element->getOp();

            GrPathRenderer* pr = NULL;
            SkTCopyOnFirstWrite<SkPath> clipPath;
            if (Element::kRect_Type == element->getType()) {
                stencilSupport = GrPathRenderer::kNoRestriction_StencilSupport;
                fillInverted = false;
            } else {
                SkASSERT(Element::kPath_Type == element->getType());
                clipPath.init(element->getPath());
                fillInverted = clipPath->isInverseFillType();
                if (fillInverted) {
                    clipPath.writable()->toggleInverseFillType();
                }
                pr = this->getContext()->getPathRenderer(*clipPath,
                                                         stroke,
                                                         fGpu,
                                                         false,
                                                         GrPathRendererChain::kStencilOnly_DrawType,
                                                         &stencilSupport);
                if (NULL == pr) {
                    return false;
                }
            }

            int passes;
            GrStencilSettings stencilSettings[GrStencilSettings::kMaxStencilClipPasses];

            bool canRenderDirectToStencil =
                GrPathRenderer::kNoRestriction_StencilSupport == stencilSupport;
            bool canDrawDirectToClip; 
                                      
                                      
                                      
            canDrawDirectToClip = GrStencilSettings::GetClipPasses(op,
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
                if (Element::kRect_Type == element->getType()) {
                    *drawState->stencil() = gDrawToStencil;
                    fGpu->drawSimpleRect(element->getRect(), NULL);
                } else {
                    SkASSERT(Element::kPath_Type == element->getType());
                    if (!clipPath->isEmpty()) {
                        if (canRenderDirectToStencil) {
                            *drawState->stencil() = gDrawToStencil;
                            pr->drawPath(*clipPath, stroke, fGpu, false);
                        } else {
                            pr->stencilPath(*clipPath, stroke, fGpu);
                        }
                    }
                }
            }

            
            
            drawState->enableState(GrGpu::kModifyStencilClip_StateBit);
            for (int p = 0; p < passes; ++p) {
                *drawState->stencil() = stencilSettings[p];
                if (canDrawDirectToClip) {
                    if (Element::kRect_Type == element->getType()) {
                        SET_RANDOM_COLOR
                        fGpu->drawSimpleRect(element->getRect(), NULL);
                    } else {
                        SkASSERT(Element::kPath_Type == element->getType());
                        SET_RANDOM_COLOR
                        pr->drawPath(*clipPath, stroke, fGpu, false);
                    }
                } else {
                    SET_RANDOM_COLOR
                    
                    
                    fGpu->drawSimpleRect(SkRect::Make(clipSpaceIBounds), NULL);
                }
            }
        }
    }
    
    SkASSERT(kNone_ClipMaskType == fCurrClipMaskType);
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
        
        SkASSERT(!drawState.isStateFlagEnabled(
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

    SkASSERT(fGpu->caps()->stencilWrapOpsSupport() || !settings.usesWrapOp());
    SkASSERT(fGpu->caps()->twoSidedStencilSupport() || !settings.isTwoSided());
    this->adjustStencilParams(&settings, clipMode, stencilBits);
    fGpu->setStencilSettings(settings);
}

void GrClipMaskManager::adjustStencilParams(GrStencilSettings* settings,
                                            StencilClipMode mode,
                                            int stencilBitCnt) {
    SkASSERT(stencilBitCnt > 0);

    if (kModifyClip_StencilClipMode == mode) {
        
        
        return;
    }

    unsigned int clipBit = (1 << (stencilBitCnt - 1));
    unsigned int userBits = clipBit - 1;

    GrStencilSettings::Face face = GrStencilSettings::kFront_Face;
    bool twoSided = fGpu->caps()->twoSidedStencilSupport();

    bool finished = false;
    while (!finished) {
        GrStencilFunc func = settings->func(face);
        uint16_t writeMask = settings->writeMask(face);
        uint16_t funcMask = settings->funcMask(face);
        uint16_t funcRef = settings->funcRef(face);

        SkASSERT((unsigned) func < kStencilFuncCount);

        writeMask &= userBits;

        if (func >= kBasicStencilFuncCount) {
            int respectClip = kRespectClip_StencilClipMode == mode;
            if (respectClip) {
                
                SkASSERT(this->isClipInStencil());
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
            SkASSERT(func >= 0 && func < kBasicStencilFuncCount);
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


GrTexture* GrClipMaskManager::createSoftwareClipMask(int32_t elementsGenID,
                                                     GrReducedClip::InitialState initialState,
                                                     const GrReducedClip::ElementList& elements,
                                                     const SkIRect& clipSpaceIBounds) {
    SkASSERT(kNone_ClipMaskType == fCurrClipMaskType);

    GrTexture* result;
    if (this->getMaskTexture(elementsGenID, clipSpaceIBounds, &result, true)) {
        return result;
    }

    if (NULL == result) {
        fAACache.reset();
        return NULL;
    }

    
    
    SkIRect maskSpaceIBounds = SkIRect::MakeWH(clipSpaceIBounds.width(), clipSpaceIBounds.height());

    GrSWMaskHelper helper(this->getContext());

    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(-clipSpaceIBounds.fLeft),
                        SkIntToScalar(-clipSpaceIBounds.fTop));
    helper.init(maskSpaceIBounds, &matrix);

    helper.clear(kAllIn_InitialState == initialState ? 0xFF : 0x00);

    SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);

    for (ElementList::Iter iter(elements.headIter()) ; NULL != iter.get(); iter.next()) {

        const Element* element = iter.get();
        SkRegion::Op op = element->getOp();

        if (SkRegion::kIntersect_Op == op || SkRegion::kReverseDifference_Op == op) {
            
            
            
            
            if (SkRegion::kReverseDifference_Op == op) {
                SkRect temp = SkRect::Make(clipSpaceIBounds);
                
                helper.draw(temp, SkRegion::kXOR_Op, false, 0xFF);
            }

            if (Element::kRect_Type == element->getType()) {
                
                SkPath temp;
                temp.addRect(element->getRect());
                temp.setFillType(SkPath::kInverseEvenOdd_FillType);

                helper.draw(temp, stroke, SkRegion::kReplace_Op,
                            element->isAA(),
                            0x00);
            } else {
                SkASSERT(Element::kPath_Type == element->getType());
                SkPath clipPath = element->getPath();
                clipPath.toggleInverseFillType();
                helper.draw(clipPath, stroke,
                            SkRegion::kReplace_Op,
                            element->isAA(),
                            0x00);
            }

            continue;
        }

        
        
        if (Element::kRect_Type == element->getType()) {
            helper.draw(element->getRect(), op, element->isAA(), 0xFF);
        } else {
            SkASSERT(Element::kPath_Type == element->getType());
            helper.draw(element->getPath(), stroke, op, element->isAA(), 0xFF);
        }
    }

    helper.toTexture(result);

    fCurrClipMaskType = kAlpha_ClipMaskType;
    return result;
}


void GrClipMaskManager::releaseResources() {
    fAACache.releaseResources();
}

void GrClipMaskManager::setGpu(GrGpu* gpu) {
    fGpu = gpu;
    fAACache.setContext(gpu->getContext());
}

void GrClipMaskManager::adjustPathStencilParams(GrStencilSettings* settings) {
    const GrDrawState& drawState = fGpu->getDrawState();
    GrClipMaskManager::StencilClipMode clipMode;
    if (this->isClipInStencil() && drawState.isClipState()) {
        clipMode = GrClipMaskManager::kRespectClip_StencilClipMode;
        
        SkASSERT(!drawState.isStateFlagEnabled(
                    GrGpu::kModifyStencilClip_StateBit));
    } else if (drawState.isStateFlagEnabled(
                    GrGpu::kModifyStencilClip_StateBit)) {
        clipMode = GrClipMaskManager::kModifyClip_StencilClipMode;
    } else {
        clipMode = GrClipMaskManager::kIgnoreClip_StencilClipMode;
    }

    
    int stencilBits = 0;
    GrStencilBuffer* stencilBuffer =
        drawState.getRenderTarget()->getStencilBuffer();
    if (NULL != stencilBuffer) {
        stencilBits = stencilBuffer->bits();
        this->adjustStencilParams(settings, clipMode, stencilBits);
    }
}
