








#include "GrTextContext.h"
#include "GrAtlas.h"
#include "GrContext.h"
#include "GrDrawTarget.h"
#include "GrFontScaler.h"
#include "GrGpuVertex.h"
#include "GrIndexBuffer.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "SkPath.h"

enum {
    kGlyphMaskStage = GrPaint::kTotalStages,
};

void GrTextContext::flushGlyphs() {
    if (NULL == fDrawTarget) {
        return;
    }
    GrDrawState* drawState = fDrawTarget->drawState();
    if (fCurrVertex > 0) {
        
        drawState->sampler(kGlyphMaskStage)->reset(SkShader::kRepeat_TileMode,
                                                   !fExtMatrix.isIdentity());

        GrAssert(GrIsALIGN4(fCurrVertex));
        GrAssert(fCurrTexture);
        drawState->createTextureEffect(kGlyphMaskStage, fCurrTexture);

        if (!GrPixelConfigIsAlphaOnly(fCurrTexture->config())) {
            if (kOne_GrBlendCoeff != fPaint.fSrcBlendCoeff ||
                kISA_GrBlendCoeff != fPaint.fDstBlendCoeff ||
                fPaint.hasTexture()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            
            drawState->setBlendConstant(fPaint.fColor);
            drawState->setBlendFunc(kConstC_GrBlendCoeff, kISC_GrBlendCoeff);
            
            
            drawState->setColor(0xffffffff);
        } else {
            
            drawState->setBlendFunc(fPaint.fSrcBlendCoeff,
                                    fPaint.fDstBlendCoeff);
            drawState->setColor(fPaint.fColor);
        }

        int nGlyphs = fCurrVertex / 4;
        fDrawTarget->setIndexSourceToBuffer(fContext->getQuadIndexBuffer());
        fDrawTarget->drawIndexedInstances(kTriangles_GrPrimitiveType,
                                          nGlyphs,
                                          4, 6);
        fDrawTarget->resetVertexSource();
        fVertices = NULL;
        fMaxVertices = 0;
        fCurrVertex = 0;
        GrSafeSetNull(fCurrTexture);
    }
    drawState->disableStages();
    fDrawTarget = NULL;
}

GrTextContext::GrTextContext(GrContext* context,
                             const GrPaint& paint,
                             const GrMatrix* extMatrix) : fPaint(paint) {
    fContext = context;
    fStrike = NULL;

    fCurrTexture = NULL;
    fCurrVertex = 0;

    if (NULL != extMatrix) {
        fExtMatrix = *extMatrix;
    } else {
        fExtMatrix.reset();
    }

    const GrClipData* clipData = context->getClip();

    GrRect devConservativeBound;
    clipData->fClipStack->getConservativeBounds(
                                     -clipData->fOrigin.fX,
                                     -clipData->fOrigin.fY,
                                     context->getRenderTarget()->width(),
                                     context->getRenderTarget()->height(),
                                     &devConservativeBound);

    if (!fExtMatrix.isIdentity()) {
        GrMatrix inverse;
        if (fExtMatrix.invert(&inverse)) {
            inverse.mapRect(&devConservativeBound);
        }
    }

    devConservativeBound.roundOut(&fClipRect);

    
    
    fOrigViewMatrix = fContext->getMatrix();
    fContext->setMatrix(fExtMatrix);

    








    bool invVMComputed = false;
    GrMatrix invVM;
    for (int t = 0; t < GrPaint::kMaxTextures; ++t) {
        if (fPaint.isTextureStageEnabled(t)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fPaint.textureSampler(t)->preConcatMatrix(invVM);
            }
        }
    }
    for (int m = 0; m < GrPaint::kMaxMasks; ++m) {
        if (fPaint.isMaskStageEnabled(m)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fPaint.maskSampler(m)->preConcatMatrix(invVM);
            }
        }
    }

    fDrawTarget = NULL;

    fVertices = NULL;
    fMaxVertices = 0;

    fVertexLayout =
        GrDrawTarget::kTextFormat_VertexLayoutBit |
        GrDrawTarget::StageTexCoordVertexLayoutBit(kGlyphMaskStage, 0);
}

GrTextContext::~GrTextContext() {
    this->flushGlyphs();
    if (fDrawTarget) {
        fDrawTarget->drawState()->disableStages();
    }
    fContext->setMatrix(fOrigViewMatrix);
}

void GrTextContext::flush() {
    this->flushGlyphs();
}

static inline void setRectFan(GrGpuTextVertex v[4], int l, int t, int r, int b,
                              int stride) {
    v[0 * stride].setI(l, t);
    v[1 * stride].setI(l, b);
    v[2 * stride].setI(r, b);
    v[3 * stride].setI(r, t);
}

void GrTextContext::drawPackedGlyph(GrGlyph::PackedID packed,
                                    GrFixed vx, GrFixed vy,
                                    GrFontScaler* scaler) {
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
            SkPath* path = SkNEW(SkPath);
            if (!scaler->getGlyphPath(glyph->glyphID(), path)) {
                
                delete path;
                return;
            }
            glyph->fPath = path;
        }

        GrPoint translate;
        translate.set(GrFixedToScalar(vx - GrIntToFixed(glyph->fBounds.fLeft)),
                      GrFixedToScalar(vy - GrIntToFixed(glyph->fBounds.fTop)));
        fContext->drawPath(fPaint, *glyph->fPath, kWinding_GrPathFill,
                           &translate);
        return;
    }

HAS_ATLAS:
    GrAssert(glyph->fAtlas);

    
    width = GrIntToFixed(width);
    height = GrIntToFixed(height);

    GrTexture* texture = glyph->fAtlas->texture();
    GrAssert(texture);

    if (fCurrTexture != texture || fCurrVertex + 4 > fMaxVertices) {
        this->flushGlyphs();
        fCurrTexture = texture;
        fCurrTexture->ref();
    }

    if (NULL == fVertices) {
        
        
        fMaxVertices = kMinRequestedVerts;
        bool flush = (NULL != fDrawTarget) &&
                     fDrawTarget->geometryHints(fVertexLayout,
                                                &fMaxVertices,
                                                NULL);
        if (flush) {
            this->flushGlyphs();
            fContext->flush();
        }
        fDrawTarget = fContext->getTextTarget(fPaint);
        fMaxVertices = kDefaultRequestedVerts;
        
        fDrawTarget->geometryHints(fVertexLayout,
                                   &fMaxVertices,
                                   NULL);

        int maxQuadVertices = 4 * fContext->getQuadIndexBuffer()->maxQuads();
        if (fMaxVertices < kMinRequestedVerts) {
            fMaxVertices = kDefaultRequestedVerts;
        } else if (fMaxVertices > maxQuadVertices) {
            
            fMaxVertices = maxQuadVertices;
        }
        bool success = fDrawTarget->reserveVertexAndIndexSpace(
                                                   fVertexLayout,
                                                   fMaxVertices,
                                                   0,
                                                   GrTCast<void**>(&fVertices),
                                                   NULL);
        GrAlwaysAssert(success);
    }

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

