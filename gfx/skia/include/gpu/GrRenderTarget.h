
















#ifndef GrRenderTarget_DEFINED
#define GrRenderTarget_DEFINED

#include "GrRect.h"
#include "GrSurface.h"

class GrStencilBuffer;
class GrTexture;








class GrRenderTarget : public GrSurface {
public:
    SK_DECLARE_INST_COUNT(GrRenderTarget)

    
    virtual size_t sizeInBytes() const SK_OVERRIDE;

    
    


    virtual GrTexture* asTexture() SK_OVERRIDE { return fTexture; }
    virtual const GrTexture* asTexture() const SK_OVERRIDE { return fTexture; }

    


    virtual GrRenderTarget* asRenderTarget() SK_OVERRIDE { return this; }
    virtual const GrRenderTarget* asRenderTarget() const  SK_OVERRIDE {
        return this;
    }

    virtual bool readPixels(int left, int top, int width, int height,
                            GrPixelConfig config,
                            void* buffer,
                            size_t rowBytes = 0,
                            uint32_t pixelOpsFlags = 0) SK_OVERRIDE;

    virtual void writePixels(int left, int top, int width, int height,
                             GrPixelConfig config,
                             const void* buffer,
                             size_t rowBytes = 0,
                             uint32_t pixelOpsFlags = 0) SK_OVERRIDE;

    
    



    virtual intptr_t getRenderTargetHandle() const = 0;

    





    virtual intptr_t getRenderTargetResolvedHandle() const = 0;

    


    bool isMultisampled() const { return 0 != fDesc.fSampleCnt; }

    


    int numSamples() const { return fDesc.fSampleCnt; }

    









    void flagAsNeedingResolve(const GrIRect* rect = NULL);

    


    void overrideResolveRect(const GrIRect rect);

    



    void flagAsResolved() { fResolveRect.setLargestInverted(); }

    


    bool needsResolve() const { return !fResolveRect.isEmpty(); }

    


    const GrIRect& getResolveRect() const { return fResolveRect; }

    





    void resolve();

    
    
    
    enum ResolveType {
        kCanResolve_ResolveType,
        kAutoResolves_ResolveType,
        kCantResolve_ResolveType,
    };
    virtual ResolveType getResolveType() const = 0;

    


    GrStencilBuffer* getStencilBuffer() const { return fStencilBuffer; }
    void setStencilBuffer(GrStencilBuffer* stencilBuffer);

protected:
    GrRenderTarget(GrGpu* gpu,
                   GrTexture* texture,
                   const GrTextureDesc& desc)
        : INHERITED(gpu, desc)
        , fStencilBuffer(NULL)
        , fTexture(texture) {
        fResolveRect.setLargestInverted();
    }

    friend class GrTexture;
    
    
    
    
    
    void onTextureReleaseRenderTarget() {
        GrAssert(NULL != fTexture);
        fTexture = NULL;
    }

    
    virtual void onAbandon() SK_OVERRIDE;
    virtual void onRelease() SK_OVERRIDE;

private:
    GrStencilBuffer*  fStencilBuffer;
    GrTexture*        fTexture; 

    GrIRect           fResolveRect;

    typedef GrSurface INHERITED;
};

#endif
