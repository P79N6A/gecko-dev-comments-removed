






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
#include "SkPathOps.h"
#include "SkPDFFont.h"
#include "SkPDFFormXObject.h"
#include "SkPDFGraphicState.h"
#include "SkPDFImage.h"
#include "SkPDFResourceDict.h"
#include "SkPDFShader.h"
#include "SkPDFStream.h"
#include "SkPDFTypes.h"
#include "SkPDFUtils.h"
#include "SkRect.h"
#include "SkRRect.h"
#include "SkString.h"
#include "SkSurface.h"
#include "SkTextFormatParams.h"
#include "SkTemplates.h"
#include "SkTypefacePriv.h"
#include "SkTSet.h"

#ifdef SK_BUILD_FOR_ANDROID
#include "SkTypeface_android.h"

struct TypefaceFallbackData {
    SkTypeface* typeface;
    int lowerBounds;
    int upperBounds;

    bool operator==(const TypefaceFallbackData& b) const {
        return typeface == b.typeface &&
               lowerBounds == b.lowerBounds &&
               upperBounds == b.upperBounds;
    }
};
#endif

#define DPI_FOR_RASTER_SCALE_ONE 72



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

static int max_glyphid_for_typeface(SkTypeface* typeface) {
    SkAutoResolveDefaultTypeface autoResolve(typeface);
    typeface = autoResolve.get();
    return typeface->countGlyphs() - 1;
}

typedef SkAutoSTMalloc<128, uint16_t> SkGlyphStorage;

static int force_glyph_encoding(const SkPaint& paint, const void* text,
                                size_t len, SkGlyphStorage* storage,
                                uint16_t** glyphIDs) {
    
    if (paint.getTextEncoding() != SkPaint::kGlyphID_TextEncoding) {
        int numGlyphs = paint.textToGlyphs(text, len, NULL);
        storage->reset(numGlyphs);
        paint.textToGlyphs(text, len, storage->get());
        *glyphIDs = storage->get();
        return numGlyphs;
    }

    
    SkASSERT((len & 1) == 0);
    int numGlyphs = SkToInt(len / 2);
    const uint16_t* input =
        reinterpret_cast<uint16_t*>(const_cast<void*>((text)));

    int maxGlyphID = max_glyphid_for_typeface(paint.getTypeface());
    int validated;
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

    for (int i = validated; i < numGlyphs; ++i) {
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

bool GraphicStateEntry::compareInitialState(const GraphicStateEntry& cur) {
    return fColor == cur.fColor &&
           fShaderIndex == cur.fShaderIndex &&
           fGraphicStateIndex == cur.fGraphicStateIndex &&
           fMatrix == cur.fMatrix &&
           fClipStack == cur.fClipStack &&
           (fTextScaleX == 0 ||
               (fTextScaleX == cur.fTextScaleX && fTextFill == cur.fTextFill));
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

#ifdef SK_PDF_USE_PATHOPS




static bool calculate_inverse_path(const SkRect& bounds, const SkPath& invPath,
                                   SkPath* outPath) {
    SkASSERT(invPath.isInverseFillType());

    SkPath clipPath;
    clipPath.addRect(bounds);

    return Op(clipPath, invPath, kIntersect_PathOp, outPath);
}





SK_COMPILE_ASSERT(SkRegion::kDifference_Op == (int)kDifference_PathOp,
                  region_pathop_mismatch);
SK_COMPILE_ASSERT(SkRegion::kIntersect_Op == (int)kIntersect_PathOp,
                  region_pathop_mismatch);
SK_COMPILE_ASSERT(SkRegion::kUnion_Op == (int)kUnion_PathOp,
                  region_pathop_mismatch);
SK_COMPILE_ASSERT(SkRegion::kXOR_Op == (int)kXOR_PathOp,
                  region_pathop_mismatch);
SK_COMPILE_ASSERT(SkRegion::kReverseDifference_Op ==
                  (int)kReverseDifference_PathOp,
                  region_pathop_mismatch);

static SkPathOp region_op_to_pathops_op(SkRegion::Op op) {
    SkASSERT(op >= 0);
    SkASSERT(op <= SkRegion::kReverseDifference_Op);
    return (SkPathOp)op;
}







static bool get_clip_stack_path(const SkMatrix& transform,
                                const SkClipStack& clipStack,
                                const SkRegion& clipRegion,
                                SkPath* outClipPath) {
    outClipPath->reset();
    outClipPath->setFillType(SkPath::kInverseWinding_FillType);

    const SkClipStack::Element* clipEntry;
    SkClipStack::Iter iter;
    iter.reset(clipStack, SkClipStack::Iter::kBottom_IterStart);
    for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
        SkPath entryPath;
        if (SkClipStack::Element::kEmpty_Type == clipEntry->getType()) {
            outClipPath->reset();
            outClipPath->setFillType(SkPath::kInverseWinding_FillType);
            continue;
        } else {
            clipEntry->asPath(&entryPath);
        }
        entryPath.transform(transform);

        if (SkRegion::kReplace_Op == clipEntry->getOp()) {
            *outClipPath = entryPath;
        } else {
            SkPathOp op = region_op_to_pathops_op(clipEntry->getOp());
            if (!Op(*outClipPath, entryPath, op, outClipPath)) {
                return false;
            }
        }
    }

    if (outClipPath->isInverseFillType()) {
        
        
        
        SkRect clipBounds = SkRect::Make(clipRegion.getBounds());
        clipBounds.outset(SK_Scalar1, SK_Scalar1);
        if (!calculate_inverse_path(clipBounds, *outClipPath, outClipPath)) {
            return false;
        }
    }
    return true;
}
#endif




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

    currentEntry()->fClipStack = clipStack;
    currentEntry()->fClipRegion = clipRegion;

    SkMatrix transform;
    transform.setTranslate(translation.fX, translation.fY);

#ifdef SK_PDF_USE_PATHOPS
    SkPath clipPath;
    if (get_clip_stack_path(transform, clipStack, clipRegion, &clipPath)) {
        emit_clip(&clipPath, NULL, fContentStream);
        return;
    }
#endif
    
    
    
    
    
    
    
    
    SkClipStack::Iter iter;
    skip_clip_stack_prefix(fEntries[0].fClipStack, clipStack, &iter);

    
    
    bool needRegion = false;
    const SkClipStack::Element* clipEntry;
    for (clipEntry = iter.next(); clipEntry; clipEntry = iter.next()) {
        if (clipEntry->getOp() != SkRegion::kIntersect_Op ||
                clipEntry->isInverseFilled()) {
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
                default: {
                    SkPath translatedPath;
                    clipEntry->asPath(&translatedPath);
                    translatedPath.transform(transform, &translatedPath);
                    emit_clip(&translatedPath, NULL, fContentStream);
                    break;
                }
            }
        }
    }
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
            SkPDFUtils::ApplyPattern(state.fShaderIndex, fContentStream);
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

SkBaseDevice* SkPDFDevice::onCreateDevice(const SkImageInfo& info, Usage usage) {
    SkMatrix initialTransform;
    initialTransform.reset();
    SkISize size = SkISize::Make(info.width(), info.height());
    return SkNEW_ARGS(SkPDFDevice, (size, size, initialTransform));
}


struct ContentEntry {
    GraphicStateEntry fState;
    SkDynamicMemoryWStream fContent;
    SkAutoTDelete<ContentEntry> fNext;

    
    
    ~ContentEntry() {
        ContentEntry* val = fNext.detach();
        while (val != NULL) {
            ContentEntry* valNext = val->fNext.detach();
            
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
          fXfermode(SkXfermode::kSrcOver_Mode),
          fDstFormXObject(NULL) {
        init(draw.fClipStack, *draw.fClip, *draw.fMatrix, paint, hasText);
    }
    ScopedContentEntry(SkPDFDevice* device, const SkClipStack* clipStack,
                       const SkRegion& clipRegion, const SkMatrix& matrix,
                       const SkPaint& paint, bool hasText = false)
        : fDevice(device),
          fContentEntry(NULL),
          fXfermode(SkXfermode::kSrcOver_Mode),
          fDstFormXObject(NULL) {
        init(clipStack, clipRegion, matrix, paint, hasText);
    }

    ~ScopedContentEntry() {
        if (fContentEntry) {
            SkPath* shape = &fShape;
            if (shape->isEmpty()) {
                shape = NULL;
            }
            fDevice->finishContentEntry(fXfermode, fDstFormXObject, shape);
        }
        SkSafeUnref(fDstFormXObject);
    }

    ContentEntry* entry() { return fContentEntry; }

    
    bool needShape() {
        switch (fXfermode) {
            case SkXfermode::kClear_Mode:
            case SkXfermode::kSrc_Mode:
            case SkXfermode::kSrcIn_Mode:
            case SkXfermode::kSrcOut_Mode:
            case SkXfermode::kDstIn_Mode:
            case SkXfermode::kDstOut_Mode:
            case SkXfermode::kSrcATop_Mode:
            case SkXfermode::kDstATop_Mode:
            case SkXfermode::kModulate_Mode:
                return true;
            default:
                return false;
        }
    }

    
    bool needSource() {
        if (fXfermode == SkXfermode::kClear_Mode) {
            return false;
        }
        return true;
    }

    



    void setShape(const SkPath& shape) {
        fShape = shape;
    }

private:
    SkPDFDevice* fDevice;
    ContentEntry* fContentEntry;
    SkXfermode::Mode fXfermode;
    SkPDFFormXObject* fDstFormXObject;
    SkPath fShape;

    void init(const SkClipStack* clipStack, const SkRegion& clipRegion,
              const SkMatrix& matrix, const SkPaint& paint, bool hasText) {
        
        if (matrix.hasPerspective()) {
            NOT_IMPLEMENTED(!matrix.hasPerspective(), false);
            return;
        }
        if (paint.getXfermode()) {
            paint.getXfermode()->asMode(&fXfermode);
        }
        fContentEntry = fDevice->setUpContentEntry(clipStack, clipRegion,
                                                   matrix, paint, hasText,
                                                   &fDstFormXObject);
    }
};



static inline SkImageInfo make_content_info(const SkISize& contentSize,
                                            const SkMatrix* initialTransform) {
    SkImageInfo info;
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
        info = SkImageInfo::MakeUnknown(abs(size.fWidth), abs(size.fHeight));
    } else {
        info = SkImageInfo::MakeUnknown(abs(contentSize.fWidth),
                                        abs(contentSize.fHeight));
    }
    return info;
}


SkPDFDevice::SkPDFDevice(const SkISize& pageSize, const SkISize& contentSize,
                         const SkMatrix& initialTransform)
    : fPageSize(pageSize)
    , fContentSize(contentSize)
    , fLastContentEntry(NULL)
    , fLastMarginContentEntry(NULL)
    , fClipStack(NULL)
    , fEncoder(NULL)
    , fRasterDpi(72.0f)
{
    const SkImageInfo info = make_content_info(contentSize, &initialTransform);

    
    
    NOT_IMPLEMENTED(initialTransform.hasPerspective(), true);

    
    
    
    fInitialTransform.setTranslate(0, SkIntToScalar(pageSize.fHeight));
    fInitialTransform.preScale(SK_Scalar1, -SK_Scalar1);
    fInitialTransform.preConcat(initialTransform);
    fLegacyBitmap.setInfo(info);

    SkIRect existingClip = SkIRect::MakeWH(info.width(), info.height());
    fExistingClipRegion.setRect(existingClip);
    this->init();
}


SkPDFDevice::SkPDFDevice(const SkISize& layerSize,
                         const SkClipStack& existingClipStack,
                         const SkRegion& existingClipRegion)
    : fPageSize(layerSize)
    , fContentSize(layerSize)
    , fExistingClipStack(existingClipStack)
    , fExistingClipRegion(existingClipRegion)
    , fLastContentEntry(NULL)
    , fLastMarginContentEntry(NULL)
    , fClipStack(NULL)
    , fEncoder(NULL)
    , fRasterDpi(72.0f)
{
    fInitialTransform.reset();
    fLegacyBitmap.setInfo(make_content_info(layerSize, NULL));

    this->init();
}

SkPDFDevice::~SkPDFDevice() {
    this->cleanUp(true);
}

void SkPDFDevice::init() {
    fAnnotations = NULL;
    fResourceDict = NULL;
    fContentEntries.free();
    fLastContentEntry = NULL;
    fMarginContentEntries.free();
    fLastMarginContentEntry = NULL;
    fDrawingArea = kContent_DrawingArea;
    if (fFontGlyphUsage.get() == NULL) {
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
    SkMatrix inverse;
    if (!contentEntry->fState.fMatrix.invert(&inverse)) {
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

void SkPDFDevice::drawRect(const SkDraw& d, const SkRect& rect,
                           const SkPaint& paint) {
    SkRect r = rect;
    r.sort();

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

void SkPDFDevice::drawRRect(const SkDraw& draw, const SkRRect& rrect, const SkPaint& paint) {
    SkPath  path;
    path.addRRect(rrect);
    this->drawPath(draw, path, paint, NULL, true);
}

void SkPDFDevice::drawOval(const SkDraw& draw, const SkRect& oval, const SkPaint& paint) {
    SkPath  path;
    path.addOval(oval);
    this->drawPath(draw, path, paint, NULL, true);
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
            matrix.preConcat(*prePathMatrix);
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

#ifdef SK_PDF_USE_PATHOPS
    if (handleInversePath(d, origPath, paint, pathIsMutable, prePathMatrix)) {
        return;
    }
#endif

    if (handleRectAnnotation(pathPtr->getBounds(), matrix, paint)) {
        return;
    }

    ScopedContentEntry content(this, d.fClipStack, *d.fClip, matrix, paint);
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
                                 const SkPaint& paint,
                                 SkCanvas::DrawBitmapRectFlags flags) {
    
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
    this->drawBitmap(draw, *bitmapPtr, matrix, paint);
}

void SkPDFDevice::drawBitmap(const SkDraw& d, const SkBitmap& bitmap,
                             const SkMatrix& matrix, const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }

    SkMatrix transform = matrix;
    transform.postConcat(*d.fMatrix);
    this->internalDrawBitmap(transform, d.fClipStack, *d.fClip, bitmap, NULL,
                             paint);
}

void SkPDFDevice::drawSprite(const SkDraw& d, const SkBitmap& bitmap,
                             int x, int y, const SkPaint& paint) {
    if (d.fClip->isEmpty()) {
        return;
    }

    SkMatrix matrix;
    matrix.setTranslate(SkIntToScalar(x), SkIntToScalar(y));
    this->internalDrawBitmap(matrix, d.fClipStack, *d.fClip, bitmap, NULL,
                             paint);
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
    int numGlyphs = force_glyph_encoding(paint, text, len, &storage, &glyphIDs);
    textPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

    SkDrawCacheProc glyphCacheProc = textPaint.getDrawCacheProc();
    align_text(glyphCacheProc, textPaint, glyphIDs, numGlyphs, &x, &y);
    content.entry()->fContent.writeText("BT\n");
    set_text_transform(x, y, textPaint.getTextSkewX(),
                       &content.entry()->fContent);
    int consumedGlyphCount = 0;
    while (numGlyphs > consumedGlyphCount) {
        updateFont(textPaint, glyphIDs[consumedGlyphCount], content.entry());
        SkPDFFont* font = content.entry()->fState.fFont;
        int availableGlyphs =
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

#ifdef SK_BUILD_FOR_ANDROID
    











    if (paint.getPaintOptionsAndroid().isUsingFontFallbacks()) {
        uint16_t* glyphIDs = NULL;
        SkGlyphStorage tmpStorage(0);
        size_t numGlyphs = 0;

        
        if (paint.getTextEncoding() == SkPaint::kGlyphID_TextEncoding) {
            numGlyphs = len / 2;
            glyphIDs = reinterpret_cast<uint16_t*>(const_cast<void*>(text));
        } else {
            numGlyphs = paint.textToGlyphs(text, len, NULL);
            tmpStorage.reset(numGlyphs);
            paint.textToGlyphs(text, len, tmpStorage.get());
            glyphIDs = tmpStorage.get();
        }

        
        SkAutoTUnref<SkTypeface> origFace(SkSafeRef(paint.getTypeface()));
        if (NULL == origFace.get()) {
            origFace.reset(SkTypeface::RefDefault());
        }
        const uint16_t origGlyphCount = origFace->countGlyphs();

        
        SkTDArray<TypefaceFallbackData> visitedTypefaces;

        
        bool usesOriginalTypeface = false;
        for (uint16_t x = 0; x < numGlyphs; ++x) {
            
            
            if (glyphIDs[x] < origGlyphCount) {
                usesOriginalTypeface = true;
                continue;
            }

            
            TypefaceFallbackData data;
            data.typeface =
                    SkGetTypefaceForGlyphID(glyphIDs[x], origFace.get(),
                                            paint.getPaintOptionsAndroid(),
                                            &data.lowerBounds,
                                            &data.upperBounds);
            
            if (data.typeface && !visitedTypefaces.contains(data)) {
                visitedTypefaces.push(data);
            }
        }

        
        if (usesOriginalTypeface) {
            TypefaceFallbackData* data = visitedTypefaces.push();
            data->typeface = origFace.get();
            data->lowerBounds = 0;
            data->upperBounds = origGlyphCount;
        }

        
        SkAutoTMalloc<SkScalar> posStorage(len * scalarsPerPos);
        SkScalar* tmpPos = posStorage.get();
        SkGlyphStorage glyphStorage(numGlyphs);
        uint16_t* tmpGlyphIDs = glyphStorage.get();

        
        
        for (int x = 0; x < visitedTypefaces.count(); ++x) {
            const TypefaceFallbackData& data = visitedTypefaces[x];

            int tmpGlyphCount = 0;
            for (uint16_t y = 0; y < numGlyphs; ++y) {
                if (glyphIDs[y] >= data.lowerBounds &&
                        glyphIDs[y] < data.upperBounds) {
                    tmpGlyphIDs[tmpGlyphCount] = glyphIDs[y] - data.lowerBounds;
                    memcpy(&(tmpPos[tmpGlyphCount * scalarsPerPos]),
                           &(pos[y * scalarsPerPos]),
                           scalarsPerPos * sizeof(SkScalar));
                    tmpGlyphCount++;
                }
            }

            
            SkPaint tmpPaint = paint;
            tmpPaint.setTypeface(data.typeface);
            tmpPaint.setTextEncoding(SkPaint::kGlyphID_TextEncoding);

            
            SkPaintOptionsAndroid paintOpts = tmpPaint.getPaintOptionsAndroid();
            paintOpts.setUseFontFallbacks(false);
            tmpPaint.setPaintOptionsAndroid(paintOpts);

            this->drawPosText(d, tmpGlyphIDs, tmpGlyphCount * 2, tmpPos, constY,
                              scalarsPerPos, tmpPaint);
        }
        return;
    }
#endif

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
    
}

void SkPDFDevice::drawDevice(const SkDraw& d, SkBaseDevice* device,
                             int x, int y, const SkPaint& paint) {
    
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
    if (content.needShape()) {
        SkPath shape;
        shape.addRect(SkRect::MakeXYWH(SkIntToScalar(x), SkIntToScalar(y),
                                       SkIntToScalar(device->width()),
                                       SkIntToScalar(device->height())));
        content.setShape(shape);
    }
    if (!content.needSource()) {
        return;
    }

    SkAutoTUnref<SkPDFFormXObject> xObject(new SkPDFFormXObject(pdfDevice));
    SkPDFUtils::DrawFormXObject(this->addXObjectResource(xObject.get()),
                                &content.entry()->fContent);

    
    fFontGlyphUsage->merge(pdfDevice->getFontGlyphUsage());
}

SkImageInfo SkPDFDevice::imageInfo() const {
    return fLegacyBitmap.info();
}

void SkPDFDevice::onAttachToCanvas(SkCanvas* canvas) {
    INHERITED::onAttachToCanvas(canvas);

    
    fClipStack = canvas->getClipStack();
}

void SkPDFDevice::onDetachFromCanvas() {
    INHERITED::onDetachFromCanvas();

    fClipStack = NULL;
}

SkSurface* SkPDFDevice::newSurface(const SkImageInfo& info) {
    return SkSurface::NewRaster(info);
}

ContentEntry* SkPDFDevice::getLastContentEntry() {
    if (fDrawingArea == kContent_DrawingArea) {
        return fLastContentEntry;
    } else {
        return fLastMarginContentEntry;
    }
}

SkAutoTDelete<ContentEntry>* SkPDFDevice::getContentEntries() {
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

SkPDFResourceDict* SkPDFDevice::getResourceDict() {
    if (NULL == fResourceDict) {
        fResourceDict = SkNEW(SkPDFResourceDict);

        if (fGraphicStateResources.count()) {
            for (int i = 0; i < fGraphicStateResources.count(); i++) {
                fResourceDict->insertResourceAsReference(
                        SkPDFResourceDict::kExtGState_ResourceType,
                        i, fGraphicStateResources[i]);
            }
        }

        if (fXObjectResources.count()) {
            for (int i = 0; i < fXObjectResources.count(); i++) {
                fResourceDict->insertResourceAsReference(
                        SkPDFResourceDict::kXObject_ResourceType,
                        i, fXObjectResources[i]);
            }
        }

        if (fFontResources.count()) {
            for (int i = 0; i < fFontResources.count(); i++) {
                fResourceDict->insertResourceAsReference(
                        SkPDFResourceDict::kFont_ResourceType,
                        i, fFontResources[i]);
            }
        }

        if (fShaderResources.count()) {
            SkAutoTUnref<SkPDFDict> patterns(new SkPDFDict());
            for (int i = 0; i < fShaderResources.count(); i++) {
                fResourceDict->insertResourceAsReference(
                        SkPDFResourceDict::kPattern_ResourceType,
                        i, fShaderResources[i]);
            }
        }
    }
    return fResourceDict;
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

#ifdef SK_PDF_USE_PATHOPS







bool SkPDFDevice::handleInversePath(const SkDraw& d, const SkPath& origPath,
                                    const SkPaint& paint, bool pathIsMutable,
                                    const SkMatrix* prePathMatrix) {
    if (!origPath.isInverseFillType()) {
        return false;
    }

    if (d.fClip->isEmpty()) {
        return false;
    }

    SkPath modifiedPath;
    SkPath* pathPtr = const_cast<SkPath*>(&origPath);
    SkPaint noInversePaint(paint);

    
    if (SkPaint::kStroke_Style == paint.getStyle() ||
        SkPaint::kStrokeAndFill_Style == paint.getStyle()) {
        bool doFillPath = paint.getFillPath(origPath, &modifiedPath);
        if (doFillPath) {
            noInversePaint.setStyle(SkPaint::kFill_Style);
            noInversePaint.setStrokeWidth(0);
            pathPtr = &modifiedPath;
        } else {
            
            
            modifiedPath.toggleInverseFillType();
            drawPath(d, modifiedPath, paint, NULL, true);
            return true;
        }
    }

    
    
    SkRect bounds;
    SkMatrix transformInverse;
    SkMatrix totalMatrix = *d.fMatrix;
    if (prePathMatrix) {
        totalMatrix.preConcat(*prePathMatrix);
    }
    if (!totalMatrix.invert(&transformInverse)) {
        return false;
    }
    bounds.set(d.fClip->getBounds());
    transformInverse.mapRect(&bounds);

    
    
    bounds.outset(paint.getStrokeWidth() + SK_Scalar1,
                  paint.getStrokeWidth() + SK_Scalar1);

    if (!calculate_inverse_path(bounds, *pathPtr, &modifiedPath)) {
        return false;
    }

    drawPath(d, modifiedPath, noInversePaint, prePathMatrix, true);
    return true;
}
#endif

bool SkPDFDevice::handleRectAnnotation(const SkRect& r, const SkMatrix& matrix,
                                       const SkPaint& p) {
    SkAnnotation* annotationInfo = p.getAnnotation();
    if (!annotationInfo) {
        return false;
    }
    SkData* urlData = annotationInfo->find(SkAnnotationKeys::URL_Key());
    if (urlData) {
        handleLinkToURL(urlData, r, matrix);
        return p.getAnnotation() != NULL;
    }
    SkData* linkToName = annotationInfo->find(
            SkAnnotationKeys::Link_Named_Dest_Key());
    if (linkToName) {
        handleLinkToNamedDest(linkToName, r, matrix);
        return p.getAnnotation() != NULL;
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
    SkData* nameData = annotationInfo->find(
            SkAnnotationKeys::Define_Named_Dest_Key());
    if (nameData) {
        for (size_t i = 0; i < count; i++) {
            defineNamedDestination(nameData, points[i], matrix);
        }
        return paint.getAnnotation() != NULL;
    }
    return false;
}

SkPDFDict* SkPDFDevice::createLinkAnnotation(const SkRect& r,
                                             const SkMatrix& matrix) {
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
        dict->insert(static_cast<const char *>(dest->nameData->data()),
                     pdfDest);
    }
}

SkPDFFormXObject* SkPDFDevice::createFormXObjectFromDevice() {
    SkPDFFormXObject* xobject = SkNEW_ARGS(SkPDFFormXObject, (this));
    
    
    
    cleanUp(false);  
    init();
    return xobject;
}

void SkPDFDevice::drawFormXObjectWithMask(int xObjectIndex,
                                          SkPDFFormXObject* mask,
                                          const SkClipStack* clipStack,
                                          const SkRegion& clipRegion,
                                          SkXfermode::Mode mode,
                                          bool invertClip) {
    if (clipRegion.isEmpty() && !invertClip) {
        return;
    }

    SkAutoTUnref<SkPDFGraphicState> sMaskGS(
        SkPDFGraphicState::GetSMaskGraphicState(
            mask, invertClip, SkPDFGraphicState::kAlpha_SMaskMode));

    SkMatrix identity;
    identity.reset();
    SkPaint paint;
    paint.setXfermodeMode(mode);
    ScopedContentEntry content(this, clipStack, clipRegion, identity, paint);
    if (!content.entry()) {
        return;
    }
    SkPDFUtils::ApplyGraphicState(addGraphicStateResource(sMaskGS.get()),
                                  &content.entry()->fContent);
    SkPDFUtils::DrawFormXObject(xObjectIndex, &content.entry()->fContent);

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

    
    
    if (xfermode == SkXfermode::kClear_Mode       ||
            xfermode == SkXfermode::kSrc_Mode     ||
            xfermode == SkXfermode::kSrcIn_Mode   ||
            xfermode == SkXfermode::kDstIn_Mode   ||
            xfermode == SkXfermode::kSrcOut_Mode  ||
            xfermode == SkXfermode::kDstOut_Mode  ||
            xfermode == SkXfermode::kSrcATop_Mode ||
            xfermode == SkXfermode::kDstATop_Mode ||
            xfermode == SkXfermode::kModulate_Mode) {
        if (!isContentEmpty()) {
            *dst = createFormXObjectFromDevice();
            SkASSERT(isContentEmpty());
        } else if (xfermode != SkXfermode::kSrc_Mode &&
                   xfermode != SkXfermode::kSrcOut_Mode) {
            
            
            return NULL;
        }
    }
    
    

    
    if (xfermode == SkXfermode::kDst_Mode) {
        return NULL;
    }

    ContentEntry* entry;
    SkAutoTDelete<ContentEntry> newEntry;

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

    SkAutoTDelete<ContentEntry>* contentEntries = getContentEntries();
    if (!lastContentEntry) {
        contentEntries->reset(entry);
        setLastContentEntry(entry);
    } else if (xfermode == SkXfermode::kDstOver_Mode) {
        entry->fNext.reset(contentEntries->detach());
        contentEntries->reset(entry);
    } else {
        lastContentEntry->fNext.reset(entry);
        setLastContentEntry(entry);
    }
    newEntry.detach();
    return entry;
}

void SkPDFDevice::finishContentEntry(SkXfermode::Mode xfermode,
                                     SkPDFFormXObject* dst,
                                     SkPath* shape) {
    if (xfermode != SkXfermode::kClear_Mode       &&
            xfermode != SkXfermode::kSrc_Mode     &&
            xfermode != SkXfermode::kDstOver_Mode &&
            xfermode != SkXfermode::kSrcIn_Mode   &&
            xfermode != SkXfermode::kDstIn_Mode   &&
            xfermode != SkXfermode::kSrcOut_Mode  &&
            xfermode != SkXfermode::kDstOut_Mode  &&
            xfermode != SkXfermode::kSrcATop_Mode &&
            xfermode != SkXfermode::kDstATop_Mode &&
            xfermode != SkXfermode::kModulate_Mode) {
        SkASSERT(!dst);
        return;
    }
    if (xfermode == SkXfermode::kDstOver_Mode) {
        SkASSERT(!dst);
        ContentEntry* firstContentEntry = getContentEntries()->get();
        if (firstContentEntry->fContent.getOffset() == 0) {
            
            
            
            SkAutoTDelete<ContentEntry>* contentEntries = getContentEntries();
            contentEntries->reset(firstContentEntry->fNext.detach());
        }
        return;
    }
    if (!dst) {
        SkASSERT(xfermode == SkXfermode::kSrc_Mode ||
                 xfermode == SkXfermode::kSrcOut_Mode);
        return;
    }

    ContentEntry* contentEntries = getContentEntries()->get();
    SkASSERT(dst);
    SkASSERT(!contentEntries->fNext.get());
    
    
    
    
    SkClipStack clipStack = contentEntries->fState.fClipStack;
    SkRegion clipRegion = contentEntries->fState.fClipRegion;

    SkMatrix identity;
    identity.reset();
    SkPaint stockPaint;

    SkAutoTUnref<SkPDFFormXObject> srcFormXObject;
    if (isContentEmpty()) {
        
        
        
        
        
        if (shape == NULL || xfermode == SkXfermode::kDstOut_Mode ||
                xfermode == SkXfermode::kSrcATop_Mode) {
            ScopedContentEntry content(this, &fExistingClipStack,
                                       fExistingClipRegion, identity,
                                       stockPaint);
            SkPDFUtils::DrawFormXObject(this->addXObjectResource(dst),
                                        &content.entry()->fContent);
            return;
        } else {
            xfermode = SkXfermode::kClear_Mode;
        }
    } else {
        SkASSERT(!fContentEntries->fNext.get());
        srcFormXObject.reset(createFormXObjectFromDevice());
    }

    
    
    if (xfermode == SkXfermode::kSrcATop_Mode) {
        
        
        
        
        drawFormXObjectWithMask(addXObjectResource(srcFormXObject.get()), dst,
                                &fExistingClipStack, fExistingClipRegion,
                                SkXfermode::kSrcOver_Mode, true);
    } else {
        SkAutoTUnref<SkPDFFormXObject> dstMaskStorage;
        SkPDFFormXObject* dstMask = srcFormXObject.get();
        if (shape != NULL) {
            
            SkDraw d;
            d.fMatrix = &identity;
            d.fClip = &clipRegion;
            d.fClipStack = &clipStack;
            SkPaint filledPaint;
            filledPaint.setColor(SK_ColorBLACK);
            filledPaint.setStyle(SkPaint::kFill_Style);
            this->drawPath(d, *shape, filledPaint, NULL, true);

            dstMaskStorage.reset(createFormXObjectFromDevice());
            dstMask = dstMaskStorage.get();
        }
        drawFormXObjectWithMask(addXObjectResource(dst), dstMask,
                                &fExistingClipStack, fExistingClipRegion,
                                SkXfermode::kSrcOver_Mode, true);
    }

    if (xfermode == SkXfermode::kClear_Mode) {
        return;
    } else if (xfermode == SkXfermode::kSrc_Mode ||
            xfermode == SkXfermode::kDstATop_Mode) {
        ScopedContentEntry content(this, &fExistingClipStack,
                                   fExistingClipRegion, identity, stockPaint);
        if (content.entry()) {
            SkPDFUtils::DrawFormXObject(
                    this->addXObjectResource(srcFormXObject.get()),
                    &content.entry()->fContent);
        }
        if (xfermode == SkXfermode::kSrc_Mode) {
            return;
        }
    } else if (xfermode == SkXfermode::kSrcATop_Mode) {
        ScopedContentEntry content(this, &fExistingClipStack,
                                   fExistingClipRegion, identity, stockPaint);
        if (content.entry()) {
            SkPDFUtils::DrawFormXObject(this->addXObjectResource(dst),
                                        &content.entry()->fContent);
        }
    }

    SkASSERT(xfermode == SkXfermode::kSrcIn_Mode   ||
             xfermode == SkXfermode::kDstIn_Mode   ||
             xfermode == SkXfermode::kSrcOut_Mode  ||
             xfermode == SkXfermode::kDstOut_Mode  ||
             xfermode == SkXfermode::kSrcATop_Mode ||
             xfermode == SkXfermode::kDstATop_Mode ||
             xfermode == SkXfermode::kModulate_Mode);

    if (xfermode == SkXfermode::kSrcIn_Mode ||
            xfermode == SkXfermode::kSrcOut_Mode ||
            xfermode == SkXfermode::kSrcATop_Mode) {
        drawFormXObjectWithMask(addXObjectResource(srcFormXObject.get()), dst,
                                &fExistingClipStack, fExistingClipRegion,
                                SkXfermode::kSrcOver_Mode,
                                xfermode == SkXfermode::kSrcOut_Mode);
    } else {
        SkXfermode::Mode mode = SkXfermode::kSrcOver_Mode;
        if (xfermode == SkXfermode::kModulate_Mode) {
            drawFormXObjectWithMask(addXObjectResource(srcFormXObject.get()),
                                    dst, &fExistingClipStack,
                                    fExistingClipRegion,
                                    SkXfermode::kSrcOver_Mode, false);
            mode = SkXfermode::kMultiply_Mode;
        }
        drawFormXObjectWithMask(addXObjectResource(dst), srcFormXObject.get(),
                                &fExistingClipStack, fExistingClipRegion, mode,
                                xfermode == SkXfermode::kDstOut_Mode);
    }
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
    NOT_IMPLEMENTED(paint.getPathEffect() != NULL, false);
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

int SkPDFDevice::addXObjectResource(SkPDFObject* xObject) {
    
    
    int result = fXObjectResources.find(xObject);
    if (result < 0) {
        result = fXObjectResources.count();
        fXObjectResources.push(xObject);
        xObject->ref();
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
        contentEntry->fContent.writeText("/");
        contentEntry->fContent.writeText(SkPDFResourceDict::getResourceName(
                SkPDFResourceDict::kFont_ResourceType,
                fontIndex).c_str());
        contentEntry->fContent.writeText(" ");
        SkPDFScalar::Append(paint.getTextSize(), &contentEntry->fContent);
        contentEntry->fContent.writeText(" Tf\n");
        contentEntry->fState.fFont = fFontResources[fontIndex];
    }
}

int SkPDFDevice::getFontResourceIndex(SkTypeface* typeface, uint16_t glyphID) {
    SkAutoTUnref<SkPDFFont> newFont(SkPDFFont::GetFontResource(typeface,
                                                               glyphID));
    int resourceIndex = fFontResources.find(newFont.get());
    if (resourceIndex < 0) {
        resourceIndex = fFontResources.count();
        fFontResources.push(newFont.get());
        newFont.get()->ref();
    }
    return resourceIndex;
}

void SkPDFDevice::internalDrawBitmap(const SkMatrix& origMatrix,
                                     const SkClipStack* clipStack,
                                     const SkRegion& origClipRegion,
                                     const SkBitmap& origBitmap,
                                     const SkIRect* srcRect,
                                     const SkPaint& paint) {
    SkMatrix matrix = origMatrix;
    SkRegion perspectiveBounds;
    const SkRegion* clipRegion = &origClipRegion;
    SkBitmap perspectiveBitmap;
    const SkBitmap* bitmap = &origBitmap;
    SkBitmap tmpSubsetBitmap;

    
    if (origMatrix.hasPerspective()) {
        if (fRasterDpi == 0) {
            return;
        }
        SkBitmap* subsetBitmap;
        if (srcRect) {
            if (!origBitmap.extractSubset(&tmpSubsetBitmap, *srcRect)) {
               return;
            }
            subsetBitmap = &tmpSubsetBitmap;
        } else {
            subsetBitmap = &tmpSubsetBitmap;
            *subsetBitmap = origBitmap;
        }
        srcRect = NULL;

        
        
        SkPath perspectiveOutline;
        perspectiveOutline.addRect(
                SkRect::MakeWH(SkIntToScalar(subsetBitmap->width()),
                               SkIntToScalar(subsetBitmap->height())));
        perspectiveOutline.transform(origMatrix);

        
        
        SkRect bounds = perspectiveOutline.getBounds();

        
        
        SkMatrix total = origMatrix;
        total.postConcat(fInitialTransform);
        total.postScale(SkIntToScalar(fRasterDpi) /
                            SkIntToScalar(DPI_FOR_RASTER_SCALE_ONE),
                        SkIntToScalar(fRasterDpi) /
                            SkIntToScalar(DPI_FOR_RASTER_SCALE_ONE));
        SkPath physicalPerspectiveOutline;
        physicalPerspectiveOutline.addRect(
                SkRect::MakeWH(SkIntToScalar(subsetBitmap->width()),
                               SkIntToScalar(subsetBitmap->height())));
        physicalPerspectiveOutline.transform(total);

        SkScalar scaleX = physicalPerspectiveOutline.getBounds().width() /
                              bounds.width();
        SkScalar scaleY = physicalPerspectiveOutline.getBounds().height() /
                              bounds.height();

        
        
        
        
        
        

        const int w = SkScalarCeilToInt(physicalPerspectiveOutline.getBounds().width());
        const int h = SkScalarCeilToInt(physicalPerspectiveOutline.getBounds().height());
        if (!perspectiveBitmap.allocPixels(SkImageInfo::MakeN32Premul(w, h))) {
            return;
        }
        perspectiveBitmap.eraseColor(SK_ColorTRANSPARENT);

        SkCanvas canvas(perspectiveBitmap);

        SkScalar deltaX = bounds.left();
        SkScalar deltaY = bounds.top();

        SkMatrix offsetMatrix = origMatrix;
        offsetMatrix.postTranslate(-deltaX, -deltaY);
        offsetMatrix.postScale(scaleX, scaleY);

        
        
        canvas.setMatrix(offsetMatrix);

        canvas.drawBitmap(*subsetBitmap, SkIntToScalar(0), SkIntToScalar(0));

        
        canvas.flush();

        
        
        matrix.setScale(1 / scaleX, 1 / scaleY);
        matrix.postTranslate(deltaX, deltaY);

        perspectiveBounds.setRect(
                SkIRect::MakeXYWH(SkScalarFloorToInt(bounds.x()),
                                  SkScalarFloorToInt(bounds.y()),
                                  SkScalarCeilToInt(bounds.width()),
                                  SkScalarCeilToInt(bounds.height())));
        clipRegion = &perspectiveBounds;
        srcRect = NULL;
        bitmap = &perspectiveBitmap;
    }

    SkMatrix scaled;
    
    scaled.setScale(SK_Scalar1, -SK_Scalar1);
    scaled.postTranslate(0, SK_Scalar1);
    
    SkIRect subset = SkIRect::MakeWH(bitmap->width(), bitmap->height());
    scaled.postScale(SkIntToScalar(subset.width()),
                     SkIntToScalar(subset.height()));
    scaled.postConcat(matrix);
    ScopedContentEntry content(this, clipStack, *clipRegion, scaled, paint);
    if (!content.entry() || (srcRect && !subset.intersect(*srcRect))) {
        return;
    }
    if (content.needShape()) {
        SkPath shape;
        shape.addRect(SkRect::MakeWH(SkIntToScalar(subset.width()),
                                     SkIntToScalar( subset.height())));
        shape.transform(matrix);
        content.setShape(shape);
    }
    if (!content.needSource()) {
        return;
    }

    SkAutoTUnref<SkPDFImage> image(
        SkPDFImage::CreateImage(*bitmap, subset, fEncoder));
    if (!image) {
        return;
    }

    SkPDFUtils::DrawFormXObject(this->addXObjectResource(image.get()),
                                &content.entry()->fContent);
}

