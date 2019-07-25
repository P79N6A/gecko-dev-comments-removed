






#ifndef SkDeferredCanvas_DEFINED
#define SkDeferredCanvas_DEFINED

#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkPicture.h"
#include "SkPixelRef.h"









class SK_API SkDeferredCanvas : public SkCanvas {
public:
    class DeviceContext;

    SkDeferredCanvas();

    



    explicit SkDeferredCanvas(SkDevice* device);

    





    explicit SkDeferredCanvas(SkDevice* device, DeviceContext* deviceContext);

    virtual ~SkDeferredCanvas();

    






    virtual SkDevice* setDevice(SkDevice* device);

    











    DeviceContext* setDeviceContext(DeviceContext* deviceContext);

    






    void setDeferredDrawing(bool deferred);

    
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

private:
    void flushIfNeeded(const SkBitmap& bitmap);

public:
    class DeviceContext : public SkRefCnt {
    public:
        virtual void prepareForDraw() {}
    };

public:
    class DeferredDevice : public SkDevice {
    public:
        






        DeferredDevice(SkDevice* immediateDevice,
            DeviceContext* deviceContext = NULL);
        ~DeferredDevice();

        




        void setDeviceContext(DeviceContext* deviceContext);

        


        SkCanvas* recordingCanvas() const {return fRecordingCanvas;}

        


        SkCanvas* immediateCanvas() const {return fImmediateCanvas;}

        


        SkDevice* immediateDevice() const {return fImmediateDevice;}

        



        bool isFreshFrame();

        void flushPending();
        void contentsCleared();
        void flushIfNeeded(const SkBitmap& bitmap);

        virtual uint32_t getDeviceCapabilities() SK_OVERRIDE;
        virtual int width() const SK_OVERRIDE;
        virtual int height() const SK_OVERRIDE;
        virtual SkGpuRenderTarget* accessRenderTarget() SK_OVERRIDE;

        virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config,
                                                   int width, int height,
                                                   bool isOpaque,
                                                   Usage usage) SK_OVERRIDE;

        virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                                 SkCanvas::Config8888 config8888) SK_OVERRIDE;

    protected:
        virtual const SkBitmap& onAccessBitmap(SkBitmap*) SK_OVERRIDE;
        virtual bool onReadPixels(const SkBitmap& bitmap,
                                  int x, int y,
                                  SkCanvas::Config8888 config8888) SK_OVERRIDE;

        
        virtual bool filterTextFlags(const SkPaint& paint, TextFlags*)
            SK_OVERRIDE
            {return false;}
        virtual void setMatrixClip(const SkMatrix&, const SkRegion&,
                                   const SkClipStack&) SK_OVERRIDE
            {}
        virtual void gainFocus(SkCanvas*, const SkMatrix&, const SkRegion&,
                               const SkClipStack&) SK_OVERRIDE
            {}

        
        
        virtual void clear(SkColor color)
            {SkASSERT(0);}
        virtual void drawPaint(const SkDraw&, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode,
                                size_t count, const SkPoint[],
                                const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawRect(const SkDraw&, const SkRect& r,
                              const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawPath(const SkDraw&, const SkPath& path,
                              const SkPaint& paint,
                              const SkMatrix* prePathMatrix = NULL,
                              bool pathIsMutable = false)
            {SkASSERT(0);}
        virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                                const SkIRect* srcRectOrNull,
                                const SkMatrix& matrix, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                                int x, int y, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawText(const SkDraw&, const void* text, size_t len,
                              SkScalar x, SkScalar y, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                                 const SkScalar pos[], SkScalar constY,
                                 int scalarsPerPos, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawTextOnPath(const SkDraw&, const void* text,
                                    size_t len, const SkPath& path,
                                    const SkMatrix* matrix,
                                    const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawPosTextOnPath(const SkDraw& draw, const void* text,
                                       size_t len, const SkPoint pos[],
                                       const SkPaint& paint,
                                       const SkPath& path,
                                       const SkMatrix* matrix)
            {SkASSERT(0);}
        virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode,
                                  int vertexCount, const SkPoint verts[],
                                  const SkPoint texs[], const SkColor colors[],
                                  SkXfermode* xmode, const uint16_t indices[],
                                  int indexCount, const SkPaint& paint)
            {SkASSERT(0);}
        virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                                const SkPaint&)
            {SkASSERT(0);}
    private:
        virtual void flush();

        SkPicture fPicture;
        SkDevice* fImmediateDevice;
        SkCanvas* fImmediateCanvas;
        SkCanvas* fRecordingCanvas;
        DeviceContext* fDeviceContext;
        bool fFreshFrame;
    };

    DeferredDevice* getDeferredDevice() const;

protected:
    virtual SkCanvas* canvasForDrawIter();

private:
    SkCanvas* drawingCanvas() const;
    bool isFullFrame(const SkRect*, const SkPaint*) const;
    void validate() const;
    void init();
    bool            fDeferredDrawing;

    typedef SkCanvas INHERITED;
};


#endif
