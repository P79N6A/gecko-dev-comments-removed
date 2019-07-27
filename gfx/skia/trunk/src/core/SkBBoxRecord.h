







#ifndef SkBBoxRecord_DEFINED
#define SkBBoxRecord_DEFINED

#include "SkPictureRecord.h"
#include "SkTDArray.h"






class SkBBoxRecord : public SkPictureRecord {
public:

    SkBBoxRecord(const SkISize& size, uint32_t recordFlags)
        : INHERITED(size, recordFlags) {
    }
    virtual ~SkBBoxRecord();

    



    virtual void handleBBox(const SkRect& bbox) = 0;

    virtual void drawOval(const SkRect& rect, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkRRect& rrect, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkRect& rect, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                            const SkPaint* paint = NULL) SK_OVERRIDE;
    virtual void drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                                      const SkRect& dst, const SkPaint* paint,
                                      DrawBitmapRectFlags flags) SK_OVERRIDE;
    virtual void drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& mat,
                                  const SkPaint* paint) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint) SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap& bitmap, int left, int top,
                            const SkPaint* paint) SK_OVERRIDE;
    virtual void drawVertices(VertexMode mode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xfer,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;

protected:
    virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) SK_OVERRIDE;
    virtual void onDrawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
                            const SkPaint&) SK_OVERRIDE;
    virtual void onDrawPosText(const void* text, size_t byteLength, const SkPoint pos[],
                               const SkPaint&) SK_OVERRIDE;
    virtual void onDrawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[],
                                SkScalar constY, const SkPaint&) SK_OVERRIDE;
    virtual void onDrawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                  const SkMatrix* matrix, const SkPaint&) SK_OVERRIDE;
    virtual void onDrawPicture(const SkPicture* picture) SK_OVERRIDE;
    virtual void willSave() SK_OVERRIDE;
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void willRestore() SK_OVERRIDE;

private:
    




    bool transformBounds(const SkRect& bounds, const SkPaint* paint);

    



    SkTDArray<const SkPaint*> fSaveStack;

    typedef SkPictureRecord INHERITED;
};

#endif
