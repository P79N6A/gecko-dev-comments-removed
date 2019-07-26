






#ifndef SkPictureRecord_DEFINED
#define SkPictureRecord_DEFINED

#include "SkCanvas.h"
#include "SkFlattenable.h"
#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
#include "SkMatrixClipStateMgr.h"
#endif
#include "SkPathHeap.h"
#include "SkPicture.h"
#include "SkPictureFlat.h"
#include "SkTemplates.h"
#include "SkWriter32.h"

class SkPictureStateTree;
class SkBBoxHierarchy;



#define MASK_24 0x00FFFFFF
#define UNPACK_8_24(combined, small, large)             \
    small = (combined >> 24) & 0xFF;                    \
    large = combined & MASK_24;
#define PACK_8_24(small, large) ((small << 24) | large)


class SkPictureRecord : public SkCanvas {
public:
    SkPictureRecord(uint32_t recordFlags, SkBaseDevice*);
    virtual ~SkPictureRecord();

    virtual SkBaseDevice* setDevice(SkBaseDevice* device) SK_OVERRIDE;

    virtual int save(SaveFlags) SK_OVERRIDE;
    virtual int saveLayer(const SkRect* bounds, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void restore() SK_OVERRIDE;
    virtual bool translate(SkScalar dx, SkScalar dy) SK_OVERRIDE;
    virtual bool scale(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool rotate(SkScalar degrees) SK_OVERRIDE;
    virtual bool skew(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool concat(const SkMatrix& matrix) SK_OVERRIDE;
    virtual void setMatrix(const SkMatrix& matrix) SK_OVERRIDE;
    virtual bool clipRect(const SkRect&, SkRegion::Op, bool) SK_OVERRIDE;
    virtual bool clipRRect(const SkRRect&, SkRegion::Op, bool) SK_OVERRIDE;
    virtual bool clipPath(const SkPath&, SkRegion::Op, bool) SK_OVERRIDE;
    virtual bool clipRegion(const SkRegion& region, SkRegion::Op op) SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode, size_t count, const SkPoint pts[],
                            const SkPaint&) SK_OVERRIDE;
    virtual void drawOval(const SkRect&, const SkPaint&) SK_OVERRIDE;
    virtual void drawRect(const SkRect&, const SkPaint&) SK_OVERRIDE;
    virtual void drawRRect(const SkRRect&, const SkPaint&) SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint&) SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap&, SkScalar left, SkScalar top,
                            const SkPaint*) SK_OVERRIDE;
    virtual void drawBitmapRectToRect(const SkBitmap&, const SkRect* src,
                                      const SkRect& dst, const SkPaint* paint,
                                      DrawBitmapRectFlags flags) SK_OVERRIDE;
    virtual void drawBitmapMatrix(const SkBitmap&, const SkMatrix&,
                                  const SkPaint*) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint*) SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap&, int left, int top,
                            const SkPaint*) SK_OVERRIDE;
    virtual void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint&) SK_OVERRIDE;
    virtual void drawPosTextH(const void* text, size_t byteLength,
                      const SkScalar xpos[], SkScalar constY, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const void* text, size_t byteLength,
                            const SkPath& path, const SkMatrix* matrix,
                                const SkPaint&) SK_OVERRIDE;
    virtual void drawPicture(SkPicture& picture) SK_OVERRIDE;
    virtual void drawVertices(VertexMode, int vertexCount,
                          const SkPoint vertices[], const SkPoint texs[],
                          const SkColor colors[], SkXfermode*,
                          const uint16_t indices[], int indexCount,
                              const SkPaint&) SK_OVERRIDE;
    virtual void drawData(const void*, size_t) SK_OVERRIDE;
    virtual void beginCommentGroup(const char* description) SK_OVERRIDE;
    virtual void addComment(const char* kywd, const char* value) SK_OVERRIDE;
    virtual void endCommentGroup() SK_OVERRIDE;
    virtual bool isDrawingToLayer() const SK_OVERRIDE;

    void addFontMetricsTopBottom(const SkPaint& paint, const SkFlatData&,
                                 SkScalar minY, SkScalar maxY);

    const SkTDArray<SkPicture* >& getPictureRefs() const {
        return fPictureRefs;
    }

    void setFlags(uint32_t recordFlags) {
        fRecordFlags = recordFlags;
    }

    const SkWriter32& writeStream() const {
        return fWriter;
    }

    void beginRecording();
    void endRecording();

private:
    void handleOptimization(int opt);
    int recordRestoreOffsetPlaceholder(SkRegion::Op);
    void fillRestoreOffsetPlaceholdersForCurrentStackLevel(uint32_t restoreOffset);

#ifndef SK_COLLAPSE_MATRIX_CLIP_STATE
    SkTDArray<int32_t> fRestoreOffsetStack;
    int fFirstSavedLayerIndex;
    enum {
        kNoSavedLayerIndex = -1
    };
#endif

    











    size_t addDraw(DrawType drawType, uint32_t* size) {
        size_t offset = fWriter.bytesWritten();

        this->predrawNotify();

    #ifdef SK_DEBUG_TRACE
        SkDebugf("add %s\n", DrawTypeToString(drawType));
    #endif

        SkASSERT(0 != *size);
        SkASSERT(((uint8_t) drawType) == drawType);

        if (0 != (*size & ~MASK_24) || *size == MASK_24) {
            fWriter.writeInt(PACK_8_24(drawType, MASK_24));
            *size += 1;
            fWriter.writeInt(*size);
        } else {
            fWriter.writeInt(PACK_8_24(drawType, *size));
        }

        return offset;
    }

    void addInt(int value) {
        fWriter.writeInt(value);
    }
    void addScalar(SkScalar scalar) {
        fWriter.writeScalar(scalar);
    }

    void addBitmap(const SkBitmap& bitmap);
    void addMatrix(const SkMatrix& matrix);
    const SkFlatData* addPaint(const SkPaint& paint) { return this->addPaintPtr(&paint); }
    const SkFlatData* addPaintPtr(const SkPaint* paint);
    void addFlatPaint(const SkFlatData* flatPaint);
    void addPath(const SkPath& path);
    void addPicture(SkPicture& picture);
    void addPoint(const SkPoint& point);
    void addPoints(const SkPoint pts[], int count);
    void addRect(const SkRect& rect);
    void addRectPtr(const SkRect* rect);
    void addIRect(const SkIRect& rect);
    void addIRectPtr(const SkIRect* rect);
    void addRRect(const SkRRect&);
    void addRegion(const SkRegion& region);
    void addText(const void* text, size_t byteLength);

    int find(const SkBitmap& bitmap);

#ifdef SK_DEBUG_DUMP
public:
    void dumpMatrices();
    void dumpPaints();
#endif

#ifdef SK_DEBUG_SIZE
public:
    size_t size() const;
    int bitmaps(size_t* size) const;
    int matrices(size_t* size) const;
    int paints(size_t* size) const;
    int paths(size_t* size) const;
    int regions(size_t* size) const;
    size_t streamlen() const;

    size_t fPointBytes, fRectBytes, fTextBytes;
    int fPointWrites, fRectWrites, fTextWrites;
#endif

#ifdef SK_DEBUG_VALIDATE
public:
    void validate(size_t initialOffset, uint32_t size) const;
private:
    void validateBitmaps() const;
    void validateMatrices() const;
    void validatePaints() const;
    void validatePaths() const;
    void validateRegions() const;
#else
public:
    void validate(size_t initialOffset, uint32_t size) const {
        SkASSERT(fWriter.bytesWritten() == initialOffset + size);
    }
#endif

protected:
    virtual SkSurface* onNewSurface(const SkImageInfo&) SK_OVERRIDE;

    
    
    static void ComputeFontMetricsTopBottom(const SkPaint& paint, SkScalar topbot[2]);

    
    static void WriteTopBot(const SkPaint& paint, const SkFlatData& flat) {
        if (!flat.isTopBotWritten()) {
            ComputeFontMetricsTopBottom(paint, flat.writableTopBot());
            SkASSERT(flat.isTopBotWritten());
        }
    }
    
    const SkFlatData* getFlatPaintData(const SkPaint& paint);
    



    void drawPosTextHImpl(const void* text, size_t byteLength,
                          const SkScalar xpos[], SkScalar constY,
                          const SkPaint& paint, const SkFlatData* flatPaintData);

    int addPathToHeap(const SkPath& path);  

    
    
    
    void recordConcat(const SkMatrix& matrix);
    int recordClipRect(const SkRect& rect, SkRegion::Op op, bool doAA);
    int recordClipRRect(const SkRRect& rrect, SkRegion::Op op, bool doAA);
    int recordClipPath(int pathID, SkRegion::Op op, bool doAA);
    int recordClipRegion(const SkRegion& region, SkRegion::Op op);
    void recordSave(SaveFlags flags);
    void recordSaveLayer(const SkRect* bounds, const SkPaint* paint, SaveFlags flags);
    void recordRestore(bool fillInSkips = true);

    
    
    SkBBoxHierarchy* fBoundingHierarchy;
    SkPictureStateTree* fStateTree;

    
    SkBitmapHeap* fBitmapHeap;

private:
    friend class MatrixClipState; 
    friend class SkMatrixClipStateMgr; 

    SkChunkFlatController fFlattenableHeap;

    SkPaintDictionary fPaints;

    SkPathHeap* fPathHeap;  
    SkWriter32 fWriter;

    
    SkTDArray<SkPicture*> fPictureRefs;

    uint32_t fRecordFlags;
    int fInitialSaveCount;

    friend class SkPicturePlayback;
    friend class SkPictureTester; 

#ifdef SK_COLLAPSE_MATRIX_CLIP_STATE
    SkMatrixClipStateMgr fMCMgr;
#endif

    typedef SkCanvas INHERITED;
};

#endif
