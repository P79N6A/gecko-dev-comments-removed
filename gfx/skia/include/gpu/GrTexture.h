







#ifndef GrTexture_DEFINED
#define GrTexture_DEFINED

#include "GrSurface.h"
#include "GrCacheID.h"

class GrRenderTarget;
class GrResourceKey;
class GrTextureParams;

class GrTexture : public GrSurface {

public:
    SK_DECLARE_INST_COUNT(GrTexture)
    GR_DECLARE_RESOURCE_CACHE_TYPE()

    
    


    enum FlagBits {
        kFirstBit = (kLastPublic_GrTextureFlagBit << 1),

        



        kReturnToCache_FlagBit        = kFirstBit,
    };

    void setFlag(GrTextureFlags flags) {
        fDesc.fFlags = fDesc.fFlags | flags;
    }
    void resetFlag(GrTextureFlags flags) {
        fDesc.fFlags = fDesc.fFlags & ~flags;
    }
    bool isSetFlag(GrTextureFlags flags) const {
        return 0 != (fDesc.fFlags & flags);
    }

    


    virtual size_t sizeInBytes() const SK_OVERRIDE {
        return (size_t) fDesc.fWidth *
                        fDesc.fHeight *
                        GrBytesPerPixel(fDesc.fConfig);
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

    


    virtual GrTexture* asTexture() SK_OVERRIDE { return this; }
    virtual const GrTexture* asTexture() const SK_OVERRIDE { return this; }

    






    virtual GrRenderTarget* asRenderTarget() SK_OVERRIDE {
        return fRenderTarget;
    }
    virtual const GrRenderTarget* asRenderTarget() const SK_OVERRIDE {
        return fRenderTarget;
    }

    
    



    GrFixed normalizeFixedX(GrFixed x) const {
        GrAssert(GrIsPow2(fDesc.fWidth));
        return x >> fShiftFixedX;
    }
    GrFixed normalizeFixedY(GrFixed y) const {
        GrAssert(GrIsPow2(fDesc.fHeight));
        return y >> fShiftFixedY;
    }

    




    void releaseRenderTarget();

    



    virtual intptr_t getTextureHandle() const = 0;

    



    virtual void invalidateCachedState() = 0;

#if GR_DEBUG
    void validate() const {
        this->INHERITED::validate();

        this->validateDesc();
    }
#else
    void validate() const {}
#endif

    static GrResourceKey ComputeKey(const GrGpu* gpu,
                                    const GrTextureParams* sampler,
                                    const GrTextureDesc& desc,
                                    const GrCacheData& cacheData,
                                    bool scratch);

    static bool NeedsResizing(const GrResourceKey& key);
    static bool IsScratchTexture(const GrResourceKey& key);
    static bool NeedsFiltering(const GrResourceKey& key);

protected:
    GrRenderTarget* fRenderTarget; 
                                   
                                   

    GrTexture(GrGpu* gpu, const GrTextureDesc& desc)
    : INHERITED(gpu, desc)
    , fRenderTarget(NULL) {

        
        fShiftFixedX = 31 - Gr_clz(fDesc.fWidth);
        fShiftFixedY = 31 - Gr_clz(fDesc.fHeight);
    }

    
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

    void validateDesc() const;

private:
    
    
    int                 fShiftFixedX;
    int                 fShiftFixedY;

    virtual void internal_dispose() const SK_OVERRIDE;

    typedef GrSurface INHERITED;
};

#endif

