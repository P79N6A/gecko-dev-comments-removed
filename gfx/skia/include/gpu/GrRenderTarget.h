
















#ifndef GrRenderTarget_DEFINED
#define GrRenderTarget_DEFINED

#include "GrRect.h"
#include "GrResource.h"

class GrStencilBuffer;
class GrTexture;








class GrRenderTarget : public GrResource {
public:

    


    int width() const { return fWidth; }
    


    int height() const { return fHeight; }

    




    GrPixelConfig config() const { return fConfig; }

    


    GrTexture* asTexture() {return fTexture;}

    



    virtual intptr_t getRenderTargetHandle() const = 0;

    





    virtual intptr_t getRenderTargetResolvedHandle() const = 0;

    


    bool isMultisampled() const { return 0 != fSampleCnt; }

    


    int numSamples() const { return fSampleCnt; }

    









    void flagAsNeedingResolve(const GrIRect* rect = NULL);

    


    void overrideResolveRect(const GrIRect rect);

    



    void flagAsResolved() { fResolveRect.setLargestInverted(); }

    


    bool needsResolve() const { return !fResolveRect.isEmpty(); }

    


    const GrIRect& getResolveRect() const { return fResolveRect; }

    
    virtual size_t sizeInBytes() const;

    













    bool readPixels(int left, int top, int width, int height,
                    GrPixelConfig config, void* buffer, size_t rowBytes);

    











    void writePixels(int left, int top, int width, int height,
                     GrPixelConfig config, const void* buffer, size_t rowBytes);

    
    
    
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
                   int width,
                   int height,
                   GrPixelConfig config,
                   int sampleCnt)
        : INHERITED(gpu)
        , fStencilBuffer(NULL)
        , fTexture(texture)
        , fWidth(width)
        , fHeight(height)
        , fConfig(config)
        , fSampleCnt(sampleCnt) {
        fResolveRect.setLargestInverted();
    }

    friend class GrTexture;
    
    
    
    
    
    void onTextureReleaseRenderTarget() {
        GrAssert(NULL != fTexture);
        fTexture = NULL;
    }

private:
    GrStencilBuffer*  fStencilBuffer;
    GrTexture*        fTexture; 
    int               fWidth;
    int               fHeight;
    GrPixelConfig     fConfig;
    int               fSampleCnt;
    GrIRect           fResolveRect;

    typedef GrResource INHERITED;
};

#endif
