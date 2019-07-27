







#ifndef GrSurface_DEFINED
#define GrSurface_DEFINED

#include "GrTypes.h"
#include "GrGpuResource.h"
#include "SkRect.h"

class GrTexture;
class GrRenderTarget;
struct SkImageInfo;

class GrSurface : public GrGpuResource {
public:
    SK_DECLARE_INST_COUNT(GrSurface);

    




    int width() const { return fDesc.fWidth; }

    




    int height() const { return fDesc.fHeight; }

    


    void getBoundsRect(SkRect* rect) const { rect->setWH(SkIntToScalar(this->width()),
                                                         SkIntToScalar(this->height())); }

    GrSurfaceOrigin origin() const {
        SkASSERT(kTopLeft_GrSurfaceOrigin == fDesc.fOrigin || kBottomLeft_GrSurfaceOrigin == fDesc.fOrigin);
        return fDesc.fOrigin;
    }

    





    GrPixelConfig config() const { return fDesc.fConfig; }

    


    const GrTextureDesc& desc() const { return fDesc; }

    SkImageInfo info() const;

    


    virtual GrTexture* asTexture() = 0;
    virtual const GrTexture* asTexture() const = 0;

    


    virtual GrRenderTarget* asRenderTarget() = 0;
    virtual const GrRenderTarget* asRenderTarget() const = 0;

    




    bool isSameAs(const GrSurface* other) const {
        const GrRenderTarget* thisRT = this->asRenderTarget();
        if (NULL != thisRT) {
            return thisRT == other->asRenderTarget();
        } else {
            const GrTexture* thisTex = this->asTexture();
            SkASSERT(NULL != thisTex); 
            return thisTex == other->asTexture();
        }
    }

    














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

    



    bool savePixels(const char* filename);

protected:
    GrSurface(GrGpu* gpu, bool isWrapped, const GrTextureDesc& desc)
    : INHERITED(gpu, isWrapped)
    , fDesc(desc) {
    }

    GrTextureDesc fDesc;

private:
    typedef GrGpuResource INHERITED;
};

#endif 
