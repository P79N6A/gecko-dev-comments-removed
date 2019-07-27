






#include "GrDistanceFieldTextContext.h"
#include "GrAtlas.h"
#include "SkColorFilter.h"
#include "GrDrawTarget.h"
#include "GrDrawTargetCaps.h"
#include "GrFontScaler.h"
#include "SkGlyphCache.h"
#include "GrGpu.h"
#include "GrIndexBuffer.h"
#include "GrStrokeInfo.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "SkDistanceFieldGen.h"
#include "SkDraw.h"
#include "SkGpuDevice.h"
#include "SkPath.h"
#include "SkRTConf.h"
#include "SkStrokeRec.h"
#include "effects/GrDistanceFieldTextureEffect.h"

SK_CONF_DECLARE(bool, c_DumpFontCache, "gpu.dumpFontCache", false,
                "Dump the contents of the font cache before every purge.");

static const int kGlyphCoordsNoColorAttributeIndex = 1;
static const int kGlyphCoordsWithColorAttributeIndex = 2;

static const int kSmallDFFontSize = 32;
static const int kSmallDFFontLimit = 32;
static const int kMediumDFFontSize = 64;
static const int kMediumDFFontLimit = 64;
static const int kLargeDFFontSize = 128;

namespace {

extern const GrVertexAttrib gTextVertexAttribs[] = {
    {kVec2f_GrVertexAttribType, 0,                kPosition_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType, sizeof(SkPoint) , kEffect_GrVertexAttribBinding}
};


extern const GrVertexAttrib gTextVertexWithColorAttribs[] = {
    {kVec2f_GrVertexAttribType,  0,                                 kPosition_GrVertexAttribBinding},
    {kVec4ub_GrVertexAttribType, sizeof(SkPoint),                   kColor_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType,  sizeof(SkPoint) + sizeof(GrColor), kEffect_GrVertexAttribBinding}
};
    
};

GrDistanceFieldTextContext::GrDistanceFieldTextContext(GrContext* context,
                                                       const SkDeviceProperties& properties,
                                                       bool enable)
                                                    : GrTextContext(context, properties) {
#if SK_FORCE_DISTANCEFIELD_FONTS
    fEnableDFRendering = true;
#else
    fEnableDFRendering = enable;
#endif
    fStrike = NULL;
    fGammaTexture = NULL;

    fCurrVertex = 0;

    fVertices = NULL;
}

GrDistanceFieldTextContext::~GrDistanceFieldTextContext() {
    this->flushGlyphs();
    SkSafeSetNull(fGammaTexture);
}

bool GrDistanceFieldTextContext::canDraw(const SkPaint& paint) {
    if (!fEnableDFRendering && !paint.isDistanceFieldTextTEMP()) {
        return false;
    }

    
    
    if (paint.getRasterizer() || paint.getMaskFilter() ||
        !fContext->getTextTarget()->caps()->shaderDerivativeSupport()) {
        return false;
    }

    
    if (paint.getStyle() != SkPaint::kFill_Style) {
        return false;
    }

    
    
    if (SkDraw::ShouldDrawTextAsPaths(paint, fContext->getMatrix())) {
        return false;
    }

    
    SkScalerContext::Rec    rec;
    SkScalerContext::MakeRec(paint, &fDeviceProperties, NULL, &rec);
    return rec.getFormat() != SkMask::kARGB32_Format;
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
        
        SkASSERT(SkIsAlign4(fCurrVertex));
        GrTexture* currTexture = fStrike->getTexture();
        SkASSERT(currTexture);
        GrTextureParams params(SkShader::kRepeat_TileMode, GrTextureParams::kBilerp_FilterMode);
        GrTextureParams gammaParams(SkShader::kClamp_TileMode, GrTextureParams::kNone_FilterMode);

        
        int coordsIdx = drawState->hasColorVertexAttribute() ? kGlyphCoordsWithColorAttributeIndex :
                                                               kGlyphCoordsNoColorAttributeIndex;
        SkColor filteredColor;
        SkColorFilter* colorFilter = fSkPaint.getColorFilter();
        if (NULL != colorFilter) {
            filteredColor = colorFilter->filterColor(fSkPaint.getColor());
        } else {
            filteredColor = fSkPaint.getColor();
        }
        if (fUseLCDText) {
            GrColor colorNoPreMul = skcolor_to_grcolor_nopremultiply(filteredColor);
            bool useBGR = SkDeviceProperties::Geometry::kBGR_Layout ==
                                                            fDeviceProperties.fGeometry.getLayout();
            drawState->addCoverageEffect(GrDistanceFieldLCDTextureEffect::Create(
                                                            currTexture,
                                                            params,
                                                            fGammaTexture,
                                                            gammaParams,
                                                            colorNoPreMul,
                                                            fContext->getMatrix().rectStaysRect() &&
                                                            fContext->getMatrix().isSimilarity(),
                                                            useBGR),
                                         coordsIdx)->unref();

            if (kOne_GrBlendCoeff != fPaint.getSrcBlendCoeff() ||
                kISA_GrBlendCoeff != fPaint.getDstBlendCoeff() ||
                fPaint.numColorStages()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            SkASSERT(!drawState->hasColorVertexAttribute());
            
            
            
            
            int a = SkColorGetA(fSkPaint.getColor());
            
            drawState->setColor(SkColorSetARGB(a, a, a, a));
            
            drawState->setBlendConstant(colorNoPreMul);
            drawState->setBlendFunc(kConstC_GrBlendCoeff, kISC_GrBlendCoeff);
        } else {
#ifdef SK_GAMMA_APPLY_TO_A8
            U8CPU lum = SkColorSpaceLuminance::computeLuminance(fDeviceProperties.fGamma,
                                                                filteredColor);
            drawState->addCoverageEffect(GrDistanceFieldTextureEffect::Create(
                                                              currTexture, params,
                                                              fGammaTexture, gammaParams,
                                                              lum/255.f,
                                                              fContext->getMatrix().isSimilarity()),
                                         coordsIdx)->unref();
#else
            drawState->addCoverageEffect(GrDistanceFieldTextureEffect::Create(
                                                              currTexture, params,
                                                              fContext->getMatrix().isSimilarity()),
                                         coordsIdx)->unref();
#endif
            
            drawState->setBlendFunc(fPaint.getSrcBlendCoeff(), fPaint.getDstBlendCoeff());
            
            
            SkASSERT(drawState->hasColorVertexAttribute());
            drawState->setColor(0xFFFFFFFF);
        }
        int nGlyphs = fCurrVertex / 4;
        fDrawTarget->setIndexSourceToBuffer(fContext->getQuadIndexBuffer());
        fDrawTarget->drawIndexedInstances(kTriangles_GrPrimitiveType,
                                          nGlyphs,
                                          4, 6);
        fCurrVertex = 0;
    }
    fDrawTarget->resetVertexSource();
    fVertices = NULL;
}

void GrDistanceFieldTextContext::drawPackedGlyph(GrGlyph::PackedID packed,
                                                 SkFixed vx, SkFixed vy,
                                                 GrFontScaler* scaler) {
    GrGlyph* glyph = fStrike->getGlyph(packed, scaler);
    if (NULL == glyph || glyph->fBounds.isEmpty()) {
        return;
    }

    SkScalar sx = SkFixedToScalar(vx);
    SkScalar sy = SkFixedToScalar(vy);



















    if (NULL == glyph->fPlot) {
        if (fStrike->addGlyphToAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        
        if (fContext->getFontCache()->freeUnusedPlot(fStrike) &&
            fStrike->addGlyphToAtlas(glyph, scaler)) {
            goto HAS_ATLAS;
        }

        if (c_DumpFontCache) {
#ifdef SK_DEVELOPER
            fContext->getFontCache()->dump();
#endif
        }

        
        this->flushGlyphs();
        fContext->flush();

        
        if (fContext->getFontCache()->freeUnusedPlot(fStrike) &&
            fStrike->addGlyphToAtlas(glyph, scaler)) {
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
        SkMatrix ctm;
        ctm.setScale(fTextRatio, fTextRatio);
        ctm.postTranslate(sx, sy);
        GrPaint tmpPaint(fPaint);
        am.setPreConcat(fContext, ctm, &tmpPaint);
        GrStrokeInfo strokeInfo(SkStrokeRec::kFill_InitStyle);
        fContext->drawPath(tmpPaint, *glyph->fPath, strokeInfo);
        return;
    }

HAS_ATLAS:
    SkASSERT(glyph->fPlot);
    GrDrawTarget::DrawToken drawToken = fDrawTarget->getCurrentDrawToken();
    glyph->fPlot->setDrawToken(drawToken);

    GrTexture* texture = glyph->fPlot->texture();
    SkASSERT(texture);

    SkScalar dx = SkIntToScalar(glyph->fBounds.fLeft + SK_DistanceFieldInset);
    SkScalar dy = SkIntToScalar(glyph->fBounds.fTop + SK_DistanceFieldInset);
    SkScalar width = SkIntToScalar(glyph->fBounds.width() - 2*SK_DistanceFieldInset);
    SkScalar height = SkIntToScalar(glyph->fBounds.height() - 2*SK_DistanceFieldInset);

    SkScalar scale = fTextRatio;
    dx *= scale;
    dy *= scale;
    sx += dx;
    sy += dy;
    width *= scale;
    height *= scale;

    SkFixed tx = SkIntToFixed(glyph->fAtlasLocation.fX + SK_DistanceFieldInset);
    SkFixed ty = SkIntToFixed(glyph->fAtlasLocation.fY + SK_DistanceFieldInset);
    SkFixed tw = SkIntToFixed(glyph->fBounds.width() - 2*SK_DistanceFieldInset);
    SkFixed th = SkIntToFixed(glyph->fBounds.height() - 2*SK_DistanceFieldInset);

    size_t vertSize = fUseLCDText ? (2 * sizeof(SkPoint))
                                  : (2 * sizeof(SkPoint) + sizeof(GrColor));
    
    SkASSERT(vertSize == fDrawTarget->getDrawState().getVertexSize());
    
    SkPoint* positions = reinterpret_cast<SkPoint*>(
        reinterpret_cast<intptr_t>(fVertices) + vertSize * fCurrVertex);
    positions->setRectFan(sx, sy, sx + width, sy + height, vertSize);
    
    
    SkPoint* textureCoords = reinterpret_cast<SkPoint*>(
            reinterpret_cast<intptr_t>(positions) + vertSize  - sizeof(SkPoint));
    textureCoords->setRectFan(SkFixedToFloat(texture->normalizeFixedX(tx)),
                              SkFixedToFloat(texture->normalizeFixedY(ty)),
                              SkFixedToFloat(texture->normalizeFixedX(tx + tw)),
                              SkFixedToFloat(texture->normalizeFixedY(ty + th)),
                              vertSize);
    if (!fUseLCDText) {
        
        GrColor* colors = reinterpret_cast<GrColor*>(positions + 1);
        for (int i = 0; i < 4; ++i) {
            *colors = fPaint.getColor();
            colors = reinterpret_cast<GrColor*>(reinterpret_cast<intptr_t>(colors) + vertSize);
        }
    }
    
    fCurrVertex += 4;
}

inline void GrDistanceFieldTextContext::init(const GrPaint& paint, const SkPaint& skPaint) {
    GrTextContext::init(paint, skPaint);

    fStrike = NULL;

    fCurrVertex = 0;

    fVertices = NULL;

    if (fSkPaint.getTextSize() <= kSmallDFFontLimit) {
        fTextRatio = fSkPaint.getTextSize()/kSmallDFFontSize;
        fSkPaint.setTextSize(SkIntToScalar(kSmallDFFontSize));
    } else if (fSkPaint.getTextSize() <= kMediumDFFontLimit) {
        fTextRatio = fSkPaint.getTextSize()/kMediumDFFontSize;
        fSkPaint.setTextSize(SkIntToScalar(kMediumDFFontSize));
    } else {
        fTextRatio = fSkPaint.getTextSize()/kLargeDFFontSize;
        fSkPaint.setTextSize(SkIntToScalar(kLargeDFFontSize));
    }

    fUseLCDText = fSkPaint.isLCDRenderText();

    fSkPaint.setLCDRenderText(false);
    fSkPaint.setAutohinted(false);
    fSkPaint.setHinting(SkPaint::kNormal_Hinting);
    fSkPaint.setSubpixelText(true);

}

inline void GrDistanceFieldTextContext::finish() {
    this->flushGlyphs();

    GrTextContext::finish();
}

static void setup_gamma_texture(GrContext* context, const SkGlyphCache* cache,
                                const SkDeviceProperties& deviceProperties,
                                GrTexture** gammaTexture) {
    if (NULL == *gammaTexture) {
        int width, height;
        size_t size;

#ifdef SK_GAMMA_CONTRAST
        SkScalar contrast = SK_GAMMA_CONTRAST;
#else
        SkScalar contrast = 0.5f;
#endif
        SkScalar paintGamma = deviceProperties.fGamma;
        SkScalar deviceGamma = deviceProperties.fGamma;

        size = SkScalerContext::GetGammaLUTSize(contrast, paintGamma, deviceGamma,
                                                &width, &height);

        SkAutoTArray<uint8_t> data((int)size);
        SkScalerContext::GetGammaLUTData(contrast, paintGamma, deviceGamma, data.get());

        
        GrTextureDesc desc;
        desc.fFlags = kDynamicUpdate_GrTextureFlagBit;
        desc.fWidth = width;
        desc.fHeight = height;
        desc.fConfig = kAlpha_8_GrPixelConfig;

        *gammaTexture = context->getGpu()->createTexture(desc, NULL, 0);
        if (NULL == *gammaTexture) {
            return;
        }

        context->writeTexturePixels(*gammaTexture,
                                    0, 0, width, height,
                                    (*gammaTexture)->config(), data.get(), 0,
                                    GrContext::kDontFlush_PixelOpsFlag);
    }
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

    if (NULL == fDrawTarget) {
        return;
    }

    SkScalar sizeRatio = fTextRatio;

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCacheNoGamma    autoCache(fSkPaint, &fDeviceProperties, NULL);
    SkGlyphCache*              cache = autoCache.getCache();
    GrFontScaler*              fontScaler = GetGrFontScaler(cache);
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(fontScaler, true);
    }

    setup_gamma_texture(fContext, cache, fDeviceProperties, &fGammaTexture);

    
    SkASSERT(NULL == fVertices);
    if (!fUseLCDText) {
        fDrawTarget->drawState()->setVertexAttribs<gTextVertexWithColorAttribs>(
                                                       SK_ARRAY_COUNT(gTextVertexWithColorAttribs));
    } else {
        fDrawTarget->drawState()->setVertexAttribs<gTextVertexAttribs>(
                                                       SK_ARRAY_COUNT(gTextVertexAttribs));
    }
    int numGlyphs = fSkPaint.textToGlyphs(text, byteLength, NULL);
    bool success = fDrawTarget->reserveVertexAndIndexSpace(4*numGlyphs,
                                                           0,
                                                           &fVertices,
                                                           NULL);
    GrAlwaysAssert(success);

    
    
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

    SkFixed fx = SkScalarToFixed(x);
    SkFixed fy = SkScalarToFixed(y);
    SkFixed fixedScale = SkScalarToFixed(sizeRatio);
    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

        if (glyph.fWidth) {
            this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                glyph.getSubXFixed(),
                                                glyph.getSubYFixed()),
                                  fx,
                                  fy,
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

    if (NULL == fDrawTarget) {
        return;
    }

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCacheNoGamma    autoCache(fSkPaint, &fDeviceProperties, NULL);
    SkGlyphCache*              cache = autoCache.getCache();
    GrFontScaler*              fontScaler = GetGrFontScaler(cache);
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(fontScaler, true);
    }

    
    SkASSERT(NULL == fVertices);
    if (!fUseLCDText) {
        fDrawTarget->drawState()->setVertexAttribs<gTextVertexWithColorAttribs>(
                                                       SK_ARRAY_COUNT(gTextVertexWithColorAttribs));
    } else {
        fDrawTarget->drawState()->setVertexAttribs<gTextVertexAttribs>(
                                                       SK_ARRAY_COUNT(gTextVertexAttribs));
    }
    int numGlyphs = fSkPaint.textToGlyphs(text, byteLength, NULL);
    bool success = fDrawTarget->reserveVertexAndIndexSpace(4*numGlyphs,
                                                           0,
                                                           &fVertices,
                                                           NULL);
    GrAlwaysAssert(success);

    setup_gamma_texture(fContext, cache, fDeviceProperties, &fGammaTexture);

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
                                      SkScalarToFixed(x),
                                      SkScalarToFixed(y),
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
                                      SkScalarToFixed(x) - (glyph.fAdvanceX >> alignShift),
                                      SkScalarToFixed(y) - (glyph.fAdvanceY >> alignShift),
                                      fontScaler);
            }
            pos += scalarsPerPosition;
        }
    }

    this->finish();
}
