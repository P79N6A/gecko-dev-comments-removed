









#include "GrTextContext.h"
#include "GrAtlas.h"
#include "GrContext.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "GrFontScaler.h"
#include "GrIndexBuffer.h"
#include "GrGpuVertex.h"
#include "GrDrawTarget.h"

enum {
    kGlyphMaskStage = GrPaint::kTotalStages,
};

void GrTextContext::flushGlyphs() {
    if (fCurrVertex > 0) {
        GrDrawTarget::AutoStateRestore asr(fDrawTarget);
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
        int nIndices = fCurrVertex + (fCurrVertex >> 1);
        GrAssert(fCurrTexture);
        drawState->setTexture(kGlyphMaskStage, fCurrTexture);

        if (!GrPixelConfigIsAlphaOnly(fCurrTexture->config())) {
            if (kOne_BlendCoeff != fPaint.fSrcBlendCoeff ||
                kISA_BlendCoeff != fPaint.fDstBlendCoeff ||
                fPaint.hasTexture()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            
            drawState->setBlendConstant(fPaint.fColor);
            drawState->setBlendFunc(kConstC_BlendCoeff, kISC_BlendCoeff);
            
            
            drawState->setColor(0xffffffff);
        } else {
            
            drawState->setBlendFunc(fPaint.fSrcBlendCoeff, fPaint.fDstBlendCoeff);
            drawState->setColor(fPaint.fColor);
        }

        fDrawTarget->setIndexSourceToBuffer(fContext->getQuadIndexBuffer());

        fDrawTarget->drawIndexed(kTriangles_PrimitiveType,
                                 0, 0, fCurrVertex, nIndices);
        fDrawTarget->resetVertexSource();
        fVertices = NULL;
        fMaxVertices = 0;
        fCurrVertex = 0;
        fCurrTexture->unref();
        fCurrTexture = NULL;
    }
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
        fExtMatrix = GrMatrix::I();
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
        if (NULL != fPaint.getTexture(t)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fPaint.textureSampler(t)->preConcatMatrix(invVM);
            }
        }
    }
    for (int m = 0; m < GrPaint::kMaxMasks; ++m) {
        if (NULL != fPaint.getMask(m)) {
            if (invVMComputed || fOrigViewMatrix.invert(&invVM)) {
                invVMComputed = true;
                fPaint.maskSampler(m)->preConcatMatrix(invVM);
            }
        }
    }

    fDrawTarget = fContext->getTextTarget(fPaint);

    fVertices = NULL;
    fMaxVertices = 0;

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

GrTextContext::~GrTextContext() {
    this->flushGlyphs();
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
        fContext->flushText();

        
        fContext->getFontCache()->purgeExceptFor(fStrike);
        if (fStrike->getGlyphAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        if (NULL == glyph->fPath) {
            GrPath* path = new GrPath;
            if (!scaler->getGlyphPath(glyph->glyphID(), path)) {
                
                delete path;
                return;
            }
            glyph->fPath = path;
        }

        GrPoint translate;
        translate.set(GrFixedToScalar(vx - GrIntToFixed(glyph->fBounds.fLeft)),
                      GrFixedToScalar(vy - GrIntToFixed(glyph->fBounds.fTop)));
        fContext->drawPath(fPaint, *glyph->fPath, kWinding_PathFill,
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
        bool flush = fDrawTarget->geometryHints(fVertexLayout,
                                               &fMaxVertices,
                                               NULL);
        if (flush) {
            this->flushGlyphs();
            fContext->flushText();
            fDrawTarget = fContext->getTextTarget(fPaint);
            fMaxVertices = kDefaultRequestedVerts;
            
            fDrawTarget->geometryHints(fVertexLayout,
                                       &fMaxVertices,
                                       NULL);
        }

        int maxQuadVertices = 4 * fContext->getQuadIndexBuffer()->maxQuads();
        if (fMaxVertices < kMinRequestedVerts) {
            fMaxVertices = kDefaultRequestedVerts;
        } else if (fMaxVertices > maxQuadVertices) {
            
            fMaxVertices = maxQuadVertices;
        }
        bool success = fDrawTarget->reserveVertexSpace(fVertexLayout, 
                                                   fMaxVertices,
                                                   GrTCast<void**>(&fVertices));
        GrAlwaysAssert(success);
    }

    GrFixed tx = GrIntToFixed(glyph->fAtlasLocation.fX);
    GrFixed ty = GrIntToFixed(glyph->fAtlasLocation.fY);

#if GR_GL_TEXT_TEXTURE_NORMALIZED
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


