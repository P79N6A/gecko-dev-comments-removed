






#include "SkPictureRecord.h"
#include "SkTSearch.h"
#include "SkPixelRef.h"
#include "SkBBoxHierarchy.h"
#include "SkPictureStateTree.h"

#define MIN_WRITER_SIZE 16384
#define HEAP_BLOCK_SIZE 4096

enum {
    kNoInitialSave = -1,
};

SkPictureRecord::SkPictureRecord(uint32_t flags) :
        fBoundingHierarchy(NULL),
        fStateTree(NULL),
        fFlattenableHeap(HEAP_BLOCK_SIZE),
        fMatrices(&fFlattenableHeap),
        fPaints(&fFlattenableHeap),
        fRegions(&fFlattenableHeap),
        fWriter(MIN_WRITER_SIZE),
        fRecordFlags(flags) {
#ifdef SK_DEBUG_SIZE
    fPointBytes = fRectBytes = fTextBytes = 0;
    fPointWrites = fRectWrites = fTextWrites = 0;
#endif

    fRestoreOffsetStack.setReserve(32);
    fInitialSaveCount = kNoInitialSave;

    fBitmapHeap = SkNEW(SkBitmapHeap);
    fFlattenableHeap.setBitmapStorage(fBitmapHeap);
    fPathHeap = NULL;   
    fFirstSavedLayerIndex = kNoSavedLayerIndex;
}

SkPictureRecord::~SkPictureRecord() {
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fPathHeap);
    SkSafeUnref(fBoundingHierarchy);
    SkSafeUnref(fStateTree);
    fFlattenableHeap.setBitmapStorage(NULL);
    fPictureRefs.unrefAll();
}



SkDevice* SkPictureRecord::setDevice(SkDevice* device) {
    SkASSERT(kNoInitialSave == fInitialSaveCount);
    this->INHERITED::setDevice(device);

    
    
    fInitialSaveCount = this->save(SkCanvas::kMatrixClip_SaveFlag);
    return device;
}

int SkPictureRecord::save(SaveFlags flags) {
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.size());

    addDraw(SAVE);
    addInt(flags);

    validate();
    return this->INHERITED::save(flags);
}

int SkPictureRecord::saveLayer(const SkRect* bounds, const SkPaint* paint,
                               SaveFlags flags) {
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.size());

    addDraw(SAVE_LAYER);
    addRectPtr(bounds);
    addPaintPtr(paint);
    addInt(flags);

    if (kNoSavedLayerIndex == fFirstSavedLayerIndex) {
        fFirstSavedLayerIndex = fRestoreOffsetStack.count();
    }

    validate();
    





    int count = this->INHERITED::save(flags);
    this->clipRectBounds(bounds, flags, NULL);
    return count;
}

bool SkPictureRecord::isDrawingToLayer() const {
    return fFirstSavedLayerIndex != kNoSavedLayerIndex;
}



static inline uint32_t getSkipableSize(unsigned drawType) {
    static const uint8_t gSizes[LAST_DRAWTYPE_ENUM + 1] = {
        0,  
        4,  
        4,  
        7,  
        2,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        2,  
        2,  
        0,  
        3,  
        2,  
        3,  
        3,  
    };

    SkASSERT(sizeof(gSizes) == LAST_DRAWTYPE_ENUM + 1);
    SkASSERT((unsigned)drawType <= (unsigned)LAST_DRAWTYPE_ENUM);
    return gSizes[drawType] * sizeof(uint32_t);
}

#ifdef TRACK_COLLAPSE_STATS
    static int gCollapseCount, gCollapseCalls;
#endif









static bool collapseSaveClipRestore(SkWriter32* writer, int32_t offset) {
#ifdef TRACK_COLLAPSE_STATS
    gCollapseCalls += 1;
#endif

    int32_t restoreOffset = (int32_t)writer->size();

    
    while (offset > 0) {
        offset = *writer->peek32(offset);
    }

    
    offset = -offset;
    if (SAVE_LAYER == *writer->peek32(offset)) {
        
        return false;
    }
    SkASSERT(SAVE == *writer->peek32(offset));

    
    
    int32_t saveOffset = offset;

    offset += getSkipableSize(SAVE);
    while (offset < restoreOffset) {
        uint32_t* block = writer->peek32(offset);
        uint32_t op = *block;
        uint32_t opSize = getSkipableSize(op);
        if (0 == opSize) {
            
            return false;
        }
        offset += opSize;
    }

#ifdef TRACK_COLLAPSE_STATS
    gCollapseCount += 1;
    SkDebugf("Collapse [%d out of %d] %g%spn", gCollapseCount, gCollapseCalls,
             (double)gCollapseCount / gCollapseCalls, "%");
#endif

    writer->rewindToOffset(saveOffset);
    return true;
}

void SkPictureRecord::restore() {
    
    
    
#if 0
    SkASSERT(fRestoreOffsetStack.count() > 1);
#endif

    
    if (fRestoreOffsetStack.count() == 0) {
        return;
    }

    if (fRestoreOffsetStack.count() == fFirstSavedLayerIndex) {
        fFirstSavedLayerIndex = kNoSavedLayerIndex;
    }

    if (!collapseSaveClipRestore(&fWriter, fRestoreOffsetStack.top())) {
        fillRestoreOffsetPlaceholdersForCurrentStackLevel((uint32_t)fWriter.size());
        this->addDraw(RESTORE);
    }

    fRestoreOffsetStack.pop();

    validate();
    return this->INHERITED::restore();
}

bool SkPictureRecord::translate(SkScalar dx, SkScalar dy) {
    addDraw(TRANSLATE);
    addScalar(dx);
    addScalar(dy);
    validate();
    return this->INHERITED::translate(dx, dy);
}

bool SkPictureRecord::scale(SkScalar sx, SkScalar sy) {
    addDraw(SCALE);
    addScalar(sx);
    addScalar(sy);
    validate();
    return this->INHERITED::scale(sx, sy);
}

bool SkPictureRecord::rotate(SkScalar degrees) {
    addDraw(ROTATE);
    addScalar(degrees);
    validate();
    return this->INHERITED::rotate(degrees);
}

bool SkPictureRecord::skew(SkScalar sx, SkScalar sy) {
    addDraw(SKEW);
    addScalar(sx);
    addScalar(sy);
    validate();
    return this->INHERITED::skew(sx, sy);
}

bool SkPictureRecord::concat(const SkMatrix& matrix) {
    validate();
    addDraw(CONCAT);
    addMatrix(matrix);
    validate();
    return this->INHERITED::concat(matrix);
}

void SkPictureRecord::setMatrix(const SkMatrix& matrix) {
    validate();
    addDraw(SET_MATRIX);
    addMatrix(matrix);
    validate();
    this->INHERITED::setMatrix(matrix);
}

static bool regionOpExpands(SkRegion::Op op) {
    switch (op) {
        case SkRegion::kUnion_Op:
        case SkRegion::kXOR_Op:
        case SkRegion::kReverseDifference_Op:
        case SkRegion::kReplace_Op:
            return true;
        case SkRegion::kIntersect_Op:
        case SkRegion::kDifference_Op:
            return false;
        default:
            SkDEBUGFAIL("unknown region op");
            return false;
    }
}

void SkPictureRecord::fillRestoreOffsetPlaceholdersForCurrentStackLevel(
    uint32_t restoreOffset) {
    int32_t offset = fRestoreOffsetStack.top();
    while (offset > 0) {
        uint32_t* peek = fWriter.peek32(offset);
        offset = *peek;
        *peek = restoreOffset;
    }

#ifdef SK_DEBUG
    
    uint32_t drawOp = *fWriter.peek32(-offset);
    SkASSERT(SAVE == drawOp || SAVE_LAYER == drawOp);
#endif
}

void SkPictureRecord::endRecording() {
    SkASSERT(kNoInitialSave != fInitialSaveCount);
    this->restoreToCount(fInitialSaveCount);
}

void SkPictureRecord::recordRestoreOffsetPlaceholder(SkRegion::Op op) {
    if (regionOpExpands(op)) {
        
        
        
        
        fillRestoreOffsetPlaceholdersForCurrentStackLevel(0);
    }

    size_t offset = fWriter.size();
    
    
    
    
    
    addInt(fRestoreOffsetStack.top());
    fRestoreOffsetStack.top() = offset;
}

bool SkPictureRecord::clipRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
    addDraw(CLIP_RECT);
    addRect(rect);
    addInt(ClipParams_pack(op, doAA));
    recordRestoreOffsetPlaceholder(op);

    validate();
    return this->INHERITED::clipRect(rect, op, doAA);
}

bool SkPictureRecord::clipPath(const SkPath& path, SkRegion::Op op, bool doAA) {
    addDraw(CLIP_PATH);
    addPath(path);
    addInt(ClipParams_pack(op, doAA));
    recordRestoreOffsetPlaceholder(op);

    validate();

    if (fRecordFlags & SkPicture::kUsePathBoundsForClip_RecordingFlag) {
        return this->INHERITED::clipRect(path.getBounds(), op, doAA);
    } else {
        return this->INHERITED::clipPath(path, op, doAA);
    }
}

bool SkPictureRecord::clipRegion(const SkRegion& region, SkRegion::Op op) {
    addDraw(CLIP_REGION);
    addRegion(region);
    addInt(ClipParams_pack(op, false));
    recordRestoreOffsetPlaceholder(op);

    validate();
    return this->INHERITED::clipRegion(region, op);
}

void SkPictureRecord::clear(SkColor color) {
    addDraw(DRAW_CLEAR);
    addInt(color);
    validate();
}

void SkPictureRecord::drawPaint(const SkPaint& paint) {
    addDraw(DRAW_PAINT);
    addPaint(paint);
    validate();
}

void SkPictureRecord::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                        const SkPaint& paint) {
    addDraw(DRAW_POINTS);
    addPaint(paint);
    addInt(mode);
    addInt(count);
    fWriter.writeMul4(pts, count * sizeof(SkPoint));
    validate();
}

void SkPictureRecord::drawRect(const SkRect& rect, const SkPaint& paint) {
    addDraw(DRAW_RECT);
    addPaint(paint);
    addRect(rect);
    validate();
}

void SkPictureRecord::drawPath(const SkPath& path, const SkPaint& paint) {
    addDraw(DRAW_PATH);
    addPaint(paint);
    addPath(path);
    validate();
}

void SkPictureRecord::drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                        const SkPaint* paint = NULL) {
    addDraw(DRAW_BITMAP);
    addPaintPtr(paint);
    addBitmap(bitmap);
    addScalar(left);
    addScalar(top);
    validate();
}

void SkPictureRecord::drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                            const SkRect& dst, const SkPaint* paint) {
    addDraw(DRAW_BITMAP_RECT);
    addPaintPtr(paint);
    addBitmap(bitmap);
    addIRectPtr(src);  
    addRect(dst);
    validate();
}

void SkPictureRecord::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& matrix,
                                       const SkPaint* paint) {
    addDraw(DRAW_BITMAP_MATRIX);
    addPaintPtr(paint);
    addBitmap(bitmap);
    addMatrix(matrix);
    validate();
}

void SkPictureRecord::drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                     const SkRect& dst, const SkPaint* paint) {
    addDraw(DRAW_BITMAP_NINE);
    addPaintPtr(paint);
    addBitmap(bitmap);
    addIRect(center);
    addRect(dst);
    validate();
}

void SkPictureRecord::drawSprite(const SkBitmap& bitmap, int left, int top,
                        const SkPaint* paint = NULL) {
    addDraw(DRAW_SPRITE);
    addPaintPtr(paint);
    addBitmap(bitmap);
    addInt(left);
    addInt(top);
    validate();
}

void SkPictureRecord::addFontMetricsTopBottom(const SkPaint& paint,
                                              SkScalar minY, SkScalar maxY) {
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    SkRect bounds;
    
    
    bounds.set(0, metrics.fTop + minY,
               SK_Scalar1, metrics.fBottom + maxY);
    (void)paint.computeFastBounds(bounds, &bounds);
    
    addScalar(bounds.fTop);
    addScalar(bounds.fBottom);
}

void SkPictureRecord::drawText(const void* text, size_t byteLength, SkScalar x,
                      SkScalar y, const SkPaint& paint) {
    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds();

    addDraw(fast ? DRAW_TEXT_TOP_BOTTOM : DRAW_TEXT);
    addPaint(paint);
    addText(text, byteLength);
    addScalar(x);
    addScalar(y);
    if (fast) {
        addFontMetricsTopBottom(paint, y, y);
    }
    validate();
}

void SkPictureRecord::drawPosText(const void* text, size_t byteLength,
                         const SkPoint pos[], const SkPaint& paint) {
    size_t points = paint.countText(text, byteLength);
    if (0 == points)
        return;

    bool canUseDrawH = true;
    SkScalar minY = pos[0].fY;
    SkScalar maxY = pos[0].fY;
    
    {
        const SkScalar firstY = pos[0].fY;
        for (size_t index = 1; index < points; index++) {
            if (pos[index].fY != firstY) {
                canUseDrawH = false;
                if (pos[index].fY < minY) {
                    minY = pos[index].fY;
                } else if (pos[index].fY > maxY) {
                    maxY = pos[index].fY;
                }
            }
        }
    }

    bool fastBounds = !paint.isVerticalText() && paint.canComputeFastBounds();
    bool fast = canUseDrawH && fastBounds;

    if (fast) {
        addDraw(DRAW_POS_TEXT_H_TOP_BOTTOM);
    } else if (canUseDrawH) {
        addDraw(DRAW_POS_TEXT_H);
    } else if (fastBounds) {
        addDraw(DRAW_POS_TEXT_TOP_BOTTOM);
    } else {
        addDraw(DRAW_POS_TEXT);
    }
    addPaint(paint);
    addText(text, byteLength);
    addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    if (canUseDrawH) {
        if (fast) {
            addFontMetricsTopBottom(paint, pos[0].fY, pos[0].fY);
        }
        addScalar(pos[0].fY);
        SkScalar* xptr = (SkScalar*)fWriter.reserve(points * sizeof(SkScalar));
        for (size_t index = 0; index < points; index++)
            *xptr++ = pos[index].fX;
    }
    else {
        fWriter.writeMul4(pos, points * sizeof(SkPoint));
        if (fastBounds) {
            addFontMetricsTopBottom(paint, minY, maxY);
        }
    }
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.size() - start;
    fPointWrites += points;
#endif
    validate();
}

void SkPictureRecord::drawPosTextH(const void* text, size_t byteLength,
                          const SkScalar xpos[], SkScalar constY,
                          const SkPaint& paint) {
    size_t points = paint.countText(text, byteLength);
    if (0 == points)
        return;

    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds();

    addDraw(fast ? DRAW_POS_TEXT_H_TOP_BOTTOM : DRAW_POS_TEXT_H);
    addPaint(paint);
    addText(text, byteLength);
    addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    if (fast) {
        addFontMetricsTopBottom(paint, constY, constY);
    }
    addScalar(constY);
    fWriter.writeMul4(xpos, points * sizeof(SkScalar));
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.size() - start;
    fPointWrites += points;
#endif
    validate();
}

void SkPictureRecord::drawTextOnPath(const void* text, size_t byteLength,
                            const SkPath& path, const SkMatrix* matrix,
                            const SkPaint& paint) {
    addDraw(DRAW_TEXT_ON_PATH);
    addPaint(paint);
    addText(text, byteLength);
    addPath(path);
    addMatrixPtr(matrix);
    validate();
}

void SkPictureRecord::drawPicture(SkPicture& picture) {
    addDraw(DRAW_PICTURE);
    addPicture(picture);
    validate();
}

void SkPictureRecord::drawVertices(VertexMode vmode, int vertexCount,
                          const SkPoint vertices[], const SkPoint texs[],
                          const SkColor colors[], SkXfermode*,
                          const uint16_t indices[], int indexCount,
                          const SkPaint& paint) {
    uint32_t flags = 0;
    if (texs) {
        flags |= DRAW_VERTICES_HAS_TEXS;
    }
    if (colors) {
        flags |= DRAW_VERTICES_HAS_COLORS;
    }
    if (indexCount > 0) {
        flags |= DRAW_VERTICES_HAS_INDICES;
    }

    addDraw(DRAW_VERTICES);
    addPaint(paint);
    addInt(flags);
    addInt(vmode);
    addInt(vertexCount);
    addPoints(vertices, vertexCount);
    if (flags & DRAW_VERTICES_HAS_TEXS) {
        addPoints(texs, vertexCount);
    }
    if (flags & DRAW_VERTICES_HAS_COLORS) {
        fWriter.writeMul4(colors, vertexCount * sizeof(SkColor));
    }
    if (flags & DRAW_VERTICES_HAS_INDICES) {
        addInt(indexCount);
        fWriter.writePad(indices, indexCount * sizeof(uint16_t));
    }
}

void SkPictureRecord::drawData(const void* data, size_t length) {
    addDraw(DRAW_DATA);
    addInt(length);
    fWriter.writePad(data, length);
}



void SkPictureRecord::addBitmap(const SkBitmap& bitmap) {
    addInt(fBitmapHeap->insert(bitmap));
}

void SkPictureRecord::addMatrix(const SkMatrix& matrix) {
    addMatrixPtr(&matrix);
}

void SkPictureRecord::addMatrixPtr(const SkMatrix* matrix) {
    this->addInt(matrix ? fMatrices.find(*matrix) : 0);
}

void SkPictureRecord::addPaint(const SkPaint& paint) {
    addPaintPtr(&paint);
}

void SkPictureRecord::addPaintPtr(const SkPaint* paint) {
    this->addInt(paint ? fPaints.find(*paint) : 0);
}

void SkPictureRecord::addPath(const SkPath& path) {
    if (NULL == fPathHeap) {
        fPathHeap = SkNEW(SkPathHeap);
    }
    addInt(fPathHeap->append(path));
}

void SkPictureRecord::addPicture(SkPicture& picture) {
    int index = fPictureRefs.find(&picture);
    if (index < 0) {    
        index = fPictureRefs.count();
        *fPictureRefs.append() = &picture;
        picture.ref();
    }
    
    addInt(index + 1);
}

void SkPictureRecord::addPoint(const SkPoint& point) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    fWriter.writePoint(point);
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.size() - start;
    fPointWrites++;
#endif
}

void SkPictureRecord::addPoints(const SkPoint pts[], int count) {
    fWriter.writeMul4(pts, count * sizeof(SkPoint));
#ifdef SK_DEBUG_SIZE
    fPointBytes += count * sizeof(SkPoint);
    fPointWrites++;
#endif
}

void SkPictureRecord::addRect(const SkRect& rect) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    fWriter.writeRect(rect);
#ifdef SK_DEBUG_SIZE
    fRectBytes += fWriter.size() - start;
    fRectWrites++;
#endif
}

void SkPictureRecord::addRectPtr(const SkRect* rect) {
    if (fWriter.writeBool(rect != NULL)) {
        fWriter.writeRect(*rect);
    }
}

void SkPictureRecord::addIRect(const SkIRect& rect) {
    fWriter.write(&rect, sizeof(rect));
}

void SkPictureRecord::addIRectPtr(const SkIRect* rect) {
    if (fWriter.writeBool(rect != NULL)) {
        *(SkIRect*)fWriter.reserve(sizeof(SkIRect)) = *rect;
    }
}

void SkPictureRecord::addRegion(const SkRegion& region) {
    addInt(fRegions.find(region));
}

void SkPictureRecord::addText(const void* text, size_t byteLength) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    addInt(byteLength);
    fWriter.writePad(text, byteLength);
#ifdef SK_DEBUG_SIZE
    fTextBytes += fWriter.size() - start;
    fTextWrites++;
#endif
}



#ifdef SK_DEBUG_SIZE
size_t SkPictureRecord::size() const {
    size_t result = 0;
    size_t sizeData;
    bitmaps(&sizeData);
    result += sizeData;
    matrices(&sizeData);
    result += sizeData;
    paints(&sizeData);
    result += sizeData;
    paths(&sizeData);
    result += sizeData;
    pictures(&sizeData);
    result += sizeData;
    regions(&sizeData);
    result += sizeData;
    result += streamlen();
    return result;
}

int SkPictureRecord::bitmaps(size_t* size) const {
    size_t result = 0;
    int count = fBitmaps.count();
    for (int index = 0; index < count; index++)
        result += sizeof(fBitmaps[index]) + fBitmaps[index]->size();
    *size = result;
    return count;
}

int SkPictureRecord::matrices(size_t* size) const {
    int count = fMatrices.count();
    *size = sizeof(fMatrices[0]) * count;
    return count;
}

int SkPictureRecord::paints(size_t* size) const {
    size_t result = 0;
    int count = fPaints.count();
    for (int index = 0; index < count; index++)
        result += sizeof(fPaints[index]) + fPaints[index]->size();
    *size = result;
    return count;
}

int SkPictureRecord::paths(size_t* size) const {
    size_t result = 0;
    int count = fPaths.count();
    for (int index = 0; index < count; index++)
        result += sizeof(fPaths[index]) + fPaths[index]->size();
    *size = result;
    return count;
}

int SkPictureRecord::regions(size_t* size) const {
    size_t result = 0;
    int count = fRegions.count();
    for (int index = 0; index < count; index++)
        result += sizeof(fRegions[index]) + fRegions[index]->size();
    *size = result;
    return count;
}

size_t SkPictureRecord::streamlen() const {
    return fWriter.size();
}
#endif

#ifdef SK_DEBUG_VALIDATE
void SkPictureRecord::validate() const {
    validateBitmaps();
    validateMatrices();
    validatePaints();
    validatePaths();
    validatePictures();
    validateRegions();
}

void SkPictureRecord::validateBitmaps() const {
    int count = fBitmaps.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatBitmap* bitPtr = fBitmaps[index];
        SkASSERT(bitPtr);
        bitPtr->validate();
    }
}

void SkPictureRecord::validateMatrices() const {
    int count = fMatrices.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatMatrix* matrix = fMatrices[index];
        SkASSERT(matrix);
        matrix->validate();
    }
}

void SkPictureRecord::validatePaints() const {
    int count = fPaints.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatPaint* paint = fPaints[index];
        SkASSERT(paint);

    }
}

void SkPictureRecord::validatePaths() const {
    int count = fPaths.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatPath* path = fPaths[index];
        SkASSERT(path);
        path->validate();
    }
}

void SkPictureRecord::validateRegions() const {
    int count = fRegions.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatRegion* region = fRegions[index];
        SkASSERT(region);
        region->validate();
    }
}
#endif

