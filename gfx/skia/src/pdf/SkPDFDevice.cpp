








#include "SkPDFDevice.h"

#include "SkColor.h"
#include "SkClipStack.h"
#include "SkData.h"
#include "SkDraw.h"
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
#include "SkTypeface.h"
#include "SkTypes.h"



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
    SkAutoGlyphCache autoCache(paint, &ident);
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
                    const SkIPoint& translation);
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
                                   SkClipStack::B2FIter* iter) {
    SkClipStack::B2FIter prefixIter(prefix);
    iter->reset(stack);

    const SkClipStack::B2FIter::Clip* prefixEntry;
    const SkClipStack::B2FIter::Clip* iterEntry;

    int count = 0;
    for (prefixEntry = prefixIter.next(); prefixEntry;
            prefixEntry = prefixIter.next(), count++) {
        iterEntry = iter->next();
        SkASSERT(iterEntry);
        
        
        if (*prefixEntry != *iterEntry) {
            SkASSERT(prefixEntry->fOp == SkRegion::kIntersect_Op);
            SkASSERT(iterEntry->fOp == SkRegion::kIntersect_Op);
            SkASSERT((iterEntry->fRect == NULL) ==
                    (prefixEntry->fRect == NULL));
            SkASSERT((iterEntry->fPath == NULL) ==
                    (prefixEntry->fPath == NULL));
            
            
            iter->reset(stack);
            for (int i = 0; i < count; i++) {
                iter->next();
            }
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
        SkPDFUtils::EmitPath(*clipPath, contentStream);
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
                                   const SkIPoint& translation) {
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

    
    
    
    
    
    
    
    
    SkClipStack::B2FIter iter;
    skip_clip_stack_prefix(fEntries[0].fClipStack, clipStack, &iter);

    
    
    bool needRegion = false;
    const SkClipStack::B2FIter::Clip* clipEntry;
    for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
        if (clipEntry->fOp != SkRegion::kIntersect_Op ||
                (clipEntry->fPath && clipEntry->fPath->isInverseFillType())) {
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
        const SkClipStack::B2FIter::Clip* clipEntry;
        for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
            SkASSERT(clipEntry->fOp == SkRegion::kIntersect_Op);
            if (clipEntry->fRect) {
                SkRect translatedClip;
                transform.mapRect(&translatedClip, *clipEntry->fRect);
                emit_clip(NULL, &translatedClip, fContentStream);
            } else if (clipEntry->fPath) {
                SkPath translatedPath;
                clipEntry->fPath->transform(transform, &translatedPath);
                emit_clip(&translatedPath, NULL, fContentStream);
            } else {
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
            fDevice->finishContentEntry(fXfermode, fDstFormXObject.get());
        }
    }

    ContentEntry* entry() { return fContentEntry; }
private:
    SkPDFDevice* fDevice;
    ContentEntry* fContentEntry;
    SkXfermode::Mode fXfermode;
    SkRefPtr<SkPDFFormXObject> fDstFormXObject;

    void init(const SkClipStack* clipStack, const SkRegion& clipRegion,
              const SkMatrix& matrix, const SkPaint& paint, bool hasText) {
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
        drawingSize.set(contentSize.fWidth, contentSize.fHeight);
        initialTransform->invert(&inverse);
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
      fLastMarginContentEntry(NULL) {
    
    
    
    fInitialTransform.setTranslate(0, pageSize.fHeight);
    fInitialTransform.preScale(1, -1);
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
      fLastMarginContentEntry(NULL) {
    fInitialTransform.reset();
    this->init();
}

SkPDFDevice::~SkPDFDevice() {
    this->cleanUp(true);
}

void SkPDFDevice::init() {
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
    if (clearFontUsage) {
        fFontGlyphUsage->reset();
    }
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
    inverse.reset();
    totalTransform.invert(&inverse);
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

    ScopedContentEntry content(this, d, paint);
    if (!content.entry()) {
        return;
    }
    SkPDFUtils::EmitPath(*pathPtr, &content.entry()->fContent);
    SkPDFUtils::PaintPath(paint.getStyle(), pathPtr->getFillType(),
                          &content.entry()->fContent);
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
    SkPaint textPaint = calculate_text_paint(paint);
    ScopedContentEntry content(this, d, textPaint, true);
    if (!content.entry()) {
        return;
    }

    
    
    size_t numGlyphs = paint.textToGlyphs(text, len, NULL);
    uint16_t* glyphIDs = reinterpret_cast<uint16_t*>(
            sk_malloc_flags(numGlyphs * 2, SK_MALLOC_TEMP | SK_MALLOC_THROW));
    SkAutoFree autoFreeGlyphIDs(glyphIDs);
    if (paint.getTextEncoding() != SkPaint::kGlyphID_TextEncoding) {
        paint.textToGlyphs(text, len, glyphIDs);
        textPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    } else {
        SkASSERT((len & 1) == 0);
        SkASSERT(len / 2 == numGlyphs);
        memcpy(glyphIDs, text, len);
    }

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
    SkASSERT(1 == scalarsPerPos || 2 == scalarsPerPos);
    SkPaint textPaint = calculate_text_paint(paint);
    ScopedContentEntry content(this, d, textPaint, true);
    if (!content.entry()) {
        return;
    }

    
    SkAutoFree glyphStorage;
    uint16_t* glyphIDs;
    size_t numGlyphs;
    if (paint.getTextEncoding() != SkPaint::kGlyphID_TextEncoding) {
        numGlyphs = paint.textToGlyphs(text, len, NULL);
        glyphIDs = reinterpret_cast<uint16_t*>(sk_malloc_flags(
                numGlyphs * 2, SK_MALLOC_TEMP | SK_MALLOC_THROW));
        glyphStorage.set(glyphIDs);
        paint.textToGlyphs(text, len, glyphIDs);
        textPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    } else {
        SkASSERT((len & 1) == 0);
        numGlyphs = len / 2;
        glyphIDs = reinterpret_cast<uint16_t*>(const_cast<void*>((text)));
    }

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
    NOT_IMPLEMENTED("drawTextOnPath", false);
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
    if (fResourceDict.get() == NULL) {
        fResourceDict = new SkPDFDict;
        fResourceDict->unref();  

        if (fGraphicStateResources.count()) {
            SkRefPtr<SkPDFDict> extGState = new SkPDFDict();
            extGState->unref();  
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
            SkRefPtr<SkPDFDict> xObjects = new SkPDFDict();
            xObjects->unref();  
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
            SkRefPtr<SkPDFDict> fonts = new SkPDFDict();
            fonts->unref();  
            for (int i = 0; i < fFontResources.count(); i++) {
                SkString nameString("F");
                nameString.appendS32(i);
                fonts->insert(nameString.c_str(),
                              new SkPDFObjRef(fFontResources[i]))->unref();
            }
            fResourceDict->insert("Font", fonts.get());
        }

        if (fShaderResources.count()) {
            SkRefPtr<SkPDFDict> patterns = new SkPDFDict();
            patterns->unref();  
            for (int i = 0; i < fShaderResources.count(); i++) {
                SkString nameString("P");
                nameString.appendS32(i);
                patterns->insert(nameString.c_str(),
                                 new SkPDFObjRef(fShaderResources[i]))->unref();
            }
            fResourceDict->insert("Pattern", patterns.get());
        }

        
        
        const char procs[][7] = {"PDF", "Text", "ImageB", "ImageC", "ImageI"};
        SkRefPtr<SkPDFArray> procSets = new SkPDFArray();
        procSets->unref();  
        procSets->reserve(SK_ARRAY_COUNT(procs));
        for (size_t i = 0; i < SK_ARRAY_COUNT(procs); i++)
            procSets->appendName(procs[i]);
        fResourceDict->insert("ProcSet", procSets.get());
    }
    return fResourceDict.get();
}

void SkPDFDevice::getResources(SkTDArray<SkPDFObject*>* resourceList) const {
    resourceList->setReserve(resourceList->count() +
                             fGraphicStateResources.count() +
                             fXObjectResources.count() +
                             fFontResources.count() +
                             fShaderResources.count());
    for (int i = 0; i < fGraphicStateResources.count(); i++) {
        resourceList->push(fGraphicStateResources[i]);
        fGraphicStateResources[i]->ref();
        fGraphicStateResources[i]->getResources(resourceList);
    }
    for (int i = 0; i < fXObjectResources.count(); i++) {
        resourceList->push(fXObjectResources[i]);
        fXObjectResources[i]->ref();
        fXObjectResources[i]->getResources(resourceList);
    }
    for (int i = 0; i < fFontResources.count(); i++) {
        resourceList->push(fFontResources[i]);
        fFontResources[i]->ref();
        fFontResources[i]->getResources(resourceList);
    }
    for (int i = 0; i < fShaderResources.count(); i++) {
        resourceList->push(fShaderResources[i]);
        fShaderResources[i]->ref();
        fShaderResources[i]->getResources(resourceList);
    }
}

const SkTDArray<SkPDFFont*>& SkPDFDevice::getFontResources() const {
    return fFontResources;
}

SkRefPtr<SkPDFArray> SkPDFDevice::getMediaBox() const {
    SkRefPtr<SkPDFInt> zero = new SkPDFInt(0);
    zero->unref();  

    SkRefPtr<SkPDFArray> mediaBox = new SkPDFArray();
    mediaBox->unref();  
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
        SkIPoint translation = this->getOrigin();
        translation.negate();
        gsState.updateClip(entry->fState.fClipStack, entry->fState.fClipRegion,
                           translation);
        gsState.updateMatrix(entry->fState.fMatrix);
        gsState.updateDrawingState(entry->fState);

        SkAutoDataUnref copy(entry->fContent.copyToData());
        data->write(copy.data(), copy.size());
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
        SkRect r = SkRect::MakeWH(this->width(), this->height());
        emit_clip(NULL, &r, &data);
    }

    SkPDFDevice::copyContentEntriesToData(fContentEntries.get(), &data);

    
    
    return data.copyToData();
}

void SkPDFDevice::createFormXObjectFromDevice(
        SkRefPtr<SkPDFFormXObject>* xobject) {
    *xobject = new SkPDFFormXObject(this);
    (*xobject)->unref();  
    
    
    
    cleanUp(false);  
    init();
}

void SkPDFDevice::clearClipFromContent(const SkClipStack* clipStack,
                                       const SkRegion& clipRegion) {
    if (clipRegion.isEmpty() || isContentEmpty()) {
        return;
    }
    SkRefPtr<SkPDFFormXObject> curContent;
    createFormXObjectFromDevice(&curContent);

    
    drawFormXObjectWithClip(curContent.get(), clipStack, clipRegion, true);
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
    SkRefPtr<SkPDFFormXObject> maskFormXObject;
    createFormXObjectFromDevice(&maskFormXObject);
    SkRefPtr<SkPDFGraphicState> sMaskGS =
        SkPDFGraphicState::GetSMaskGraphicState(maskFormXObject.get(),
                                                invertClip);
    sMaskGS->unref();  

    
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

    sMaskGS = SkPDFGraphicState::GetNoSMaskGraphicState();
    sMaskGS->unref();  
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &content.entry()->fContent);
}

ContentEntry* SkPDFDevice::setUpContentEntry(const SkClipStack* clipStack,
                                             const SkRegion& clipRegion,
                                             const SkMatrix& matrix,
                                             const SkPaint& paint,
                                             bool hasText,
                                             SkRefPtr<SkPDFFormXObject>* dst) {
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
            createFormXObjectFromDevice(dst);
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

    SkRefPtr<SkPDFFormXObject> srcFormXObject;
    if (!isContentEmpty()) {
        createFormXObjectFromDevice(&srcFormXObject);
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

    SkRefPtr<SkPDFGraphicState> sMaskGS;
    if (xfermode == SkXfermode::kSrcIn_Mode ||
            xfermode == SkXfermode::kSrcOut_Mode) {
        sMaskGS = SkPDFGraphicState::GetSMaskGraphicState(
                dst, xfermode == SkXfermode::kSrcOut_Mode);
        fXObjectResources.push(srcFormXObject.get());
        srcFormXObject->ref();
    } else {
        sMaskGS = SkPDFGraphicState::GetSMaskGraphicState(
                srcFormXObject.get(), xfermode == SkXfermode::kDstOut_Mode);
        
    }
    sMaskGS->unref();  
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &inClipContentEntry.entry()->fContent);

    SkPDFUtils::DrawFormXObject(fXObjectResources.count() - 1,
                                &inClipContentEntry.entry()->fContent);

    sMaskGS = SkPDFGraphicState::GetNoSMaskGraphicState();
    sMaskGS->unref();  
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

    
    SkRefPtr<SkPDFObject> pdfShader;
    const SkShader* shader = paint.getShader();
    SkColor color = paint.getColor();
    if (shader) {
        
        
        SkMatrix transform = matrix;
        transform.postConcat(fInitialTransform);

        
        
        SkIRect bounds = clipRegion.getBounds();
        pdfShader = SkPDFShader::GetPDFShader(*shader, transform, bounds);
        SkSafeUnref(pdfShader.get());  

        if (pdfShader.get()) {
            
            
            int resourceIndex = fShaderResources.find(pdfShader.get());
            if (resourceIndex < 0) {
                resourceIndex = fShaderResources.count();
                fShaderResources.push(pdfShader.get());
                pdfShader->ref();
            }
            entry->fShaderIndex = resourceIndex;
        } else {
            
            
            entry->fShaderIndex = -1;
            entry->fColor = 0;
            color = 0;

            
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
    } else {
        entry->fShaderIndex = -1;
        entry->fColor = SkColorSetA(paint.getColor(), 0xFF);
        color = paint.getColor();
    }

    SkRefPtr<SkPDFGraphicState> newGraphicState;
    if (color == paint.getColor()) {
        newGraphicState = SkPDFGraphicState::GetGraphicStateForPaint(paint);
    } else {
        SkPaint newPaint = paint;
        newPaint.setColor(color);
        newGraphicState = SkPDFGraphicState::GetGraphicStateForPaint(newPaint);
    }
    newGraphicState->unref();  
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
    SkRefPtr<SkPDFFont> newFont = SkPDFFont::GetFontResource(typeface, glyphID);
    newFont->unref();  
    int resourceIndex = fFontResources.find(newFont.get());
    if (resourceIndex < 0) {
        resourceIndex = fFontResources.count();
        fFontResources.push(newFont.get());
        newFont->ref();
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
    
    scaled.setScale(1, -1);
    scaled.postTranslate(0, 1);
    
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

    SkPDFImage* image = SkPDFImage::CreateImage(bitmap, subset, paint);
    if (!image) {
        return;
    }

    fXObjectResources.push(image);  
    SkPDFUtils::DrawFormXObject(fXObjectResources.count() - 1,
                                &content.entry()->fContent);
}
