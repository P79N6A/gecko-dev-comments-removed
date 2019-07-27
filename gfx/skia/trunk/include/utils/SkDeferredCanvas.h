






#ifndef SkDeferredCanvas_DEFINED
#define SkDeferredCanvas_DEFINED

#include "SkCanvas.h"
#include "SkPixelRef.h"

class SkDeferredDevice;
class SkImage;
class SkSurface;









class SK_API SkDeferredCanvas : public SkCanvas {
public:
    class SK_API NotificationClient;

    



    static SkDeferredCanvas* Create(SkSurface* surface);



    virtual ~SkDeferredCanvas();

    







    SkSurface* setSurface(SkSurface* surface);

    












    NotificationClient* setNotificationClient(NotificationClient* notificationClient);

    






    void setDeferredDrawing(bool deferred);

    


    bool isDeferredDrawing() const;

    









    bool isFreshFrame() const;

    



    bool hasPendingCommands() const;

    






    SkImage* newImageSnapshot();

    





    void setMaxRecordingStorage(size_t maxStorage);

    



    size_t storageAllocatedForRecording() const;

    






    size_t freeMemoryIfPossible(size_t bytesToFree);

    



    void setBitmapSizeThreshold(size_t sizeThreshold);
    size_t getBitmapSizeThreshold() const { return fBitmapSizeThreshold; }

    


    void silentFlush();

    
    virtual bool isDrawingToLayer() const SK_OVERRIDE;
    virtual void clear(SkColor) SK_OVERRIDE;
    virtual void drawPaint(const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(PointMode mode, size_t count, const SkPoint pts[],
                            const SkPaint& paint) SK_OVERRIDE;
    virtual void drawOval(const SkRect&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkRect& rect, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkRRect&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkPath& path, const SkPaint& paint)
                          SK_OVERRIDE;
    virtual void drawBitmap(const SkBitmap& bitmap, SkScalar left,
                            SkScalar top, const SkPaint* paint)
                            SK_OVERRIDE;
    virtual void drawBitmapRectToRect(const SkBitmap& bitmap, const SkRect* src,
                                      const SkRect& dst, const SkPaint* paint,
                                      DrawBitmapRectFlags flags) SK_OVERRIDE;

    virtual void drawBitmapMatrix(const SkBitmap& bitmap, const SkMatrix& m,
                                  const SkPaint* paint) SK_OVERRIDE;
    virtual void drawBitmapNine(const SkBitmap& bitmap, const SkIRect& center,
                                const SkRect& dst, const SkPaint* paint)
                                SK_OVERRIDE;
    virtual void drawSprite(const SkBitmap& bitmap, int left, int top,
                            const SkPaint* paint) SK_OVERRIDE;
    virtual void drawVertices(VertexMode vmode, int vertexCount,
                              const SkPoint vertices[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;
    virtual SkDrawFilter* setDrawFilter(SkDrawFilter* filter) SK_OVERRIDE;

protected:
    virtual void willSave() SK_OVERRIDE;
    virtual SaveLayerStrategy willSaveLayer(const SkRect*, const SkPaint*, SaveFlags) SK_OVERRIDE;
    virtual void willRestore() SK_OVERRIDE;

    virtual void didConcat(const SkMatrix&) SK_OVERRIDE;
    virtual void didSetMatrix(const SkMatrix&) SK_OVERRIDE;

    virtual void onDrawDRRect(const SkRRect&, const SkRRect&, const SkPaint&) SK_OVERRIDE;
    virtual void onDrawText(const void* text, size_t byteLength, SkScalar x, SkScalar y,
                            const SkPaint&) SK_OVERRIDE;
    virtual void onDrawPosText(const void* text, size_t byteLength, const SkPoint pos[],
                               const SkPaint&) SK_OVERRIDE;
    virtual void onDrawPosTextH(const void* text, size_t byteLength, const SkScalar xpos[],
                                SkScalar constY, const SkPaint&) SK_OVERRIDE;
    virtual void onDrawTextOnPath(const void* text, size_t byteLength, const SkPath& path,
                                  const SkMatrix* matrix, const SkPaint&) SK_OVERRIDE;

    virtual void onClipRect(const SkRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRRect(const SkRRect&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipPath(const SkPath&, SkRegion::Op, ClipEdgeStyle) SK_OVERRIDE;
    virtual void onClipRegion(const SkRegion&, SkRegion::Op) SK_OVERRIDE;

    virtual void onDrawPicture(const SkPicture* picture) SK_OVERRIDE;

public:
    class NotificationClient {
    public:
        virtual ~NotificationClient() {}

        



        virtual void prepareForDraw() {}

        





        virtual void storageAllocatedForRecordingChanged(
            size_t newAllocatedStorage) {}

        


        virtual void flushedDrawCommands() {}

        




        virtual void skippedPendingDrawCommands() {}
    };

protected:
    virtual SkCanvas* canvasForDrawIter();
    SkDeferredDevice* getDeferredDevice() const;

private:
    SkDeferredCanvas(SkDeferredDevice*);

    void recordedDrawCommand();
    SkCanvas* drawingCanvas() const;
    SkCanvas* immediateCanvas() const;
    bool isFullFrame(const SkRect*, const SkPaint*) const;
    void validate() const;
    void init();

    size_t fBitmapSizeThreshold;
    bool   fDeferredDrawing;

    friend class SkDeferredCanvasTester; 
    typedef SkCanvas INHERITED;
};


#endif
