









#ifndef SkGPipePriv_DEFINED
#define SkGPipePriv_DEFINED

#include "SkTypes.h"

#define UNIMPLEMENTED


enum PaintFlats {
    kColorFilter_PaintFlat,
    kDrawLooper_PaintFlat,
    kImageFilter_PaintFlat,
    kMaskFilter_PaintFlat,
    kPathEffect_PaintFlat,
    kRasterizer_PaintFlat,
    kShader_PaintFlat,
    kXfermode_PaintFlat,

    kLast_PaintFlat = kXfermode_PaintFlat
};
#define kCount_PaintFlats   (kLast_PaintFlat + 1)

enum DrawOps {
    kSkip_DrawOp,   

    
    kClipPath_DrawOp,
    kClipRegion_DrawOp,
    kClipRect_DrawOp,
    kClipRRect_DrawOp,
    kConcat_DrawOp,
    kDrawBitmap_DrawOp,
    kDrawBitmapMatrix_DrawOp,
    kDrawBitmapNine_DrawOp,
    kDrawBitmapRectToRect_DrawOp,
    kDrawClear_DrawOp,
    kDrawData_DrawOp,
    kDrawDRRect_DrawOp,
    kDrawOval_DrawOp,
    kDrawPaint_DrawOp,
    kDrawPath_DrawOp,
    kDrawPicture_DrawOp,
    kDrawPoints_DrawOp,
    kDrawPosText_DrawOp,
    kDrawPosTextH_DrawOp,
    kDrawRect_DrawOp,
    kDrawRRect_DrawOp,
    kDrawSprite_DrawOp,
    kDrawText_DrawOp,
    kDrawTextOnPath_DrawOp,
    kDrawVertices_DrawOp,
    kRestore_DrawOp,
    kRotate_DrawOp,
    kSave_DrawOp,
    kSaveLayer_DrawOp,
    kScale_DrawOp,
    kSetMatrix_DrawOp,
    kSkew_DrawOp,
    kTranslate_DrawOp,

    kPaintOp_DrawOp,
    kSetTypeface_DrawOp,
    kSetAnnotation_DrawOp,

    kDef_Typeface_DrawOp,
    kDef_Flattenable_DrawOp,
    kDef_Bitmap_DrawOp,
    kDef_Factory_DrawOp,

    
    kReportFlags_DrawOp,
    kShareBitmapHeap_DrawOp,
    kDone_DrawOp,
};














#define DRAWOPS_OP_BITS     8
#define DRAWOPS_FLAG_BITS   4
#define DRAWOPS_DATA_BITS   20

#define DRAWOPS_OP_MASK     ((1 << DRAWOPS_OP_BITS) - 1)
#define DRAWOPS_FLAG_MASK   ((1 << DRAWOPS_FLAG_BITS) - 1)
#define DRAWOPS_DATA_MASK   ((1 << DRAWOPS_DATA_BITS) - 1)

static inline unsigned DrawOp_unpackOp(uint32_t op32) {
    return (op32 >> (DRAWOPS_FLAG_BITS + DRAWOPS_DATA_BITS));
}

static inline unsigned DrawOp_unpackFlags(uint32_t op32) {
    return (op32 >> DRAWOPS_DATA_BITS) & DRAWOPS_FLAG_MASK;
}

static inline unsigned DrawOp_unpackData(uint32_t op32) {
    return op32 & DRAWOPS_DATA_MASK;
}

static inline uint32_t DrawOp_packOpFlagData(DrawOps op, unsigned flags, unsigned data) {
    SkASSERT(0 == (op & ~DRAWOPS_OP_MASK));
    SkASSERT(0 == (flags & ~DRAWOPS_FLAG_MASK));
    SkASSERT(0 == (data & ~DRAWOPS_DATA_MASK));

    return (op << (DRAWOPS_FLAG_BITS + DRAWOPS_DATA_BITS)) |
           (flags << DRAWOPS_DATA_BITS) |
            data;
}




enum {
    kSaveLayer_HasBounds_DrawOpFlag = 1 << 0,
    kSaveLayer_HasPaint_DrawOpFlag = 1 << 1,
};
enum {
    kClear_HasColor_DrawOpFlag  = 1 << 0
};
enum {
    kDrawTextOnPath_HasMatrix_DrawOpFlag = 1 << 0
};
enum {
    kDrawVertices_HasTexs_DrawOpFlag     = 1 << 0,
    kDrawVertices_HasColors_DrawOpFlag   = 1 << 1,
    kDrawVertices_HasIndices_DrawOpFlag  = 1 << 2,
    kDrawVertices_HasXfermode_DrawOpFlag = 1 << 3,
};
enum {
    kDrawBitmap_HasPaint_DrawOpFlag   = 1 << 0,
    
    
    kDrawBitmap_HasSrcRect_DrawOpFlag = 1 << 1,
    
    
    kDrawBitmap_Bleed_DrawOpFlag      = 1 << 2,
};
enum {
    kClip_HasAntiAlias_DrawOpFlag = 1 << 0,
};


class BitmapInfo : SkNoncopyable {
public:
    BitmapInfo(SkBitmap* bitmap, uint32_t genID, int toBeDrawnCount)
        : fBitmap(bitmap)
        , fGenID(genID)
        , fBytesAllocated(0)
        , fMoreRecentlyUsed(NULL)
        , fLessRecentlyUsed(NULL)
        , fToBeDrawnCount(toBeDrawnCount)
    {}

    ~BitmapInfo() {
        SkASSERT(0 == fToBeDrawnCount);
        SkDELETE(fBitmap);
    }

    void addDraws(int drawsToAdd) {
        if (0 == fToBeDrawnCount) {
            
            
            
            fToBeDrawnCount = drawsToAdd;
        } else {
            sk_atomic_add(&fToBeDrawnCount, drawsToAdd);
        }
    }

    void decDraws() {
        sk_atomic_dec(&fToBeDrawnCount);
    }

    int drawCount() const {
        return fToBeDrawnCount;
    }

    SkBitmap* fBitmap;
    
    
    
    
    
    uint32_t fGenID;
    
    
    
    size_t fBytesAllocated;
    
    BitmapInfo* fMoreRecentlyUsed;
    BitmapInfo* fLessRecentlyUsed;
private:
    int      fToBeDrawnCount;
};

static inline bool shouldFlattenBitmaps(uint32_t flags) {
    return SkToBool(flags & SkGPipeWriter::kCrossProcess_Flag
            && !(flags & SkGPipeWriter::kSharedAddressSpace_Flag));
}



enum PaintOps {
    kReset_PaintOp,     

    kFlags_PaintOp,     
    kColor_PaintOp,     
    kFilterLevel_PaintOp,   
    kStyle_PaintOp,     
    kJoin_PaintOp,      
    kCap_PaintOp,       
    kWidth_PaintOp,     
    kMiter_PaintOp,     

    kEncoding_PaintOp,  
    kHinting_PaintOp,   
    kAlign_PaintOp,     
    kTextSize_PaintOp,  
    kTextScaleX_PaintOp,
    kTextSkewX_PaintOp, 
    kTypeface_PaintOp,  

    kFlatIndex_PaintOp, 
};

#define PAINTOPS_OP_BITS     8
#define PAINTOPS_FLAG_BITS   4
#define PAINTOPS_DATA_BITS   20

#define PAINTOPS_OP_MASK     ((1 << PAINTOPS_OP_BITS) - 1)
#define PAINTOPS_FLAG_MASK   ((1 << PAINTOPS_FLAG_BITS) - 1)
#define PAINTOPS_DATA_MASK   ((1 << PAINTOPS_DATA_BITS) - 1)

static inline unsigned PaintOp_unpackOp(uint32_t op32) {
    return (op32 >> (PAINTOPS_FLAG_BITS + PAINTOPS_DATA_BITS));
}

static inline unsigned PaintOp_unpackFlags(uint32_t op32) {
    return (op32 >> PAINTOPS_DATA_BITS) & PAINTOPS_FLAG_MASK;
}

static inline unsigned PaintOp_unpackData(uint32_t op32) {
    return op32 & PAINTOPS_DATA_MASK;
}

static inline uint32_t PaintOp_packOp(PaintOps op) {
    SkASSERT(0 == (op & ~PAINTOPS_OP_MASK));

    return op << (PAINTOPS_FLAG_BITS + PAINTOPS_DATA_BITS);
}

static inline uint32_t PaintOp_packOpData(PaintOps op, unsigned data) {
    SkASSERT(0 == (op & ~PAINTOPS_OP_MASK));
    SkASSERT(0 == (data & ~PAINTOPS_DATA_MASK));

    return (op << (PAINTOPS_FLAG_BITS + PAINTOPS_DATA_BITS)) | data;
}

static inline uint32_t PaintOp_packOpFlagData(PaintOps op, unsigned flags, unsigned data) {
    SkASSERT(0 == (op & ~PAINTOPS_OP_MASK));
    SkASSERT(0 == (flags & ~PAINTOPS_FLAG_MASK));
    SkASSERT(0 == (data & ~PAINTOPS_DATA_MASK));

    return (op << (PAINTOPS_FLAG_BITS + PAINTOPS_DATA_BITS)) |
    (flags << PAINTOPS_DATA_BITS) |
    data;
}

#endif
