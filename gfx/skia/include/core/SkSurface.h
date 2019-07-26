






#ifndef SkSurface_DEFINED
#define SkSurface_DEFINED

#include "SkRefCnt.h"
#include "SkImage.h"

class SkCanvas;
class SkPaint;
class GrContext;
class GrRenderTarget;









class SkSurface : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkSurface)

    






    static SkSurface* NewRasterDirect(const SkImage::Info&, SkColorSpace*,
                                      void* pixels, size_t rowBytes);

    






    static SkSurface* NewRaster(const SkImage::Info&, SkColorSpace*);

    




    static SkSurface* NewPicture(int width, int height);

    


    static SkSurface* NewRenderTargetDirect(GrContext*, GrRenderTarget*);

    



    static SkSurface* NewRenderTarget(GrContext*, const SkImage::Info&,
                                      SkColorSpace*, int sampleCount = 0);

    int width() const { return fWidth; }
    int height() const { return fHeight; }

    








    uint32_t generationID();

    



    void notifyContentChanged();

    





    SkCanvas* getCanvas();

    












    SkSurface* newSurface(const SkImage::Info&, SkColorSpace*);

    




    SkImage* newImageShapshot();

    






    void draw(SkCanvas*, SkScalar x, SkScalar y, const SkPaint*);

protected:
    SkSurface(int width, int height);

    
    void dirtyGenerationID() {
        fGenerationID = 0;
    }

private:
    const int   fWidth;
    const int   fHeight;
    uint32_t    fGenerationID;

    typedef SkRefCnt INHERITED;
};

#endif
