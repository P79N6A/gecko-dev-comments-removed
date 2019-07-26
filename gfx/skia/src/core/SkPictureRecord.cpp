






#include "SkPictureRecord.h"
#include "SkTSearch.h"
#include "SkPixelRef.h"
#include "SkRRect.h"
#include "SkBBoxHierarchy.h"
#include "SkPictureStateTree.h"

#define MIN_WRITER_SIZE 16384
#define HEAP_BLOCK_SIZE 4096

enum {
    
    kNoInitialSave = -1,
};


static int const kUInt32Size = 4;

static const uint32_t kSaveLayerNoBoundsSize = 4 * kUInt32Size;
static const uint32_t kSaveLayerWithBoundsSize = 4 * kUInt32Size + sizeof(SkRect);

SkPictureRecord::SkPictureRecord(uint32_t flags, SkDevice* device) :
        INHERITED(device),
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

    fBitmapHeap = SkNEW(SkBitmapHeap);
    fFlattenableHeap.setBitmapStorage(fBitmapHeap);
    fPathHeap = NULL;   
    fFirstSavedLayerIndex = kNoSavedLayerIndex;

    fInitialSaveCount = kNoInitialSave;
}

SkPictureRecord::~SkPictureRecord() {
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fPathHeap);
    SkSafeUnref(fBoundingHierarchy);
    SkSafeUnref(fStateTree);
    fFlattenableHeap.setBitmapStorage(NULL);
    fPictureRefs.unrefAll();
}






static inline uint32_t getPaintOffset(DrawType op, uint32_t opSize) {
    
    static const uint8_t gPaintOffsets[LAST_DRAWTYPE_ENUM + 1] = {
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        1,  
        1,  
        1,  
        1,  
        0,  
        0,  
        1,  
        1,  
        1,  
        0,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        1,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
        0,  
    };

    SkASSERT(sizeof(gPaintOffsets) == LAST_DRAWTYPE_ENUM + 1);
    SkASSERT((unsigned)op <= (unsigned)LAST_DRAWTYPE_ENUM);

    int overflow = 0;
    if (0 != (opSize & ~MASK_24) || opSize == MASK_24) {
        
        
        overflow = sizeof(uint32_t);
    }

    if (SAVE_LAYER == op) {
        static const uint32_t kSaveLayerNoBoundsPaintOffset = 2 * kUInt32Size;
        static const uint32_t kSaveLayerWithBoundsPaintOffset = 2 * kUInt32Size + sizeof(SkRect);

        if (kSaveLayerNoBoundsSize == opSize) {
            return kSaveLayerNoBoundsPaintOffset + overflow;
        } else {
            SkASSERT(kSaveLayerWithBoundsSize == opSize);
            return kSaveLayerWithBoundsPaintOffset + overflow;
        }
    }

    SkASSERT(0 != gPaintOffsets[op]);   
    return gPaintOffsets[op] * sizeof(uint32_t) + overflow;
}

SkDevice* SkPictureRecord::setDevice(SkDevice* device) {
    SkASSERT(!"eeek, don't try to change the device on a recording canvas");
    return this->INHERITED::setDevice(device);
}

int SkPictureRecord::save(SaveFlags flags) {
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.size());

    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(SAVE, &size);
    addInt(flags);

    validate(initialOffset, size);
    return this->INHERITED::save(flags);
}

int SkPictureRecord::saveLayer(const SkRect* bounds, const SkPaint* paint,
                               SaveFlags flags) {
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.size());

    
    uint32_t size = 2 * kUInt32Size;
    if (NULL != bounds) {
        size += sizeof(*bounds); 
    }
    
    size += 2 * kUInt32Size;

    SkASSERT(kSaveLayerNoBoundsSize == size || kSaveLayerWithBoundsSize == size);

    uint32_t initialOffset = this->addDraw(SAVE_LAYER, &size);
    addRectPtr(bounds);
    SkASSERT(initialOffset+getPaintOffset(SAVE_LAYER, size) == fWriter.size());
    addPaintPtr(paint);
    addInt(flags);

    if (kNoSavedLayerIndex == fFirstSavedLayerIndex) {
        fFirstSavedLayerIndex = fRestoreOffsetStack.count();
    }

    validate(initialOffset, size);
    





    int count = this->INHERITED::save(flags);
    this->clipRectBounds(bounds, flags, NULL);
    return count;
}

bool SkPictureRecord::isDrawingToLayer() const {
    return fFirstSavedLayerIndex != kNoSavedLayerIndex;
}




static DrawType peek_op_and_size(SkWriter32* writer, int32_t offset, uint32_t* size) {
    uint32_t* peek = writer->peek32(offset);

    uint32_t op;
    UNPACK_8_24(*peek, op, *size);
    if (MASK_24 == *size) {
        
        *size = *writer->peek32(offset+kUInt32Size);
    }
    return (DrawType) op;
}

#ifdef TRACK_COLLAPSE_STATS
    static int gCollapseCount, gCollapseCalls;
#endif


static bool is_simple(const SkPaint& p) {
    intptr_t orAccum = (intptr_t)p.getPathEffect()  |
                       (intptr_t)p.getShader()      |
                       (intptr_t)p.getXfermode()    |
                       (intptr_t)p.getMaskFilter()  |
                       (intptr_t)p.getColorFilter() |
                       (intptr_t)p.getRasterizer()  |
                       (intptr_t)p.getLooper()      |
                       (intptr_t)p.getImageFilter();
    return 0 == orAccum;
}



struct CommandInfo {
    DrawType fActualOp;
    uint32_t fOffset;
    uint32_t fSize;
};








static bool match(SkWriter32* writer, uint32_t offset,
                  int* pattern, CommandInfo* result, int numCommands) {
    SkASSERT(offset < writer->size());

    uint32_t curOffset = offset;
    uint32_t curSize = 0;
    int numMatched;
    for (numMatched = 0; numMatched < numCommands && curOffset < writer->size(); ++numMatched) {
        DrawType op = peek_op_and_size(writer, curOffset, &curSize);
        while (NOOP == op && curOffset < writer->size()) {
            curOffset += curSize;
            op = peek_op_and_size(writer, curOffset, &curSize);
        }

        if (curOffset >= writer->size()) {
            return false; 
        }

        if (kDRAW_BITMAP_FLAVOR == pattern[numMatched]) {
            if (DRAW_BITMAP != op && DRAW_BITMAP_MATRIX != op &&
                DRAW_BITMAP_NINE != op && DRAW_BITMAP_RECT_TO_RECT != op) {
                return false;
            }
        } else if (op != pattern[numMatched]) {
            return false;
        }

        result[numMatched].fActualOp = op;
        result[numMatched].fOffset = curOffset;
        result[numMatched].fSize = curSize;

        curOffset += curSize;
    }

    if (numMatched != numCommands) {
        return false;
    }

    curOffset += curSize;
    if (curOffset < writer->size()) {
        
        return false;
    }

    return true;
}


static bool merge_savelayer_paint_into_drawbitmp(SkWriter32* writer,
                                                 SkPaintDictionary* paintDict,
                                                 const CommandInfo& saveLayerInfo,
                                                 const CommandInfo& dbmInfo);









static bool remove_save_layer1(SkWriter32* writer, int32_t offset,
                               SkPaintDictionary* paintDict) {
    
    
    while (offset > 0) {
        offset = *writer->peek32(offset);
    }

    int pattern[] = { SAVE_LAYER, kDRAW_BITMAP_FLAVOR,  };
    CommandInfo result[SK_ARRAY_COUNT(pattern)];

    if (!match(writer, -offset, pattern, result, SK_ARRAY_COUNT(pattern))) {
        return false;
    }

    if (kSaveLayerWithBoundsSize == result[0].fSize) {
        
        return false;
    }


    return merge_savelayer_paint_into_drawbitmp(writer, paintDict,
                                                result[0], result[1]);
}





static void convert_command_to_noop(SkWriter32* writer, uint32_t offset) {
    uint32_t* ptr = writer->peek32(offset);
    *ptr = (*ptr & MASK_24) | (NOOP << 24);
}





static bool merge_savelayer_paint_into_drawbitmp(SkWriter32* writer,
                                                 SkPaintDictionary* paintDict,
                                                 const CommandInfo& saveLayerInfo,
                                                 const CommandInfo& dbmInfo) {
    SkASSERT(SAVE_LAYER == saveLayerInfo.fActualOp);
    SkASSERT(DRAW_BITMAP == dbmInfo.fActualOp ||
             DRAW_BITMAP_MATRIX == dbmInfo.fActualOp ||
             DRAW_BITMAP_NINE == dbmInfo.fActualOp ||
             DRAW_BITMAP_RECT_TO_RECT == dbmInfo.fActualOp);

    uint32_t dbmPaintOffset = getPaintOffset(dbmInfo.fActualOp, dbmInfo.fSize);
    uint32_t slPaintOffset = getPaintOffset(SAVE_LAYER, saveLayerInfo.fSize);

    
    uint32_t dbmPaintId = *writer->peek32(dbmInfo.fOffset+dbmPaintOffset);
    uint32_t saveLayerPaintId = *writer->peek32(saveLayerInfo.fOffset+slPaintOffset);

    if (0 == saveLayerPaintId) {
        
        
        convert_command_to_noop(writer, saveLayerInfo.fOffset);
        return true;
    }

    if (0 == dbmPaintId) {
        
        
        convert_command_to_noop(writer, saveLayerInfo.fOffset);
        uint32_t* ptr = writer->peek32(dbmInfo.fOffset+dbmPaintOffset);
        SkASSERT(0 == *ptr);
        *ptr = saveLayerPaintId;
        return true;
    }

    SkAutoTDelete<SkPaint> saveLayerPaint(paintDict->unflatten(saveLayerPaintId));
    if (NULL == saveLayerPaint.get() || !is_simple(*saveLayerPaint)) {
        return false;
    }

    
    
    
    
    
    
    SkColor layerColor = saveLayerPaint->getColor() | 0xFF000000; 

    SkAutoTDelete<SkPaint> dbmPaint(paintDict->unflatten(dbmPaintId));
    if (NULL == dbmPaint.get() || dbmPaint->getColor() != layerColor) {
        return false;
    }

    SkColor newColor = SkColorSetA(dbmPaint->getColor(),
                                   SkColorGetA(saveLayerPaint->getColor()));
    dbmPaint->setColor(newColor);

    const SkFlatData* data = paintDict->findAndReturnFlat(*dbmPaint);
    if (NULL == data) {
        return false;
    }

    
    convert_command_to_noop(writer, saveLayerInfo.fOffset);
    uint32_t* ptr = writer->peek32(dbmInfo.fOffset+dbmPaintOffset);
    SkASSERT(dbmPaintId == *ptr);
    *ptr = data->index();
    return true;
}












static bool remove_save_layer2(SkWriter32* writer, int32_t offset,
                               SkPaintDictionary* paintDict) {

    
    
    while (offset > 0) {
        offset = *writer->peek32(offset);
    }

    int pattern[] = { SAVE_LAYER, SAVE, CLIP_RECT, kDRAW_BITMAP_FLAVOR, RESTORE,  };
    CommandInfo result[SK_ARRAY_COUNT(pattern)];

    if (!match(writer, -offset, pattern, result, SK_ARRAY_COUNT(pattern))) {
        return false;
    }

    if (kSaveLayerWithBoundsSize == result[0].fSize) {
        
        return false;
    }

    return merge_savelayer_paint_into_drawbitmp(writer, paintDict,
                                                result[0], result[3]);
}









static bool collapse_save_clip_restore(SkWriter32* writer, int32_t offset,
                                       SkPaintDictionary* paintDict) {
#ifdef TRACK_COLLAPSE_STATS
    gCollapseCalls += 1;
#endif

    int32_t restoreOffset = (int32_t)writer->size();

    
    while (offset > 0) {
        offset = *writer->peek32(offset);
    }

    
    offset = -offset;
    uint32_t opSize;
    DrawType op = peek_op_and_size(writer, offset, &opSize);
    if (SAVE_LAYER == op) {
        
        return false;
    }
    SkASSERT(SAVE == op);

    
    
    int32_t saveOffset = offset;

    offset += opSize;
    while (offset < restoreOffset) {
        op = peek_op_and_size(writer, offset, &opSize);
        if ((op > CONCAT && op < ROTATE) || (SAVE_LAYER == op)) {
            
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

typedef bool (*PictureRecordOptProc)(SkWriter32* writer, int32_t offset,
                                     SkPaintDictionary* paintDict);
enum PictureRecordOptType {
    kRewind_OptType,  
    kCollapseSaveLayer_OptType,  
};

struct PictureRecordOpt {
    PictureRecordOptProc fProc;
    PictureRecordOptType fType;
};





static const PictureRecordOpt gPictureRecordOpts[] = {
    { collapse_save_clip_restore, kRewind_OptType },
    { remove_save_layer1,         kCollapseSaveLayer_OptType },
    { remove_save_layer2,         kCollapseSaveLayer_OptType }
};




static void apply_optimization_to_bbh(PictureRecordOptType opt, SkPictureStateTree* stateTree,
                                      SkBBoxHierarchy* boundingHierarchy) {
    switch (opt) {
    case kCollapseSaveLayer_OptType:
        if (NULL != stateTree) {
            stateTree->saveCollapsed();
        }
        break;
    case kRewind_OptType:
        if (NULL != boundingHierarchy) {
            boundingHierarchy->rewindInserts();
        }
        
        
        
        break;
    default:
        SkASSERT(0);
    }
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

    uint32_t initialOffset, size;
    size_t opt;
    for (opt = 0; opt < SK_ARRAY_COUNT(gPictureRecordOpts); ++opt) {
        if ((*gPictureRecordOpts[opt].fProc)(&fWriter, fRestoreOffsetStack.top(), &fPaints)) {
            
            size = 0;
            initialOffset = fWriter.size();
            apply_optimization_to_bbh(gPictureRecordOpts[opt].fType,
                                      fStateTree, fBoundingHierarchy);
            break;
        }
    }

    if (SK_ARRAY_COUNT(gPictureRecordOpts) == opt) {
        
        fillRestoreOffsetPlaceholdersForCurrentStackLevel((uint32_t)fWriter.size());
        size = 1 * kUInt32Size; 
        initialOffset = this->addDraw(RESTORE, &size);
    }

    fRestoreOffsetStack.pop();

    validate(initialOffset, size);
    return this->INHERITED::restore();
}

bool SkPictureRecord::translate(SkScalar dx, SkScalar dy) {
    
    uint32_t size = 1 * kUInt32Size + 2 * sizeof(SkScalar);
    uint32_t initialOffset = this->addDraw(TRANSLATE, &size);
    addScalar(dx);
    addScalar(dy);
    validate(initialOffset, size);
    return this->INHERITED::translate(dx, dy);
}

bool SkPictureRecord::scale(SkScalar sx, SkScalar sy) {
    
    uint32_t size = 1 * kUInt32Size + 2 * sizeof(SkScalar);
    uint32_t initialOffset = this->addDraw(SCALE, &size);
    addScalar(sx);
    addScalar(sy);
    validate(initialOffset, size);
    return this->INHERITED::scale(sx, sy);
}

bool SkPictureRecord::rotate(SkScalar degrees) {
    
    uint32_t size = 1 * kUInt32Size + sizeof(SkScalar);
    uint32_t initialOffset = this->addDraw(ROTATE, &size);
    addScalar(degrees);
    validate(initialOffset, size);
    return this->INHERITED::rotate(degrees);
}

bool SkPictureRecord::skew(SkScalar sx, SkScalar sy) {
    
    uint32_t size = 1 * kUInt32Size + 2 * sizeof(SkScalar);
    uint32_t initialOffset = this->addDraw(SKEW, &size);
    addScalar(sx);
    addScalar(sy);
    validate(initialOffset, size);
    return this->INHERITED::skew(sx, sy);
}

bool SkPictureRecord::concat(const SkMatrix& matrix) {
    validate(fWriter.size(), 0);
    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(CONCAT, &size);
    addMatrix(matrix);
    validate(initialOffset, size);
    return this->INHERITED::concat(matrix);
}

void SkPictureRecord::setMatrix(const SkMatrix& matrix) {
    validate(fWriter.size(), 0);
    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(SET_MATRIX, &size);
    addMatrix(matrix);
    validate(initialOffset, size);
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

void SkPictureRecord::fillRestoreOffsetPlaceholdersForCurrentStackLevel(uint32_t restoreOffset) {
    int32_t offset = fRestoreOffsetStack.top();
    while (offset > 0) {
        uint32_t* peek = fWriter.peek32(offset);
        offset = *peek;
        *peek = restoreOffset;
    }

#ifdef SK_DEBUG
    
    uint32_t opSize;
    DrawType drawOp = peek_op_and_size(&fWriter, -offset, &opSize);
    SkASSERT(SAVE == drawOp || SAVE_LAYER == drawOp);
#endif
}

void SkPictureRecord::beginRecording() {
    
    
    
    fInitialSaveCount = this->save(kMatrixClip_SaveFlag);
}

void SkPictureRecord::endRecording() {
    SkASSERT(kNoInitialSave != fInitialSaveCount);
    this->restoreToCount(fInitialSaveCount);
}

void SkPictureRecord::recordRestoreOffsetPlaceholder(SkRegion::Op op) {
    if (fRestoreOffsetStack.isEmpty()) {
        return;
    }

    if (regionOpExpands(op)) {
        
        
        
        
        fillRestoreOffsetPlaceholdersForCurrentStackLevel(0);
    }

    size_t offset = fWriter.size();
    
    
    
    
    
    addInt(fRestoreOffsetStack.top());
    fRestoreOffsetStack.top() = offset;
}

bool SkPictureRecord::clipRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
    
    uint32_t size = 1 * kUInt32Size + sizeof(rect) + 1 * kUInt32Size;
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
    uint32_t initialOffset = this->addDraw(CLIP_RECT, &size);
    addRect(rect);
    addInt(ClipParams_pack(op, doAA));
    recordRestoreOffsetPlaceholder(op);

    validate(initialOffset, size);
    return this->INHERITED::clipRect(rect, op, doAA);
}

bool SkPictureRecord::clipRRect(const SkRRect& rrect, SkRegion::Op op, bool doAA) {
    if (rrect.isRect()) {
        return this->SkPictureRecord::clipRect(rrect.getBounds(), op, doAA);
    }

    
    uint32_t size = 1 * kUInt32Size + SkRRect::kSizeInMemory + 1 * kUInt32Size;
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
    uint32_t initialOffset = this->addDraw(CLIP_RRECT, &size);
    addRRect(rrect);
    addInt(ClipParams_pack(op, doAA));
    recordRestoreOffsetPlaceholder(op);

    validate(initialOffset, size);

    if (fRecordFlags & SkPicture::kUsePathBoundsForClip_RecordingFlag) {
        return this->INHERITED::clipRect(rrect.getBounds(), op, doAA);
    } else {
        return this->INHERITED::clipRRect(rrect, op, doAA);
    }
}

bool SkPictureRecord::clipPath(const SkPath& path, SkRegion::Op op, bool doAA) {

    SkRect r;
    if (!path.isInverseFillType() && path.isRect(&r)) {
        return this->clipRect(r, op, doAA);
    }

    
    uint32_t size = 3 * kUInt32Size;
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
    uint32_t initialOffset = this->addDraw(CLIP_PATH, &size);
    addPath(path);
    addInt(ClipParams_pack(op, doAA));
    recordRestoreOffsetPlaceholder(op);

    validate(initialOffset, size);

    if (fRecordFlags & SkPicture::kUsePathBoundsForClip_RecordingFlag) {
        return this->INHERITED::clipRect(path.getBounds(), op, doAA);
    } else {
        return this->INHERITED::clipPath(path, op, doAA);
    }
}

bool SkPictureRecord::clipRegion(const SkRegion& region, SkRegion::Op op) {
    
    uint32_t size = 3 * kUInt32Size;
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
    uint32_t initialOffset = this->addDraw(CLIP_REGION, &size);
    addRegion(region);
    addInt(ClipParams_pack(op, false));
    recordRestoreOffsetPlaceholder(op);

    validate(initialOffset, size);
    return this->INHERITED::clipRegion(region, op);
}

void SkPictureRecord::clear(SkColor color) {
    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_CLEAR, &size);
    addInt(color);
    validate(initialOffset, size);
}

void SkPictureRecord::drawPaint(const SkPaint& paint) {
    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_PAINT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_PAINT, size) == fWriter.size());
    addPaint(paint);
    validate(initialOffset, size);
}

void SkPictureRecord::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                                 const SkPaint& paint) {
    
    uint32_t size = 4 * kUInt32Size + count * sizeof(SkPoint);
    uint32_t initialOffset = this->addDraw(DRAW_POINTS, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_POINTS, size) == fWriter.size());
    addPaint(paint);
    addInt(mode);
    addInt(count);
    fWriter.writeMul4(pts, count * sizeof(SkPoint));
    validate(initialOffset, size);
}

void SkPictureRecord::drawOval(const SkRect& oval, const SkPaint& paint) {
    
    uint32_t size = 2 * kUInt32Size + sizeof(oval);
    uint32_t initialOffset = this->addDraw(DRAW_OVAL, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_OVAL, size) == fWriter.size());
    addPaint(paint);
    addRect(oval);
    validate(initialOffset, size);
}

void SkPictureRecord::drawRect(const SkRect& rect, const SkPaint& paint) {
    
    uint32_t size = 2 * kUInt32Size + sizeof(rect);
    uint32_t initialOffset = this->addDraw(DRAW_RECT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_RECT, size) == fWriter.size());
    addPaint(paint);
    addRect(rect);
    validate(initialOffset, size);
}

void SkPictureRecord::drawRRect(const SkRRect& rrect, const SkPaint& paint) {
    uint32_t initialOffset, size;
    if (rrect.isRect()) {
        
        size = 2 * kUInt32Size + sizeof(SkRect);
        initialOffset = this->addDraw(DRAW_RECT, &size);
        SkASSERT(initialOffset+getPaintOffset(DRAW_RECT, size) == fWriter.size());
        addPaint(paint);
        addRect(rrect.getBounds());
    } else if (rrect.isOval()) {
        
        size = 2 * kUInt32Size + sizeof(SkRect);
        initialOffset = this->addDraw(DRAW_OVAL, &size);
        SkASSERT(initialOffset+getPaintOffset(DRAW_OVAL, size) == fWriter.size());
        addPaint(paint);
        addRect(rrect.getBounds());
    } else {
        
        size = 2 * kUInt32Size + SkRRect::kSizeInMemory;
        initialOffset = this->addDraw(DRAW_RRECT, &size);
        SkASSERT(initialOffset+getPaintOffset(DRAW_RRECT, size) == fWriter.size());
        addPaint(paint);
        addRRect(rrect);
    }
    validate(initialOffset, size);
}

void SkPictureRecord::drawPath(const SkPath& path, const SkPaint& paint) {
    
    uint32_t size = 3 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_PATH, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_PATH, size) == fWriter.size());
    addPaint(paint);
    addPath(path);
    validate(initialOffset, size);
}

void SkPictureRecord::drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                        const SkPaint* paint = NULL) {
    
    uint32_t size = 3 * kUInt32Size + 2 * sizeof(SkScalar);
    uint32_t initialOffset = this->addDraw(DRAW_BITMAP, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP, size) == fWriter.size());
    addPaintPtr(paint);
    addBitmap(bitmap);
    addScalar(left);
    addScalar(top);
    validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                            const SkRect& dst, const SkPaint* paint) {
    
    uint32_t size = 4 * kUInt32Size;
    if (NULL != src) {
        size += sizeof(*src);   
    }
    size += sizeof(dst);        

    uint32_t initialOffset = this->addDraw(DRAW_BITMAP_RECT_TO_RECT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_RECT_TO_RECT, size) == fWriter.size());
    addPaintPtr(paint);
    addBitmap(bitmap);
    addRectPtr(src);  
    addRect(dst);
    validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& matrix,
                                       const SkPaint* paint) {
    
    uint32_t size = 4 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_BITMAP_MATRIX, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_MATRIX, size) == fWriter.size());
    addPaintPtr(paint);
    addBitmap(bitmap);
    addMatrix(matrix);
    validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                     const SkRect& dst, const SkPaint* paint) {
    
    uint32_t size = 3 * kUInt32Size + sizeof(center) + sizeof(dst);
    uint32_t initialOffset = this->addDraw(DRAW_BITMAP_NINE, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_NINE, size) == fWriter.size());
    addPaintPtr(paint);
    addBitmap(bitmap);
    addIRect(center);
    addRect(dst);
    validate(initialOffset, size);
}

void SkPictureRecord::drawSprite(const SkBitmap& bitmap, int left, int top,
                        const SkPaint* paint = NULL) {
    
    uint32_t size = 5 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_SPRITE, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_SPRITE, size) == fWriter.size());
    addPaintPtr(paint);
    addBitmap(bitmap);
    addInt(left);
    addInt(top);
    validate(initialOffset, size);
}




static void computeFontMetricsTopBottom(const SkPaint& paint, SkScalar topbot[2]) {
    SkPaint::FontMetrics metrics;
    paint.getFontMetrics(&metrics);
    SkRect bounds;
    
    
    bounds.set(0, metrics.fTop, SK_Scalar1, metrics.fBottom);
    (void)paint.computeFastBounds(bounds, &bounds);
    topbot[0] = bounds.fTop;
    topbot[1] = bounds.fBottom;
}

void SkPictureRecord::addFontMetricsTopBottom(const SkPaint& paint, const SkFlatData& flat,
                                              SkScalar minY, SkScalar maxY) {
    if (!flat.isTopBotWritten()) {
        computeFontMetricsTopBottom(paint, flat.writableTopBot());
        SkASSERT(flat.isTopBotWritten());
    }
    addScalar(flat.topBot()[0] + minY);
    addScalar(flat.topBot()[1] + maxY);
}

void SkPictureRecord::drawText(const void* text, size_t byteLength, SkScalar x,
                      SkScalar y, const SkPaint& paint) {
    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds();

    
    uint32_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 2 * sizeof(SkScalar);
    if (fast) {
        size += 2 * sizeof(SkScalar); 
    }

    DrawType op = fast ? DRAW_TEXT_TOP_BOTTOM : DRAW_TEXT;
    uint32_t initialOffset = this->addDraw(op, &size);
    SkASSERT(initialOffset+getPaintOffset(op, size) == fWriter.size());
    const SkFlatData* flatPaintData = addPaint(paint);
    SkASSERT(flatPaintData);
    addText(text, byteLength);
    addScalar(x);
    addScalar(y);
    if (fast) {
        addFontMetricsTopBottom(paint, *flatPaintData, y, y);
    }
    validate(initialOffset, size);
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

    
    uint32_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 1 * kUInt32Size;
    if (canUseDrawH) {
        if (fast) {
            size += 2 * sizeof(SkScalar); 
        }
        
        size += sizeof(SkScalar) + points * sizeof(SkScalar);
    } else {
        
        size += points * sizeof(SkPoint);
        if (fastBounds) {
            size += 2 * sizeof(SkScalar); 
        }
    }

    DrawType op;
    if (fast) {
        op = DRAW_POS_TEXT_H_TOP_BOTTOM;
    } else if (canUseDrawH) {
        op = DRAW_POS_TEXT_H;
    } else if (fastBounds) {
        op = DRAW_POS_TEXT_TOP_BOTTOM;
    } else {
        op = DRAW_POS_TEXT;
    }
    uint32_t initialOffset = this->addDraw(op, &size);
    SkASSERT(initialOffset+getPaintOffset(op, size) == fWriter.size());
    const SkFlatData* flatPaintData = addPaint(paint);
    SkASSERT(flatPaintData);
    addText(text, byteLength);
    addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    if (canUseDrawH) {
        if (fast) {
            addFontMetricsTopBottom(paint, *flatPaintData, pos[0].fY, pos[0].fY);
        }
        addScalar(pos[0].fY);
        SkScalar* xptr = (SkScalar*)fWriter.reserve(points * sizeof(SkScalar));
        for (size_t index = 0; index < points; index++)
            *xptr++ = pos[index].fX;
    } else {
        fWriter.writeMul4(pos, points * sizeof(SkPoint));
        if (fastBounds) {
            addFontMetricsTopBottom(paint, *flatPaintData, minY, maxY);
        }
    }
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.size() - start;
    fPointWrites += points;
#endif
    validate(initialOffset, size);
}

void SkPictureRecord::drawPosTextH(const void* text, size_t byteLength,
                          const SkScalar xpos[], SkScalar constY,
                          const SkPaint& paint) {
    size_t points = paint.countText(text, byteLength);
    if (0 == points)
        return;

    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds();

    
    uint32_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 1 * kUInt32Size;
    if (fast) {
        size += 2 * sizeof(SkScalar); 
    }
    
    size += 1 * kUInt32Size + points * sizeof(SkScalar);

    uint32_t initialOffset = this->addDraw(fast ? DRAW_POS_TEXT_H_TOP_BOTTOM : DRAW_POS_TEXT_H,
                                           &size);
    const SkFlatData* flatPaintData = addPaint(paint);
    SkASSERT(flatPaintData);
    addText(text, byteLength);
    addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.size();
#endif
    if (fast) {
        addFontMetricsTopBottom(paint, *flatPaintData, constY, constY);
    }
    addScalar(constY);
    fWriter.writeMul4(xpos, points * sizeof(SkScalar));
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.size() - start;
    fPointWrites += points;
#endif
    validate(initialOffset, size);
}

void SkPictureRecord::drawTextOnPath(const void* text, size_t byteLength,
                            const SkPath& path, const SkMatrix* matrix,
                            const SkPaint& paint) {
    
    uint32_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_TEXT_ON_PATH, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_TEXT_ON_PATH, size) == fWriter.size());
    addPaint(paint);
    addText(text, byteLength);
    addPath(path);
    addMatrixPtr(matrix);
    validate(initialOffset, size);
}

void SkPictureRecord::drawPicture(SkPicture& picture) {
    
    uint32_t size = 2 * kUInt32Size;
    uint32_t initialOffset = this->addDraw(DRAW_PICTURE, &size);
    addPicture(picture);
    validate(initialOffset, size);
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

    
    uint32_t size = 5 * kUInt32Size + vertexCount * sizeof(SkPoint);
    if (flags & DRAW_VERTICES_HAS_TEXS) {
        size += vertexCount * sizeof(SkPoint);  
    }
    if (flags & DRAW_VERTICES_HAS_COLORS) {
        size += vertexCount * sizeof(SkColor);  
    }
    if (flags & DRAW_VERTICES_HAS_INDICES) {
        
        size += 1 * kUInt32Size + SkAlign4(indexCount * sizeof(uint16_t));
    }

    uint32_t initialOffset = this->addDraw(DRAW_VERTICES, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_VERTICES, size) == fWriter.size());
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
    validate(initialOffset, size);
}

void SkPictureRecord::drawData(const void* data, size_t length) {
    
    uint32_t size = 2 * kUInt32Size + SkAlign4(length);
    uint32_t initialOffset = this->addDraw(DRAW_DATA, &size);
    addInt(length);
    fWriter.writePad(data, length);
    validate(initialOffset, size);
}



void SkPictureRecord::addBitmap(const SkBitmap& bitmap) {
    const int index = fBitmapHeap->insert(bitmap);
    
    
    
    SkASSERT(index != SkBitmapHeap::INVALID_SLOT);
    addInt(index);
}

void SkPictureRecord::addMatrix(const SkMatrix& matrix) {
    addMatrixPtr(&matrix);
}

void SkPictureRecord::addMatrixPtr(const SkMatrix* matrix) {
    this->addInt(matrix ? fMatrices.find(*matrix) : 0);
}

const SkFlatData* SkPictureRecord::addPaintPtr(const SkPaint* paint) {
    const SkFlatData* data = paint ? fPaints.findAndReturnFlat(*paint) : NULL;
    int index = data ? data->index() : 0;
    this->addInt(index);
    return data;
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

void SkPictureRecord::addRRect(const SkRRect& rrect) {
    fWriter.writeRRect(rrect);
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
void SkPictureRecord::validate(uint32_t initialOffset, uint32_t size) const {
    SkASSERT(fWriter.size() == initialOffset + size);

    validateBitmaps();
    validateMatrices();
    validatePaints();
    validatePaths();
    validateRegions();
}

void SkPictureRecord::validateBitmaps() const {
    int count = fBitmapHeap->count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkBitmap* bitPtr = fBitmapHeap->getBitmap(index);
        SkASSERT(bitPtr);
        bitPtr->validate();
    }
}

void SkPictureRecord::validateMatrices() const {
    int count = fMatrices.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatData* matrix = fMatrices[index];
        SkASSERT(matrix);

    }
}

void SkPictureRecord::validatePaints() const {
    int count = fPaints.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatData* paint = fPaints[index];
        SkASSERT(paint);

    }
}

void SkPictureRecord::validatePaths() const {
    if (NULL == fPathHeap) {
        return;
    }

    int count = fPathHeap->count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkPath& path = (*fPathHeap)[index];
        path.validate();
    }
}

void SkPictureRecord::validateRegions() const {
    int count = fRegions.count();
    SkASSERT((unsigned) count < 0x1000);
    for (int index = 0; index < count; index++) {
        const SkFlatData* region = fRegions[index];
        SkASSERT(region);

    }
}
#endif
