









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
    





    SkGpuDevice(GrContext*, SkBitmap::Config, int width, int height);

    


    SkGpuDevice(GrContext*, GrRenderTarget*);

    




    SkGpuDevice(GrContext*, GrTexture*);

    virtual ~SkGpuDevice();

    GrContext* context() const { return fContext; }

    



    virtual void gainFocus(const SkMatrix&, const SkRegion&) SK_OVERRIDE;

    virtual SkGpuRenderTarget* accessRenderTarget() SK_OVERRIDE;

    

    virtual void clear(SkColor color) SK_OVERRIDE;
    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888) SK_OVERRIDE;

    virtual void setMatrixClip(const SkMatrix& matrix, const SkRegion& clip,
                               const SkClipStack&) SK_OVERRIDE;

    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint) SK_OVERRIDE;
    virtual void drawRect(const SkDraw&, const SkRect& r,
                          const SkPaint& paint) SK_OVERRIDE;
    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE;
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkIRect* srcRectOrNull,
                            const SkMatrix&, const SkPaint&) SK_OVERRIDE;
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
    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE;
    virtual bool filterTextFlags(const SkPaint&, TextFlags*) SK_OVERRIDE;

    virtual void flush();

    virtual void onAttachToCanvas(SkCanvas* canvas) SK_OVERRIDE;
    virtual void onDetachFromCanvas() SK_OVERRIDE;

    



    virtual void makeRenderTargetCurrent();

    virtual bool canHandleImageFilter(SkImageFilter*) SK_OVERRIDE;
    virtual bool filterImage(SkImageFilter*, const SkBitmap&, const SkMatrix&,
                             SkBitmap*, SkIPoint*) SK_OVERRIDE;

    class SkAutoCachedTexture; 

protected:
    bool isBitmapInTextureCache(const SkBitmap& bitmap,
                                const GrTextureParams& params) const;

    
    virtual bool onReadPixels(const SkBitmap& bitmap,
                              int x, int y,
                              SkCanvas::Config8888 config8888) SK_OVERRIDE;

private:
    GrContext*      fContext;

    GrSkDrawProcs*  fDrawProcs;

    GrClipData      fClipData;

    
    GrRenderTarget*     fRenderTarget;
    bool                fNeedClear;
    bool                fNeedPrepareRenderTarget;

    
    void initFromRenderTarget(GrContext*, GrRenderTarget*, bool cached);

    
    SkGpuDevice(GrContext*, GrTexture* texture, bool needClear);

    
    virtual SkDevice* onCreateCompatibleDevice(SkBitmap::Config config,
                                               int width, int height,
                                               bool isOpaque,
                                               Usage usage) SK_OVERRIDE;

    SkDrawProcs* initDrawForText(GrTextContext*);
    bool bindDeviceAsTexture(GrPaint* paint);

    void prepareRenderTarget(const SkDraw&);
    bool shouldTileBitmap(const SkBitmap& bitmap,
                          const GrTextureParams& sampler,
                          const SkIRect* srcRectPtr,
                          int* tileSize) const;
    void internalDrawBitmap(const SkDraw&, const SkBitmap&,
                            const SkIRect&, const SkMatrix&, GrPaint* grPaint);

    


    GrTextContext* getTextContext();

    typedef SkDevice INHERITED;
};

#endif

