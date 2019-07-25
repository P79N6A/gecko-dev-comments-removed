









#ifndef GrTexture_DEFINED
#define GrTexture_DEFINED

#include "GrResource.h"

class GrRenderTarget;

class GrTexture : public GrResource {

public:
    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    




    int allocatedWidth() const { return fAllocatedWidth; }

    




    int allocatedHeight() const { return fAllocatedHeight; }

    



    GrFixed normalizeFixedX(GrFixed x) const { GrAssert(GrIsPow2(fWidth));
                                               return x >> fShiftFixedX; }
    GrFixed normalizeFixedY(GrFixed y) const { GrAssert(GrIsPow2(fHeight));
                                               return y >> fShiftFixedY; }

    


    GrPixelConfig config() const { return fConfig; }

    


    virtual size_t sizeInBytes() const {
        return (size_t) fAllocatedWidth *
                        fAllocatedHeight *
                        GrBytesPerPixel(fConfig);
    }

    











    virtual void uploadTextureData(int x,
                                   int y,
                                   int width,
                                   int height,
                                   const void* srcData,
                                   size_t rowBytes) = 0;

    











    bool readPixels(int left, int top, int width, int height,
                    GrPixelConfig config, void* buffer);

    






    GrRenderTarget* asRenderTarget() { return fRenderTarget; }

    




    void releaseRenderTarget();

    



    virtual intptr_t getTextureHandle() const = 0;

#if GR_DEBUG
    void validate() const {
        this->INHERITED::validate();
    }
#else
    void validate() const {}
#endif

protected:
    GrRenderTarget* fRenderTarget; 
                                   
                                   

    GrTexture(GrGpu* gpu,
              int width,
              int height,
              int allocatedWidth,
              int allocatedHeight,
              GrPixelConfig config)
    : INHERITED(gpu)
    , fRenderTarget(NULL)
    , fWidth(width)
    , fHeight(height)
    , fAllocatedWidth(allocatedWidth)
    , fAllocatedHeight(allocatedHeight)
    , fConfig(config) {
        
        fShiftFixedX = 31 - Gr_clz(fWidth);
        fShiftFixedY = 31 - Gr_clz(fHeight);
    }

    
    virtual void onRelease() {
        this->releaseRenderTarget();
    }

    virtual void onAbandon();

private:
    int fWidth;
    int fHeight;
    int fAllocatedWidth;
    int fAllocatedHeight;

    
    
    int      fShiftFixedX;
    int      fShiftFixedY;

    GrPixelConfig fConfig;

    typedef GrResource INHERITED;
};

#endif

