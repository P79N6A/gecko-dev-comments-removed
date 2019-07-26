






#include "GrDistanceFieldTextContext.h"
#include "GrAtlas.h"
#include "GrDrawTarget.h"
#include "GrFontScaler.h"
#include "SkGlyphCache.h"
#include "GrIndexBuffer.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "SkDraw.h"
#include "SkGpuDevice.h"
#include "SkPath.h"
#include "SkRTConf.h"
#include "SkStrokeRec.h"
#include "effects/GrDistanceFieldTextureEffect.h"

static const int kGlyphCoordsAttributeIndex = 1;

static const int kBaseDFFontSize = 32;

SK_CONF_DECLARE(bool, c_DumpFontCache, "gpu.dumpFontCache", false,
                "Dump the contents of the font cache before every purge.");

GrDistanceFieldTextContext::GrDistanceFieldTextContext(GrContext* context,
                                                       const SkDeviceProperties& properties)
                                                    : GrTextContext(context, properties) {
    fStrike = NULL;

    fCurrTexture = NULL;
    fCurrVertex = 0;

    fVertices = NULL;
    fMaxVertices = 0;
}

GrDistanceFieldTextContext::~GrDistanceFieldTextContext() {
    this->flushGlyphs();
}

bool GrDistanceFieldTextContext::canDraw(const SkPaint& paint) {
    return !paint.getRasterizer() && !paint.getMaskFilter() &&
           paint.getStyle() == SkPaint::kFill_Style &&
           !SkDraw::ShouldDrawTextAsPaths(paint, fContext->getMatrix());
}

static inline GrColor skcolor_to_grcolor_nopremultiply(SkColor c) {
    unsigned r = SkColorGetR(c);
    unsigned g = SkColorGetG(c);
    unsigned b = SkColorGetB(c);
    return GrColorPackRGBA(r, g, b, 0xff);
}

void GrDistanceFieldTextContext::flushGlyphs() {
    if (NULL == fDrawTarget) {
        return;
    }

    GrDrawState* drawState = fDrawTarget->drawState();
    GrDrawState::AutoRestoreEffects are(drawState);
    drawState->setFromPaint(fPaint, fContext->getMatrix(), fContext->getRenderTarget());

    if (fCurrVertex > 0) {
        
        SkASSERT(GrIsALIGN4(fCurrVertex));
        SkASSERT(fCurrTexture);
        GrTextureParams params(SkShader::kRepeat_TileMode, GrTextureParams::kBilerp_FilterMode);

        
        drawState->addCoverageEffect(
                                GrDistanceFieldTextureEffect::Create(fCurrTexture, params),
                                kGlyphCoordsAttributeIndex)->unref();

        if (!GrPixelConfigIsAlphaOnly(fCurrTexture->config())) {
            if (kOne_GrBlendCoeff != fPaint.getSrcBlendCoeff() ||
                kISA_GrBlendCoeff != fPaint.getDstBlendCoeff() ||
                fPaint.numColorStages()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            
            
            
            
            int a = SkColorGetA(fSkPaint.getColor());
            
            drawState->setColor(SkColorSetARGB(a, a, a, a));
            
            drawState->setBlendConstant(skcolor_to_grcolor_nopremultiply(fSkPaint.getColor()));
            drawState->setBlendFunc(kConstC_GrBlendCoeff, kISC_GrBlendCoeff);
        } else {
            
            drawState->setBlendFunc(fPaint.getSrcBlendCoeff(), fPaint.getDstBlendCoeff());
            drawState->setColor(fPaint.getColor());
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
        SkSafeSetNull(fCurrTexture);
    }
}

namespace {


extern const GrVertexAttrib gTextVertexAttribs[] = {
    {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding}
};

};

void GrDistanceFieldTextContext::drawPackedGlyph(GrGlyph::PackedID packed,
                                                 GrFixed vx, GrFixed vy,
                                                 GrFontScaler* scaler) {
    if (NULL == fDrawTarget) {
        return;
    }
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(scaler, true);
    }

    GrGlyph* glyph = fStrike->getGlyph(packed, scaler);
    if (NULL == glyph || glyph->fBounds.isEmpty()) {
        return;
    }

    SkScalar sx = SkFixedToScalar(vx);
    SkScalar sy = SkFixedToScalar(vy);



















    if (NULL == glyph->fPlot) {
        if (fStrike->getGlyphAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        
        fContext->getFontCache()->freePlotExceptFor(fStrike);
        if (fStrike->getGlyphAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        if (c_DumpFontCache) {
#ifdef SK_DEVELOPER
            fContext->getFontCache()->dump();
#endif
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

        GrContext::AutoMatrix am;
        SkMatrix translate;
        translate.setTranslate(sx, sy);
        GrPaint tmpPaint(fPaint);
        am.setPreConcat(fContext, translate, &tmpPaint);
        SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);
        fContext->drawPath(tmpPaint, *glyph->fPath, stroke);
        return;
    }

HAS_ATLAS:
    SkASSERT(glyph->fPlot);
    GrDrawTarget::DrawToken drawToken = fDrawTarget->getCurrentDrawToken();
    glyph->fPlot->setDrawToken(drawToken);

    GrTexture* texture = glyph->fPlot->texture();
    SkASSERT(texture);

    if (fCurrTexture != texture || fCurrVertex + 4 > fMaxVertices) {
        this->flushGlyphs();
        fCurrTexture = texture;
        fCurrTexture->ref();
    }

    if (NULL == fVertices) {
       
        
        fMaxVertices = kMinRequestedVerts;
        fDrawTarget->drawState()->setVertexAttribs<gTextVertexAttribs>(
            SK_ARRAY_COUNT(gTextVertexAttribs));
        bool flush = fDrawTarget->geometryHints(&fMaxVertices, NULL);
        if (flush) {
            this->flushGlyphs();
            fContext->flush();
            fDrawTarget->drawState()->setVertexAttribs<gTextVertexAttribs>(
                SK_ARRAY_COUNT(gTextVertexAttribs));
        }
        fMaxVertices = kDefaultRequestedVerts;
        
        fDrawTarget->geometryHints(&fMaxVertices, NULL);

        int maxQuadVertices = 4 * fContext->getQuadIndexBuffer()->maxQuads();
        if (fMaxVertices < kMinRequestedVerts) {
            fMaxVertices = kDefaultRequestedVerts;
        } else if (fMaxVertices > maxQuadVertices) {
            
            fMaxVertices = maxQuadVertices;
        }
        bool success = fDrawTarget->reserveVertexAndIndexSpace(fMaxVertices,
                                                               0,
                                                               GrTCast<void**>(&fVertices),
                                                               NULL);
        GrAlwaysAssert(success);
        SkASSERT(2*sizeof(GrPoint) == fDrawTarget->getDrawState().getVertexSize());
    }

    SkScalar dx = SkIntToScalar(glyph->fBounds.fLeft);
    SkScalar dy = SkIntToScalar(glyph->fBounds.fTop);
    SkScalar width = SkIntToScalar(glyph->fBounds.width());
    SkScalar height = SkIntToScalar(glyph->fBounds.height());

    SkScalar scale = fTextRatio;
    dx *= scale;
    dy *= scale;
    sx += dx;
    sy += dy;
    width *= scale;
    height *= scale;

    GrFixed tx = SkIntToFixed(glyph->fAtlasLocation.fX);
    GrFixed ty = SkIntToFixed(glyph->fAtlasLocation.fY);
    GrFixed tw = SkIntToFixed(glyph->fBounds.width());
    GrFixed th = SkIntToFixed(glyph->fBounds.height());

    fVertices[2*fCurrVertex].setRectFan(sx,
                                        sy,
                                        sx + width,
                                        sy + height,
                                        2 * sizeof(SkPoint));
    fVertices[2*fCurrVertex+1].setRectFan(SkFixedToFloat(texture->normalizeFixedX(tx)),
                                          SkFixedToFloat(texture->normalizeFixedY(ty)),
                                          SkFixedToFloat(texture->normalizeFixedX(tx + tw)),
                                          SkFixedToFloat(texture->normalizeFixedY(ty + th)),
                                          2 * sizeof(SkPoint));
    fCurrVertex += 4;
}

inline void GrDistanceFieldTextContext::init(const GrPaint& paint, const SkPaint& skPaint) {
    GrTextContext::init(paint, skPaint);

    fStrike = NULL;

    fCurrTexture = NULL;
    fCurrVertex = 0;

    fVertices = NULL;
    fMaxVertices = 0;

    fTextRatio = fSkPaint.getTextSize()/kBaseDFFontSize;

    fSkPaint.setTextSize(SkIntToScalar(kBaseDFFontSize));
    fSkPaint.setLCDRenderText(false);
    fSkPaint.setAutohinted(false);
    fSkPaint.setSubpixelText(false);
}

inline void GrDistanceFieldTextContext::finish() {
    flushGlyphs();

    GrTextContext::finish();
}

void GrDistanceFieldTextContext::drawText(const GrPaint& paint, const SkPaint& skPaint,
                                          const char text[], size_t byteLength,
                                          SkScalar x, SkScalar y) {
    SkASSERT(byteLength == 0 || text != NULL);

    
    if (text == NULL || byteLength == 0 
        || fSkPaint.getRasterizer()) {
        return;
    }

    this->init(paint, skPaint);

    SkScalar sizeRatio = fTextRatio;

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCache    autoCache(fSkPaint, &fDeviceProperties, NULL);
    SkGlyphCache*       cache = autoCache.getCache();
    GrFontScaler*       fontScaler = GetGrFontScaler(cache);

    
    
    const char* stop = text + byteLength;
    if (fSkPaint.getTextAlign() != SkPaint::kLeft_Align) {
        SkFixed    stopX = 0;
        SkFixed    stopY = 0;

        const char* textPtr = text;
        while (textPtr < stop) {
            
            
            const SkGlyph& glyph = glyphCacheProc(cache, &textPtr, 0, 0);

            stopX += glyph.fAdvanceX;
            stopY += glyph.fAdvanceY;
        }
        SkASSERT(textPtr == stop);

        SkScalar alignX = SkFixedToScalar(stopX)*sizeRatio;
        SkScalar alignY = SkFixedToScalar(stopY)*sizeRatio;

        if (fSkPaint.getTextAlign() == SkPaint::kCenter_Align) {
            alignX = SkScalarHalf(alignX);
            alignY = SkScalarHalf(alignY);
        }

        x -= alignX;
        y -= alignY;
    }

    SkFixed fx = SkScalarToFixed(x) + SK_FixedHalf;
    SkFixed fy = SkScalarToFixed(y) + SK_FixedHalf;
    SkFixed fixedScale = SkScalarToFixed(sizeRatio);
    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, fx, fy);

        if (glyph.fWidth) {
            this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                glyph.getSubXFixed(),
                                                glyph.getSubYFixed()),
                                  SkFixedFloorToFixed(fx),
                                  SkFixedFloorToFixed(fy),
                                  fontScaler);
        }

        fx += SkFixedMul_portable(glyph.fAdvanceX, fixedScale);
        fy += SkFixedMul_portable(glyph.fAdvanceY, fixedScale);
    }

    this->finish();
}

void GrDistanceFieldTextContext::drawPosText(const GrPaint& paint, const SkPaint& skPaint,
                                             const char text[], size_t byteLength,
                                             const SkScalar pos[], SkScalar constY,
                                             int scalarsPerPosition) {

    SkASSERT(byteLength == 0 || text != NULL);
    SkASSERT(1 == scalarsPerPosition || 2 == scalarsPerPosition);

    
    if (text == NULL || byteLength == 0 ) {
        return;
    }

    this->init(paint, skPaint);

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCache    autoCache(fSkPaint, &fDeviceProperties, NULL);
    SkGlyphCache*       cache = autoCache.getCache();
    GrFontScaler*       fontScaler = GetGrFontScaler(cache);

    const char*        stop = text + byteLength;

    if (SkPaint::kLeft_Align == fSkPaint.getTextAlign()) {
        while (text < stop) {
            
            const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

            if (glyph.fWidth) {
                SkScalar x = pos[0];
                SkScalar y = scalarsPerPosition == 1 ? constY : pos[1];

                this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                    glyph.getSubXFixed(),
                                                    glyph.getSubYFixed()),
                                      SkScalarToFixed(x) + SK_FixedHalf, 
                                      SkScalarToFixed(y) + SK_FixedHalf, 
                                      fontScaler);
            }
            pos += scalarsPerPosition;
        }
    } else {
        int alignShift = SkPaint::kCenter_Align == fSkPaint.getTextAlign() ? 1 : 0;
        while (text < stop) {
            
            const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

            if (glyph.fWidth) {
                SkScalar x = pos[0];
                SkScalar y = scalarsPerPosition == 1 ? constY : pos[1];

                this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                    glyph.getSubXFixed(),
                                                    glyph.getSubYFixed()),
                                      SkScalarToFixed(x) - (glyph.fAdvanceX >> alignShift)
                                        + SK_FixedHalf, 
                                      SkScalarToFixed(y) - (glyph.fAdvanceY >> alignShift)
                                        + SK_FixedHalf, 
                                      fontScaler);
            }
            pos += scalarsPerPosition;
        }
    }

    this->finish();
}
