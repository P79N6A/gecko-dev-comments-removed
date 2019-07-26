







#ifndef SkBitmapDevice_DEFINED
#define SkBitmapDevice_DEFINED

#include "SkDevice.h"


class SK_API SkBitmapDevice : public SkBaseDevice {
public:
    SK_DECLARE_INST_COUNT(SkBitmapDevice)

    




    SkBitmapDevice(const SkBitmap& bitmap);

    




    SkBitmapDevice(const SkBitmap& bitmap, const SkDeviceProperties& deviceProperties);

#ifdef SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG
    












    SkBitmapDevice(SkBitmap::Config config, int width, int height, bool isOpaque = false);

    













    SkBitmapDevice(SkBitmap::Config config, int width, int height, bool isOpaque,
                   const SkDeviceProperties& deviceProperties);
#endif
    static SkBitmapDevice* Create(const SkImageInfo&,
                                  const SkDeviceProperties* = NULL);

    

    virtual int width() const SK_OVERRIDE { return fBitmap.width(); }
    

    virtual int height() const SK_OVERRIDE { return fBitmap.height(); }

    


    virtual bool isOpaque() const SK_OVERRIDE { return fBitmap.isOpaque(); }

    

    virtual SkBitmap::Config config() const SK_OVERRIDE { return fBitmap.config(); }

    virtual SkImageInfo imageInfo() const SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_WRITEPIXELSCONFIG
    














    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888) SK_OVERRIDE;
#endif
    


    virtual GrRenderTarget* accessRenderTarget() SK_OVERRIDE { return NULL; }

protected:
    







    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*) SK_OVERRIDE;

    


    virtual void clear(SkColor color) SK_OVERRIDE;

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawOval(const SkDraw&, const SkRect& oval,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkDraw&, const SkRRect& rr,
                           const SkPaint& paint) SK_OVERRIDE;

    










    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint,
                          const SkMatrix* prePathMatrix = NULL,
                          bool pathIsMutable = false) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkMatrix& matrix, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint) SK_OVERRIDE;

    



    virtual void drawBitmapRect(const SkDraw&, const SkBitmap&,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE;

    



    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE;
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE;
    


    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE;

    

    




    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE;

    SkPixelRef* getPixelRef() const { return fBitmap.pixelRef(); }
    
    SkPixelRef* setPixelRef(SkPixelRef* pr) {
        fBitmap.setPixelRef(pr);
        return pr;
    }

    






    virtual bool onReadPixels(const SkBitmap&, int x, int y, SkCanvas::Config8888) SK_OVERRIDE;
    virtual bool onWritePixels(const SkImageInfo&, const void*, size_t, int, int) SK_OVERRIDE;
    virtual void* onAccessPixels(SkImageInfo* info, size_t* rowBytes) SK_OVERRIDE;

    


    virtual void lockPixels() SK_OVERRIDE;
    virtual void unlockPixels() SK_OVERRIDE;

    





    virtual bool allowImageFilter(const SkImageFilter*) SK_OVERRIDE;

    






    virtual bool canHandleImageFilter(const SkImageFilter*) SK_OVERRIDE;

    






    virtual bool filterImage(const SkImageFilter*, const SkBitmap&, const SkImageFilter::Context&,
                             SkBitmap* result, SkIPoint* offset) SK_OVERRIDE;

private:
    friend class SkCanvas;
    friend struct DeviceCM; 
    friend class SkDraw;
    friend class SkDrawIter;
    friend class SkDeviceFilteredPaint;
    friend class SkDeviceImageFilterProxy;

    friend class SkSurface_Raster;

    
    
    
    virtual void replaceBitmapBackendForRasterSurface(const SkBitmap&) SK_OVERRIDE;

#ifdef SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG
    
    void init(SkBitmap::Config config, int width, int height, bool isOpaque);
#endif

    virtual SkBaseDevice* onCreateDevice(const SkImageInfo&, Usage) SK_OVERRIDE;

    

    virtual void flush() SK_OVERRIDE {}

    virtual SkSurface* newSurface(const SkImageInfo&) SK_OVERRIDE;
    virtual const void* peekPixels(SkImageInfo*, size_t* rowBytes) SK_OVERRIDE;

    SkBitmap    fBitmap;

    typedef SkBaseDevice INHERITED;
};

#endif 
