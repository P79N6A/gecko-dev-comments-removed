







#ifndef GrSurface_DEFINED
#define GrSurface_DEFINED

#include "GrTypes.h"
#include "GrResource.h"

class GrTexture;
class GrRenderTarget;

class GrSurface : public GrResource {
public:
    SK_DECLARE_INST_COUNT(GrSurface);

    




    int width() const { return fDesc.fWidth; }

    




    int height() const { return fDesc.fHeight; }

    GrSurfaceOrigin origin() const {
        GrAssert(kTopLeft_GrSurfaceOrigin == fDesc.fOrigin || kBottomLeft_GrSurfaceOrigin == fDesc.fOrigin);
        return fDesc.fOrigin;
    }

    





    GrPixelConfig config() const { return fDesc.fConfig; }

    


    const GrTextureDesc& desc() const { return fDesc; }

    


    virtual GrTexture* asTexture() = 0;
    virtual const GrTexture* asTexture() const = 0;

    


    virtual GrRenderTarget* asRenderTarget() = 0;
    virtual const GrRenderTarget* asRenderTarget() const = 0;

    














    virtual bool readPixels(int left, int top, int width, int height,
                            GrPixelConfig config,
                            void* buffer,
                            size_t rowBytes = 0,
                            uint32_t pixelOpsFlags = 0) = 0;

    












    virtual void writePixels(int left, int top, int width, int height,
                             GrPixelConfig config,
                             const void* buffer,
                             size_t rowBytes = 0,
                             uint32_t pixelOpsFlags = 0) = 0;

protected:
    GrSurface(GrGpu* gpu, bool isWrapped, const GrTextureDesc& desc)
    : INHERITED(gpu, isWrapped)
    , fDesc(desc) {
    }

    GrTextureDesc fDesc;

private:
    typedef GrResource INHERITED;
};

#endif 
