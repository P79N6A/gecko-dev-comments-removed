







#ifndef GrTexture_DEFINED
#define GrTexture_DEFINED

#include "GrSurface.h"
#include "GrRenderTarget.h"
#include "SkPoint.h"
#include "SkRefCnt.h"

class GrResourceKey;
class GrTextureParams;
class GrTextureImpl;

class GrTexture : public GrSurface {
public:
    


    virtual size_t gpuMemorySize() const SK_OVERRIDE;

    
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
    virtual GrRenderTarget* asRenderTarget() SK_OVERRIDE { return fRenderTarget.get(); }
    virtual const GrRenderTarget* asRenderTarget() const SK_OVERRIDE { return fRenderTarget.get(); }

    



    SkFixed normalizeFixedX(SkFixed x) const {
        SkASSERT(SkIsPow2(fDesc.fWidth));
        return x >> fShiftFixedX;
    }
    SkFixed normalizeFixedY(SkFixed y) const {
        SkASSERT(SkIsPow2(fDesc.fHeight));
        return y >> fShiftFixedY;
    }

    



    virtual GrBackendObject getTextureHandle() const = 0;

    



    virtual void textureParamsModified() = 0;
    SK_ATTR_DEPRECATED("Renamed to textureParamsModified.")
    void invalidateCachedState() { this->textureParamsModified(); }

    


    enum FlagBits {
        kFirstBit = (kLastPublic_GrTextureFlagBit << 1),

        



        kReturnToCache_FlagBit        = kFirstBit,
    };

    void resetFlag(GrTextureFlags flags) {
        fDesc.fFlags = fDesc.fFlags & ~flags;
    }

#ifdef SK_DEBUG
    void validate() const {
        this->INHERITED::validate();
        this->validateDesc();
    }
#endif

    GrTextureImpl* impl() { return reinterpret_cast<GrTextureImpl*>(this); }
    const GrTextureImpl* impl() const { return reinterpret_cast<const GrTextureImpl*>(this); }

protected:
    
    
    SkAutoTUnref<GrRenderTarget> fRenderTarget;

    GrTexture(GrGpu* gpu, bool isWrapped, const GrTextureDesc& desc)
    : INHERITED(gpu, isWrapped, desc)
    , fRenderTarget(NULL) {
        
        fShiftFixedX = 31 - SkCLZ(fDesc.fWidth);
        fShiftFixedY = 31 - SkCLZ(fDesc.fHeight);
    }

    virtual ~GrTexture();

    
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

    void validateDesc() const;

private:
    virtual void internal_dispose() const SK_OVERRIDE;

    
    
    int                 fShiftFixedX;
    int                 fShiftFixedY;

    typedef GrSurface INHERITED;
};

class GrTextureImpl : public GrTexture {
public:
    SK_DECLARE_INST_COUNT(GrTextureImpl)

    void setFlag(GrTextureFlags flags) {
        fDesc.fFlags = fDesc.fFlags | flags;
    }
    void resetFlag(GrTextureFlags flags) {
        fDesc.fFlags = fDesc.fFlags & ~flags;
    }
    bool isSetFlag(GrTextureFlags flags) const {
        return 0 != (fDesc.fFlags & flags);
    }

    void dirtyMipMaps(bool mipMapsDirty);

    bool mipMapsAreDirty() const {
        return kValid_MipMapsStatus != fMipMapsStatus;
    }

    bool hasMipMaps() const {
        return kNotAllocated_MipMapsStatus != fMipMapsStatus;
    }

    static GrResourceKey ComputeKey(const GrGpu* gpu,
                                    const GrTextureParams* params,
                                    const GrTextureDesc& desc,
                                    const GrCacheID& cacheID);
    static GrResourceKey ComputeScratchKey(const GrTextureDesc& desc);
    static bool NeedsResizing(const GrResourceKey& key);
    static bool NeedsBilerp(const GrResourceKey& key);

protected:
    GrTextureImpl(GrGpu* gpu, bool isWrapped, const GrTextureDesc& desc)
    : INHERITED(gpu, isWrapped, desc)
    , fMipMapsStatus(kNotAllocated_MipMapsStatus) {
    }

private:
    enum MipMapsStatus {
        kNotAllocated_MipMapsStatus,
        kAllocated_MipMapsStatus,
        kValid_MipMapsStatus
    };

    MipMapsStatus       fMipMapsStatus;

    typedef GrTexture INHERITED;
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
