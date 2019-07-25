








#ifndef SkDevice_DEFINED
#define SkDevice_DEFINED

#include "SkRefCnt.h"
#include "SkBitmap.h"
#include "SkCanvas.h"
#include "SkColor.h"

class SkClipStack;
class SkDraw;
struct SkIRect;
class SkMatrix;
class SkMetaData;
class SkRegion;


class SkGpuRenderTarget;

class SK_API SkDevice : public SkRefCnt {
public:
    




    SkDevice(const SkBitmap& bitmap);

    












    SkDevice(SkBitmap::Config config, int width, int height, bool isOpaque = false);

    virtual ~SkDevice();

    










    SkDevice* createCompatibleDevice(SkBitmap::Config config, 
                                     int width, int height,
                                     bool isOpaque);

    SkMetaData& getMetaData();

    enum Capabilities {
        kGL_Capability     = 0x1,  
        kVector_Capability = 0x2,  
        kAll_Capabilities  = 0x3
    };
    virtual uint32_t getDeviceCapabilities() { return 0; }

    

    virtual int width() const { return fBitmap.width(); }
    

    virtual int height() const { return fBitmap.height(); }

    




    void getGlobalBounds(SkIRect* bounds) const;

    


    bool isOpaque() const { return fBitmap.isOpaque(); }

    

    SkBitmap::Config config() const { return fBitmap.getConfig(); }

    





    const SkBitmap& accessBitmap(bool changePixels);

    





    virtual bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap);

    




    virtual void writePixels(const SkBitmap& bitmap, int x, int y);

    


    virtual SkGpuRenderTarget* accessRenderTarget() { return NULL; }

protected:
    enum Usage {
       kGeneral_Usage,
       kSaveLayer_Usage, 
    };

    



    const SkIPoint& getOrigin() const { return fOrigin; }

    struct TextFlags {
        uint32_t            fFlags;     
        SkPaint::Hinting    fHinting;
    };

    







    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*);

    











    virtual void setMatrixClip(const SkMatrix&, const SkRegion&,
                               const SkClipStack&);

    


    virtual void gainFocus(SkCanvas*, const SkMatrix&, const SkRegion&,
                           const SkClipStack&) {}

    


    virtual void clear(SkColor color);

    


    void eraseColor(SkColor eraseColor) { this->clear(eraseColor); }

    




    virtual void drawPaint(const SkDraw&, const SkPaint& paint);
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint);
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint);
    










    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint,
                          const SkMatrix* prePathMatrix = NULL,
                          bool pathIsMutable = false);
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkIRect* srcRectOrNull,
                            const SkMatrix& matrix, const SkPaint& paint);
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint);
    



    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint& paint);
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint& paint);
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint);
#ifdef ANDROID
    virtual void drawPosTextOnPath(const SkDraw& draw, const void* text, size_t len,
                                   const SkPoint pos[], const SkPaint& paint,
                                   const SkPath& path, const SkMatrix* matrix);
#endif
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint);
    


    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&);

    

    



    virtual void onAccessBitmap(SkBitmap*);

    SkPixelRef* getPixelRef() const { return fBitmap.pixelRef(); }
    
    SkPixelRef* setPixelRef(SkPixelRef* pr, size_t offset) {
        fBitmap.setPixelRef(pr, offset);
        return pr;
    }

    


    virtual void lockPixels();
    virtual void unlockPixels();

private:
    friend class SkCanvas;
    friend struct DeviceCM; 
    friend class SkDraw;
    friend class SkDrawIter;
    friend class SkDeviceFilteredPaint;

    
    void setOrigin(int x, int y) { fOrigin.set(x, y); }
    
    SkDevice* createCompatibleDeviceForSaveLayer(SkBitmap::Config config, 
                                                 int width, int height,
                                                 bool isOpaque);

    


    virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config, 
                                               int width, int height, 
                                               bool isOpaque,
                                               Usage usage);

    

    virtual void flush() {}

    SkBitmap    fBitmap;
    SkIPoint    fOrigin;
    SkMetaData* fMetaData;
};

#endif
