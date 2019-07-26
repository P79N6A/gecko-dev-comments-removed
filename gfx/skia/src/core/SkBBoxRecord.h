







#ifndef SkBBoxRecord_DEFINED
#define SkBBoxRecord_DEFINED

#include "SkPictureRecord.h"






class SkBBoxRecord : public SkPictureRecord {
public:

    SkBBoxRecord(uint32_t recordFlags) :INHERITED(recordFlags) { }
    virtual ~SkBBoxRecord() { }

    



    virtual void handleBBox(const SkRect& bbox) = 0;

    virtual void drawRect(const SkRect& rect, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left, SkScalar top,
                            const SkPaint* paint = NULL) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                const SkRect& dst, const SkPaint* paint) SK_OVERRIDE;
    virtual void drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& mat,
                                  const SkPaint* paint) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint) SK_OVERRIDE;
    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPosTextH(const void* text, size_t byteLength,
                              const SkScalar xpos[], SkScalar constY,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap& bitmap, int left, int top,
                            const SkPaint* paint) SK_OVERRIDE;
    virtual void drawTextOnPath(const void* text, size_t byteLength,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
    virtual void drawVertices(VertexMode mode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xfer,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPicture(SkPicture& picture) SK_OVERRIDE;

private:
    




    bool transformBounds(const SkRect& bounds, const SkPaint* paint);

    typedef SkPictureRecord INHERITED;
};

#endif

