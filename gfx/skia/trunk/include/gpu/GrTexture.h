







#ifndef GrTexture_DEFINED
#define GrTexture_DEFINED

#include "GrSurface.h"
#include "SkPoint.h"
#include "GrRenderTarget.h"

class GrResourceKey;
class GrTextureParams;

class GrTexture : public GrSurface {

public:
    SK_DECLARE_INST_COUNT(GrTexture)
    
    


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

    void dirtyMipMaps(bool mipMapsDirty) {
        fMipMapsDirty = mipMapsDirty;
    }

    bool mipMapsAreDirty() const {
        return fMipMapsDirty;
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
        return fRenderTarget.get();
    }
    virtual const GrRenderTarget* asRenderTarget() const SK_OVERRIDE {
        return fRenderTarget.get();
    }

    
    



    GrFixed normalizeFixedX(GrFixed x) const {
        SkASSERT(GrIsPow2(fDesc.fWidth));
        return x >> fShiftFixedX;
    }
    GrFixed normalizeFixedY(GrFixed y) const {
        SkASSERT(GrIsPow2(fDesc.fHeight));
        return y >> fShiftFixedY;
    }

    



    virtual GrBackendObject getTextureHandle() const = 0;

    



    virtual void invalidateCachedState() = 0;

#ifdef SK_DEBUG
    void validate() const {
        this->INHERITED::validate();

        this->validateDesc();
    }
#endif

    static GrResourceKey ComputeKey(const GrGpu* gpu,
                                    const GrTextureParams* params,
                                    const GrTextureDesc& desc,
                                    const GrCacheID& cacheID);
    static GrResourceKey ComputeScratchKey(const GrTextureDesc& desc);
    static bool NeedsResizing(const GrResourceKey& key);
    static bool NeedsBilerp(const GrResourceKey& key);

protected:
    
    
    SkAutoTUnref<GrRenderTarget> fRenderTarget;

    GrTexture(GrGpu* gpu, bool isWrapped, const GrTextureDesc& desc)
    : INHERITED(gpu, isWrapped, desc)
    , fRenderTarget(NULL)
    , fMipMapsDirty(true) {

        
        fShiftFixedX = 31 - SkCLZ(fDesc.fWidth);
        fShiftFixedY = 31 - SkCLZ(fDesc.fHeight);
    }
    virtual ~GrTexture();

    
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

    void validateDesc() const;

private:
    
    
    int                 fShiftFixedX;
    int                 fShiftFixedY;

    bool                fMipMapsDirty;

    virtual void internal_dispose() const SK_OVERRIDE;

    typedef GrSurface INHERITED;
};




class GrDeviceCoordTexture {
public:
    GrDeviceCoordTexture() { fOffset.set(0, 0); }

    GrDeviceCoordTexture(const GrDeviceCoordTexture& other) {
        *this = other;
    }

    GrDeviceCoordTexture(GrTexture* texture, const SkIPoint& offset)
        : fTexture(SkSafeRef(texture))
        , fOffset(offset) {
    }

    GrDeviceCoordTexture& operator=(const GrDeviceCoordTexture& other) {
        fTexture.reset(SkSafeRef(other.fTexture.get()));
        fOffset = other.fOffset;
        return *this;
    }

    const SkIPoint& offset() const { return fOffset; }

    void setOffset(const SkIPoint& offset) { fOffset = offset; }
    void setOffset(int ox, int oy) { fOffset.set(ox, oy); }

    GrTexture* texture() const { return fTexture.get(); }

    GrTexture* setTexture(GrTexture* texture) {
        fTexture.reset(SkSafeRef(texture));
        return texture;
    }
private:
    SkAutoTUnref<GrTexture> fTexture;
    SkIPoint                fOffset;
};

#endif
