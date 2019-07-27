









#ifndef SkGpuDevice_DEFINED
#define SkGpuDevice_DEFINED

#include "SkGr.h"
#include "SkBitmap.h"
#include "SkDevice.h"
#include "SkPicture.h"
#include "SkRegion.h"
#include "GrContext.h"

struct SkDrawProcs;
struct GrSkDrawProcs;

class GrTextContext;





class SK_API SkGpuDevice : public SkBaseDevice {
public:
    enum Flags {
        kNeedClear_Flag = 1 << 0,  
        kCached_Flag    = 1 << 1,  
        kDFFonts_Flag   = 1 << 2,  
    };

    





    static SkGpuDevice* Create(GrSurface* surface, unsigned flags = 0);

    





    static SkGpuDevice* Create(GrContext*, const SkImageInfo&, int sampleCount);

    



    SkGpuDevice(GrContext*, GrRenderTarget*, unsigned flags = 0);

    





    SkGpuDevice(GrContext*, GrTexture*, unsigned flags = 0);

    virtual ~SkGpuDevice();

    GrContext* context() const { return fContext; }

    virtual GrRenderTarget* accessRenderTarget() SK_OVERRIDE;

    virtual SkImageInfo imageInfo() const SK_OVERRIDE {
        return fRenderTarget ? fRenderTarget->info() : SkImageInfo::MakeUnknown();
    }

    virtual void clear(SkColor color) SK_OVERRIDE;
    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRRect(const SkDraw&, const SkRRect& r,
                           const SkPaint& paint) SK_OVERRIDE;
    virtual void drawDRRect(const SkDraw& draw, const SkRRect& outer,
                            const SkRRect& inner, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawOval(const SkDraw&, const SkRect& oval,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkMatrix&, const SkPaint&) SK_OVERRIDE;
    virtual void drawBitmapRect(const SkDraw&, const SkBitmap&,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE;
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint);
    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y, const SkPaint&) SK_OVERRIDE;
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint&) SK_OVERRIDE;
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint&) SK_OVERRIDE;
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint&) SK_OVERRIDE;
    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE;
    virtual bool filterTextFlags(const SkPaint&, TextFlags*) SK_OVERRIDE;

    virtual void flush() SK_OVERRIDE;

    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;

    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE;

    



    virtual void makeRenderTargetCurrent();

    virtual bool canHandleImageFilter(const SkImageFilter*) SK_OVERRIDE;
    virtual bool filterImage(const SkImageFilter*, const SkBitmap&,
                             const SkImageFilter::Context&,
                             SkBitmap*, SkIPoint*) SK_OVERRIDE;

    class SkAutoCachedTexture; 


protected:
    virtual bool onReadPixels(const SkImageInfo&, void*, size_t, int, int) SK_OVERRIDE;
    virtual bool onWritePixels(const SkImageInfo&, const void*, size_t, int, int) SK_OVERRIDE;

    
    virtual void EXPERIMENTAL_optimize(const SkPicture* picture) SK_OVERRIDE;
    
    virtual bool EXPERIMENTAL_drawPicture(SkCanvas* canvas, const SkPicture* picture) SK_OVERRIDE;

private:
    GrContext*      fContext;

    GrSkDrawProcs*  fDrawProcs;

    GrClipData      fClipData;

    GrTextContext*  fMainTextContext;
    GrTextContext*  fFallbackTextContext;

    
    GrRenderTarget*     fRenderTarget;
    bool                fNeedClear;

    
    SkBitmap fLegacyBitmap;

    
    void initFromRenderTarget(GrContext*, GrRenderTarget*, unsigned flags);

    virtual SkBaseDevice* onCreateDevice(const SkImageInfo&, Usage) SK_OVERRIDE;

    virtual SkSurface* newSurface(const SkImageInfo&) SK_OVERRIDE;

    
    
    void prepareDraw(const SkDraw&, bool forceIdentity);

    


    void drawBitmapCommon(const SkDraw&,
                          const SkBitmap& bitmap,
                          const SkRect* srcRectPtr,
                          const SkSize* dstSizePtr,      
                          const SkPaint&,
                          SkCanvas::DrawBitmapRectFlags flags);

    




    
    bool shouldTileBitmap(const SkBitmap& bitmap,
                          const GrTextureParams& sampler,
                          const SkRect* srcRectPtr,
                          int maxTileSize,
                          int* tileSize,
                          SkIRect* clippedSrcRect) const;
    void internalDrawBitmap(const SkBitmap&,
                            const SkRect&,
                            const GrTextureParams& params,
                            const SkPaint& paint,
                            SkCanvas::DrawBitmapRectFlags flags,
                            bool bicubic,
                            bool needsTextureDomain);
    void drawTiledBitmap(const SkBitmap& bitmap,
                         const SkRect& srcRect,
                         const SkIRect& clippedSrcRect,
                         const GrTextureParams& params,
                         const SkPaint& paint,
                         SkCanvas::DrawBitmapRectFlags flags,
                         int tileSize,
                         bool bicubic);

    bool drawDashLine(const SkPoint pts[2], const SkPaint& paint);

    static SkPicture::AccelData::Key ComputeAccelDataKey();

    typedef SkBaseDevice INHERITED;
};

#endif
