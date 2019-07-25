









#ifndef SkGpuDevice_DEFINED
#define SkGpuDevice_DEFINED

#include "SkGr.h"
#include "SkBitmap.h"
#include "SkDevice.h"
#include "SkRegion.h"
#include "GrContext.h"

struct SkDrawProcs;
struct GrSkDrawProcs;
class GrTextContext;





class SK_API SkGpuDevice : public SkDevice {
public:
    






    SkGpuDevice(GrContext*, SkBitmap::Config,
                int width, int height, 
                SkDevice::Usage usage = SkDevice::kGeneral_Usage);

    


    SkGpuDevice(GrContext*, GrRenderTarget*);

    




    SkGpuDevice(GrContext*, GrTexture*);

    virtual ~SkGpuDevice();

    GrContext* context() const { return fContext; }

    



    virtual void gainFocus(SkCanvas*, const SkMatrix&, const SkRegion&,
                           const SkClipStack& clipStack);

    virtual SkGpuRenderTarget* accessRenderTarget() { 
        return (SkGpuRenderTarget*)fRenderTarget; 
    }

    

    virtual void clear(SkColor color);
    virtual bool readPixels(const SkIRect& srcRect, SkBitmap* bitmap);
    virtual void writePixels(const SkBitmap& bitmap, int x, int y);

    virtual void setMatrixClip(const SkMatrix& matrix, const SkRegion& clip,
                               const SkClipStack&);

    virtual void drawPaint(const SkDraw&, const SkPaint& paint);
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint);
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint);
    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable);
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
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint);
    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&);
    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*);

    virtual void flush() { fContext->flush(false); }

    



    virtual void makeRenderTargetCurrent();

protected:
    typedef GrContext::TextureCacheEntry TexCache;
    enum TexType {
        kBitmap_TexType,
        kDeviceRenderTarget_TexType,
        kSaveLayerDeviceRenderTarget_TexType
    };
    TexCache lockCachedTexture(const SkBitmap& bitmap,
                               const GrSamplerState& sampler,
                               TexType type = kBitmap_TexType);
    void unlockCachedTexture(TexCache);

    class SkAutoCachedTexture {
    public:
        SkAutoCachedTexture();
        SkAutoCachedTexture(SkGpuDevice* device,
                            const SkBitmap& bitmap,
                            const GrSamplerState& sampler,
                            GrTexture** texture);
        ~SkAutoCachedTexture();

        GrTexture* set(SkGpuDevice*, const SkBitmap&, const GrSamplerState&);

    private:
        SkGpuDevice*    fDevice;
        TexCache        fTex;
    };
    friend class SkAutoTexCache;

private:
    GrContext*      fContext;

    GrSkDrawProcs*  fDrawProcs;

    
    TexCache            fCache;
    GrTexture*          fTexture;
    GrRenderTarget*     fRenderTarget;
    bool                fNeedClear;
    bool                fNeedPrepareRenderTarget;

    
    void initFromRenderTarget(GrContext*, GrRenderTarget*);

    
    
    
    
    
    
    
    bool skPaint2GrPaintNoShader(const SkPaint& skPaint,
                                 bool justAlpha,
                                 GrPaint* grPaint,
                                 bool constantColor);

    
    
    
    
    
    
    bool skPaint2GrPaintShader(const SkPaint& skPaint,
                               SkAutoCachedTexture* act,
                               const SkMatrix& ctm,
                               GrPaint* grPaint,
                               bool constantColor);

    
    virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config, 
                                               int width, int height, 
                                               bool isOpaque,
                                               Usage usage);

    SkDrawProcs* initDrawForText(GrTextContext*);
    bool bindDeviceAsTexture(GrPaint* paint);

    void prepareRenderTarget(const SkDraw&);
    void internalDrawBitmap(const SkDraw&, const SkBitmap&,
                            const SkIRect&, const SkMatrix&, GrPaint* grPaint);

    typedef SkDevice INHERITED;
};

#endif

