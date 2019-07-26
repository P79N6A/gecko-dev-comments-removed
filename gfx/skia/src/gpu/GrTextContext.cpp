








#include "GrTextContext.h"
#include "GrAtlas.h"
#include "GrContext.h"
#include "GrDrawTarget.h"
#include "GrFontScaler.h"
#include "GrIndexBuffer.h"
#include "GrTextStrike.h"
#include "GrTextStrike_impl.h"
#include "SkPath.h"
#include "SkStrokeRec.h"



static const int kGlyphMaskStage = GrPaint::kTotalStages;
static const int kGlyphCoordsAttributeIndex = 1;

void GrTextContext::flushGlyphs() {
    if (NULL == fDrawTarget) {
        return;
    }
    GrDrawState* drawState = fDrawTarget->drawState();
    if (fCurrVertex > 0) {
        
        GrAssert(GrIsALIGN4(fCurrVertex));
        GrAssert(fCurrTexture);
        GrTextureParams params(SkShader::kRepeat_TileMode, false);

        
        drawState->setEffect(kGlyphMaskStage,
                             GrSimpleTextureEffect::CreateWithCustomCoords(fCurrTexture, params),
                             kGlyphCoordsAttributeIndex)->unref();

        if (!GrPixelConfigIsAlphaOnly(fCurrTexture->config())) {
            if (kOne_GrBlendCoeff != fPaint.getSrcBlendCoeff() ||
                kISA_GrBlendCoeff != fPaint.getDstBlendCoeff() ||
                fPaint.hasColorStage()) {
                GrPrintf("LCD Text will not draw correctly.\n");
            }
            
            drawState->setBlendConstant(fPaint.getColor());
            drawState->setBlendFunc(kConstC_GrBlendCoeff, kISC_GrBlendCoeff);
            
            
            drawState->setColor(0xffffffff);
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
        GrSafeSetNull(fCurrTexture);
    }
    drawState->disableStages();
    fDrawTarget = NULL;
}

GrTextContext::GrTextContext(GrContext* context, const GrPaint& paint) : fPaint(paint) {
    fContext = context;
    fStrike = NULL;

    fCurrTexture = NULL;
    fCurrVertex = 0;

    const GrClipData* clipData = context->getClip();

    GrRect devConservativeBound;
    clipData->fClipStack->getConservativeBounds(
                                     -clipData->fOrigin.fX,
                                     -clipData->fOrigin.fY,
                                     context->getRenderTarget()->width(),
                                     context->getRenderTarget()->height(),
                                     &devConservativeBound);

    devConservativeBound.roundOut(&fClipRect);

    fAutoMatrix.setIdentity(fContext, &fPaint);

    fDrawTarget = NULL;

    fVertices = NULL;
    fMaxVertices = 0;
}

GrTextContext::~GrTextContext() {
    this->flushGlyphs();
    if (fDrawTarget) {
        fDrawTarget->drawState()->disableStages();
    }
}

void GrTextContext::flush() {
    this->flushGlyphs();
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

    vx += SkIntToFixed(glyph->fBounds.fLeft);
    vy += SkIntToFixed(glyph->fBounds.fTop);

    
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

        GrContext::AutoMatrix am;
        SkMatrix translate;
        translate.setTranslate(SkFixedToScalar(vx - SkIntToFixed(glyph->fBounds.fLeft)),
                               SkFixedToScalar(vy - SkIntToFixed(glyph->fBounds.fTop)));
        GrPaint tmpPaint(fPaint);
        am.setPreConcat(fContext, translate, &tmpPaint);
        SkStrokeRec stroke(SkStrokeRec::kFill_InitStyle);
        fContext->drawPath(tmpPaint, *glyph->fPath, stroke);
        return;
    }

HAS_ATLAS:
    GrAssert(glyph->fAtlas);

    
    width = SkIntToFixed(width);
    height = SkIntToFixed(height);

    GrTexture* texture = glyph->fAtlas->texture();
    GrAssert(texture);

    if (fCurrTexture != texture || fCurrVertex + 4 > fMaxVertices) {
        this->flushGlyphs();
        fCurrTexture = texture;
        fCurrTexture->ref();
    }

    if (NULL == fVertices) {
        
        static const GrVertexAttrib kVertexAttribs[] = {
            {kVec2f_GrVertexAttribType, 0,               kPosition_GrVertexAttribBinding},
            {kVec2f_GrVertexAttribType, sizeof(GrPoint), kEffect_GrVertexAttribBinding}
        };

       
        
        fMaxVertices = kMinRequestedVerts;
        bool flush = false;
        fDrawTarget = fContext->getTextTarget(fPaint);
        if (NULL != fDrawTarget) {
            fDrawTarget->drawState()->setVertexAttribs(kVertexAttribs, SK_ARRAY_COUNT(kVertexAttribs));
            flush = fDrawTarget->geometryHints(&fMaxVertices, NULL);
        }
        if (flush) {
            this->flushGlyphs();
            fContext->flush();
            
            fDrawTarget = fContext->getTextTarget(fPaint);
            fDrawTarget->drawState()->setVertexAttribs(kVertexAttribs, SK_ARRAY_COUNT(kVertexAttribs));
        }
        fMaxVertices = kDefaultRequestedVerts;
        
        fDrawTarget->geometryHints(&fMaxVertices, NULL);

        int maxQuadVertices = 4 * fContext->getQuadIndexBuffer()->maxQuads();
        if (fMaxVertices < kMinRequestedVerts) {
            fMaxVertices = kDefaultRequestedVerts;
        } else if (fMaxVertices > maxQuadVertices) {
            
            fMaxVertices = maxQuadVertices;
        }
        bool success = fDrawTarget->reserveVertexAndIndexSpace(
                                                   fMaxVertices,
                                                   0,
                                                   GrTCast<void**>(&fVertices),
                                                   NULL);
        GrAlwaysAssert(success);
        GrAssert(2*sizeof(GrPoint) == fDrawTarget->getDrawState().getVertexSize());
    }

    GrFixed tx = SkIntToFixed(glyph->fAtlasLocation.fX);
    GrFixed ty = SkIntToFixed(glyph->fAtlasLocation.fY);

    fVertices[2*fCurrVertex].setRectFan(SkFixedToFloat(vx),
                                        SkFixedToFloat(vy),
                                        SkFixedToFloat(vx + width),
                                        SkFixedToFloat(vy + height),
                                        2 * sizeof(SkPoint));
    fVertices[2*fCurrVertex+1].setRectFan(SkFixedToFloat(texture->normalizeFixedX(tx)),
                                          SkFixedToFloat(texture->normalizeFixedY(ty)),
                                          SkFixedToFloat(texture->normalizeFixedX(tx + width)),
                                          SkFixedToFloat(texture->normalizeFixedY(ty + height)),
                                          2 * sizeof(SkPoint));
    fCurrVertex += 4;
}
