






#include "GrStencilAndCoverTextContext.h"
#include "GrDrawTarget.h"
#include "GrGpu.h"
#include "GrPath.h"
#include "GrPathRange.h"
#include "SkAutoKern.h"
#include "SkDraw.h"
#include "SkDrawProcs.h"
#include "SkGlyphCache.h"
#include "SkGpuDevice.h"
#include "SkPath.h"
#include "SkTextMapStateProc.h"

class GrStencilAndCoverTextContext::GlyphPathRange : public GrGpuResource {
    static const int kMaxGlyphCount = 1 << 16; 
    static const int kGlyphGroupSize = 16; 

public:
    static GlyphPathRange* Create(GrContext* context,
                                  SkGlyphCache* cache,
                                  const SkStrokeRec& stroke) {
        static const GrCacheID::Domain gGlyphPathRangeDomain = GrCacheID::GenerateDomain();

        GrCacheID::Key key;
        key.fData32[0] = cache->getDescriptor().getChecksum();
        key.fData32[1] = cache->getScalerContext()->getTypeface()->uniqueID();
        key.fData64[1] = GrPath::ComputeStrokeKey(stroke);

        GrResourceKey resourceKey(GrCacheID(gGlyphPathRangeDomain, key),
                                  GrPathRange::resourceType(), 0);
        SkAutoTUnref<GlyphPathRange> glyphs(
            static_cast<GlyphPathRange*>(context->findAndRefCachedResource(resourceKey)));

        if (NULL == glyphs ||
            !glyphs->fDesc->equals(cache->getDescriptor() )) {
            glyphs.reset(SkNEW_ARGS(GlyphPathRange, (context, cache->getDescriptor(), stroke)));
            context->addResourceToCache(resourceKey, glyphs);
        }

        return glyphs.detach();
    }

    const GrPathRange* pathRange() const { return fPathRange.get(); }

    void preloadGlyph(uint16_t glyphID, SkGlyphCache* cache) {
        const uint16_t groupIndex = glyphID / kGlyphGroupSize;
        const uint16_t groupByte = groupIndex >> 3;
        const uint8_t groupBit = 1 << (groupIndex & 7);

        const bool hasGlyph = 0 != (fLoadedGlyphs[groupByte] & groupBit);
        if (hasGlyph) {
            return;
        }

        
        
        const uint16_t groupFirstID = groupIndex * kGlyphGroupSize;
        const uint16_t groupLastID = groupFirstID + kGlyphGroupSize - 1;
        SkPath skPath;
        for (int id = groupFirstID; id <= groupLastID; ++id) {
            const SkGlyph& skGlyph = cache->getGlyphIDMetrics(id);
            if (const SkPath* skPath = cache->findPath(skGlyph)) {
                fPathRange->initAt(id, *skPath);
            } 
        }

        fLoadedGlyphs[groupByte] |= groupBit;
        this->didChangeGpuMemorySize();
    }

    
    virtual size_t gpuMemorySize() const SK_OVERRIDE { return fPathRange->gpuMemorySize(); }

private:
    GlyphPathRange(GrContext* context, const SkDescriptor& desc, const SkStrokeRec& stroke)
        : INHERITED(context->getGpu(), false)
        , fDesc(desc.copy())
        
        
        
        , fPathRange(context->getGpu()->createPathRange(kMaxGlyphCount, stroke)) {
        memset(fLoadedGlyphs, 0, sizeof(fLoadedGlyphs));
    }

    ~GlyphPathRange() {
        this->release();
        SkDescriptor::Free(fDesc);
    }

    virtual void onRelease() SK_OVERRIDE {
        INHERITED::onRelease();
        fPathRange.reset(NULL);
    }

    virtual void onAbandon() SK_OVERRIDE {
        INHERITED::onAbandon();
        fPathRange->abandon();
        fPathRange.reset(NULL);
    }


    static const int kMaxGroupCount = (kMaxGlyphCount + (kGlyphGroupSize - 1)) / kGlyphGroupSize;
    SkDescriptor* const fDesc;
    uint8_t fLoadedGlyphs[(kMaxGroupCount + 7) >> 3]; 
    SkAutoTUnref<GrPathRange> fPathRange;

    typedef GrGpuResource INHERITED;
};


GrStencilAndCoverTextContext::GrStencilAndCoverTextContext(
    GrContext* context, const SkDeviceProperties& properties)
    : GrTextContext(context, properties)
    , fStroke(SkStrokeRec::kFill_InitStyle)
    , fPendingGlyphCount(0) {
}

GrStencilAndCoverTextContext::~GrStencilAndCoverTextContext() {
}

void GrStencilAndCoverTextContext::drawText(const GrPaint& paint,
                                            const SkPaint& skPaint,
                                            const char text[],
                                            size_t byteLength,
                                            SkScalar x, SkScalar y) {
    SkASSERT(byteLength == 0 || text != NULL);

    if (text == NULL || byteLength == 0 ) {
        return;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    this->init(paint, skPaint, byteLength, kUseIfNeeded_DeviceSpaceGlyphsBehavior);

    SkMatrix* glyphCacheTransform = NULL;
    
    if (fNeedsDeviceSpaceGlyphs) {
        SkPoint loc;
        fContextInitialMatrix.mapXY(x, y, &loc);
        x = loc.fX;
        y = loc.fY;
        glyphCacheTransform = &fContextInitialMatrix;
    }

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();
    SkAutoGlyphCache autoCache(fSkPaint, &fDeviceProperties, glyphCacheTransform);
    fGlyphCache = autoCache.getCache();
    fGlyphs = GlyphPathRange::Create(fContext, fGlyphCache, fStroke);
    fTransformType = GrDrawTarget::kTranslate_PathTransformType;

    const char* stop = text + byteLength;

    
    if (fSkPaint.getTextAlign() != SkPaint::kLeft_Align) {
        SkFixed    stopX = 0;
        SkFixed    stopY = 0;

        const char* textPtr = text;
        while (textPtr < stop) {
            
            
            const SkGlyph& glyph = glyphCacheProc(fGlyphCache, &textPtr, 0, 0);

            stopX += glyph.fAdvanceX;
            stopY += glyph.fAdvanceY;
        }
        SkASSERT(textPtr == stop);

        SkScalar alignX = SkFixedToScalar(stopX) * fTextRatio;
        SkScalar alignY = SkFixedToScalar(stopY) * fTextRatio;

        if (fSkPaint.getTextAlign() == SkPaint::kCenter_Align) {
            alignX = SkScalarHalf(alignX);
            alignY = SkScalarHalf(alignY);
        }

        x -= alignX;
        y -= alignY;
    }

    SkAutoKern autokern;

    SkFixed fixedSizeRatio = SkScalarToFixed(fTextRatio);

    SkFixed fx = SkScalarToFixed(x);
    SkFixed fy = SkScalarToFixed(y);
    while (text < stop) {
        const SkGlyph& glyph = glyphCacheProc(fGlyphCache, &text, 0, 0);
        fx += SkFixedMul_portable(autokern.adjust(glyph), fixedSizeRatio);
        if (glyph.fWidth) {
            this->appendGlyph(glyph.getGlyphID(), SkFixedToScalar(fx), SkFixedToScalar(fy));
        }

        fx += SkFixedMul_portable(glyph.fAdvanceX, fixedSizeRatio);
        fy += SkFixedMul_portable(glyph.fAdvanceY, fixedSizeRatio);
    }

    this->finish();
}

void GrStencilAndCoverTextContext::drawPosText(const GrPaint& paint,
                                               const SkPaint& skPaint,
                                               const char text[],
                                               size_t byteLength,
                                               const SkScalar pos[],
                                               SkScalar constY,
                                               int scalarsPerPosition) {
    SkASSERT(byteLength == 0 || text != NULL);
    SkASSERT(1 == scalarsPerPosition || 2 == scalarsPerPosition);

    
    if (text == NULL || byteLength == 0) {
        return;
    }

    
    
    
    
    
    
    

    const float textTranslateY = (1 == scalarsPerPosition ? constY : 0);
    this->init(paint, skPaint, byteLength, kDoNotUse_DeviceSpaceGlyphsBehavior, textTranslateY);

    SkDrawCacheProc glyphCacheProc = fSkPaint.getDrawCacheProc();

    SkAutoGlyphCache autoCache(fSkPaint, &fDeviceProperties, NULL);
    fGlyphCache = autoCache.getCache();
    fGlyphs = GlyphPathRange::Create(fContext, fGlyphCache, fStroke);

    const char* stop = text + byteLength;

    if (SkPaint::kLeft_Align == fSkPaint.getTextAlign()) {
        if (1 == scalarsPerPosition) {
            fTransformType = GrDrawTarget::kTranslateX_PathTransformType;
            while (text < stop) {
                const SkGlyph& glyph = glyphCacheProc(fGlyphCache, &text, 0, 0);
                if (glyph.fWidth) {
                    this->appendGlyph(glyph.getGlyphID(), *pos);
                }
                pos++;
            }
        } else {
            SkASSERT(2 == scalarsPerPosition);
            fTransformType = GrDrawTarget::kTranslate_PathTransformType;
            while (text < stop) {
                const SkGlyph& glyph = glyphCacheProc(fGlyphCache, &text, 0, 0);
                if (glyph.fWidth) {
                    this->appendGlyph(glyph.getGlyphID(), pos[0], pos[1]);
                }
                pos += 2;
            }
        }
    } else {
        fTransformType = GrDrawTarget::kTranslate_PathTransformType;
        SkTextMapStateProc tmsProc(SkMatrix::I(), 0, scalarsPerPosition);
        SkTextAlignProcScalar alignProc(fSkPaint.getTextAlign());
        while (text < stop) {
            const SkGlyph& glyph = glyphCacheProc(fGlyphCache, &text, 0, 0);
            if (glyph.fWidth) {
                SkPoint tmsLoc;
                tmsProc(pos, &tmsLoc);
                SkPoint loc;
                alignProc(tmsLoc, glyph, &loc);

                this->appendGlyph(glyph.getGlyphID(), loc.x(), loc.y());
            }
            pos += scalarsPerPosition;
        }
    }

    this->finish();
}

bool GrStencilAndCoverTextContext::canDraw(const SkPaint& paint) {
    if (paint.getRasterizer()) {
        return false;
    }
    if (paint.getMaskFilter()) {
        return false;
    }
    if (paint.getPathEffect()) {
        return false;
    }

    
    if (paint.getStyle() == SkPaint::kStroke_Style
        && paint.getStrokeWidth() == 0
        && fContext->getMatrix().hasPerspective()) {
        return false;
    }

    
    SkScalerContext::Rec    rec;
    SkScalerContext::MakeRec(paint, &fDeviceProperties, NULL, &rec);
    return rec.getFormat() != SkMask::kARGB32_Format;
}

void GrStencilAndCoverTextContext::init(const GrPaint& paint,
                                        const SkPaint& skPaint,
                                        size_t textByteLength,
                                        DeviceSpaceGlyphsBehavior deviceSpaceGlyphsBehavior,
                                        SkScalar textTranslateY) {
    GrTextContext::init(paint, skPaint);

    fContextInitialMatrix = fContext->getMatrix();

    bool otherBackendsWillDrawAsPaths =
        SkDraw::ShouldDrawTextAsPaths(skPaint, fContextInitialMatrix);

    if (otherBackendsWillDrawAsPaths) {
        
        fSkPaint.setLinearText(true);
        fTextRatio = fSkPaint.getTextSize() / SkPaint::kCanonicalTextSizeForPaths;
        fTextInverseRatio = SkPaint::kCanonicalTextSizeForPaths / fSkPaint.getTextSize();
        fSkPaint.setTextSize(SkIntToScalar(SkPaint::kCanonicalTextSizeForPaths));
        if (fSkPaint.getStyle() != SkPaint::kFill_Style) {
            
            
            fSkPaint.setStrokeWidth(fSkPaint.getStrokeWidth() / fTextRatio);
        }
        fNeedsDeviceSpaceGlyphs = false;
    } else {
        fTextRatio = fTextInverseRatio = 1.0f;
        fNeedsDeviceSpaceGlyphs =
            kUseIfNeeded_DeviceSpaceGlyphsBehavior == deviceSpaceGlyphsBehavior &&
            (fContextInitialMatrix.getType() &
                (SkMatrix::kScale_Mask | SkMatrix::kAffine_Mask)) != 0;
        
        SkASSERT(!fContextInitialMatrix.hasPerspective());
    }

    fStroke = SkStrokeRec(fSkPaint);

    if (fNeedsDeviceSpaceGlyphs) {
        SkASSERT(1.0f == fTextRatio);
        SkASSERT(0.0f == textTranslateY);
        fPaint.localCoordChangeInverse(fContextInitialMatrix);
        fContext->setIdentityMatrix();

        
        
        fStroke.setStrokeStyle(-1, false);
    } else {
        if (1.0f != fTextRatio || 0.0f != textTranslateY) {
            SkMatrix textMatrix;
            textMatrix.setTranslate(0, textTranslateY);
            textMatrix.preScale(fTextRatio, fTextRatio);
            fPaint.localCoordChange(textMatrix);
            fContext->concatMatrix(textMatrix);
        }

        if (fSkPaint.getStrokeWidth() == 0.0f) {
            if (fSkPaint.getStyle() == SkPaint::kStrokeAndFill_Style) {
                fStroke.setStrokeStyle(-1, false);
            } else if (fSkPaint.getStyle() == SkPaint::kStroke_Style) {
                
                const SkMatrix& ctm = fContext->getMatrix();
                SkScalar strokeWidth = SK_Scalar1 /
                    (SkVector::Make(ctm.getScaleX(), ctm.getSkewY()).length());
                fStroke.setStrokeStyle(strokeWidth, false);
            }
        }

        
        
        fSkPaint.setStyle(SkPaint::kFill_Style);
    }
    fStateRestore.set(fDrawTarget->drawState());

    fDrawTarget->drawState()->setFromPaint(fPaint, fContext->getMatrix(),
                                           fContext->getRenderTarget());

    GR_STATIC_CONST_SAME_STENCIL(kStencilPass,
                                 kZero_StencilOp,
                                 kZero_StencilOp,
                                 kNotEqual_StencilFunc,
                                 0xffff,
                                 0x0000,
                                 0xffff);

    *fDrawTarget->drawState()->stencil() = kStencilPass;

    SkASSERT(0 == fPendingGlyphCount);
}

inline void GrStencilAndCoverTextContext::appendGlyph(uint16_t glyphID, float x) {
    SkASSERT(GrDrawTarget::kTranslateX_PathTransformType == fTransformType);

    if (fPendingGlyphCount >= kGlyphBufferSize) {
        this->flush();
    }

    fGlyphs->preloadGlyph(glyphID, fGlyphCache);

    fIndexBuffer[fPendingGlyphCount] = glyphID;
    fTransformBuffer[fPendingGlyphCount] = fTextInverseRatio * x;

    ++fPendingGlyphCount;
}

inline void GrStencilAndCoverTextContext::appendGlyph(uint16_t glyphID, float x, float y) {
    SkASSERT(GrDrawTarget::kTranslate_PathTransformType == fTransformType);

    if (fPendingGlyphCount >= kGlyphBufferSize) {
        this->flush();
    }

    fGlyphs->preloadGlyph(glyphID, fGlyphCache);

    fIndexBuffer[fPendingGlyphCount] = glyphID;
    fTransformBuffer[2 * fPendingGlyphCount] = fTextInverseRatio * x;
    fTransformBuffer[2 * fPendingGlyphCount + 1] = fTextInverseRatio * y;

    ++fPendingGlyphCount;
}

void GrStencilAndCoverTextContext::flush() {
    if (0 == fPendingGlyphCount) {
        return;
    }

    fDrawTarget->drawPaths(fGlyphs->pathRange(), fIndexBuffer, fPendingGlyphCount,
                           fTransformBuffer, fTransformType, SkPath::kWinding_FillType);

    fPendingGlyphCount = 0;
}

void GrStencilAndCoverTextContext::finish() {
    this->flush();

    SkSafeUnref(fGlyphs);
    fGlyphs = NULL;
    fGlyphCache = NULL;

    fDrawTarget->drawState()->stencil()->setDisabled();
    fStateRestore.set(NULL);
    fContext->setMatrix(fContextInitialMatrix);
    GrTextContext::finish();
}

