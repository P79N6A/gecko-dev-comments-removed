






#include "SkPictureRecord.h"
#include "SkTSearch.h"
#include "SkPixelRef.h"
#include "SkRRect.h"
#include "SkBBoxHierarchy.h"
#include "SkDevice.h"
#include "SkPictureStateTree.h"

#define HEAP_BLOCK_SIZE 4096



static const bool kBeClever =
#ifdef SK_RECORD_LITERAL_PICTURES
    false;
#else
    true;
#endif

enum {
    
    kNoInitialSave = -1,
};


static int const kUInt32Size = 4;

static const uint32_t kSaveSize = kUInt32Size;
static const uint32_t kSaveLayerNoBoundsSize = 4 * kUInt32Size;
static const uint32_t kSaveLayerWithBoundsSize = 4 * kUInt32Size + sizeof(SkRect);

SkPictureRecord::SkPictureRecord(const SkISize& dimensions, uint32_t flags)
    : INHERITED(dimensions.width(), dimensions.height())
    , fBoundingHierarchy(NULL)
    , fStateTree(NULL)
    , fFlattenableHeap(HEAP_BLOCK_SIZE)
    , fPaints(&fFlattenableHeap)
    , fRecordFlags(flags)
    , fOptsEnabled(kBeClever) {
#ifdef SK_DEBUG_SIZE
    fPointBytes = fRectBytes = fTextBytes = 0;
    fPointWrites = fRectWrites = fTextWrites = 0;
#endif

    fBitmapHeap = SkNEW(SkBitmapHeap);
    fFlattenableHeap.setBitmapStorage(fBitmapHeap);

#ifndef SK_COLLAPSE_MATRIX_CLIP_STATE
    fFirstSavedLayerIndex = kNoSavedLayerIndex;
#endif

    fInitialSaveCount = kNoInitialSave;

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.init(this);
#endif
}

SkPictureRecord::~SkPictureRecord() {
    SkSafeUnref(fBitmapHeap);
    SkSafeUnref(fBoundingHierarchy);
    SkSafeUnref(fStateTree);
    fFlattenableHeap.setBitmapStorage(NULL);
    fPictureRefs.unrefAll();
}






static inline size_t getPaintOffset(DrawType op, size_t opSize) {
    
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
        0,  
        0,  
        0,  
        1,  
        0,  
        0,  
    };

    SK_COMPILE_ASSERT(sizeof(gPaintOffsets) == LAST_DRAWTYPE_ENUM + 1,
                      need_to_be_in_sync);
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

void SkPictureRecord::willSave() {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.save();
#else
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.bytesWritten());
    this->recordSave();
#endif

    this->INHERITED::willSave();
}

void SkPictureRecord::recordSave() {
    
    size_t size = kSaveSize;
    size_t initialOffset = this->addDraw(SAVE, &size);

    this->validate(initialOffset, size);
}

SkCanvas::SaveLayerStrategy SkPictureRecord::willSaveLayer(const SkRect* bounds,
                                                           const SkPaint* paint, SaveFlags flags) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.saveLayer(bounds, paint, flags);
#else
    
    
    fRestoreOffsetStack.push(-(int32_t)fWriter.bytesWritten());
    this->recordSaveLayer(bounds, paint, flags);
    if (kNoSavedLayerIndex == fFirstSavedLayerIndex) {
        fFirstSavedLayerIndex = fRestoreOffsetStack.count();
    }
#endif

    this->INHERITED::willSaveLayer(bounds, paint, flags);
    




    return kNoLayer_SaveLayerStrategy;
}

void SkPictureRecord::recordSaveLayer(const SkRect* bounds, const SkPaint* paint,
                                      SaveFlags flags) {
    
    size_t size = 2 * kUInt32Size;
    if (NULL != bounds) {
        size += sizeof(*bounds); 
    }
    
    size += 2 * kUInt32Size;

    SkASSERT(kSaveLayerNoBoundsSize == size || kSaveLayerWithBoundsSize == size);

    size_t initialOffset = this->addDraw(SAVE_LAYER, &size);
    this->addRectPtr(bounds);
    SkASSERT(initialOffset+getPaintOffset(SAVE_LAYER, size) == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addInt(flags);

    this->validate(initialOffset, size);
}

bool SkPictureRecord::isDrawingToLayer() const {
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    return fMCMgr.isDrawingToLayer();
#else
    return fFirstSavedLayerIndex != kNoSavedLayerIndex;
#endif
}




#ifdef SK_DEBUG
static DrawType peek_op(SkWriter32* writer, size_t offset) {
    return (DrawType)(writer->readTAt<uint32_t>(offset) >> 24);
}
#endif




static DrawType peek_op_and_size(SkWriter32* writer, size_t offset, uint32_t* size) {
    uint32_t peek = writer->readTAt<uint32_t>(offset);

    uint32_t op;
    UNPACK_8_24(peek, op, *size);
    if (MASK_24 == *size) {
        
        *size = writer->readTAt<uint32_t>(offset + kUInt32Size);
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
    SkASSERT(offset < writer->bytesWritten());

    uint32_t curOffset = offset;
    uint32_t curSize = 0;
    int numMatched;
    for (numMatched = 0; numMatched < numCommands && curOffset < writer->bytesWritten(); ++numMatched) {
        DrawType op = peek_op_and_size(writer, curOffset, &curSize);
        while (NOOP == op) {
            curOffset += curSize;
            if (curOffset >= writer->bytesWritten()) {
                return false;
            }
            op = peek_op_and_size(writer, curOffset, &curSize);
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
    if (curOffset < writer->bytesWritten()) {
        
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
        offset = writer->readTAt<uint32_t>(offset);
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
    uint32_t command = writer->readTAt<uint32_t>(offset);
    writer->overwriteTAt(offset, (command & MASK_24) | (NOOP << 24));
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

    size_t dbmPaintOffset = getPaintOffset(dbmInfo.fActualOp, dbmInfo.fSize);
    size_t slPaintOffset = getPaintOffset(SAVE_LAYER, saveLayerInfo.fSize);

    
    uint32_t dbmPaintId = writer->readTAt<uint32_t>(dbmInfo.fOffset + dbmPaintOffset);
    uint32_t saveLayerPaintId = writer->readTAt<uint32_t>(saveLayerInfo.fOffset + slPaintOffset);

    if (0 == saveLayerPaintId) {
        
        
        convert_command_to_noop(writer, saveLayerInfo.fOffset);
        return true;
    }

    if (0 == dbmPaintId) {
        
        
        convert_command_to_noop(writer, saveLayerInfo.fOffset);
        writer->overwriteTAt(dbmInfo.fOffset + dbmPaintOffset, saveLayerPaintId);
        return true;
    }

    SkAutoTDelete<SkPaint> saveLayerPaint(paintDict->unflatten(saveLayerPaintId));
    if (NULL == saveLayerPaint.get() || !is_simple(*saveLayerPaint)) {
        return false;
    }

    
    
    
    
    
    
    SkColor layerColor = saveLayerPaint->getColor() | 0xFF000000; 

    SkAutoTDelete<SkPaint> dbmPaint(paintDict->unflatten(dbmPaintId));
    if (NULL == dbmPaint.get() || dbmPaint->getColor() != layerColor || !is_simple(*dbmPaint)) {
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
    writer->overwriteTAt(dbmInfo.fOffset + dbmPaintOffset, data->index());
    return true;
}












static bool remove_save_layer2(SkWriter32* writer, int32_t offset,
                               SkPaintDictionary* paintDict) {

    
    
    while (offset > 0) {
        offset = writer->readTAt<uint32_t>(offset);
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

static bool is_drawing_op(DrawType op) {
    return (op > CONCAT && op < ROTATE) || DRAW_DRRECT == op;
}









static bool collapse_save_clip_restore(SkWriter32* writer, int32_t offset,
                                       SkPaintDictionary* paintDict) {
#ifdef TRACK_COLLAPSE_STATS
    gCollapseCalls += 1;
#endif

    int32_t restoreOffset = (int32_t)writer->bytesWritten();

    
    while (offset > 0) {
        offset = writer->readTAt<uint32_t>(offset);
    }

    
    offset = -offset;
    uint32_t opSize;
    DrawType op = peek_op_and_size(writer, offset, &opSize);
    if (SAVE_LAYER == op) {
        
        return false;
    }
    SkASSERT(SAVE == op);
    SkASSERT(kSaveSize == opSize);

    
    
    int32_t saveOffset = offset;

    offset += opSize;
    while (offset < restoreOffset) {
        op = peek_op_and_size(writer, offset, &opSize);
        if (is_drawing_op(op) || (SAVE_LAYER == op)) {
            
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

enum PictureRecordOptFlags {
    kSkipIfBBoxHierarchy_Flag = 0x1,  
                                      
};

struct PictureRecordOpt {
    PictureRecordOptProc fProc;
    PictureRecordOptType fType;
    unsigned fFlags;
};





static const PictureRecordOpt gPictureRecordOpts[] = {
    
    
    
    
    
    { collapse_save_clip_restore, kRewind_OptType, kSkipIfBBoxHierarchy_Flag },
    { remove_save_layer1,         kCollapseSaveLayer_OptType, 0 },
    { remove_save_layer2,         kCollapseSaveLayer_OptType, 0 }
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

void SkPictureRecord::willRestore() {
    
    
    
#if 0
    SkASSERT(fRestoreOffsetStack.count() > 1);
#endif

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    if (fMCMgr.getSaveCount() == 1) {
        return;
    }

    fMCMgr.restore();
#else
    
    if (fRestoreOffsetStack.count() == 0) {
        return;
    }

    if (fRestoreOffsetStack.count() == fFirstSavedLayerIndex) {
        fFirstSavedLayerIndex = kNoSavedLayerIndex;
    }

    size_t opt = 0;
    if (fOptsEnabled) {
        for (opt = 0; opt < SK_ARRAY_COUNT(gPictureRecordOpts); ++opt) {
            if (0 != (gPictureRecordOpts[opt].fFlags & kSkipIfBBoxHierarchy_Flag)
                && NULL != fBoundingHierarchy) {
                continue;
            }
            if ((*gPictureRecordOpts[opt].fProc)(&fWriter, fRestoreOffsetStack.top(), &fPaints)) {
                
                apply_optimization_to_bbh(gPictureRecordOpts[opt].fType,
                                          fStateTree, fBoundingHierarchy);
                break;
            }
        }
    }

    if (!fOptsEnabled || SK_ARRAY_COUNT(gPictureRecordOpts) == opt) {
        
        this->recordRestore();
    }

    fRestoreOffsetStack.pop();
#endif

    this->INHERITED::willRestore();
}

void SkPictureRecord::recordRestore(bool fillInSkips) {
    if (fillInSkips) {
        this->fillRestoreOffsetPlaceholdersForCurrentStackLevel((uint32_t)fWriter.bytesWritten());
    }
    size_t size = 1 * kUInt32Size; 
    size_t initialOffset = this->addDraw(RESTORE, &size);
    this->validate(initialOffset, size);
}

void SkPictureRecord::recordTranslate(const SkMatrix& m) {
    SkASSERT(SkMatrix::kTranslate_Mask == m.getType());

    
    size_t size = 1 * kUInt32Size + 2 * sizeof(SkScalar);
    size_t initialOffset = this->addDraw(TRANSLATE, &size);
    this->addScalar(m.getTranslateX());
    this->addScalar(m.getTranslateY());
    this->validate(initialOffset, size);
}

void SkPictureRecord::recordScale(const SkMatrix& m) {
    SkASSERT(SkMatrix::kScale_Mask == m.getType());

    
    size_t size = 1 * kUInt32Size + 2 * sizeof(SkScalar);
    size_t initialOffset = this->addDraw(SCALE, &size);
    this->addScalar(m.getScaleX());
    this->addScalar(m.getScaleY());
    this->validate(initialOffset, size);
}

void SkPictureRecord::didConcat(const SkMatrix& matrix) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.concat(matrix);
#else
    switch (matrix.getType()) {
        case SkMatrix::kTranslate_Mask:
            this->recordTranslate(matrix);
            break;
        case SkMatrix::kScale_Mask:
            this->recordScale(matrix);
            break;
        default:
            this->recordConcat(matrix);
            break;
    }
#endif
    this->INHERITED::didConcat(matrix);
}

void SkPictureRecord::recordConcat(const SkMatrix& matrix) {
    this->validate(fWriter.bytesWritten(), 0);
    
    size_t size = kUInt32Size + matrix.writeToMemory(NULL);
    size_t initialOffset = this->addDraw(CONCAT, &size);
    this->addMatrix(matrix);
    this->validate(initialOffset, size);
}

void SkPictureRecord::didSetMatrix(const SkMatrix& matrix) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.setMatrix(matrix);
#else
    this->validate(fWriter.bytesWritten(), 0);
    
    size_t size = kUInt32Size + matrix.writeToMemory(NULL);
    size_t initialOffset = this->addDraw(SET_MATRIX, &size);
    this->addMatrix(matrix);
    this->validate(initialOffset, size);
#endif
    this->INHERITED::didSetMatrix(matrix);
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

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
void SkPictureRecord::fillRestoreOffsetPlaceholdersForCurrentStackLevel(uint32_t restoreOffset) {
    fMCMgr.fillInSkips(&fWriter, restoreOffset);
}
#else
void SkPictureRecord::fillRestoreOffsetPlaceholdersForCurrentStackLevel(uint32_t restoreOffset) {
    int32_t offset = fRestoreOffsetStack.top();
    while (offset > 0) {
        uint32_t peek = fWriter.readTAt<uint32_t>(offset);
        fWriter.overwriteTAt(offset, restoreOffset);
        offset = peek;
    }

#ifdef SK_DEBUG
    
    uint32_t opSize;
    DrawType drawOp = peek_op_and_size(&fWriter, -offset, &opSize);
    SkASSERT(SAVE == drawOp || SAVE_LAYER == drawOp);
#endif
}
#endif

void SkPictureRecord::beginRecording() {
    
    
    
    fInitialSaveCount = this->save();
}

void SkPictureRecord::endRecording() {
    SkASSERT(kNoInitialSave != fInitialSaveCount);
    this->restoreToCount(fInitialSaveCount);
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.finish();
#endif
}

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
int SkPictureRecord::recordRestoreOffsetPlaceholder(SkRegion::Op op) {
    size_t offset = fWriter.bytesWritten();
    this->addInt(-1);
    return offset;
}
#else
size_t SkPictureRecord::recordRestoreOffsetPlaceholder(SkRegion::Op op) {
    if (fRestoreOffsetStack.isEmpty()) {
        return -1;
    }

    
    
    
    
    
    int32_t prevOffset = fRestoreOffsetStack.top();

    if (regionOpExpands(op)) {
        
        
        
        
        this->fillRestoreOffsetPlaceholdersForCurrentStackLevel(0);

        
        
        prevOffset = 0;
    }

    size_t offset = fWriter.bytesWritten();
    this->addInt(prevOffset);
    fRestoreOffsetStack.top() = SkToU32(offset);
    return offset;
}
#endif

void SkPictureRecord::onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.clipRect(rect, op, doAA);
#else
    this->recordClipRect(rect, op, kSoft_ClipEdgeStyle == edgeStyle);
#endif
    this->INHERITED::onClipRect(rect, op, edgeStyle);
}

size_t SkPictureRecord::recordClipRect(const SkRect& rect, SkRegion::Op op, bool doAA) {
    
    size_t size = 1 * kUInt32Size + sizeof(rect) + 1 * kUInt32Size;
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    size += kUInt32Size;    
#else
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
#endif
    size_t initialOffset = this->addDraw(CLIP_RECT, &size);
    this->addRect(rect);
    this->addInt(ClipParams_pack(op, doAA));
    size_t offset = this->recordRestoreOffsetPlaceholder(op);

    this->validate(initialOffset, size);
    return offset;
}

void SkPictureRecord::onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle edgeStyle) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.clipRRect(rrect, op, doAA);
#else
    this->recordClipRRect(rrect, op, kSoft_ClipEdgeStyle == edgeStyle);
#endif
    this->updateClipConservativelyUsingBounds(rrect.getBounds(), op, false);
}

size_t SkPictureRecord::recordClipRRect(const SkRRect& rrect, SkRegion::Op op, bool doAA) {
    
    size_t size = 1 * kUInt32Size + SkRRect::kSizeInMemory + 1 * kUInt32Size;
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    size += kUInt32Size;    
#else
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
#endif
    size_t initialOffset = this->addDraw(CLIP_RRECT, &size);
    this->addRRect(rrect);
    this->addInt(ClipParams_pack(op, doAA));
    size_t offset = recordRestoreOffsetPlaceholder(op);
    this->validate(initialOffset, size);
    return offset;
}

void SkPictureRecord::onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle edgeStyle) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.clipPath(path, op, doAA);
#else
    int pathID = this->addPathToHeap(path);
    this->recordClipPath(pathID, op, kSoft_ClipEdgeStyle == edgeStyle);
#endif

    this->updateClipConservativelyUsingBounds(path.getBounds(), op,
                                              path.isInverseFillType());
}

size_t SkPictureRecord::recordClipPath(int pathID, SkRegion::Op op, bool doAA) {
    
    size_t size = 3 * kUInt32Size;
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    size += kUInt32Size;    
#else
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
#endif
    size_t initialOffset = this->addDraw(CLIP_PATH, &size);
    this->addInt(pathID);
    this->addInt(ClipParams_pack(op, doAA));
    size_t offset = recordRestoreOffsetPlaceholder(op);
    this->validate(initialOffset, size);
    return offset;
}

void SkPictureRecord::onClipRegion(const SkRegion& region, SkRegion::Op op) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.clipRegion(region, op);
#else
    this->recordClipRegion(region, op);
#endif
    this->INHERITED::onClipRegion(region, op);
}

size_t SkPictureRecord::recordClipRegion(const SkRegion& region, SkRegion::Op op) {
    
    size_t size = 2 * kUInt32Size + region.writeToMemory(NULL);
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    size += kUInt32Size;    
#else
    
    if (!fRestoreOffsetStack.isEmpty()) {
        
        size += kUInt32Size;
    }
#endif
    size_t initialOffset = this->addDraw(CLIP_REGION, &size);
    this->addRegion(region);
    this->addInt(ClipParams_pack(op, false));
    size_t offset = this->recordRestoreOffsetPlaceholder(op);

    this->validate(initialOffset, size);
    return offset;
}

void SkPictureRecord::clear(SkColor color) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size;
    size_t initialOffset = this->addDraw(DRAW_CLEAR, &size);
    this->addInt(color);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawPaint(const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size;
    size_t initialOffset = this->addDraw(DRAW_PAINT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_PAINT, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                                 const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 4 * kUInt32Size + count * sizeof(SkPoint);
    size_t initialOffset = this->addDraw(DRAW_POINTS, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_POINTS, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    if (paint.getPathEffect() != NULL) {
        SkPathEffect::DashInfo info;
        SkPathEffect::DashType dashType = paint.getPathEffect()->asADash(&info);
        if (2 == count && SkPaint::kRound_Cap != paint.getStrokeCap() &&
            SkPathEffect::kDash_DashType == dashType && 2 == info.fCount) {
            fContentInfo.incFastPathDashEffects();
        }
    }
    this->addInt(mode);
    this->addInt(SkToInt(count));
    fWriter.writeMul4(pts, count * sizeof(SkPoint));
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawOval(const SkRect& oval, const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size + sizeof(oval);
    size_t initialOffset = this->addDraw(DRAW_OVAL, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_OVAL, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addRect(oval);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawRect(const SkRect& rect, const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size + sizeof(rect);
    size_t initialOffset = this->addDraw(DRAW_RECT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_RECT, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addRect(rect);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawRRect(const SkRRect& rrect, const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    if (rrect.isRect() && kBeClever) {
        this->SkPictureRecord::drawRect(rrect.getBounds(), paint);
    } else if (rrect.isOval() && kBeClever) {
        this->SkPictureRecord::drawOval(rrect.getBounds(), paint);
    } else {
        
        size_t size = 2 * kUInt32Size + SkRRect::kSizeInMemory;
        size_t initialOffset = this->addDraw(DRAW_RRECT, &size);
        SkASSERT(initialOffset+getPaintOffset(DRAW_RRECT, size) == fWriter.bytesWritten());
        this->addPaint(paint);
        this->addRRect(rrect);
        this->validate(initialOffset, size);
    }
}

void SkPictureRecord::onDrawDRRect(const SkRRect& outer, const SkRRect& inner,
                                   const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size + SkRRect::kSizeInMemory * 2;
    size_t initialOffset = this->addDraw(DRAW_DRRECT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_DRRECT, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addRRect(outer);
    this->addRRect(inner);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawPath(const SkPath& path, const SkPaint& paint) {

    if (paint.isAntiAlias() && !path.isConvex()) {
        fContentInfo.incAAConcavePaths();

        if (SkPaint::kStroke_Style == paint.getStyle() &&
            0 == paint.getStrokeWidth()) {
            fContentInfo.incAAHairlineConcavePaths();
        }
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 3 * kUInt32Size;
    size_t initialOffset = this->addDraw(DRAW_PATH, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_PATH, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addPath(path);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                                 const SkPaint* paint = NULL) {
    if (bitmap.drawsNothing() && kBeClever) {
        return;
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 3 * kUInt32Size + 2 * sizeof(SkScalar);
    size_t initialOffset = this->addDraw(DRAW_BITMAP, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP, size) == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addBitmap(bitmap);
    this->addScalar(left);
    this->addScalar(top);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                                           const SkRect& dst, const SkPaint* paint,
                                           DrawBitmapRectFlags flags) {
    if (bitmap.drawsNothing() && kBeClever) {
        return;
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif
    
    size_t size = 5 * kUInt32Size;
    if (NULL != src) {
        size += sizeof(*src);   
    }
    size += sizeof(dst);        

    size_t initialOffset = this->addDraw(DRAW_BITMAP_RECT_TO_RECT, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_RECT_TO_RECT, size)
             == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addBitmap(bitmap);
    this->addRectPtr(src);  
    this->addRect(dst);
    this->addInt(flags);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& matrix,
                                       const SkPaint* paint) {
    if (bitmap.drawsNothing() && kBeClever) {
        return;
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 3 * kUInt32Size + matrix.writeToMemory(NULL);
    size_t initialOffset = this->addDraw(DRAW_BITMAP_MATRIX, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_MATRIX, size) == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addBitmap(bitmap);
    this->addMatrix(matrix);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                     const SkRect& dst, const SkPaint* paint) {
    if (bitmap.drawsNothing() && kBeClever) {
        return;
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 3 * kUInt32Size + sizeof(center) + sizeof(dst);
    size_t initialOffset = this->addDraw(DRAW_BITMAP_NINE, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_BITMAP_NINE, size) == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addBitmap(bitmap);
    this->addIRect(center);
    this->addRect(dst);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawSprite(const SkBitmap& bitmap, int left, int top,
                                 const SkPaint* paint = NULL) {
    if (bitmap.drawsNothing() && kBeClever) {
        return;
    }

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 5 * kUInt32Size;
    size_t initialOffset = this->addDraw(DRAW_SPRITE, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_SPRITE, size) == fWriter.bytesWritten());
    this->addPaintPtr(paint);
    this->addBitmap(bitmap);
    this->addInt(left);
    this->addInt(top);
    this->validate(initialOffset, size);
}

void SkPictureRecord::ComputeFontMetricsTopBottom(const SkPaint& paint, SkScalar topbot[2]) {
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
    WriteTopBot(paint, flat);
    this->addScalar(flat.topBot()[0] + minY);
    this->addScalar(flat.topBot()[1] + maxY);
}

void SkPictureRecord::onDrawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
                                 const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds() && kBeClever;

    
    size_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 2 * sizeof(SkScalar);
    if (fast) {
        size += 2 * sizeof(SkScalar); 
    }

    DrawType op = fast ? DRAW_TEXT_TOP_BOTTOM : DRAW_TEXT;
    size_t initialOffset = this->addDraw(op, &size);
    SkASSERT(initialOffset+getPaintOffset(op, size) == fWriter.bytesWritten());
    const SkFlatData* flatPaintData = addPaint(paint);
    SkASSERT(flatPaintData);
    this->addText(text, byteLength);
    this->addScalar(x);
    this->addScalar(y);
    if (fast) {
        this->addFontMetricsTopBottom(paint, *flatPaintData, y, y);
    }
    this->validate(initialOffset, size);
}

void SkPictureRecord::onDrawPosText(const void* text, size_t byteLength, const SkPoint pos[],
                                    const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    int points = paint.countText(text, byteLength);
    if (0 == points)
        return;

    bool canUseDrawH = true;
    SkScalar minY = pos[0].fY;
    SkScalar maxY = pos[0].fY;
    
    {
        const SkScalar firstY = pos[0].fY;
        for (int index = 1; index < points; index++) {
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

    bool fastBounds = !paint.isVerticalText() && paint.canComputeFastBounds() && kBeClever;
    bool fast = canUseDrawH && fastBounds && kBeClever;

    
    size_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 1 * kUInt32Size;
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
    size_t initialOffset = this->addDraw(op, &size);
    SkASSERT(initialOffset+getPaintOffset(op, size) == fWriter.bytesWritten());
    const SkFlatData* flatPaintData = this->addPaint(paint);
    SkASSERT(flatPaintData);
    this->addText(text, byteLength);
    this->addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.bytesWritten();
#endif
    if (canUseDrawH) {
        if (fast) {
            this->addFontMetricsTopBottom(paint, *flatPaintData, pos[0].fY, pos[0].fY);
        }
        this->addScalar(pos[0].fY);
        SkScalar* xptr = (SkScalar*)fWriter.reserve(points * sizeof(SkScalar));
        for (int index = 0; index < points; index++)
            *xptr++ = pos[index].fX;
    } else {
        fWriter.writeMul4(pos, points * sizeof(SkPoint));
        if (fastBounds) {
            this->addFontMetricsTopBottom(paint, *flatPaintData, minY, maxY);
        }
    }
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.bytesWritten() - start;
    fPointWrites += points;
#endif
    this->validate(initialOffset, size);
}

void SkPictureRecord::onDrawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[],
                                     SkScalar constY, const SkPaint& paint) {
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    const SkFlatData* flatPaintData = this->getFlatPaintData(paint);
    this->drawPosTextHImpl(text, byteLength, xpos, constY, paint, flatPaintData);
}

void SkPictureRecord::drawPosTextHImpl(const void* text, size_t byteLength,
                          const SkScalar xpos[], SkScalar constY,
                          const SkPaint& paint, const SkFlatData* flatPaintData) {
    int points = paint.countText(text, byteLength);
    if (0 == points && kBeClever) {
        return;
    }

    bool fast = !paint.isVerticalText() && paint.canComputeFastBounds() && kBeClever;

    
    size_t size = 3 * kUInt32Size + SkAlign4(byteLength) + 1 * kUInt32Size;
    if (fast) {
        size += 2 * sizeof(SkScalar); 
    }
    
    size += 1 * kUInt32Size + points * sizeof(SkScalar);
    size_t initialOffset = this->addDraw(fast ? DRAW_POS_TEXT_H_TOP_BOTTOM : DRAW_POS_TEXT_H,
                                         &size);
    SkASSERT(flatPaintData);
    this->addFlatPaint(flatPaintData);

    this->addText(text, byteLength);
    this->addInt(points);

#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.bytesWritten();
#endif
    if (fast) {
        this->addFontMetricsTopBottom(paint, *flatPaintData, constY, constY);
    }
    this->addScalar(constY);
    fWriter.writeMul4(xpos, points * sizeof(SkScalar));
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.bytesWritten() - start;
    fPointWrites += points;
#endif
    this->validate(initialOffset, size);
}

void SkPictureRecord::onDrawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                       const SkMatrix* matrix, const SkPaint& paint) {
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    const SkMatrix& m = matrix ? *matrix : SkMatrix::I();
    size_t size = 3 * kUInt32Size + SkAlign4(byteLength) + kUInt32Size + m.writeToMemory(NULL);
    size_t initialOffset = this->addDraw(DRAW_TEXT_ON_PATH, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_TEXT_ON_PATH, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addText(text, byteLength);
    this->addPath(path);
    this->addMatrix(m);
    this->validate(initialOffset, size);
}

void SkPictureRecord::onDrawPicture(const SkPicture* picture) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size;
    size_t initialOffset = this->addDraw(DRAW_PICTURE, &size);
    this->addPicture(picture);
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawVertices(VertexMode vmode, int vertexCount,
                          const SkPoint vertices[], const SkPoint texs[],
                          const SkColor colors[], SkXfermode* xfer,
                          const uint16_t indices[], int indexCount,
                          const SkPaint& paint) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

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
    if (NULL != xfer) {
        SkXfermode::Mode mode;
        if (xfer->asMode(&mode) && SkXfermode::kModulate_Mode != mode) {
            flags |= DRAW_VERTICES_HAS_XFER;
        }
    }

    
    size_t size = 5 * kUInt32Size + vertexCount * sizeof(SkPoint);
    if (flags & DRAW_VERTICES_HAS_TEXS) {
        size += vertexCount * sizeof(SkPoint);  
    }
    if (flags & DRAW_VERTICES_HAS_COLORS) {
        size += vertexCount * sizeof(SkColor);  
    }
    if (flags & DRAW_VERTICES_HAS_INDICES) {
        
        size += 1 * kUInt32Size + SkAlign4(indexCount * sizeof(uint16_t));
    }
    if (flags & DRAW_VERTICES_HAS_XFER) {
        size += kUInt32Size;    
    }

    size_t initialOffset = this->addDraw(DRAW_VERTICES, &size);
    SkASSERT(initialOffset+getPaintOffset(DRAW_VERTICES, size) == fWriter.bytesWritten());
    this->addPaint(paint);
    this->addInt(flags);
    this->addInt(vmode);
    this->addInt(vertexCount);
    this->addPoints(vertices, vertexCount);
    if (flags & DRAW_VERTICES_HAS_TEXS) {
        this->addPoints(texs, vertexCount);
    }
    if (flags & DRAW_VERTICES_HAS_COLORS) {
        fWriter.writeMul4(colors, vertexCount * sizeof(SkColor));
    }
    if (flags & DRAW_VERTICES_HAS_INDICES) {
        this->addInt(indexCount);
        fWriter.writePad(indices, indexCount * sizeof(uint16_t));
    }
    if (flags & DRAW_VERTICES_HAS_XFER) {
        SkXfermode::Mode mode = SkXfermode::kModulate_Mode;
        (void)xfer->asMode(&mode);
        this->addInt(mode);
    }
    this->validate(initialOffset, size);
}

void SkPictureRecord::drawData(const void* data, size_t length) {

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    fMCMgr.call(SkMatrixClipStateMgr::kOther_CallType);
#endif

    
    size_t size = 2 * kUInt32Size + SkAlign4(length);
    size_t initialOffset = this->addDraw(DRAW_DATA, &size);
    this->addInt(SkToInt(length));
    fWriter.writePad(data, length);
    this->validate(initialOffset, size);
}

void SkPictureRecord::beginCommentGroup(const char* description) {
    
    size_t length = strlen(description);
    size_t size = 2 * kUInt32Size + SkAlign4(length + 1);
    size_t initialOffset = this->addDraw(BEGIN_COMMENT_GROUP, &size);
    fWriter.writeString(description, length);
    this->validate(initialOffset, size);
}

void SkPictureRecord::addComment(const char* kywd, const char* value) {
    
    size_t kywdLen = strlen(kywd);
    size_t valueLen = strlen(value);
    size_t size = 3 * kUInt32Size + SkAlign4(kywdLen + 1) + SkAlign4(valueLen + 1);
    size_t initialOffset = this->addDraw(COMMENT, &size);
    fWriter.writeString(kywd, kywdLen);
    fWriter.writeString(value, valueLen);
    this->validate(initialOffset, size);
}

void SkPictureRecord::endCommentGroup() {
    
    size_t size = 1 * kUInt32Size;
    size_t initialOffset = this->addDraw(END_COMMENT_GROUP, &size);
    this->validate(initialOffset, size);
}


static const uint32_t kPushCullOpSize = 2 * kUInt32Size + sizeof(SkRect);
void SkPictureRecord::onPushCull(const SkRect& cullRect) {
    size_t size = kPushCullOpSize;
    size_t initialOffset = this->addDraw(PUSH_CULL, &size);
    
    SkASSERT(size == kPushCullOpSize);

    this->addRect(cullRect);
    fCullOffsetStack.push(SkToU32(fWriter.bytesWritten()));
    this->addInt(0);
    this->validate(initialOffset, size);
}

void SkPictureRecord::onPopCull() {
    SkASSERT(!fCullOffsetStack.isEmpty());

    uint32_t cullSkipOffset = fCullOffsetStack.top();
    fCullOffsetStack.pop();

    
    if ((size_t)(cullSkipOffset + kUInt32Size) == fWriter.bytesWritten() && kBeClever) {
        SkASSERT(fWriter.bytesWritten() >= kPushCullOpSize);
        SkASSERT(PUSH_CULL == peek_op(&fWriter, fWriter.bytesWritten() - kPushCullOpSize));
        fWriter.rewindToOffset(fWriter.bytesWritten() - kPushCullOpSize);
        return;
    }

    
    size_t size = kUInt32Size;
    size_t initialOffset = this->addDraw(POP_CULL, &size);

    
    fWriter.overwriteTAt<uint32_t>(cullSkipOffset, SkToU32(fWriter.bytesWritten()));

    this->validate(initialOffset, size);
}



SkSurface* SkPictureRecord::onNewSurface(const SkImageInfo& info) {
    return NULL;
}

int SkPictureRecord::addBitmap(const SkBitmap& bitmap) {
    const int index = fBitmapHeap->insert(bitmap);
    
    
    
    SkASSERT(index != SkBitmapHeap::INVALID_SLOT);
    this->addInt(index);
    return index;
}

void SkPictureRecord::addMatrix(const SkMatrix& matrix) {
    fWriter.writeMatrix(matrix);
}

const SkFlatData* SkPictureRecord::getFlatPaintData(const SkPaint& paint) {
    return fPaints.findAndReturnFlat(paint);
}

const SkFlatData* SkPictureRecord::addPaintPtr(const SkPaint* paint) {
    if (NULL != paint && NULL != paint->getPathEffect()) {
        fContentInfo.incPaintWithPathEffectUses();
    }

    const SkFlatData* data = paint ? getFlatPaintData(*paint) : NULL;
    this->addFlatPaint(data);
    return data;
}

void SkPictureRecord::addFlatPaint(const SkFlatData* flatPaint) {
    int index = flatPaint ? flatPaint->index() : 0;
    this->addInt(index);
}

int SkPictureRecord::addPathToHeap(const SkPath& path) {
    if (NULL == fPathHeap) {
        fPathHeap.reset(SkNEW(SkPathHeap));
    }
#ifdef SK_DEDUP_PICTURE_PATHS
    return fPathHeap->insert(path);
#else
    return fPathHeap->append(path);
#endif
}

void SkPictureRecord::addPath(const SkPath& path) {
    this->addInt(this->addPathToHeap(path));
}

void SkPictureRecord::addPicture(const SkPicture* picture) {
    int index = fPictureRefs.find(picture);
    if (index < 0) {    
        index = fPictureRefs.count();
        *fPictureRefs.append() = picture;
        picture->ref();
    }
    
    this->addInt(index + 1);
}

void SkPictureRecord::addPoint(const SkPoint& point) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.bytesWritten();
#endif
    fWriter.writePoint(point);
#ifdef SK_DEBUG_SIZE
    fPointBytes += fWriter.bytesWritten() - start;
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

void SkPictureRecord::addNoOp() {
    size_t size = kUInt32Size; 
    this->addDraw(NOOP, &size);
}

void SkPictureRecord::addRect(const SkRect& rect) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.bytesWritten();
#endif
    fWriter.writeRect(rect);
#ifdef SK_DEBUG_SIZE
    fRectBytes += fWriter.bytesWritten() - start;
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
    fWriter.writeRegion(region);
}

void SkPictureRecord::addText(const void* text, size_t byteLength) {
#ifdef SK_DEBUG_SIZE
    size_t start = fWriter.bytesWritten();
#endif
    addInt(SkToInt(byteLength));
    fWriter.writePad(text, byteLength);
#ifdef SK_DEBUG_SIZE
    fTextBytes += fWriter.bytesWritten() - start;
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
