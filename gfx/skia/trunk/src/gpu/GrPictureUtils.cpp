






#include "GrPictureUtils.h"
#include "SkDevice.h"
#include "SkDraw.h"
#include "SkPaintPriv.h"
#include "SkPictureData.h"
#include "SkPicturePlayback.h"

SkPicture::AccelData::Key GPUAccelData::ComputeAccelDataKey() {
    static const SkPicture::AccelData::Key gGPUID = SkPicture::AccelData::GenerateDomain();

    return gGPUID;
}









class GrGatherDevice : public SkBaseDevice {
public:
    SK_DECLARE_INST_COUNT(GrGatherDevice)

    GrGatherDevice(int width, int height, SkPicturePlayback* playback, GPUAccelData* accelData,
                   int saveLayerDepth) {
        fPlayback = playback;
        fSaveLayerDepth = saveLayerDepth;
        fInfo.fValid = true;
        fInfo.fSize.set(width, height);
        fInfo.fPaint = NULL;
        fInfo.fSaveLayerOpID = fPlayback->curOpID();
        fInfo.fRestoreOpID = 0;
        fInfo.fHasNestedLayers = false;
        fInfo.fIsNested = (2 == fSaveLayerDepth);

        fEmptyBitmap.setInfo(SkImageInfo::MakeUnknown(fInfo.fSize.fWidth, fInfo.fSize.fHeight));
        fAccelData = accelData;
        fAlreadyDrawn = false;
    }

    virtual ~GrGatherDevice() { }

    virtual SkImageInfo imageInfo() const SK_OVERRIDE {
        return fEmptyBitmap.info();
    }

#ifdef SK_SUPPORT_LEGACY_WRITEPIXELSCONFIG
    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888) SK_OVERRIDE {
        NotSupported();
    }
#endif
    virtual GrRenderTarget* accessRenderTarget() SK_OVERRIDE { return NULL; }

protected:
    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*) SK_OVERRIDE {
        return false;
    }
    virtual void clear(SkColor color) SK_OVERRIDE {
        NothingToDo();
    }
    virtual void drawPaint(const SkDraw& draw, const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawPoints(const SkDraw& draw, SkCanvas::PointMode mode, size_t count,
                            const SkPoint points[], const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawRect(const SkDraw& draw, const SkRect& rect,
                          const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawOval(const SkDraw& draw, const SkRect& rect,
                          const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawRRect(const SkDraw& draw, const SkRRect& rrect,
                           const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawPath(const SkDraw& draw, const SkPath& path,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE {
    }
    virtual void drawBitmap(const SkDraw& draw, const SkBitmap& bitmap,
                            const SkMatrix& matrix, const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawBitmapRect(const SkDraw& draw, const SkBitmap& bitmap,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE {
    }
    virtual void drawText(const SkDraw& draw, const void* text, size_t len,
                          SkScalar x, SkScalar y,
                          const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawPosText(const SkDraw& draw, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int scalarsPerPos, const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawTextOnPath(const SkDraw& draw, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawVertices(const SkDraw& draw, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE {
    }
    virtual void drawDevice(const SkDraw& draw, SkBaseDevice* deviceIn, int x, int y,
                            const SkPaint& paint) SK_OVERRIDE {
        
        GrGatherDevice* device = static_cast<GrGatherDevice*>(deviceIn);

        if (device->fAlreadyDrawn) {
            return;
        }

        device->fInfo.fRestoreOpID = fPlayback->curOpID();
        device->fInfo.fCTM = *draw.fMatrix;
        device->fInfo.fCTM.postTranslate(SkIntToScalar(-device->getOrigin().fX),
                                         SkIntToScalar(-device->getOrigin().fY));

        device->fInfo.fOffset = device->getOrigin();

        if (NeedsDeepCopy(paint)) {
            
            device->fInfo.fPaint = NULL;
            device->fInfo.fValid = false;
        } else {
            device->fInfo.fPaint = SkNEW_ARGS(SkPaint, (paint));
        }

        fAccelData->addSaveLayerInfo(device->fInfo);
        device->fAlreadyDrawn = true;
    }
    
    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE {
        return fEmptyBitmap;
    }
#ifdef SK_SUPPORT_LEGACY_READPIXELSCONFIG
    virtual bool onReadPixels(const SkBitmap& bitmap,
                              int x, int y,
                              SkCanvas::Config8888 config8888) SK_OVERRIDE {
        NotSupported();
        return false;
    }
#endif
    virtual void lockPixels() SK_OVERRIDE { NothingToDo(); }
    virtual void unlockPixels() SK_OVERRIDE { NothingToDo(); }
    virtual bool allowImageFilter(const SkImageFilter*) SK_OVERRIDE { return false; }
    virtual bool canHandleImageFilter(const SkImageFilter*) SK_OVERRIDE { return false; }
    virtual bool filterImage(const SkImageFilter*, const SkBitmap&, const SkImageFilter::Context&,
                             SkBitmap* result, SkIPoint* offset) SK_OVERRIDE {
        return false;
    }

private:
    
    SkPicturePlayback *fPlayback;

    SkBitmap fEmptyBitmap; 

    
    GPUAccelData* fAccelData;

    
    
    bool   fAlreadyDrawn;

    
    GPUAccelData::SaveLayerInfo fInfo;

    
    int fSaveLayerDepth;

    virtual void replaceBitmapBackendForRasterSurface(const SkBitmap&) SK_OVERRIDE {
        NotSupported();
    }

    virtual SkBaseDevice* onCreateDevice(const SkImageInfo& info, Usage usage) SK_OVERRIDE {
        
        SkASSERT(kSaveLayer_Usage == usage);

        fInfo.fHasNestedLayers = true;
        return SkNEW_ARGS(GrGatherDevice, (info.width(), info.height(), fPlayback,
                                           fAccelData, fSaveLayerDepth+1));
    }

    virtual void flush() SK_OVERRIDE {}

    static void NotSupported() {
        SkDEBUGFAIL("this method should never be called");
    }

    static void NothingToDo() {}

    typedef SkBaseDevice INHERITED;
};









class SK_API GrGatherCanvas : public SkCanvas {
public:
    GrGatherCanvas(GrGatherDevice* device) : INHERITED(device) {}

protected:
    
    virtual void onClipRect(const SkRect& rect, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->INHERITED::onClipRect(rect, op, kHard_ClipEdgeStyle);
    }

    
    
    virtual void onClipPath(const SkPath& path, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->updateClipConservativelyUsingBounds(path.getBounds(), op,
                                                  path.isInverseFillType());
    }
    virtual void onClipRRect(const SkRRect& rrect, SkRegion::Op op, ClipEdgeStyle) SK_OVERRIDE {
        this->updateClipConservativelyUsingBounds(rrect.getBounds(), op, false);
    }

    virtual void onDrawPicture(const SkPicture* picture) SK_OVERRIDE {
        if (NULL != picture->fData.get()) {
            
            
            
            SkPicturePlayback playback(picture);
            playback.setUseBBH(false);
            playback.draw(this, NULL);
        } else {
            
            
            picture->draw(this);
        }
    }

private:
    typedef SkCanvas INHERITED;
};



void GatherGPUInfo(const SkPicture* pict, GPUAccelData* accelData) {
    if (NULL == pict || 0 == pict->width() || 0 == pict->height()) {
        return ;
    }

    
    
    SkPicturePlayback playback(pict);
    playback.setUseBBH(false);

    GrGatherDevice device(pict->width(), pict->height(), &playback, accelData, 0);
    GrGatherCanvas canvas(&device);

    canvas.clipRect(SkRect::MakeWH(SkIntToScalar(pict->width()),
                                   SkIntToScalar(pict->height())),
                    SkRegion::kIntersect_Op, false);
    playback.draw(&canvas, NULL);
}
