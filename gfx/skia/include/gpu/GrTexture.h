









#ifndef GrTexture_DEFINED
#define GrTexture_DEFINED

#include "GrResource.h"

class GrRenderTarget;

class GrTexture : public GrResource {

public:
    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    



    GrFixed normalizeFixedX(GrFixed x) const { GrAssert(GrIsPow2(fWidth));
                                               return x >> fShiftFixedX; }
    GrFixed normalizeFixedY(GrFixed y) const { GrAssert(GrIsPow2(fHeight));
                                               return y >> fShiftFixedY; }

    


    GrPixelConfig config() const { return fConfig; }

    


    virtual size_t sizeInBytes() const {
        return (size_t) fWidth * fHeight * GrBytesPerPixel(fConfig);
    }

    













    bool readPixels(int left, int top, int width, int height,
                    GrPixelConfig config, void* buffer,
                    size_t rowBytes);

    










    void writePixels(int left, int top, int width, int height,
                     GrPixelConfig config, const void* buffer,
                     size_t rowBytes);

    






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
              GrPixelConfig config)
    : INHERITED(gpu)
    , fRenderTarget(NULL)
    , fWidth(width)
    , fHeight(height)
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

    
    
    int      fShiftFixedX;
    int      fShiftFixedY;

    GrPixelConfig fConfig;

    typedef GrResource INHERITED;
};

#endif

