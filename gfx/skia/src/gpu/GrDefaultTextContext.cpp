









#include "GrAtlas.h"
#include "GrDefaultTextContext.h"
#include "GrContext.h"
#include "GrDrawTarget.h"
#include "GrFontScaler.h"
#include "GrGpuVertex.h"
#include "GrTemplates.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"

void GrDefaultTextContext::flushGlyphs() {
    GrAssert(this->isValid());
    if (fCurrVertex > 0) {
        GrDrawState* drawState = fDrawTarget->drawState();
        
        GrSamplerState::Filter filter;
        if (fExtMatrix.isIdentity()) {
            filter = GrSamplerState::kNearest_Filter;
        } else {
            filter = GrSamplerState::kBilinear_Filter;
        }
        drawState->sampler(kGlyphMaskStage)->reset(
            GrSamplerState::kRepeat_WrapMode,filter);

        GrAssert(GrIsALIGN4(fCurrVertex));
        GrAssert(fCurrTexture);
        drawState->setTexture(kGlyphMaskStage, fCurrTexture);

        if (!GrPixelConfigIsAlphaOnly(fCurrTexture->config())) {
            if (kOne_BlendCoeff != fGrPaint.fSrcBlendCoeff ||
                kISA_BlendCoeff != fGrPaint.fDstBlendCoeff ||
                fGrPaint.hasTexture()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            
            drawState->setBlendConstant(fGrPaint.fColor);
            drawState->setBlendFunc(kConstC_BlendCoeff, kISC_BlendCoeff);
            
            
            drawState->setColor(0xffffffff);
        } else {
            
            drawState->setBlendFunc(fGrPaint.fSrcBlendCoeff, fGrPaint.fDstBlendCoeff);
            drawState->setColor(fGrPaint.fColor);
        }

        fDrawTarget->setIndexSourceToBuffer(fContext->getQuadIndexBuffer());
        int nGlyphs = fCurrVertex / 4;
        fDrawTarget->drawIndexedInstances(kTriangles_PrimitiveType,
                                          nGlyphs,
                                          4, 6);
        fVertices = NULL;
        this->INHERITED::reset();
    }
}

GrDefaultTextContext::GrDefaultTextContext() {
}

GrDefaultTextContext::~GrDefaultTextContext() {
}

void GrDefaultTextContext::init(GrContext* context,
                                const GrPaint& paint,
                                const GrMatrix* extMatrix) {
    this->INHERITED::init(context, paint, extMatrix);

    fStrike = NULL;

    if (NULL != extMatrix) {
        fExtMatrix = *extMatrix;
    } else {
        fExtMatrix.reset();
    }
    if (context->getClip().hasConservativeBounds()) {
        if (!fExtMatrix.isIdentity()) {
            GrMatrix inverse;
            GrRect r = context->getClip().getConservativeBounds();
            if (fExtMatrix.invert(&inverse)) {
                inverse.mapRect(&r);
                r.roundOut(&fClipRect);
            }
        } else {
            context->getClip().getConservativeBounds().roundOut(&fClipRect);
        }
    } else {
        fClipRect.setLargest();
    }

    
    
    fOrigViewMatrix = fContext->getMatrix();
    fContext->setMatrix(fExtMatrix);

    








    bool invVMComputed = false;
    GrMatrix invVM;
    for (int t = 0; t < GrPaint::kMaxTextures; ++t) {
        if (NULL != fGrPaint.getTexture(t)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fGrPaint.textureSampler(t)->preConcatMatrix(invVM);
            }
        }
    }
    for (int m = 0; m < GrPaint::kMaxMasks; ++m) {
        if (NULL != fGrPaint.getMask(m)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fGrPaint.maskSampler(m)->preConcatMatrix(invVM);
            }
        }
    }

    
    
    fDrawTarget = fContext->getTextTarget(fGrPaint);

    fVertices = NULL;

    fVertexLayout = 
        GrDrawTarget::kTextFormat_VertexLayoutBit |
        GrDrawTarget::StageTexCoordVertexLayoutBit(kGlyphMaskStage, 0);

    int stageMask = paint.getActiveStageMask();
    if (stageMask) {
        for (int i = 0; i < GrPaint::kTotalStages; ++i) {
            if ((1 << i) & stageMask) {
                fVertexLayout |= 
                    GrDrawTarget::StagePosAsTexCoordVertexLayoutBit(i);
                GrAssert(i != kGlyphMaskStage);
            }
        }
    }
}

void GrDefaultTextContext::finish() {
    this->flush();

    fStrike = NULL;
    fContext->setMatrix(fOrigViewMatrix);

    this->INHERITED::finish();
}

void GrDefaultTextContext::flush() {
    GrAssert(this->isValid());
    this->flushGlyphs();
}

static inline void setRectFan(GrGpuTextVertex v[4], int l, int t, int r, int b,
                              int stride) {
    v[0 * stride].setI(l, t);
    v[1 * stride].setI(l, b);
    v[2 * stride].setI(r, b);
    v[3 * stride].setI(r, t);
}

void GrDefaultTextContext::drawPackedGlyph(GrGlyph::PackedID packed,
                                    GrFixed vx, GrFixed vy,
                                    GrFontScaler* scaler) {
    GrAssert(this->isValid());
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(scaler);
    }

    GrGlyph* glyph = fStrike->getGlyph(packed, scaler);
    if (NULL == glyph || glyph->fBounds.isEmpty()) {
        return;
    }

    vx += GrIntToFixed(glyph->fBounds.fLeft);
    vy += GrIntToFixed(glyph->fBounds.fTop);

    
    GrFixed width = glyph->fBounds.width();
    GrFixed height = glyph->fBounds.height();

    
    if (true || NULL == glyph->fAtlas) {
        int x = vx >> 16;
        int y = vy >> 16;
        if (fClipRect.quickReject(x, y, x + width, y + height)) {

            return;
        }
    }

    if (NULL == glyph->fAtlas) {
        if (fStrike->getGlyphAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        
        this->flushGlyphs();
        fContext->flush();

        
        fContext->getFontCache()->purgeExceptFor(fStrike);
        if (fStrike->getGlyphAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        if (NULL == glyph->fPath) {
            SkPath* path = new SkPath;
            if (!scaler->getGlyphPath(glyph->glyphID(), path)) {
                
                delete path;
                return;
            }
            glyph->fPath = path;
        }

        GrPoint translate;
        translate.set(GrFixedToScalar(vx - GrIntToFixed(glyph->fBounds.fLeft)),
                      GrFixedToScalar(vy - GrIntToFixed(glyph->fBounds.fTop)));
        fContext->drawPath(fGrPaint, *glyph->fPath, kWinding_PathFill,
                           &translate);
        return;
    }

HAS_ATLAS:
    GrAssert(glyph->fAtlas);

    
    width = GrIntToFixed(width);
    height = GrIntToFixed(height);

    GrTexture* texture = glyph->fAtlas->texture();
    this->prepareForGlyph(texture);

    this->setupVertexBuff(GrTCast<void**>(&fVertices),
                          fVertexLayout);

    GrFixed tx = GrIntToFixed(glyph->fAtlasLocation.fX);
    GrFixed ty = GrIntToFixed(glyph->fAtlasLocation.fY);

#if GR_TEXT_SCALAR_IS_USHORT
    int x = vx >> 16;
    int y = vy >> 16;
    int w = width >> 16;
    int h = height >> 16;

    setRectFan(&fVertices[2*fCurrVertex], x, y, x + w, y + h, 2);
    setRectFan(&fVertices[2*fCurrVertex+1],
               texture->normalizeFixedX(tx),
               texture->normalizeFixedY(ty),
               texture->normalizeFixedX(tx + width),
               texture->normalizeFixedY(ty + height),
               2);
#else
    fVertices[2*fCurrVertex].setXRectFan(vx, vy, vx + width, vy + height,
                                        2 * sizeof(GrGpuTextVertex));
    fVertices[2*fCurrVertex+1].setXRectFan(texture->normalizeFixedX(tx),
                                          texture->normalizeFixedY(ty),
                                          texture->normalizeFixedX(tx + width),
                                          texture->normalizeFixedY(ty + height),
                                          2 * sizeof(GrGpuTextVertex));
#endif
    fCurrVertex += 4;
}
