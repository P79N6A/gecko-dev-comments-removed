








#ifndef SkDevice_DEFINED
#define SkDevice_DEFINED

#include "SkRefCnt.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColor.h"
#include "SkDeviceProperties.h"
#include "SkImageFilter.h"








class SkClipStack;
class SkDraw;
struct SkIRect;
class SkMatrix;
class SkMetaData;
class SkRegion;

class GrRenderTarget;

class SK_API SkBaseDevice : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkBaseDevice)

    


    SkBaseDevice();

    


    SkBaseDevice(const SkDeviceProperties& deviceProperties);

    virtual ~SkBaseDevice();

#ifdef SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG
    










    SkBaseDevice* createCompatibleDevice(SkBitmap::Config config,
                                         int width, int height,
                                         bool isOpaque);
#endif
    SkBaseDevice* createCompatibleDevice(const SkImageInfo&);

    SkMetaData& getMetaData();

#ifdef SK_SUPPORT_LEGACY_GETDEVICECAPABILITIES
    enum Capabilities {
        kVector_Capability = 0x1,
    };
    virtual uint32_t getDeviceCapabilities() { return 0; }
#endif

    

    virtual int width() const = 0;
    

    virtual int height() const = 0;

    
    virtual const SkDeviceProperties& getDeviceProperties() const {
        
        return fLeakyProperties;
    }

    



    virtual SkImageInfo imageInfo() const;

    




    void getGlobalBounds(SkIRect* bounds) const {
        SkASSERT(bounds);
        const SkIPoint& origin = this->getOrigin();
        bounds->setXYWH(origin.x(), origin.y(), this->width(), this->height());
    }


    


    virtual bool isOpaque() const = 0;

    

    virtual SkBitmap::Config config() const = 0;

    





    const SkBitmap& accessBitmap(bool changePixels);

#ifdef SK_SUPPORT_LEGACY_WRITEPIXELSCONFIG
    














    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888 = SkCanvas::kNative_Premul_Config8888);
#endif

    bool writePixelsDirect(const SkImageInfo&, const void*, size_t rowBytes, int x, int y);

    void* accessPixels(SkImageInfo* info, size_t* rowBytes);

    


    virtual GrRenderTarget* accessRenderTarget() = 0;


    



    const SkIPoint& getOrigin() const { return fOrigin; }

    





    virtual void onAttachToCanvas(SkCanvas*) {
        SkASSERT(!fAttachedToCanvas);
        this->lockPixels();
#ifdef SK_DEBUG
        fAttachedToCanvas = true;
#endif
    };

    





    virtual void onDetachFromCanvas() {
        SkASSERT(fAttachedToCanvas);
        this->unlockPixels();
#ifdef SK_DEBUG
        fAttachedToCanvas = false;
#endif
    };

protected:
    enum Usage {
       kGeneral_Usage,
       kSaveLayer_Usage  
    };

    struct TextFlags {
        uint32_t            fFlags;     
        SkPaint::Hinting    fHinting;
    };

    







    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*) = 0;

    















     virtual void setMatrixClip(const SkMatrix&, const SkRegion&,
                                const SkClipStack&) {};

    


    virtual void clear(SkColor color) = 0;

    SK_ATTR_DEPRECATED("use clear() instead")
    void eraseColor(SkColor eraseColor) { this->clear(eraseColor); }

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint) = 0;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint) = 0;
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint) = 0;
    virtual void drawOval(const SkDraw&, const SkRect& oval,
                          const SkPaint& paint) = 0;
    virtual void drawRRect(const SkDraw&, const SkRRect& rr,
                           const SkPaint& paint) = 0;

    
    virtual void drawDRRect(const SkDraw&, const SkRRect& outer,
                            const SkRRect& inner, const SkPaint&);

    










    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint,
                          const SkMatrix* prePathMatrix = NULL,
                          bool pathIsMutable = false) = 0;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkMatrix& matrix, const SkPaint& paint) = 0;
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint) = 0;

    



    virtual void drawBitmapRect(const SkDraw&, const SkBitmap&,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) = 0;

    



    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint& paint) = 0;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint& paint) = 0;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) = 0;
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) = 0;
    


    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y,
                            const SkPaint&) = 0;

    


























    bool readPixels(SkBitmap* bitmap,
                    int x, int y,
                    SkCanvas::Config8888 config8888);

    

    



    virtual const SkBitmap& onAccessBitmap() = 0;

    


    virtual void lockPixels() = 0;
    virtual void unlockPixels() = 0;

    





    virtual bool allowImageFilter(const SkImageFilter*) = 0;

    






    virtual bool canHandleImageFilter(const SkImageFilter*) = 0;

    






    virtual bool filterImage(const SkImageFilter*, const SkBitmap&,
                             const SkImageFilter::Context& ctx,
                             SkBitmap* result, SkIPoint* offset) = 0;

    
    
    static const SkCanvas::Config8888 kPMColorAlias;

protected:
    
    virtual SkSurface* newSurface(const SkImageInfo&);

    
    virtual const void* peekPixels(SkImageInfo*, size_t* rowBytes);

    






    virtual bool onReadPixels(const SkBitmap& bitmap,
                              int x, int y,
                              SkCanvas::Config8888 config8888);

    





    virtual bool onWritePixels(const SkImageInfo&, const void*, size_t, int x, int y);

    


    virtual void* onAccessPixels(SkImageInfo* info, size_t* rowBytes);

    





    SkDeviceProperties fLeakyProperties;

    



    virtual void EXPERIMENTAL_optimize(SkPicture* picture);

    









    virtual bool EXPERIMENTAL_drawPicture(const SkPicture& picture);

private:
    friend class SkCanvas;
    friend struct DeviceCM; 
    friend class SkDraw;
    friend class SkDrawIter;
    friend class SkDeviceFilteredPaint;
    friend class SkDeviceImageFilterProxy;
    friend class SkDeferredDevice;    

    friend class SkSurface_Raster;

    
    
    
    
    virtual void replaceBitmapBackendForRasterSurface(const SkBitmap&) = 0;

    
    void setOrigin(int x, int y) { fOrigin.set(x, y); }
    
    SkBaseDevice* createCompatibleDeviceForSaveLayer(const SkImageInfo&);

#ifdef SK_SUPPORT_LEGACY_COMPATIBLEDEVICE_CONFIG
    




    virtual SkBaseDevice* onCreateCompatibleDevice(SkBitmap::Config config,
                                                   int width, int height,
                                                   bool isOpaque, Usage) {
        return NULL;
    }
#endif
    virtual SkBaseDevice* onCreateDevice(const SkImageInfo&, Usage) {
        return NULL;
    }

    

    virtual void flush() = 0;

    SkIPoint    fOrigin;
    SkMetaData* fMetaData;

#ifdef SK_DEBUG
    bool        fAttachedToCanvas;
#endif

    typedef SkRefCnt INHERITED;
};

#endif
