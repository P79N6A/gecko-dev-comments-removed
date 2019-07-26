






#include "SkPDFDevice.h"

#include "SkAnnotation.h"
#include "SkColor.h"
#include "SkClipStack.h"
#include "SkData.h"
#include "SkDraw.h"
#include "SkFontHost.h"
#include "SkGlyphCache.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkPDFFont.h"
#include "SkPDFFormXObject.h"
#include "SkPDFGraphicState.h"
#include "SkPDFImage.h"
#include "SkPDFShader.h"
#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkPDFUtils.h"
#include "SkRect.h"
#include "SkString.h"
#include "SkTextFormatParams.h"
#include "SkTemplates.h"
#include "SkTypefacePriv.h"
#include "SkTSet.h"



static void emit_pdf_color(SkColor color, SkWStream* result) {
    SkASSERT(SkColorGetA(color) == 0xFF);  
    SkScalar colorMax = SkIntToScalar(0xFF);
    SkPDFScalar::Append(
            SkScalarDiv(SkIntToScalar(SkColorGetR(color)), colorMax), result);
    result->writeText(" ");
    SkPDFScalar::Append(
            SkScalarDiv(SkIntToScalar(SkColorGetG(color)), colorMax), result);
    result->writeText(" ");
    SkPDFScalar::Append(
            SkScalarDiv(SkIntToScalar(SkColorGetB(color)), colorMax), result);
    result->writeText(" ");
}

static SkPaint calculate_text_paint(const SkPaint& paint) {
    SkPaint result = paint;
    if (result.isFakeBoldText()) {
        SkScalar fakeBoldScale = SkScalarInterpFunc(result.getTextSize(),
                                                    kStdFakeBoldInterpKeys,
                                                    kStdFakeBoldInterpValues,
                                                    kStdFakeBoldInterpLength);
        SkScalar width = SkScalarMul(result.getTextSize(), fakeBoldScale);
        if (result.getStyle() == SkPaint::kFill_Style) {
            result.setStyle(SkPaint::kStrokeAndFill_Style);
        } else {
            width += result.getStrokeWidth();
        }
        result.setStrokeWidth(width);
    }
    return result;
}


static void align_text(SkDrawCacheProc glyphCacheProc, const SkPaint& paint,
                       const uint16_t* glyphs, size_t len,
                       SkScalar* x, SkScalar* y) {
    if (paint.getTextAlign() == SkPaint::kLeft_Align) {
        return;
    }

    SkMatrix ident;
    ident.reset();
    SkAutoGlyphCache autoCache(paint, NULL, &ident);
    SkGlyphCache* cache = autoCache.getCache();

    const char* start = reinterpret_cast<const char*>(glyphs);
    const char* stop = reinterpret_cast<const char*>(glyphs + len);
    SkFixed xAdv = 0, yAdv = 0;

    
    while (start < stop) {
        const SkGlyph& glyph = glyphCacheProc(cache, &start, 0, 0);
        xAdv += glyph.fAdvanceX;
        yAdv += glyph.fAdvanceY;
    };
    if (paint.getTextAlign() == SkPaint::kLeft_Align) {
        return;
    }

    SkScalar xAdj = SkFixedToScalar(xAdv);
    SkScalar yAdj = SkFixedToScalar(yAdv);
    if (paint.getTextAlign() == SkPaint::kCenter_Align) {
        xAdj = SkScalarHalf(xAdj);
        yAdj = SkScalarHalf(yAdj);
    }
    *x = *x - xAdj;
    *y = *y - yAdj;
}

static size_t max_glyphid_for_typeface(SkTypeface* typeface) {
    SkAutoResolveDefaultTypeface autoResolve(typeface);
    typeface = autoResolve.get();

    SkAdvancedTypefaceMetrics* metrics;
    metrics = typeface->getAdvancedTypefaceMetrics(
            SkAdvancedTypefaceMetrics::kNo_PerGlyphInfo,
            NULL, 0);

    int lastGlyphID = 0;
    if (metrics) {
        lastGlyphID = metrics->fLastGlyphID;
        metrics->unref();
    }
    return lastGlyphID;
}

typedef SkAutoSTMalloc<128, uint16_t> SkGlyphStorage;

static size_t force_glyph_encoding(const SkPaint& paint, const void* text,
                                   size_t len, SkGlyphStorage* storage,
                                   uint16_t** glyphIDs) {
    
    if (paint.getTextEncoding() != SkPaint::kGlyphID_TextEncoding) {
        size_t numGlyphs = paint.textToGlyphs(text, len, NULL);
        storage->reset(numGlyphs);
        paint.textToGlyphs(text, len, storage->get());
        *glyphIDs = storage->get();
        return numGlyphs;
    }

    
    SkASSERT((len & 1) == 0);
    size_t numGlyphs = len / 2;
    const uint16_t* input =
        reinterpret_cast<uint16_t*>(const_cast<void*>((text)));

    int maxGlyphID = max_glyphid_for_typeface(paint.getTypeface());
    size_t validated;
    for (validated = 0; validated < numGlyphs; ++validated) {
        if (input[validated] > maxGlyphID) {
            break;
        }
    }
    if (validated >= numGlyphs) {
        *glyphIDs = reinterpret_cast<uint16_t*>(const_cast<void*>((text)));
        return numGlyphs;
    }

    
    storage->reset(numGlyphs);
    if (validated > 0) {
        memcpy(storage->get(), input, validated * sizeof(uint16_t));
    }

    for (size_t i = validated; i < numGlyphs; ++i) {
        storage->get()[i] = input[i];
        if (input[i] > maxGlyphID) {
            storage->get()[i] = 0;
        }
    }
    *glyphIDs = storage->get();
    return numGlyphs;
}

static void set_text_transform(SkScalar x, SkScalar y, SkScalar textSkewX,
                               SkWStream* content) {
    
    
    content->writeText("1 0 ");
    SkPDFScalar::Append(0 - textSkewX, content);
    content->writeText(" -1 ");
    SkPDFScalar::Append(x, content);
    content->writeText(" ");
    SkPDFScalar::Append(y, content);
    content->writeText(" Tm\n");
}



struct GraphicStateEntry {
    GraphicStateEntry();

    
    bool compareInitialState(const GraphicStateEntry& b);

    SkMatrix fMatrix;
    
    
    
    
    SkClipStack fClipStack;
    SkRegion fClipRegion;

    
    
    SkColor fColor;
    SkScalar fTextScaleX;  
    SkPaint::Style fTextFill;  
    int fShaderIndex;
    int fGraphicStateIndex;

    
    
    SkPDFFont* fFont;
    
    
    SkScalar fTextSize;
};

GraphicStateEntry::GraphicStateEntry() : fColor(SK_ColorBLACK),
                                         fTextScaleX(SK_Scalar1),
                                         fTextFill(SkPaint::kFill_Style),
                                         fShaderIndex(-1),
                                         fGraphicStateIndex(-1),
                                         fFont(NULL),
                                         fTextSize(SK_ScalarNaN) {
    fMatrix.reset();
}

bool GraphicStateEntry::compareInitialState(const GraphicStateEntry& b) {
    return fColor == b.fColor &&
           fShaderIndex == b.fShaderIndex &&
           fGraphicStateIndex == b.fGraphicStateIndex &&
           fMatrix == b.fMatrix &&
           fClipStack == b.fClipStack &&
               (fTextScaleX == 0 ||
                b.fTextScaleX == 0 ||
                (fTextScaleX == b.fTextScaleX && fTextFill == b.fTextFill));
}

class GraphicStackState {
public:
    GraphicStackState(const SkClipStack& existingClipStack,
                      const SkRegion& existingClipRegion,
                      SkWStream* contentStream)
            : fStackDepth(0),
              fContentStream(contentStream) {
        fEntries[0].fClipStack = existingClipStack;
        fEntries[0].fClipRegion = existingClipRegion;
    }

    void updateClip(const SkClipStack& clipStack, const SkRegion& clipRegion,
                    const SkPoint& translation);
    void updateMatrix(const SkMatrix& matrix);
    void updateDrawingState(const GraphicStateEntry& state);

    void drainStack();

private:
    void push();
    void pop();
    GraphicStateEntry* currentEntry() { return &fEntries[fStackDepth]; }

    
    static const int kMaxStackDepth = 12;
    GraphicStateEntry fEntries[kMaxStackDepth + 1];
    int fStackDepth;
    SkWStream* fContentStream;
};

void GraphicStackState::drainStack() {
    while (fStackDepth) {
        pop();
    }
}

void GraphicStackState::push() {
    SkASSERT(fStackDepth < kMaxStackDepth);
    fContentStream->writeText("q\n");
    fStackDepth++;
    fEntries[fStackDepth] = fEntries[fStackDepth - 1];
}

void GraphicStackState::pop() {
    SkASSERT(fStackDepth > 0);
    fContentStream->writeText("Q\n");
    fStackDepth--;
}




static void skip_clip_stack_prefix(const SkClipStack& prefix,
                                   const SkClipStack& stack,
                                   SkClipStack::Iter* iter) {
    SkClipStack::B2TIter prefixIter(prefix);
    iter->reset(stack, SkClipStack::Iter::kBottom_IterStart);

    const SkClipStack::Element* prefixEntry;
    const SkClipStack::Element* iterEntry;

    for (prefixEntry = prefixIter.next(); prefixEntry;
            prefixEntry = prefixIter.next()) {
        iterEntry = iter->next();
        SkASSERT(iterEntry);
        
        
        if (*prefixEntry != *iterEntry) {
            SkASSERT(prefixEntry->getOp() == SkRegion::kIntersect_Op);
            SkASSERT(iterEntry->getOp() == SkRegion::kIntersect_Op);
            SkASSERT(iterEntry->getType() == prefixEntry->getType());
            
            iter->prev();
            prefixEntry = prefixIter.next();
            break;
        }
    }

    SkASSERT(prefixEntry == NULL);
}

static void emit_clip(SkPath* clipPath, SkRect* clipRect,
                      SkWStream* contentStream) {
    SkASSERT(clipPath || clipRect);

    SkPath::FillType clipFill;
    if (clipPath) {
        SkPDFUtils::EmitPath(*clipPath, SkPaint::kFill_Style, contentStream);
        clipFill = clipPath->getFillType();
    } else {
        SkPDFUtils::AppendRectangle(*clipRect, contentStream);
        clipFill = SkPath::kWinding_FillType;
    }

    NOT_IMPLEMENTED(clipFill == SkPath::kInverseEvenOdd_FillType, false);
    NOT_IMPLEMENTED(clipFill == SkPath::kInverseWinding_FillType, false);
    if (clipFill == SkPath::kEvenOdd_FillType) {
        contentStream->writeText("W* n\n");
    } else {
        contentStream->writeText("W n\n");
    }
}




void GraphicStackState::updateClip(const SkClipStack& clipStack,
                                   const SkRegion& clipRegion,
                                   const SkPoint& translation) {
    if (clipStack == currentEntry()->fClipStack) {
        return;
    }

    while (fStackDepth > 0) {
        pop();
        if (clipStack == currentEntry()->fClipStack) {
            return;
        }
    }
    push();

    
    
    
    
    
    
    
    
    SkClipStack::Iter iter;
    skip_clip_stack_prefix(fEntries[0].fClipStack, clipStack, &iter);

    
    
    bool needRegion = false;
    const SkClipStack::Element* clipEntry;
    for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
        if (clipEntry->getOp() != SkRegion::kIntersect_Op || clipEntry->isInverseFilled()) {
            needRegion = true;
            break;
        }
    }

    if (needRegion) {
        SkPath clipPath;
        SkAssertResult(clipRegion.getBoundaryPath(&clipPath));
        emit_clip(&clipPath, NULL, fContentStream);
    } else {
        skip_clip_stack_prefix(fEntries[0].fClipStack, clipStack, &iter);
        SkMatrix transform;
        transform.setTranslate(translation.fX, translation.fY);
        const SkClipStack::Element* clipEntry;
        for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
            SkASSERT(clipEntry->getOp() == SkRegion::kIntersect_Op);
            switch (clipEntry->getType()) {
                case SkClipStack::Element::kRect_Type: {
                    SkRect translatedClip;
                    transform.mapRect(&translatedClip, clipEntry->getRect());
                    emit_clip(NULL, &translatedClip, fContentStream);
                    break;
                }
                case SkClipStack::Element::kPath_Type: {
                    SkPath translatedPath;
                    clipEntry->getPath().transform(transform, &translatedPath);
                    emit_clip(&translatedPath, NULL, fContentStream);
                    break;
                }
                default:
                    SkASSERT(false);
            }
        }
    }
    currentEntry()->fClipStack = clipStack;
    currentEntry()->fClipRegion = clipRegion;
}

void GraphicStackState::updateMatrix(const SkMatrix& matrix) {
    if (matrix == currentEntry()->fMatrix) {
        return;
    }

    if (currentEntry()->fMatrix.getType() != SkMatrix::kIdentity_Mask) {
        SkASSERT(fStackDepth > 0);
        SkASSERT(fEntries[fStackDepth].fClipStack ==
                 fEntries[fStackDepth -1].fClipStack);
        pop();

        SkASSERT(currentEntry()->fMatrix.getType() == SkMatrix::kIdentity_Mask);
    }
    if (matrix.getType() == SkMatrix::kIdentity_Mask) {
        return;
    }

    push();
    SkPDFUtils::AppendTransform(matrix, fContentStream);
    currentEntry()->fMatrix = matrix;
}

void GraphicStackState::updateDrawingState(const GraphicStateEntry& state) {
    
    if (state.fShaderIndex >= 0) {
        if (state.fShaderIndex != currentEntry()->fShaderIndex) {
            fContentStream->writeText("/Pattern CS /Pattern cs /P");
            fContentStream->writeDecAsText(state.fShaderIndex);
            fContentStream->writeText(" SCN /P");
            fContentStream->writeDecAsText(state.fShaderIndex);
            fContentStream->writeText(" scn\n");
            currentEntry()->fShaderIndex = state.fShaderIndex;
        }
    } else {
        if (state.fColor != currentEntry()->fColor ||
                currentEntry()->fShaderIndex >= 0) {
            emit_pdf_color(state.fColor, fContentStream);
            fContentStream->writeText("RG ");
            emit_pdf_color(state.fColor, fContentStream);
            fContentStream->writeText("rg\n");
            currentEntry()->fColor = state.fColor;
            currentEntry()->fShaderIndex = -1;
        }
    }

    if (state.fGraphicStateIndex != currentEntry()->fGraphicStateIndex) {
        SkPDFUtils::ApplyGraphicState(state.fGraphicStateIndex, fContentStream);
        currentEntry()->fGraphicStateIndex = state.fGraphicStateIndex;
    }

    if (state.fTextScaleX) {
        if (state.fTextScaleX != currentEntry()->fTextScaleX) {
            SkScalar pdfScale = SkScalarMul(state.fTextScaleX,
                                            SkIntToScalar(100));
            SkPDFScalar::Append(pdfScale, fContentStream);
            fContentStream->writeText(" Tz\n");
            currentEntry()->fTextScaleX = state.fTextScaleX;
        }
        if (state.fTextFill != currentEntry()->fTextFill) {
            SK_COMPILE_ASSERT(SkPaint::kFill_Style == 0, enum_must_match_value);
            SK_COMPILE_ASSERT(SkPaint::kStroke_Style == 1,
                              enum_must_match_value);
            SK_COMPILE_ASSERT(SkPaint::kStrokeAndFill_Style == 2,
                              enum_must_match_value);
            fContentStream->writeDecAsText(state.fTextFill);
            fContentStream->writeText(" Tr\n");
            currentEntry()->fTextFill = state.fTextFill;
        }
    }
}

SkDevice* SkPDFDevice::onCreateCompatibleDevice(SkBitmap::Config config,
                                                int width, int height,
                                                bool isOpaque,
                                                Usage usage) {
    SkMatrix initialTransform;
    initialTransform.reset();
    SkISize size = SkISize::Make(width, height);
    return SkNEW_ARGS(SkPDFDevice, (size, size, initialTransform));
}


struct ContentEntry {
    GraphicStateEntry fState;
    SkDynamicMemoryWStream fContent;
    SkTScopedPtr<ContentEntry> fNext;

    
    
    ~ContentEntry() {
        ContentEntry* val = fNext.release();
        while (val != NULL) {
            ContentEntry* valNext = val->fNext.release();
            
            delete val;
            val = valNext;
        }
    }
};



class ScopedContentEntry {
public:
    ScopedContentEntry(SkPDFDevice* device, const SkDraw& draw,
                       const SkPaint& paint, bool hasText = false)
        : fDevice(device),
          fContentEntry(NULL),
          fXfermode(SkXfermode::kSrcOver_Mode) {
        init(draw.fClipStack, *draw.fClip, *draw.fMatrix, paint, hasText);
    }
    ScopedContentEntry(SkPDFDevice* device, const SkClipStack* clipStack,
                       const SkRegion& clipRegion, const SkMatrix& matrix,
                       const SkPaint& paint, bool hasText = false)
        : fDevice(device),
          fContentEntry(NULL),
          fXfermode(SkXfermode::kSrcOver_Mode) {
        init(clipStack, clipRegion, matrix, paint, hasText);
    }

    ~ScopedContentEntry() {
        if (fContentEntry) {
            fDevice->finishContentEntry(fXfermode, fDstFormXObject);
        }
        SkSafeUnref(fDstFormXObject);
    }

    ContentEntry* entry() { return fContentEntry; }
private:
    SkPDFDevice* fDevice;
    ContentEntry* fContentEntry;
    SkXfermode::Mode fXfermode;
    SkPDFFormXObject* fDstFormXObject;

    void init(const SkClipStack* clipStack, const SkRegion& clipRegion,
              const SkMatrix& matrix, const SkPaint& paint, bool hasText) {
        fDstFormXObject = NULL;
        if (paint.getXfermode()) {
            paint.getXfermode()->asMode(&fXfermode);
        }
        fContentEntry = fDevice->setUpContentEntry(clipStack, clipRegion,
                                                   matrix, paint, hasText,
                                                   &fDstFormXObject);
    }
};



static inline SkBitmap makeContentBitmap(const SkISize& contentSize,
                                         const SkMatrix* initialTransform) {
    SkBitmap bitmap;
    if (initialTransform) {
        
        SkVector drawingSize;
        SkMatrix inverse;
        drawingSize.set(SkIntToScalar(contentSize.fWidth),
                        SkIntToScalar(contentSize.fHeight));
        if (!initialTransform->invert(&inverse)) {
            
            SkASSERT(false);
            inverse.reset();
        }
        inverse.mapVectors(&drawingSize, 1);
        SkISize size = SkSize::Make(drawingSize.fX, drawingSize.fY).toRound();
        bitmap.setConfig(SkBitmap::kNo_Config, abs(size.fWidth),
                         abs(size.fHeight));
    } else {
        bitmap.setConfig(SkBitmap::kNo_Config, abs(contentSize.fWidth),
                         abs(contentSize.fHeight));
    }

    return bitmap;
}


SkPDFDevice::SkPDFDevice(const SkISize& pageSize, const SkISize& contentSize,
                         const SkMatrix& initialTransform)
    : SkDevice(makeContentBitmap(contentSize, &initialTransform)),
      fPageSize(pageSize),
      fContentSize(contentSize),
      fLastContentEntry(NULL),
      fLastMarginContentEntry(NULL),
      fClipStack(NULL) {
    
    
    
    fInitialTransform.setTranslate(0, SkIntToScalar(pageSize.fHeight));
    fInitialTransform.preScale(SK_Scalar1, -SK_Scalar1);
    fInitialTransform.preConcat(initialTransform);

    SkIRect existingClip = SkIRect::MakeWH(this->width(), this->height());
    fExistingClipRegion.setRect(existingClip);

    this->init();
}


SkPDFDevice::SkPDFDevice(const SkISize& layerSize,
                         const SkClipStack& existingClipStack,
                         const SkRegion& existingClipRegion)
    : SkDevice(makeContentBitmap(layerSize, NULL)),
      fPageSize(layerSize),
      fContentSize(layerSize),
      fExistingClipStack(existingClipStack),
      fExistingClipRegion(existingClipRegion),
      fLastContentEntry(NULL),
      fLastMarginContentEntry(NULL),
      fClipStack(NULL) {
    fInitialTransform.reset();
    this->init();
}

SkPDFDevice::~SkPDFDevice() {
    this->cleanUp(true);
}

void SkPDFDevice::init() {
    fAnnotations = NULL;
    fResourceDict = NULL;
    fContentEntries.reset();
    fLastContentEntry = NULL;
    fMarginContentEntries.reset();
    fLastMarginContentEntry = NULL;
    fDrawingArea = kContent_DrawingArea;
    if (fFontGlyphUsage == NULL) {
        fFontGlyphUsage.reset(new SkPDFGlyphSetMap());
    }
}

void SkPDFDevice::cleanUp(bool clearFontUsage) {
    fGraphicStateResources.unrefAll();
    fXObjectResources.unrefAll();
    fFontResources.unrefAll();
    fShaderResources.unrefAll();
    SkSafeUnref(fAnnotations);
    SkSafeUnref(fResourceDict);
    fNamedDestinations.deleteAll();

    if (clearFontUsage) {
        fFontGlyphUsage->reset();
    }
}

uint32_t SkPDFDevice::getDeviceCapabilities() {
    return kVector_Capability;
}

void SkPDFDevice::clear(SkColor color) {
    this->cleanUp(true);
    this->init();

    SkPaint paint;
    paint.setColor(color);
    paint.setStyle(SkPaint::kFill_Style);
    SkMatrix identity;
    identity.reset();
    ScopedContentEntry content(this, &fExistingClipStack, fExistingClipRegion,
                               identity, paint);
    internalDrawPaint(paint, content.entry());
}

void SkPDFDevice::drawPaint(const SkDraw& d, const SkPaint& paint) {
    SkPaint newPaint = paint;
    newPaint.setStyle(SkPaint::kFill_Style);
    ScopedContentEntry content(this, d, newPaint);
    internalDrawPaint(newPaint, content.entry());
}

void SkPDFDevice::internalDrawPaint(const SkPaint& paint,
                                    ContentEntry* contentEntry) {
    if (!contentEntry) {
        return;
    }
    SkRect bbox = SkRect::MakeWH(SkIntToScalar(this->width()),
                                 SkIntToScalar(this->height()));
    SkMatrix totalTransform = fInitialTransform;
    totalTransform.preConcat(contentEntry->fState.fMatrix);
    SkMatrix inverse;
    if (!totalTransform.invert(&inverse)) {
        return;
    }
    inverse.mapRect(&bbox);

    SkPDFUtils::AppendRectangle(bbox, &contentEntry->fContent);
    SkPDFUtils::PaintPath(paint.getStyle(), SkPath::kWinding_FillType,
                          &contentEntry->fContent);
}

void SkPDFDevice::drawPoints(const SkDraw& d, SkCanvas::PointMode mode,
                             size_t count, const SkPoint* points,
                             const SkPaint& passedPaint) {
    if (count == 0) {
        return;
    }

    if (handlePointAnnotation(points, count, *d.fMatrix, passedPaint)) {
        return;
    }

    
    
    
    if (passedPaint.getPathEffect()) {
        if (d.fClip->isEmpty()) {
            return;
        }
        SkDraw pointDraw(d);
        pointDraw.fDevice = this;
        pointDraw.drawPoints(mode, count, points, passedPaint, true);
        return;
    }

    const SkPaint* paint = &passedPaint;
    SkPaint modifiedPaint;

    if (mode == SkCanvas::kPoints_PointMode &&
            paint->getStrokeCap() != SkPaint::kRound_Cap) {
        modifiedPaint = *paint;
        paint = &modifiedPaint;
        if (paint->getStrokeWidth()) {
            
            
            modifiedPaint.setStyle(SkPaint::kFill_Style);
            SkScalar strokeWidth = paint->getStrokeWidth();
            SkScalar halfStroke = SkScalarHalf(strokeWidth);
            for (size_t i = 0; i < count; i++) {
                SkRect r = SkRect::MakeXYWH(points[i].fX, points[i].fY, 0, 0);
                r.inset(-halfStroke, -halfStroke);
                drawRect(d, r, modifiedPaint);
            }
            return;
        } else {
            modifiedPaint.setStrokeCap(SkPaint::kRound_Cap);
        }
    }

    ScopedContentEntry content(this, d, *paint);
    if (!content.entry()) {
        return;
    }

    switch (mode) {
        case SkCanvas::kPolygon_PointMode:
            SkPDFUtils::MoveTo(points[0].fX, points[0].fY,
                               &content.entry()->fContent);
            for (size_t i = 1; i < count; i++) {
                SkPDFUtils::AppendLine(points[i].fX, points[i].fY,
                                       &content.entry()->fContent);
            }
            SkPDFUtils::StrokePath(&content.entry()->fContent);
            break;
        case SkCanvas::kLines_PointMode:
            for (size_t i = 0; i < count/2; i++) {
                SkPDFUtils::MoveTo(points[i * 2].fX, points[i * 2].fY,
                                   &content.entry()->fContent);
                SkPDFUtils::AppendLine(points[i * 2 + 1].fX,
                                       points[i * 2 + 1].fY,
                                       &content.entry()->fContent);
                SkPDFUtils::StrokePath(&content.entry()->fContent);
            }
            break;
        case SkCanvas::kPoints_PointMode:
            SkASSERT(paint->getStrokeCap() == SkPaint::kRound_Cap);
            for (size_t i = 0; i < count; i++) {
                SkPDFUtils::MoveTo(points[i].fX, points[i].fY,
                                   &content.entry()->fContent);
                SkPDFUtils::ClosePath(&content.entry()->fContent);
                SkPDFUtils::StrokePath(&content.entry()->fContent);
            }
            break;
        default:
            SkASSERT(false);
    }
}

void SkPDFDevice::drawRect(const SkDraw& d, const SkRect& r,
                           const SkPaint& paint) {
    if (paint.getPathEffect()) {
        if (d.fClip->isEmpty()) {
            return;
        }
        SkPath path;
        path.addRect(r);
        drawPath(d, path, paint, NULL, true);
        return;
    }

    if (handleRectAnnotation(r, *d.fMatrix, paint)) {
        return;
    }

    ScopedContentEntry content(this, d, paint);
    if (!content.entry()) {
        return;
    }
    SkPDFUtils::AppendRectangle(r, &content.entry()->fContent);
    SkPDFUtils::PaintPath(paint.getStyle(), SkPath::kWinding_FillType,
                          &content.entry()->fContent);
}

void SkPDFDevice::drawPath(const SkDraw& d, const SkPath& origPath,
                           const SkPaint& paint, const SkMatrix* prePathMatrix,
                           bool pathIsMutable) {
    SkPath modifiedPath;
    SkPath* pathPtr = const_cast<SkPath*>(&origPath);

    SkMatrix matrix = *d.fMatrix;
    if (prePathMatrix) {
        if (paint.getPathEffect() || paint.getStyle() != SkPaint::kFill_Style) {
            if (!pathIsMutable) {
                pathPtr = &modifiedPath;
                pathIsMutable = true;
            }
            origPath.transform(*prePathMatrix, pathPtr);
        } else {
            if (!matrix.preConcat(*prePathMatrix)) {
                return;
            }
        }
    }

    if (paint.getPathEffect()) {
        if (d.fClip->isEmpty()) {
            return;
        }
        if (!pathIsMutable) {
            pathPtr = &modifiedPath;
            pathIsMutable = true;
        }
        bool fill = paint.getFillPath(origPath, pathPtr);

        SkPaint noEffectPaint(paint);
        noEffectPaint.setPathEffect(NULL);
        if (fill) {
            noEffectPaint.setStyle(SkPaint::kFill_Style);
        } else {
            noEffectPaint.setStyle(SkPaint::kStroke_Style);
            noEffectPaint.setStrokeWidth(0);
        }
        drawPath(d, *pathPtr, noEffectPaint, NULL, true);
        return;
    }

    if (handleRectAnnotation(pathPtr->getBounds(), *d.fMatrix, paint)) {
        return;
    }

    ScopedContentEntry content(this, d, paint);
    if (!content.entry()) {
        return;
    }
    SkPDFUtils::EmitPath(*pathPtr, paint.getStyle(),
                         &content.entry()->fContent);
    SkPDFUtils::PaintPath(paint.getStyle(), pathPtr->getFillType(),
                          &content.entry()->fContent);
}

void SkPDFDevice::drawBitmapRect(const SkDraw& draw, const SkBitmap& bitmap,
                                 const SkRect* src, const SkRect& dst,
                                 const SkPaint& paint) {
    SkMatrix    matrix;
    SkRect      bitmapBounds, tmpSrc, tmpDst;
    SkBitmap    tmpBitmap;

    bitmapBounds.isetWH(bitmap.width(), bitmap.height());

    
    if (src) {
        tmpSrc = *src;
    } else {
        tmpSrc = bitmapBounds;
    }
    matrix.setRectToRect(tmpSrc, dst, SkMatrix::kFill_ScaleToFit);

    const SkBitmap* bitmapPtr = &bitmap;

    
    
    if (src) {
        if (!bitmapBounds.contains(*src)) {
            if (!tmpSrc.intersect(bitmapBounds)) {
                return; 
            }
            
            matrix.mapRect(&tmpDst, tmpSrc);
        }

        
        
        
        SkIRect srcIR;
        tmpSrc.roundOut(&srcIR);
        if (!bitmap.extractSubset(&tmpBitmap, srcIR)) {
            return;
        }
        bitmapPtr = &tmpBitmap;

        
        SkScalar dx = 0, dy = 0;
        if (srcIR.fLeft > 0) {
            dx = SkIntToScalar(srcIR.fLeft);
        }
        if (srcIR.fTop > 0) {
            dy = SkIntToScalar(srcIR.fTop);
        }
        if (dx || dy) {
            matrix.preTranslate(dx, dy);
        }
    }
    this->drawBitmap(draw, *bitmapPtr, NULL, matrix, paint);
}

void SkPDFDevice::drawBitmap(const SkDraw& d, const SkBitmap& bitmap,
                             const SkIRect* srcRect, const SkMatrix& matrix,
                             const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }

    SkMatrix transform = matrix;
    transform.postConcat(*d.fMatrix);
    internalDrawBitmap(transform, d.fClipStack, *d.fClip, bitmap, srcRect,
                       paint);
}

void SkPDFDevice::drawSprite(const SkDraw& d, const SkBitmap& bitmap,
                             int x, int y, const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }

    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(x), SkIntToScalar(y));
    internalDrawBitmap(matrix, d.fClipStack, *d.fClip, bitmap, NULL, paint);
}

void SkPDFDevice::drawText(const SkDraw& d, const void* text, size_t len,
                           SkScalar x, SkScalar y, const SkPaint& paint) {
    NOT_IMPLEMENTED(paint.getMaskFilter() != NULL, false);
    if (paint.getMaskFilter() != NULL) {
        
        
        return;
    }
    SkPaint textPaint = calculate_text_paint(paint);
    ScopedContentEntry content(this, d, textPaint, true);
    if (!content.entry()) {
        return;
    }

    SkGlyphStorage storage(0);
    uint16_t* glyphIDs = NULL;
    size_t numGlyphs = force_glyph_encoding(paint, text, len, &storage,
                                            &glyphIDs);
    textPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

    SkDrawCacheProc glyphCacheProc = textPaint.getDrawCacheProc();
    align_text(glyphCacheProc, textPaint, glyphIDs, numGlyphs, &x, &y);
    content.entry()->fContent.writeText("BT\n");
    set_text_transform(x, y, textPaint.getTextSkewX(),
                       &content.entry()->fContent);
    size_t consumedGlyphCount = 0;
    while (numGlyphs > consumedGlyphCount) {
        updateFont(textPaint, glyphIDs[consumedGlyphCount], content.entry());
        SkPDFFont* font = content.entry()->fState.fFont;
        size_t availableGlyphs =
            font->glyphsToPDFFontEncoding(glyphIDs + consumedGlyphCount,
                                          numGlyphs - consumedGlyphCount);
        fFontGlyphUsage->noteGlyphUsage(font, glyphIDs + consumedGlyphCount,
                                        availableGlyphs);
        SkString encodedString =
            SkPDFString::FormatString(glyphIDs + consumedGlyphCount,
                                      availableGlyphs, font->multiByteGlyphs());
        content.entry()->fContent.writeText(encodedString.c_str());
        consumedGlyphCount += availableGlyphs;
        content.entry()->fContent.writeText(" Tj\n");
    }
    content.entry()->fContent.writeText("ET\n");
}

void SkPDFDevice::drawPosText(const SkDraw& d, const void* text, size_t len,
                              const SkScalar pos[], SkScalar constY,
                              int scalarsPerPos, const SkPaint& paint) {
    NOT_IMPLEMENTED(paint.getMaskFilter() != NULL, false);
    if (paint.getMaskFilter() != NULL) {
        
        
        return;
    }
    SkASSERT(1 == scalarsPerPos || 2 == scalarsPerPos);
    SkPaint textPaint = calculate_text_paint(paint);
    ScopedContentEntry content(this, d, textPaint, true);
    if (!content.entry()) {
        return;
    }

    SkGlyphStorage storage(0);
    uint16_t* glyphIDs = NULL;
    size_t numGlyphs = force_glyph_encoding(paint, text, len, &storage,
                                            &glyphIDs);
    textPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

    SkDrawCacheProc glyphCacheProc = textPaint.getDrawCacheProc();
    content.entry()->fContent.writeText("BT\n");
    updateFont(textPaint, glyphIDs[0], content.entry());
    for (size_t i = 0; i < numGlyphs; i++) {
        SkPDFFont* font = content.entry()->fState.fFont;
        uint16_t encodedValue = glyphIDs[i];
        if (font->glyphsToPDFFontEncoding(&encodedValue, 1) != 1) {
            updateFont(textPaint, glyphIDs[i], content.entry());
            i--;
            continue;
        }
        fFontGlyphUsage->noteGlyphUsage(font, &encodedValue, 1);
        SkScalar x = pos[i * scalarsPerPos];
        SkScalar y = scalarsPerPos == 1 ? constY : pos[i * scalarsPerPos + 1];
        align_text(glyphCacheProc, textPaint, glyphIDs + i, 1, &x, &y);
        set_text_transform(x, y, textPaint.getTextSkewX(),
                           &content.entry()->fContent);
        SkString encodedString =
            SkPDFString::FormatString(&encodedValue, 1,
                                      font->multiByteGlyphs());
        content.entry()->fContent.writeText(encodedString.c_str());
        content.entry()->fContent.writeText(" Tj\n");
    }
    content.entry()->fContent.writeText("ET\n");
}

void SkPDFDevice::drawTextOnPath(const SkDraw& d, const void* text, size_t len,
                                 const SkPath& path, const SkMatrix* matrix,
                                 const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }
    d.drawTextOnPath((const char*)text, len, path, matrix, paint);
}

void SkPDFDevice::drawVertices(const SkDraw& d, SkCanvas::VertexMode,
                               int vertexCount, const SkPoint verts[],
                               const SkPoint texs[], const SkColor colors[],
                               SkXfermode* xmode, const uint16_t indices[],
                               int indexCount, const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }
    NOT_IMPLEMENTED("drawVerticies", true);
}

void SkPDFDevice::drawDevice(const SkDraw& d, SkDevice* device, int x, int y,
                             const SkPaint& paint) {
    if ((device->getDeviceCapabilities() & kVector_Capability) == 0) {
        
        SkDevice::drawDevice(d, device, x, y, paint);
        return;
    }

    
    SkPDFDevice* pdfDevice = static_cast<SkPDFDevice*>(device);
    if (pdfDevice->isContentEmpty()) {
        return;
    }

    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(x), SkIntToScalar(y));
    ScopedContentEntry content(this, d.fClipStack, *d.fClip, matrix, paint);
    if (!content.entry()) {
        return;
    }

    SkPDFFormXObject* xobject = new SkPDFFormXObject(pdfDevice);
    fXObjectResources.push(xobject);  
    SkPDFUtils::DrawFormXObject(fXObjectResources.count() - 1,
                                &content.entry()->fContent);

    
    fFontGlyphUsage->merge(pdfDevice->getFontGlyphUsage());
}

void SkPDFDevice::onAttachToCanvas(SkCanvas* canvas) {
    INHERITED::onAttachToCanvas(canvas);

    
    fClipStack = canvas->getClipStack();
}

void SkPDFDevice::onDetachFromCanvas() {
    INHERITED::onDetachFromCanvas();

    fClipStack = NULL;
}

ContentEntry* SkPDFDevice::getLastContentEntry() {
    if (fDrawingArea == kContent_DrawingArea) {
        return fLastContentEntry;
    } else {
        return fLastMarginContentEntry;
    }
}

SkTScopedPtr<ContentEntry>* SkPDFDevice::getContentEntries() {
    if (fDrawingArea == kContent_DrawingArea) {
        return &fContentEntries;
    } else {
        return &fMarginContentEntries;
    }
}

void SkPDFDevice::setLastContentEntry(ContentEntry* contentEntry) {
    if (fDrawingArea == kContent_DrawingArea) {
        fLastContentEntry = contentEntry;
    } else {
        fLastMarginContentEntry = contentEntry;
    }
}

void SkPDFDevice::setDrawingArea(DrawingArea drawingArea) {
    
    
    fDrawingArea = drawingArea;
}

SkPDFDict* SkPDFDevice::getResourceDict() {
    if (NULL == fResourceDict) {
        fResourceDict = SkNEW(SkPDFDict);

        if (fGraphicStateResources.count()) {
            SkAutoTUnref<SkPDFDict> extGState(new SkPDFDict());
            for (int i = 0; i < fGraphicStateResources.count(); i++) {
                SkString nameString("G");
                nameString.appendS32(i);
                extGState->insert(
                        nameString.c_str(),
                        new SkPDFObjRef(fGraphicStateResources[i]))->unref();
            }
            fResourceDict->insert("ExtGState", extGState.get());
        }

        if (fXObjectResources.count()) {
            SkAutoTUnref<SkPDFDict> xObjects(new SkPDFDict());
            for (int i = 0; i < fXObjectResources.count(); i++) {
                SkString nameString("X");
                nameString.appendS32(i);
                xObjects->insert(
                        nameString.c_str(),
                        new SkPDFObjRef(fXObjectResources[i]))->unref();
            }
            fResourceDict->insert("XObject", xObjects.get());
        }

        if (fFontResources.count()) {
            SkAutoTUnref<SkPDFDict> fonts(new SkPDFDict());
            for (int i = 0; i < fFontResources.count(); i++) {
                SkString nameString("F");
                nameString.appendS32(i);
                fonts->insert(nameString.c_str(),
                              new SkPDFObjRef(fFontResources[i]))->unref();
            }
            fResourceDict->insert("Font", fonts.get());
        }

        if (fShaderResources.count()) {
            SkAutoTUnref<SkPDFDict> patterns(new SkPDFDict());
            for (int i = 0; i < fShaderResources.count(); i++) {
                SkString nameString("P");
                nameString.appendS32(i);
                patterns->insert(nameString.c_str(),
                                 new SkPDFObjRef(fShaderResources[i]))->unref();
            }
            fResourceDict->insert("Pattern", patterns.get());
        }

        
        
        const char procs[][7] = {"PDF", "Text", "ImageB", "ImageC", "ImageI"};
        SkAutoTUnref<SkPDFArray> procSets(new SkPDFArray());
        procSets->reserve(SK_ARRAY_COUNT(procs));
        for (size_t i = 0; i < SK_ARRAY_COUNT(procs); i++)
            procSets->appendName(procs[i]);
        fResourceDict->insert("ProcSet", procSets.get());
    }
    return fResourceDict;
}

void SkPDFDevice::getResources(const SkTSet<SkPDFObject*>& knownResourceObjects,
                               SkTSet<SkPDFObject*>* newResourceObjects,
                               bool recursive) const {
    
    newResourceObjects->setReserve(newResourceObjects->count() +
                                   fGraphicStateResources.count() +
                                   fXObjectResources.count() +
                                   fFontResources.count() +
                                   fShaderResources.count());
    for (int i = 0; i < fGraphicStateResources.count(); i++) {
        if (!knownResourceObjects.contains(fGraphicStateResources[i]) &&
                !newResourceObjects->contains(fGraphicStateResources[i])) {
            newResourceObjects->add(fGraphicStateResources[i]);
            fGraphicStateResources[i]->ref();
            if (recursive) {
                fGraphicStateResources[i]->getResources(knownResourceObjects,
                                                        newResourceObjects);
            }
        }
    }
    for (int i = 0; i < fXObjectResources.count(); i++) {
        if (!knownResourceObjects.contains(fXObjectResources[i]) &&
                !newResourceObjects->contains(fXObjectResources[i])) {
            newResourceObjects->add(fXObjectResources[i]);
            fXObjectResources[i]->ref();
            if (recursive) {
                fXObjectResources[i]->getResources(knownResourceObjects,
                                                   newResourceObjects);
            }
        }
    }
    for (int i = 0; i < fFontResources.count(); i++) {
        if (!knownResourceObjects.contains(fFontResources[i]) &&
                !newResourceObjects->contains(fFontResources[i])) {
            newResourceObjects->add(fFontResources[i]);
            fFontResources[i]->ref();
            if (recursive) {
                fFontResources[i]->getResources(knownResourceObjects,
                                                newResourceObjects);
            }
        }
    }
    for (int i = 0; i < fShaderResources.count(); i++) {
        if (!knownResourceObjects.contains(fShaderResources[i]) &&
                !newResourceObjects->contains(fShaderResources[i])) {
            newResourceObjects->add(fShaderResources[i]);
            fShaderResources[i]->ref();
            if (recursive) {
                fShaderResources[i]->getResources(knownResourceObjects,
                                                  newResourceObjects);
            }
        }
    }
}

const SkTDArray<SkPDFFont*>& SkPDFDevice::getFontResources() const {
    return fFontResources;
}

SkPDFArray* SkPDFDevice::copyMediaBox() const {
    
    SkAutoTUnref<SkPDFInt> zero(SkNEW_ARGS(SkPDFInt, (0)));

    SkPDFArray* mediaBox = SkNEW(SkPDFArray);
    mediaBox->reserve(4);
    mediaBox->append(zero.get());
    mediaBox->append(zero.get());
    mediaBox->appendInt(fPageSize.fWidth);
    mediaBox->appendInt(fPageSize.fHeight);
    return mediaBox;
}

SkStream* SkPDFDevice::content() const {
    SkMemoryStream* result = new SkMemoryStream;
    result->setData(this->copyContentToData())->unref();
    return result;
}

void SkPDFDevice::copyContentEntriesToData(ContentEntry* entry,
        SkWStream* data) const {
    
    
    GraphicStackState gsState(fExistingClipStack, fExistingClipRegion, data);
    while (entry != NULL) {
        SkPoint translation;
        translation.iset(this->getOrigin());
        translation.negate();
        gsState.updateClip(entry->fState.fClipStack, entry->fState.fClipRegion,
                           translation);
        gsState.updateMatrix(entry->fState.fMatrix);
        gsState.updateDrawingState(entry->fState);

        SkAutoDataUnref copy(entry->fContent.copyToData());
        data->write(copy->data(), copy->size());
        entry = entry->fNext.get();
    }
    gsState.drainStack();
}

SkData* SkPDFDevice::copyContentToData() const {
    SkDynamicMemoryWStream data;
    if (fInitialTransform.getType() != SkMatrix::kIdentity_Mask) {
        SkPDFUtils::AppendTransform(fInitialTransform, &data);
    }

    
    
    
    
    
    SkPDFDevice::copyContentEntriesToData(fMarginContentEntries.get(), &data);

    
    
    
    
    if (fPageSize != fContentSize) {
        SkRect r = SkRect::MakeWH(SkIntToScalar(this->width()),
                                  SkIntToScalar(this->height()));
        emit_clip(NULL, &r, &data);
    }

    SkPDFDevice::copyContentEntriesToData(fContentEntries.get(), &data);

    
    
    return data.copyToData();
}

bool SkPDFDevice::handleRectAnnotation(const SkRect& r, const SkMatrix& matrix,
                                       const SkPaint& p) {
    SkAnnotation* annotationInfo = p.getAnnotation();
    if (!annotationInfo) {
        return false;
    }
    SkData* urlData = annotationInfo->find(SkAnnotationKeys::URL_Key());
    if (urlData) {
        handleLinkToURL(urlData, r, matrix);
        return p.isNoDrawAnnotation();
    }
    SkData* linkToName = annotationInfo->find(SkAnnotationKeys::Link_Named_Dest_Key());
    if (linkToName) {
        handleLinkToNamedDest(linkToName, r, matrix);
        return p.isNoDrawAnnotation();
    }
    return false;
}

bool SkPDFDevice::handlePointAnnotation(const SkPoint* points, size_t count,
                                        const SkMatrix& matrix,
                                        const SkPaint& paint) {
    SkAnnotation* annotationInfo = paint.getAnnotation();
    if (!annotationInfo) {
        return false;
    }
    SkData* nameData = annotationInfo->find(SkAnnotationKeys::Define_Named_Dest_Key());
    if (nameData) {
        for (size_t i = 0; i < count; i++) {
            defineNamedDestination(nameData, points[i], matrix);
        }
        return paint.isNoDrawAnnotation();
    }
    return false;
}

SkPDFDict* SkPDFDevice::createLinkAnnotation(const SkRect& r, const SkMatrix& matrix) {
    SkMatrix transform = matrix;
    transform.postConcat(fInitialTransform);
    SkRect translatedRect;
    transform.mapRect(&translatedRect, r);

    if (NULL == fAnnotations) {
        fAnnotations = SkNEW(SkPDFArray);
    }
    SkPDFDict* annotation(SkNEW_ARGS(SkPDFDict, ("Annot")));
    annotation->insertName("Subtype", "Link");
    fAnnotations->append(annotation);

    SkAutoTUnref<SkPDFArray> border(SkNEW(SkPDFArray));
    border->reserve(3);
    border->appendInt(0);  
    border->appendInt(0);  
    border->appendInt(0);  
    annotation->insert("Border", border.get());

    SkAutoTUnref<SkPDFArray> rect(SkNEW(SkPDFArray));
    rect->reserve(4);
    rect->appendScalar(translatedRect.fLeft);
    rect->appendScalar(translatedRect.fTop);
    rect->appendScalar(translatedRect.fRight);
    rect->appendScalar(translatedRect.fBottom);
    annotation->insert("Rect", rect.get());

    return annotation;
}

void SkPDFDevice::handleLinkToURL(SkData* urlData, const SkRect& r,
                                  const SkMatrix& matrix) {
    SkAutoTUnref<SkPDFDict> annotation(createLinkAnnotation(r, matrix));

    SkString url(static_cast<const char *>(urlData->data()),
                 urlData->size() - 1);
    SkAutoTUnref<SkPDFDict> action(SkNEW_ARGS(SkPDFDict, ("Action")));
    action->insertName("S", "URI");
    action->insert("URI", SkNEW_ARGS(SkPDFString, (url)))->unref();
    annotation->insert("A", action.get());
}

void SkPDFDevice::handleLinkToNamedDest(SkData* nameData, const SkRect& r,
                                        const SkMatrix& matrix) {
    SkAutoTUnref<SkPDFDict> annotation(createLinkAnnotation(r, matrix));
    SkString name(static_cast<const char *>(nameData->data()),
                  nameData->size() - 1);
    annotation->insert("Dest", SkNEW_ARGS(SkPDFName, (name)))->unref();
}

struct NamedDestination {
    const SkData* nameData;
    SkPoint point;

    NamedDestination(const SkData* nameData, const SkPoint& point)
        : nameData(nameData), point(point) {
        nameData->ref();
    }

    ~NamedDestination() {
        nameData->unref();
    }
};

void SkPDFDevice::defineNamedDestination(SkData* nameData, const SkPoint& point,
                                         const SkMatrix& matrix) {
    SkMatrix transform = matrix;
    transform.postConcat(fInitialTransform);
    SkPoint translatedPoint;
    transform.mapXY(point.x(), point.y(), &translatedPoint);
    fNamedDestinations.push(
        SkNEW_ARGS(NamedDestination, (nameData, translatedPoint)));
}

void SkPDFDevice::appendDestinations(SkPDFDict* dict, SkPDFObject* page) {
    int nDest = fNamedDestinations.count();
    for (int i = 0; i < nDest; i++) {
        NamedDestination* dest = fNamedDestinations[i];
        SkAutoTUnref<SkPDFArray> pdfDest(SkNEW(SkPDFArray));
        pdfDest->reserve(5);
        pdfDest->append(SkNEW_ARGS(SkPDFObjRef, (page)))->unref();
        pdfDest->appendName("XYZ");
        pdfDest->appendScalar(dest->point.x());
        pdfDest->appendScalar(dest->point.y());
        pdfDest->appendInt(0);  
        dict->insert(static_cast<const char *>(dest->nameData->data()), pdfDest);
    }
}

SkPDFFormXObject* SkPDFDevice::createFormXObjectFromDevice() {
    SkPDFFormXObject* xobject = SkNEW_ARGS(SkPDFFormXObject, (this));
    
    
    
    cleanUp(false);  
    init();
    return xobject;
}

void SkPDFDevice::clearClipFromContent(const SkClipStack* clipStack,
                                       const SkRegion& clipRegion) {
    if (clipRegion.isEmpty() || isContentEmpty()) {
        return;
    }
    SkAutoTUnref<SkPDFFormXObject> curContent(createFormXObjectFromDevice());

    
    drawFormXObjectWithClip(curContent, clipStack, clipRegion, true);
}

void SkPDFDevice::drawFormXObjectWithClip(SkPDFFormXObject* xobject,
                                          const SkClipStack* clipStack,
                                          const SkRegion& clipRegion,
                                          bool invertClip) {
    if (clipRegion.isEmpty() && !invertClip) {
        return;
    }

    
    SkMatrix identity;
    identity.reset();
    SkDraw draw;
    draw.fMatrix = &identity;
    draw.fClip = &clipRegion;
    draw.fClipStack = clipStack;
    SkPaint stockPaint;
    this->drawPaint(draw, stockPaint);
    SkAutoTUnref<SkPDFFormXObject> maskFormXObject(createFormXObjectFromDevice());
    SkAutoTUnref<SkPDFGraphicState> sMaskGS(
        SkPDFGraphicState::GetSMaskGraphicState(maskFormXObject, invertClip));

    
    ScopedContentEntry content(this, &fExistingClipStack, fExistingClipRegion,
                                 identity, stockPaint);
    if (!content.entry()) {
        return;
    }
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &content.entry()->fContent);
    SkPDFUtils::DrawFormXObject(fXObjectResources.count(),
                                &content.entry()->fContent);
    fXObjectResources.push(xobject);
    xobject->ref();

    sMaskGS.reset(SkPDFGraphicState::GetNoSMaskGraphicState());
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &content.entry()->fContent);
}

ContentEntry* SkPDFDevice::setUpContentEntry(const SkClipStack* clipStack,
                                             const SkRegion& clipRegion,
                                             const SkMatrix& matrix,
                                             const SkPaint& paint,
                                             bool hasText,
                                             SkPDFFormXObject** dst) {
    *dst = NULL;
    if (clipRegion.isEmpty()) {
        return NULL;
    }

    
    SkClipStack synthesizedClipStack;
    if (clipStack == NULL) {
        if (clipRegion == fExistingClipRegion) {
            clipStack = &fExistingClipStack;
        } else {
            
            
            
            synthesizedClipStack = fExistingClipStack;
            SkPath clipPath;
            clipRegion.getBoundaryPath(&clipPath);
            synthesizedClipStack.clipDevPath(clipPath, SkRegion::kReplace_Op,
                                             false);
            clipStack = &synthesizedClipStack;
        }
    }

    SkXfermode::Mode xfermode = SkXfermode::kSrcOver_Mode;
    if (paint.getXfermode()) {
        paint.getXfermode()->asMode(&xfermode);
    }

    if (xfermode == SkXfermode::kClear_Mode ||
            xfermode == SkXfermode::kSrc_Mode) {
        this->clearClipFromContent(clipStack, clipRegion);
    } else if (xfermode == SkXfermode::kSrcIn_Mode ||
               xfermode == SkXfermode::kDstIn_Mode ||
               xfermode == SkXfermode::kSrcOut_Mode ||
               xfermode == SkXfermode::kDstOut_Mode) {
        
        
        
        if (isContentEmpty()) {
            return NULL;
        } else {
            *dst = createFormXObjectFromDevice();
        }
    }
    
    

    
    if (xfermode == SkXfermode::kClear_Mode ||
            xfermode == SkXfermode::kDst_Mode) {
        return NULL;
    }

    ContentEntry* entry;
    SkTScopedPtr<ContentEntry> newEntry;

    ContentEntry* lastContentEntry = getLastContentEntry();
    if (lastContentEntry && lastContentEntry->fContent.getOffset() == 0) {
        entry = lastContentEntry;
    } else {
        newEntry.reset(new ContentEntry);
        entry = newEntry.get();
    }

    populateGraphicStateEntryFromPaint(matrix, *clipStack, clipRegion, paint,
                                       hasText, &entry->fState);
    if (lastContentEntry && xfermode != SkXfermode::kDstOver_Mode &&
            entry->fState.compareInitialState(lastContentEntry->fState)) {
        return lastContentEntry;
    }

    SkTScopedPtr<ContentEntry>* contentEntries = getContentEntries();
    if (!lastContentEntry) {
        contentEntries->reset(entry);
        setLastContentEntry(entry);
    } else if (xfermode == SkXfermode::kDstOver_Mode) {
        entry->fNext.reset(contentEntries->release());
        contentEntries->reset(entry);
    } else {
        lastContentEntry->fNext.reset(entry);
        setLastContentEntry(entry);
    }
    newEntry.release();
    return entry;
}

void SkPDFDevice::finishContentEntry(const SkXfermode::Mode xfermode,
                                     SkPDFFormXObject* dst) {
    if (xfermode != SkXfermode::kSrcIn_Mode &&
            xfermode != SkXfermode::kDstIn_Mode &&
            xfermode != SkXfermode::kSrcOut_Mode &&
            xfermode != SkXfermode::kDstOut_Mode) {
        SkASSERT(!dst);
        return;
    }

    ContentEntry* contentEntries = getContentEntries()->get();
    SkASSERT(dst);
    SkASSERT(!contentEntries->fNext.get());
    
    
    SkClipStack clipStack = contentEntries->fState.fClipStack;
    SkRegion clipRegion = contentEntries->fState.fClipRegion;

    SkAutoTUnref<SkPDFFormXObject> srcFormXObject;
    if (!isContentEmpty()) {
        srcFormXObject.reset(createFormXObjectFromDevice());
    }

    drawFormXObjectWithClip(dst, &clipStack, clipRegion, true);

    
    if (!srcFormXObject.get()) {
        return;
    }

    SkMatrix identity;
    identity.reset();
    SkPaint stockPaint;
    ScopedContentEntry inClipContentEntry(this, &fExistingClipStack,
                                          fExistingClipRegion, identity,
                                          stockPaint);
    if (!inClipContentEntry.entry()) {
        return;
    }

    SkAutoTUnref<SkPDFGraphicState> sMaskGS;
    if (xfermode == SkXfermode::kSrcIn_Mode ||
            xfermode == SkXfermode::kSrcOut_Mode) {
        sMaskGS.reset(SkPDFGraphicState::GetSMaskGraphicState(
                dst, xfermode == SkXfermode::kSrcOut_Mode));
        fXObjectResources.push(srcFormXObject.get());
        srcFormXObject.get()->ref();
    } else {
        sMaskGS.reset(SkPDFGraphicState::GetSMaskGraphicState(
                srcFormXObject.get(), xfermode == SkXfermode::kDstOut_Mode));
        
    }
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &inClipContentEntry.entry()->fContent);

    SkPDFUtils::DrawFormXObject(fXObjectResources.count() - 1,
                                &inClipContentEntry.entry()->fContent);

    sMaskGS.reset(SkPDFGraphicState::GetNoSMaskGraphicState());
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &inClipContentEntry.entry()->fContent);
}

bool SkPDFDevice::isContentEmpty() {
    ContentEntry* contentEntries = getContentEntries()->get();
    if (!contentEntries || contentEntries->fContent.getOffset() == 0) {
        SkASSERT(!contentEntries || !contentEntries->fNext.get());
        return true;
    }
    return false;
}

void SkPDFDevice::populateGraphicStateEntryFromPaint(
        const SkMatrix& matrix,
        const SkClipStack& clipStack,
        const SkRegion& clipRegion,
        const SkPaint& paint,
        bool hasText,
        GraphicStateEntry* entry) {
    SkASSERT(paint.getPathEffect() == NULL);

    NOT_IMPLEMENTED(paint.getMaskFilter() != NULL, false);
    NOT_IMPLEMENTED(paint.getColorFilter() != NULL, false);

    entry->fMatrix = matrix;
    entry->fClipStack = clipStack;
    entry->fClipRegion = clipRegion;
    entry->fColor = SkColorSetA(paint.getColor(), 0xFF);
    entry->fShaderIndex = -1;

    
    SkAutoTUnref<SkPDFObject> pdfShader;
    const SkShader* shader = paint.getShader();
    SkColor color = paint.getColor();
    if (shader) {
        
        
        SkMatrix transform = matrix;
        transform.postConcat(fInitialTransform);

        
        
        SkIRect bounds = clipRegion.getBounds();

        
        
        SkRect boundsTemp;
        boundsTemp.set(bounds);
        fInitialTransform.mapRect(&boundsTemp);
        boundsTemp.roundOut(&bounds);

        pdfShader.reset(SkPDFShader::GetPDFShader(*shader, transform, bounds));

        if (pdfShader.get()) {
            
            
            int resourceIndex = fShaderResources.find(pdfShader.get());
            if (resourceIndex < 0) {
                resourceIndex = fShaderResources.count();
                fShaderResources.push(pdfShader.get());
                pdfShader.get()->ref();
            }
            entry->fShaderIndex = resourceIndex;
        } else {
            
            
            SkShader::GradientInfo gradientInfo;
            SkColor gradientColor;
            gradientInfo.fColors = &gradientColor;
            gradientInfo.fColorOffsets = NULL;
            gradientInfo.fColorCount = 1;
            if (shader->asAGradient(&gradientInfo) ==
                    SkShader::kColor_GradientType) {
                entry->fColor = SkColorSetA(gradientColor, 0xFF);
                color = gradientColor;
            }
        }
    }

    SkAutoTUnref<SkPDFGraphicState> newGraphicState;
    if (color == paint.getColor()) {
        newGraphicState.reset(
                SkPDFGraphicState::GetGraphicStateForPaint(paint));
    } else {
        SkPaint newPaint = paint;
        newPaint.setColor(color);
        newGraphicState.reset(
                SkPDFGraphicState::GetGraphicStateForPaint(newPaint));
    }
    int resourceIndex = addGraphicStateResource(newGraphicState.get());
    entry->fGraphicStateIndex = resourceIndex;

    if (hasText) {
        entry->fTextScaleX = paint.getTextScaleX();
        entry->fTextFill = paint.getStyle();
    } else {
        entry->fTextScaleX = 0;
    }
}

int SkPDFDevice::addGraphicStateResource(SkPDFGraphicState* gs) {
    
    
    int result = fGraphicStateResources.find(gs);
    if (result < 0) {
        result = fGraphicStateResources.count();
        fGraphicStateResources.push(gs);
        gs->ref();
    }
    return result;
}

void SkPDFDevice::updateFont(const SkPaint& paint, uint16_t glyphID,
                             ContentEntry* contentEntry) {
    SkTypeface* typeface = paint.getTypeface();
    if (contentEntry->fState.fFont == NULL ||
            contentEntry->fState.fTextSize != paint.getTextSize() ||
            !contentEntry->fState.fFont->hasGlyph(glyphID)) {
        int fontIndex = getFontResourceIndex(typeface, glyphID);
        contentEntry->fContent.writeText("/F");
        contentEntry->fContent.writeDecAsText(fontIndex);
        contentEntry->fContent.writeText(" ");
        SkPDFScalar::Append(paint.getTextSize(), &contentEntry->fContent);
        contentEntry->fContent.writeText(" Tf\n");
        contentEntry->fState.fFont = fFontResources[fontIndex];
    }
}

int SkPDFDevice::getFontResourceIndex(SkTypeface* typeface, uint16_t glyphID) {
    SkAutoTUnref<SkPDFFont> newFont(SkPDFFont::GetFontResource(typeface, glyphID));
    int resourceIndex = fFontResources.find(newFont.get());
    if (resourceIndex < 0) {
        resourceIndex = fFontResources.count();
        fFontResources.push(newFont.get());
        newFont.get()->ref();
    }
    return resourceIndex;
}

void SkPDFDevice::internalDrawBitmap(const SkMatrix& matrix,
                                     const SkClipStack* clipStack,
                                     const SkRegion& clipRegion,
                                     const SkBitmap& bitmap,
                                     const SkIRect* srcRect,
                                     const SkPaint& paint) {
    SkMatrix scaled;
    
    scaled.setScale(SK_Scalar1, -SK_Scalar1);
    scaled.postTranslate(0, SK_Scalar1);
    
    SkIRect subset = SkIRect::MakeWH(bitmap.width(), bitmap.height());
    scaled.postScale(SkIntToScalar(subset.width()),
                     SkIntToScalar(subset.height()));
    scaled.postConcat(matrix);
    ScopedContentEntry content(this, clipStack, clipRegion, scaled, paint);
    if (!content.entry()) {
        return;
    }

    if (srcRect && !subset.intersect(*srcRect)) {
        return;
    }

    SkPDFImage* image = SkPDFImage::CreateImage(bitmap, subset);
    if (!image) {
        return;
    }

    fXObjectResources.push(image);  
    SkPDFUtils::DrawFormXObject(fXObjectResources.count() - 1,
                                &content.entry()->fContent);
}

bool SkPDFDevice::onReadPixels(const SkBitmap& bitmap, int x, int y,
                               SkCanvas::Config8888) {
    return false;
}

bool SkPDFDevice::allowImageFilter(SkImageFilter*) {
    return false;
}
