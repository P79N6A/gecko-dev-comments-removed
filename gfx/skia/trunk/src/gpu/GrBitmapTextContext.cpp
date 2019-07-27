






#include "GrBitmapTextContext.h"
#include "GrAtlas.h"
#include "GrDrawTarget.h"
#include "GrFontScaler.h"
#include "GrIndexBuffer.h"
#include "GrStrokeInfo.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "SkColorPriv.h"
#include "SkPath.h"
#include "SkRTConf.h"
#include "SkStrokeRec.h"
#include "effects/GrCustomCoordsTextureEffect.h"

#include "SkAutoKern.h"
#include "SkDraw.h"
#include "SkDrawProcs.h"
#include "SkGlyphCache.h"
#include "SkGpuDevice.h"
#include "SkGr.h"
#include "SkTextMapStateProc.h"

SK_CONF_DECLARE(bool, c_DumpFontCache, "gpu.dumpFontCache", false,
                "Dump the contents of the font cache before every purge.");

static const int kGlyphCoordsNoColorAttributeIndex = 1;
static const int kGlyphCoordsWithColorAttributeIndex = 2;

namespace {

extern const GrVertexAttrib gTextVertexAttribs[] = {
    {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType, sizeof(SkPoint) , kEffect_GrVertexAttribBinding}
};


extern const GrVertexAttrib gTextVertexWithColorAttribs[] = {
    {kVec2f_GrVertexAttribType,  0,                                 kPosition_GrVertexAttribBinding},
    {kVec4ub_GrVertexAttribType, sizeof(SkPoint),                   kColor_GrVertexAttribBinding},
    {kVec2f_GrVertexAttribType,  sizeof(SkPoint) + sizeof(GrColor), kEffect_GrVertexAttribBinding}
};

};

GrBitmapTextContext::GrBitmapTextContext(GrContext* context,
                                         const SkDeviceProperties& properties)
                                       : GrTextContext(context, properties) {
    fStrike = NULL;

    fCurrVertex = 0;
    fEffectTextureUniqueID = SK_InvalidUniqueID;

    fVertices = NULL;

    fVertexBounds.setLargestInverted();
}

GrBitmapTextContext::~GrBitmapTextContext() {
    this->flushGlyphs();
}

bool GrBitmapTextContext::canDraw(const SkPaint& paint) {
    return !SkDraw::ShouldDrawTextAsPaths(paint, fContext->getMatrix());
}

static inline GrColor skcolor_to_grcolor_nopremultiply(SkColor c) {
    unsigned r = SkColorGetR(c);
    unsigned g = SkColorGetG(c);
    unsigned b = SkColorGetB(c);
    return GrColorPackRGBA(r, g, b, 0xff);
}

void GrBitmapTextContext::flushGlyphs() {
    if (NULL == fDrawTarget) {
        return;
    }

    GrDrawState* drawState = fDrawTarget->drawState();
    GrDrawState::AutoRestoreEffects are(drawState);
    drawState->setFromPaint(fPaint, SkMatrix::I(), fContext->getRenderTarget());

    if (fCurrVertex > 0) {
        
        SkASSERT(SkIsAlign4(fCurrVertex));
        GrTextureParams params(SkShader::kRepeat_TileMode, GrTextureParams::kNone_FilterMode);

        GrTexture* currTexture = fStrike->getTexture();
        SkASSERT(currTexture);
        uint32_t textureUniqueID = currTexture->getUniqueID();
        
        if (textureUniqueID != fEffectTextureUniqueID) {
            fCachedEffect.reset(GrCustomCoordsTextureEffect::Create(currTexture, params));
            fEffectTextureUniqueID = textureUniqueID;
        }

        
        int coordsIdx = drawState->hasColorVertexAttribute() ? kGlyphCoordsWithColorAttributeIndex :
                                                               kGlyphCoordsNoColorAttributeIndex;
        drawState->addCoverageEffect(fCachedEffect.get(), coordsIdx);
        SkASSERT(NULL != fStrike);
        switch (fStrike->getMaskFormat()) {
            
            case kARGB_GrMaskFormat:
                SkASSERT(!drawState->hasColorVertexAttribute());
                drawState->setBlendFunc(fPaint.getSrcBlendCoeff(), fPaint.getDstBlendCoeff());
                drawState->setColor(0xffffffff);
                break;
            
            case kA888_GrMaskFormat:
            case kA565_GrMaskFormat: {
                if (kOne_GrBlendCoeff != fPaint.getSrcBlendCoeff() ||
                    kISA_GrBlendCoeff != fPaint.getDstBlendCoeff() ||
                    fPaint.numColorStages()) {
                    GrPrintf("LCD Text will not draw correctly.\n");
                }
                SkASSERT(!drawState->hasColorVertexAttribute());
                
                
                
                
                int a = SkColorGetA(fSkPaint.getColor());
                
                drawState->setColor(SkColorSetARGB(a, a, a, a));
                
                drawState->setBlendConstant(skcolor_to_grcolor_nopremultiply(fSkPaint.getColor()));
                drawState->setBlendFunc(kConstC_GrBlendCoeff, kISC_GrBlendCoeff);
                break;
            }
            
            case kA8_GrMaskFormat:
                
                drawState->setBlendFunc(fPaint.getSrcBlendCoeff(), fPaint.getDstBlendCoeff());
                
                
                SkASSERT(drawState->hasColorVertexAttribute());
                drawState->setColor(0xFFFFFFFF);
                break;
            default:
                SkFAIL("Unexepected mask format.");
        }
        int nGlyphs = fCurrVertex / 4;
        fDrawTarget->setIndexSourceToBuffer(fContext->getQuadIndexBuffer());
        fDrawTarget->drawIndexedInstances(kTriangles_GrPrimitiveType,
                                          nGlyphs,
                                          4, 6, &fVertexBounds);

        fCurrVertex = 0;
        fVertexBounds.setLargestInverted();
    }

    fDrawTarget->resetVertexSource();
    fVertices = NULL;
}

inline void GrBitmapTextContext::init(const GrPaint& paint, const SkPaint& skPaint) {
    GrTextContext::init(paint, skPaint);

    fStrike = NULL;

    fCurrVertex = 0;

    fVertices = NULL;
}

inline void GrBitmapTextContext::finish() {
    this->flushGlyphs();

    GrTextContext::finish();
}

void GrBitmapTextContext::drawText(const GrPaint& paint, const SkPaint& skPaint,
                                   const char text[], size_t byteLength,
                                   SkScalar x, SkScalar y) {
    SkASSERT(byteLength == 0 || text != NULL);

    
    if (text == NULL || byteLength == 0 ) {
        return;
    }

    this->init(paint, skPaint);

    if (NULL == fDrawTarget) {
        return;
    }

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCache    autoCache(fSkPaint, &fDeviceProperties, &fContext->getMatrix());
    SkGlyphCache*       cache = autoCache.getCache();
    GrFontScaler*       fontScaler = GetGrFontScaler(cache);
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(fontScaler, false);
    }

    
    {
        SkPoint loc;
        fContext->getMatrix().mapXY(x, y, &loc);
        x = loc.fX;
        y = loc.fY;
    }

    
    if (fSkPaint.getTextAlign() != SkPaint::kLeft_Align) {
        SkVector    stop;

        MeasureText(cache, glyphCacheProc, text, byteLength, &stop);

        SkScalar    stopX = stop.fX;
        SkScalar    stopY = stop.fY;

        if (fSkPaint.getTextAlign() == SkPaint::kCenter_Align) {
            stopX = SkScalarHalf(stopX);
            stopY = SkScalarHalf(stopY);
        }
        x -= stopX;
        y -= stopY;
    }

    const char* stop = text + byteLength;

    
    SkASSERT(NULL == fVertices);
    bool useColorVerts = kA8_GrMaskFormat == fStrike->getMaskFormat();
    if (useColorVerts) {
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

    SkAutoKern autokern;

    SkFixed fxMask = ~0;
    SkFixed fyMask = ~0;
    SkFixed halfSampleX, halfSampleY;
    if (cache->isSubpixel()) {
        halfSampleX = halfSampleY = (SK_FixedHalf >> SkGlyph::kSubBits);
        SkAxisAlignment baseline = SkComputeAxisAlignmentForHText(fContext->getMatrix());
        if (kX_SkAxisAlignment == baseline) {
            fyMask = 0;
            halfSampleY = SK_FixedHalf;
        } else if (kY_SkAxisAlignment == baseline) {
            fxMask = 0;
            halfSampleX = SK_FixedHalf;
        }
    } else {
        halfSampleX = halfSampleY = SK_FixedHalf;
    }

    SkFixed fx = SkScalarToFixed(x) + halfSampleX;
    SkFixed fy = SkScalarToFixed(y) + halfSampleY;

    GrContext::AutoMatrix  autoMatrix;
    autoMatrix.setIdentity(fContext, &fPaint);

    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &text, fx & fxMask, fy & fyMask);

        fx += autokern.adjust(glyph);

        if (glyph.fWidth) {
            this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                          glyph.getSubXFixed(),
                                          glyph.getSubYFixed()),
                                  SkFixedFloorToFixed(fx),
                                  SkFixedFloorToFixed(fy),
                                  fontScaler);
        }

        fx += glyph.fAdvanceX;
        fy += glyph.fAdvanceY;
    }

    this->finish();
}

void GrBitmapTextContext::drawPosText(const GrPaint& paint, const SkPaint& skPaint,
                                      const char text[], size_t byteLength,
                                      const SkScalar pos[], SkScalar constY,
                                      int scalarsPerPosition) {
    SkASSERT(byteLength == 0 || text != NULL);
    SkASSERT(1 == scalarsPerPosition || 2 == scalarsPerPosition);

    
    if (text == NULL || byteLength == 0) {
        return;
    }

    this->init(paint, skPaint);

    if (NULL == fDrawTarget) {
        return;
    }

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCache    autoCache(fSkPaint, &fDeviceProperties, &fContext->getMatrix());
    SkGlyphCache*       cache = autoCache.getCache();
    GrFontScaler*       fontScaler = GetGrFontScaler(cache);
    
    if (NULL == fStrike) {
        fStrike = fContext->getFontCache()->getStrike(fontScaler, false);
    }

    
    SkMatrix ctm = fContext->getMatrix();
    GrContext::AutoMatrix  autoMatrix;
    autoMatrix.setIdentity(fContext, &fPaint);

    
    SkASSERT(NULL == fVertices);
    bool useColorVerts = kA8_GrMaskFormat == fStrike->getMaskFormat();
    if (useColorVerts) {
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

    const char*        stop = text + byteLength;
    SkTextAlignProc    alignProc(fSkPaint.getTextAlign());
    SkTextMapStateProc tmsProc(ctm, constY, scalarsPerPosition);
    SkFixed halfSampleX = 0, halfSampleY = 0;

    if (cache->isSubpixel()) {
        
        SkAxisAlignment baseline = SkComputeAxisAlignmentForHText(ctm);

        SkFixed fxMask = ~0;
        SkFixed fyMask = ~0;
        if (kX_SkAxisAlignment == baseline) {
            fyMask = 0;
#ifndef SK_IGNORE_SUBPIXEL_AXIS_ALIGN_FIX
            halfSampleY = SK_FixedHalf;
#endif
        } else if (kY_SkAxisAlignment == baseline) {
            fxMask = 0;
#ifndef SK_IGNORE_SUBPIXEL_AXIS_ALIGN_FIX
            halfSampleX = SK_FixedHalf;
#endif
        }

        if (SkPaint::kLeft_Align == fSkPaint.getTextAlign()) {
            while (text < stop) {
                SkPoint tmsLoc;
                tmsProc(pos, &tmsLoc);
                SkFixed fx = SkScalarToFixed(tmsLoc.fX) + halfSampleX;
                SkFixed fy = SkScalarToFixed(tmsLoc.fY) + halfSampleY;

                const SkGlyph& glyph = glyphCacheProc(cache, &text,
                                                      fx & fxMask, fy & fyMask);

                if (glyph.fWidth) {
                    this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                        glyph.getSubXFixed(),
                                                        glyph.getSubYFixed()),
                                          SkFixedFloorToFixed(fx),
                                          SkFixedFloorToFixed(fy),
                                          fontScaler);
                }
                pos += scalarsPerPosition;
            }
        } else {
            while (text < stop) {
                const char* currentText = text;
                const SkGlyph& metricGlyph = glyphCacheProc(cache, &text, 0, 0);

                if (metricGlyph.fWidth) {
                    SkDEBUGCODE(SkFixed prevAdvX = metricGlyph.fAdvanceX;)
                    SkDEBUGCODE(SkFixed prevAdvY = metricGlyph.fAdvanceY;)
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);
                    SkIPoint fixedLoc;
                    alignProc(tmsLoc, metricGlyph, &fixedLoc);

                    SkFixed fx = fixedLoc.fX + halfSampleX;
                    SkFixed fy = fixedLoc.fY + halfSampleY;

                    
                    const SkGlyph& glyph = glyphCacheProc(cache, &currentText,
                                                          fx & fxMask, fy & fyMask);
                    
                    SkASSERT(prevAdvX == glyph.fAdvanceX);
                    SkASSERT(prevAdvY == glyph.fAdvanceY);
                    SkASSERT(glyph.fWidth);

                    this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                        glyph.getSubXFixed(),
                                                        glyph.getSubYFixed()),
                                          SkFixedFloorToFixed(fx),
                                          SkFixedFloorToFixed(fy),
                                          fontScaler);
                }
                pos += scalarsPerPosition;
            }
        }
    } else {    

        if (SkPaint::kLeft_Align == fSkPaint.getTextAlign()) {
            while (text < stop) {
                
                const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

                if (glyph.fWidth) {
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);

                    SkFixed fx = SkScalarToFixed(tmsLoc.fX) + SK_FixedHalf; 
                    SkFixed fy = SkScalarToFixed(tmsLoc.fY) + SK_FixedHalf; 
                    this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                        glyph.getSubXFixed(),
                                                        glyph.getSubYFixed()),
                                          SkFixedFloorToFixed(fx),
                                          SkFixedFloorToFixed(fy),
                                          fontScaler);
                }
                pos += scalarsPerPosition;
            }
        } else {
            while (text < stop) {
                
                const SkGlyph& glyph = glyphCacheProc(cache, &text, 0, 0);

                if (glyph.fWidth) {
                    SkPoint tmsLoc;
                    tmsProc(pos, &tmsLoc);

                    SkIPoint fixedLoc;
                    alignProc(tmsLoc, glyph, &fixedLoc);

                    SkFixed fx = fixedLoc.fX + SK_FixedHalf; 
                    SkFixed fy = fixedLoc.fY + SK_FixedHalf; 
                    this->drawPackedGlyph(GrGlyph::Pack(glyph.getGlyphID(),
                                                        glyph.getSubXFixed(),
                                                        glyph.getSubYFixed()),
                                          SkFixedFloorToFixed(fx),
                                          SkFixedFloorToFixed(fy),
                                          fontScaler);
                }
                pos += scalarsPerPosition;
            }
        }
    }

    this->finish();
}

void GrBitmapTextContext::drawPackedGlyph(GrGlyph::PackedID packed,
                                          SkFixed vx, SkFixed vy,
                                          GrFontScaler* scaler) {
    GrGlyph* glyph = fStrike->getGlyph(packed, scaler);
    if (NULL == glyph || glyph->fBounds.isEmpty()) {
        return;
    }

    vx += SkIntToFixed(glyph->fBounds.fLeft);
    vy += SkIntToFixed(glyph->fBounds.fTop);

    
    SkFixed width = glyph->fBounds.width();
    SkFixed height = glyph->fBounds.height();

    
    if (true || NULL == glyph->fPlot) {
        int x = vx >> 16;
        int y = vy >> 16;
        if (fClipRect.quickReject(x, y, x + width, y + height)) {

            return;
        }
    }

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
        SkMatrix translate;
        translate.setTranslate(SkFixedToScalar(vx - SkIntToFixed(glyph->fBounds.fLeft)),
                               SkFixedToScalar(vy - SkIntToFixed(glyph->fBounds.fTop)));
        GrPaint tmpPaint(fPaint);
        am.setPreConcat(fContext, translate, &tmpPaint);
        GrStrokeInfo strokeInfo(SkStrokeRec::kFill_InitStyle);
        fContext->drawPath(tmpPaint, *glyph->fPath, strokeInfo);
        return;
    }

HAS_ATLAS:
    SkASSERT(glyph->fPlot);
    GrDrawTarget::DrawToken drawToken = fDrawTarget->getCurrentDrawToken();
    glyph->fPlot->setDrawToken(drawToken);

    
    width = SkIntToFixed(width);
    height = SkIntToFixed(height);

    GrTexture* texture = glyph->fPlot->texture();
    SkASSERT(texture);

    SkFixed tx = SkIntToFixed(glyph->fAtlasLocation.fX);
    SkFixed ty = SkIntToFixed(glyph->fAtlasLocation.fY);

    SkRect r;
    r.fLeft = SkFixedToFloat(vx);
    r.fTop = SkFixedToFloat(vy);
    r.fRight = SkFixedToFloat(vx + width);
    r.fBottom = SkFixedToFloat(vy + height);

    fVertexBounds.growToInclude(r);

    bool useColorVerts = kA8_GrMaskFormat == fStrike->getMaskFormat();
    size_t vertSize = useColorVerts ? (2 * sizeof(SkPoint) + sizeof(GrColor)) :
                                      (2 * sizeof(SkPoint));

    SkASSERT(vertSize == fDrawTarget->getDrawState().getVertexSize());

    SkPoint* positions = reinterpret_cast<SkPoint*>(
        reinterpret_cast<intptr_t>(fVertices) + vertSize * fCurrVertex);
    positions->setRectFan(r.fLeft, r.fTop, r.fRight, r.fBottom, vertSize);

    
    SkPoint* textureCoords = reinterpret_cast<SkPoint*>(
            reinterpret_cast<intptr_t>(positions) + vertSize  - sizeof(SkPoint));
    textureCoords->setRectFan(SkFixedToFloat(texture->normalizeFixedX(tx)),
                              SkFixedToFloat(texture->normalizeFixedY(ty)),
                              SkFixedToFloat(texture->normalizeFixedX(tx + width)),
                              SkFixedToFloat(texture->normalizeFixedY(ty + height)),
                              vertSize);
    if (useColorVerts) {
        
        GrColor* colors = reinterpret_cast<GrColor*>(positions + 1);
        for (int i = 0; i < 4; ++i) {
           *colors = fPaint.getColor();
           colors = reinterpret_cast<GrColor*>(reinterpret_cast<intptr_t>(colors) + vertSize);
        }
    }
    fCurrVertex += 4;
}
