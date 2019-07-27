






#ifndef SkSurface_Base_DEFINED
#define SkSurface_Base_DEFINED

#include "SkSurface.h"
#include "SkCanvas.h"

class SkSurface_Base : public SkSurface {
public:
    SkSurface_Base(int width, int height);
    explicit SkSurface_Base(const SkImageInfo&);
    virtual ~SkSurface_Base();

    





    virtual SkCanvas* onNewCanvas() = 0;

    virtual SkSurface* onNewSurface(const SkImageInfo&) = 0;

    





    virtual SkImage* onNewImageSnapshot() = 0;

    








    virtual void onDraw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

    



    virtual void onDiscard() {}

    




    virtual void onCopyOnWrite(ContentChangeMode) = 0;

    inline SkCanvas* getCachedCanvas();
    inline SkImage* getCachedImage();

    
    uint32_t newGenerationID();

private:
    SkCanvas*   fCachedCanvas;
    SkImage*    fCachedImage;

    void aboutToDraw(ContentChangeMode mode);
    friend class SkCanvas;
    friend class SkSurface;

    typedef SkSurface INHERITED;
};

SkCanvas* SkSurface_Base::getCachedCanvas() {
    if (NULL == fCachedCanvas) {
        fCachedCanvas = this->onNewCanvas();
        if (NULL != fCachedCanvas) {
            fCachedCanvas->setSurfaceBase(this);
        }
    }
    return fCachedCanvas;
}

SkImage* SkSurface_Base::getCachedImage() {
    if (NULL == fCachedImage) {
        fCachedImage = this->onNewImageSnapshot();
        SkASSERT(!fCachedCanvas || fCachedCanvas->getSurfaceBase() == this);
    }
    return fCachedImage;
}

#endif
