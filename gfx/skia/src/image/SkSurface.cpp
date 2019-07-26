






#include "SkSurface_Base.h"
#include "SkImagePriv.h"
#include "SkCanvas.h"

SK_DEFINE_INST_COUNT(SkSurface)



void SkSurface_Base::installIntoCanvasForDirtyNotification() {
    if (fCachedCanvas) {
        fCachedCanvas->setSurfaceBase(this);
    }
}

SkSurface_Base::SkSurface_Base(int width, int height) : INHERITED(width, height) {
    fCachedCanvas = NULL;
    fCachedImage = NULL;
}

SkSurface_Base::~SkSurface_Base() {
    
    if (fCachedCanvas) {
        fCachedCanvas->setSurfaceBase(NULL);
    }

    SkSafeUnref(fCachedImage);
    SkSafeUnref(fCachedCanvas);
}

void SkSurface_Base::onDraw(SkCanvas* canvas, SkScalar x, SkScalar y,
                            const SkPaint* paint) {
    SkImage* image = this->newImageShapshot();
    if (image) {
        image->draw(canvas, x, y, paint);
        image->unref();
    }
}

void SkSurface_Base::onCopyOnWrite(SkImage*, SkCanvas*) {}

SkCanvas* SkSurface_Base::getCachedCanvas() {
    if (NULL == fCachedCanvas) {
        fCachedCanvas = this->onNewCanvas();
        this->installIntoCanvasForDirtyNotification();
    }
    return fCachedCanvas;
}

SkImage* SkSurface_Base::getCachedImage() {
    if (NULL == fCachedImage) {
        fCachedImage = this->onNewImageShapshot();
        this->installIntoCanvasForDirtyNotification();
    }
    return fCachedImage;
}

void SkSurface_Base::aboutToDraw(SkCanvas* canvas) {
    this->dirtyGenerationID();

    if (canvas) {
        SkASSERT(canvas == fCachedCanvas);
        SkASSERT(canvas->getSurfaceBase() == this);
        canvas->setSurfaceBase(NULL);
    }

    if (fCachedImage) {
        
        
        
        if (fCachedImage->getRefCnt() > 1) {
            this->onCopyOnWrite(fCachedImage, canvas);
        }

        
        
        fCachedImage->unref();
        fCachedImage = NULL;
    }
}

uint32_t SkSurface_Base::newGenerationID() {
    this->installIntoCanvasForDirtyNotification();

    static int32_t gID;
    return sk_atomic_inc(&gID) + 1;
}

static SkSurface_Base* asSB(SkSurface* surface) {
    return static_cast<SkSurface_Base*>(surface);
}



SkSurface::SkSurface(int width, int height) : fWidth(width), fHeight(height) {
    SkASSERT(width >= 0);
    SkASSERT(height >= 0);
    fGenerationID = 0;
}

uint32_t SkSurface::generationID() {
    if (0 == fGenerationID) {
        fGenerationID = asSB(this)->newGenerationID();
    }
    return fGenerationID;
}

void SkSurface::notifyContentChanged() {
    asSB(this)->aboutToDraw(NULL);
}

SkCanvas* SkSurface::getCanvas() {
    return asSB(this)->getCachedCanvas();
}

SkImage* SkSurface::newImageShapshot() {
    SkImage* image = asSB(this)->getCachedImage();
    SkSafeRef(image);   
    return image;
}

SkSurface* SkSurface::newSurface(const SkImage::Info& info, SkColorSpace* cs) {
    return asSB(this)->onNewSurface(info, cs);
}

void SkSurface::draw(SkCanvas* canvas, SkScalar x, SkScalar y,
                     const SkPaint* paint) {
    return asSB(this)->onDraw(canvas, x, y, paint);
}

