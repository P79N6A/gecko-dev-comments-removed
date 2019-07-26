






#ifndef SkSurface_Base_DEFINED
#define SkSurface_Base_DEFINED

#include "SkSurface.h"

class SkSurface_Base : public SkSurface {
public:
    SkSurface_Base(int width, int height);
    virtual ~SkSurface_Base();

    





    virtual SkCanvas* onNewCanvas() = 0;

    virtual SkSurface* onNewSurface(const SkImage::Info&) = 0;

    





    virtual SkImage* onNewImageShapshot() = 0;

    








    virtual void onDraw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

    






    virtual void onCopyOnWrite(SkImage* cachedImage, SkCanvas*) = 0;

    inline SkCanvas* getCachedCanvas();
    inline SkImage* getCachedImage();

    
    uint32_t newGenerationID();

private:
    SkCanvas*   fCachedCanvas;
    SkImage*    fCachedImage;

    void aboutToDraw(SkCanvas*);
    friend class SkCanvas;
    friend class SkSurface;

    inline void installIntoCanvasForDirtyNotification();

    typedef SkSurface INHERITED;
};

#endif
