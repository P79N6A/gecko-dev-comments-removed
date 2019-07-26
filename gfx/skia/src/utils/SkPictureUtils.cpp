






#include "SkPictureUtils.h"
#include "SkCanvas.h"
#include "SkData.h"
#include "SkDevice.h"
#include "SkPixelRef.h"
#include "SkShader.h"
#include "SkRRect.h"

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
    SkASSERT(!"this method should never be called");
}

static void nothing_to_do() {}






class GatherPixelRefDevice : public SkDevice {
private:
    PixelRefSet*  fPRSet;

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

public:
    GatherPixelRefDevice(const SkBitmap& bm, PixelRefSet* prset) : SkDevice(bm) {
        fPRSet = prset;
    }

    virtual void clear(SkColor color) SK_OVERRIDE {
        nothing_to_do();
    }
    virtual void writePixels(const SkBitmap& bitmap, int x, int y,
                             SkCanvas::Config8888 config8888) SK_OVERRIDE {
        not_supported();
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
                            const SkIRect* srcRectOrNull,
                            const SkMatrix&, const SkPaint&) SK_OVERRIDE {
        this->addBitmap(bitmap);
    }
    virtual void drawBitmapRect(const SkDraw&, const SkBitmap& bitmap,
                                const SkRect* srcOrNull, const SkRect& dst,
                                const SkPaint&) SK_OVERRIDE {
        this->addBitmap(bitmap);
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
    virtual void drawDevice(const SkDraw&, SkDevice*, int x, int y,
                            const SkPaint&) SK_OVERRIDE {
        nothing_to_do();
    }

protected:
    virtual bool onReadPixels(const SkBitmap& bitmap,
                              int x, int y,
                              SkCanvas::Config8888 config8888) SK_OVERRIDE {
        not_supported();
        return false;
    }
};

class NoSaveLayerCanvas : public SkCanvas {
public:
    NoSaveLayerCanvas(SkDevice* device) : INHERITED(device) {}

    
    virtual int saveLayer(const SkRect* bounds, const SkPaint* paint,
                          SaveFlags flags) SK_OVERRIDE {

        
        

        int count = this->INHERITED::save(flags);
        if (bounds) {
            this->INHERITED::clipRectBounds(bounds, flags, NULL);
        }
        return count;
    }

    
    virtual bool clipRect(const SkRect& rect, SkRegion::Op op,
                          bool doAA) SK_OVERRIDE {
        return this->INHERITED::clipRect(rect, op, false);
    }

    
    
    virtual bool clipPath(const SkPath& path, SkRegion::Op op,
                          bool doAA) SK_OVERRIDE {
        return this->INHERITED::clipRect(path.getBounds(), op, false);
    }
    virtual bool clipRRect(const SkRRect& rrect, SkRegion::Op op,
                           bool doAA) SK_OVERRIDE {
        return this->INHERITED::clipRect(rrect.getBounds(), op, false);
    }

private:
    typedef SkCanvas INHERITED;
};

SkData* SkPictureUtils::GatherPixelRefs(SkPicture* pict, const SkRect& area) {
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

    SkBitmap emptyBitmap;
    emptyBitmap.setConfig(SkBitmap::kARGB_8888_Config, pict->width(), pict->height());
    

    GatherPixelRefDevice device(emptyBitmap, &prset);
    NoSaveLayerCanvas canvas(&device);

    canvas.clipRect(area, SkRegion::kIntersect_Op, false);
    canvas.drawPicture(*pict);

    SkData* data = NULL;
    int count = array.count();
    if (count > 0) {
        data = SkData::NewFromMalloc(array.detach(), count * sizeof(SkPixelRef*));
    }
    return data;
}
