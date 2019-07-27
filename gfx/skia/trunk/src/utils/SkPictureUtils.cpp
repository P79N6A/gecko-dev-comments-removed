






#include "SkBitmapDevice.h"
#include "SkCanvas.h"
#include "SkData.h"
#include "SkNoSaveLayerCanvas.h"
#include "SkPictureUtils.h"
#include "SkPixelRef.h"
#include "SkRRect.h"
#include "SkShader.h"

class PixelRefSet {
public:
    PixelRefSet(SkTDArray<SkPixelRef*>* array) : fArray(array) {}

    
    
    
    void add(SkPixelRef* pr) {
        uint32_t genID = pr->getGenerationID();
        if (fGenID.find(genID) < 0) {
            *fArray->append() = pr;
            *fGenID.append() = genID;

        } else {

        }
    }

private:
    SkTDArray<SkPixelRef*>* fArray;
    SkTDArray<uint32_t>     fGenID;
};

static void not_supported() {
    SkDEBUGFAIL("this method should never be called");
}

static void nothing_to_do() {}






class GatherPixelRefDevice : public SkBaseDevice {
public:
    SK_DECLARE_INST_COUNT(GatherPixelRefDevice)

    GatherPixelRefDevice(int width, int height, PixelRefSet* prset) {
        fSize.set(width, height);
        fEmptyBitmap.setInfo(SkImageInfo::MakeUnknown(width, height));
        fPRSet = prset;
    }

    virtual SkImageInfo imageInfo() const SK_OVERRIDE {
        return SkImageInfo::MakeUnknown(fSize.width(), fSize.height());
    }
    virtual GrRenderTarget* accessRenderTarget() SK_OVERRIDE { return NULL; }
    virtual bool filterTextFlags(const SkPaint& paint, TextFlags*) SK_OVERRIDE {
        return false;
    }
    
    virtual const SkBitmap& onAccessBitmap() SK_OVERRIDE {
        return fEmptyBitmap;
    }
    virtual void lockPixels() SK_OVERRIDE { nothing_to_do(); }
    virtual void unlockPixels() SK_OVERRIDE { nothing_to_do(); }
    virtual bool allowImageFilter(const SkImageFilter*) SK_OVERRIDE { return false; }
    virtual bool canHandleImageFilter(const SkImageFilter*) SK_OVERRIDE { return false; }
    virtual bool filterImage(const SkImageFilter*, const SkBitmap&, const SkImageFilter::Context&,
                             SkBitmap* result, SkIPoint* offset) SK_OVERRIDE {
        return false;
    }

    virtual void clear(SkColor color) SK_OVERRIDE {
        nothing_to_do();
    }

    virtual void drawPaint(const SkDraw&, const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawPoints(const SkDraw&, SkCanvas::PointMode mode, size_t count,
                            const SkPoint[], const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawRect(const SkDraw&, const SkRect&,
                          const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawRRect(const SkDraw&, const SkRRect&,
                           const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawOval(const SkDraw&, const SkRect&,
                          const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawPath(const SkDraw&, const SkPath& path,
                          const SkPaint& paint, const SkMatrix* prePathMatrix,
                          bool pathIsMutable) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawBitmap(const SkDraw&, const SkBitmap& bitmap,
                            const SkMatrix&, const SkPaint& paint) SK_OVERRIDE {
        this->addBitmap(bitmap);
        if (kAlpha_8_SkColorType == bitmap.colorType()) {
            this->addBitmapFromPaint(paint);
        }
    }
    virtual void drawBitmapRect(const SkDraw&, const SkBitmap& bitmap,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint& paint,
                                SkCanvas::DrawBitmapRectFlags flags) SK_OVERRIDE {
        this->addBitmap(bitmap);
        if (kAlpha_8_SkColorType == bitmap.colorType()) {
            this->addBitmapFromPaint(paint);
        }
    }
    virtual void drawSprite(const SkDraw&, const SkBitmap& bitmap,
                            int x, int y, const SkPaint& paint) SK_OVERRIDE {
        this->addBitmap(bitmap);
    }
    virtual void drawText(const SkDraw&, const void* text, size_t len,
                          SkScalar x, SkScalar y,
                          const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawPosText(const SkDraw&, const void* text, size_t len,
                             const SkScalar pos[], SkScalar constY,
                             int, const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawTextOnPath(const SkDraw&, const void* text, size_t len,
                                const SkPath& path, const SkMatrix* matrix,
                                const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawVertices(const SkDraw&, SkCanvas::VertexMode, int vertexCount,
                              const SkPoint verts[], const SkPoint texs[],
                              const SkColor colors[], SkXfermode* xmode,
                              const uint16_t indices[], int indexCount,
                              const SkPaint& paint) SK_OVERRIDE {
        this->addBitmapFromPaint(paint);
    }
    virtual void drawDevice(const SkDraw&, SkBaseDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE {
        nothing_to_do();
    }

protected:
    virtual void replaceBitmapBackendForRasterSurface(const SkBitmap&) SK_OVERRIDE {
        not_supported();
    }
    virtual SkBaseDevice* onCreateDevice(const SkImageInfo& info, Usage usage) SK_OVERRIDE {
        
        SkASSERT(kSaveLayer_Usage == usage);
        return SkNEW_ARGS(GatherPixelRefDevice, (info.width(), info.height(), fPRSet));
    }
    virtual void flush() SK_OVERRIDE {}

private:
    PixelRefSet*  fPRSet;
    SkBitmap fEmptyBitmap;  
    SkISize fSize;

    void addBitmap(const SkBitmap& bm) {
      fPRSet->add(bm.pixelRef());
    }

    void addBitmapFromPaint(const SkPaint& paint) {
      SkShader* shader = paint.getShader();
      if (shader) {
          SkBitmap bm;
          
          
          
          if (SkShader::kNone_GradientType == shader->asAGradient(NULL) &&
              shader->asABitmap(&bm, NULL, NULL)) {
              fPRSet->add(bm.pixelRef());
          }
      }
    }

    typedef SkBaseDevice INHERITED;
};

SkData* SkPictureUtils::GatherPixelRefs(const SkPicture* pict, const SkRect& area) {
    if (NULL == pict) {
        return NULL;
    }

    
    if (!SkRect::Intersects(area,
                            SkRect::MakeWH(SkIntToScalar(pict->width()),
                                           SkIntToScalar(pict->height())))) {
        return NULL;
    }

    SkTDArray<SkPixelRef*> array;
    PixelRefSet prset(&array);

    GatherPixelRefDevice device(pict->width(), pict->height(), &prset);
    SkNoSaveLayerCanvas canvas(&device);

    canvas.clipRect(area, SkRegion::kIntersect_Op, false);
    canvas.drawPicture(pict);

    SkData* data = NULL;
    int count = array.count();
    if (count > 0) {
        data = SkData::NewFromMalloc(array.detach(), count * sizeof(SkPixelRef*));
    }
    return data;
}
