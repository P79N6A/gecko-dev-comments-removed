






#ifndef SkDeferredCanvas_DEFINED
#define SkDeferredCanvas_DEFINED

#include "SkCanvas.h"
#include "SkPixelRef.h"

class DeferredDevice;









class SK_API SkDeferredCanvas : public SkCanvas {
public:
    class NotificationClient;

    SkDeferredCanvas();

    



    explicit SkDeferredCanvas(SkDevice* device);

    virtual ~SkDeferredCanvas();

    







    virtual SkDevice* setDevice(SkDevice* device);

    












    NotificationClient* setNotificationClient(NotificationClient* notificationClient);

    






    void setDeferredDrawing(bool deferred);

    


    bool isDeferredDrawing() const;

    









    bool isFreshFrame() const;

    





    void setMaxRecordingStorage(size_t maxStorage);

    



    size_t storageAllocatedForRecording() const;

    






    size_t freeMemoryIfPossible(size_t bytesToFree);

    
    virtual int save(SaveFlags flags) SK_OVERRIDE;
    virtual int saveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags flags) SK_OVERRIDE;
    virtual void restore() SK_OVERRIDE;
    virtual bool isDrawingToLayer() const SK_OVERRIDE;
    virtual bool translate(SkScalar dx, SkScalar dy) SK_OVERRIDE;
    virtual bool scale(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool rotate(SkScalar degrees) SK_OVERRIDE;
    virtual bool skew(SkScalar sx, SkScalar sy) SK_OVERRIDE;
    virtual bool concat(const SkMatrix& matrix) SK_OVERRIDE;
    virtual void setMatrix(const SkMatrix& matrix) SK_OVERRIDE;
    virtual bool clipRect(const SkRect& rect, SkRegion::Op op,
                          bool doAntiAlias) SK_OVERRIDE;
    virtual bool clipPath(const SkPath& path, SkRegion::Op op,
                          bool doAntiAlias) SK_OVERRIDE;
    virtual bool clipRegion(const SkRegion& deviceRgn,
                            SkRegion::Op op) SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkRect& rect, const SkPaint& paint)
                          SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint& paint)
                          SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left,
                            SkScalar top, const SkPaint* paint)
                            SK_OVERRIDE;
    virtual void drawBitmapRect(const SkBitmap& bitmap, const SkIRect* src,
                                const SkRect& dst, const SkPaint* paint)
                                SK_OVERRIDE;

    virtual void drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& m,
                                  const SkPaint* paint) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint)
                                SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap& bitmap, int left, int top,
                            const SkPaint* paint) SK_OVERRIDE;
    virtual void drawText(const void* text, size_t byteLength, SkScalar x,
                          SkScalar y, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPosText(const void* text, size_t byteLength,
                             const SkPoint pos[], const SkPaint& paint)
                             SK_OVERRIDE;
    virtual void drawPosTextH(const void* text, size_t byteLength,
                              const SkScalar xpos[], SkScalar constY,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual void drawTextOnPath(const void* text, size_t byteLength,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPicture(SkPicture& picture) SK_OVERRIDE;
    virtual void drawVertices(VertexMode vmode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual SkBounder* setBounder(SkBounder* bounder) SK_OVERRIDE;
    virtual SkDrawFilter* setDrawFilter(SkDrawFilter* filter) SK_OVERRIDE;

public:
    class NotificationClient {
    public:
        



        virtual void prepareForDraw() {}

        





        virtual void storageAllocatedForRecordingChanged(
            size_t newAllocatedStorage) {}

        


        virtual void flushedDrawCommands() {}

    private:
        typedef SkRefCnt INHERITED;
    };

protected:
    virtual SkCanvas* canvasForDrawIter();
    DeferredDevice* getDeferredDevice() const;

private:
    void recordedDrawCommand();
    SkCanvas* drawingCanvas() const;
    SkCanvas* immediateCanvas() const;
    bool isFullFrame(const SkRect*, const SkPaint*) const;
    void validate() const;
    void init();
    bool            fDeferredDrawing;

    friend class SkDeferredCanvasTester; 
    typedef SkCanvas INHERITED;
};


#endif
