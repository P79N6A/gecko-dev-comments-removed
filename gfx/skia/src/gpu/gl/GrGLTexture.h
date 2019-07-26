







#ifndef GrGLTexture_DEFINED
#define GrGLTexture_DEFINED

#include "GrGpu.h"
#include "GrGLRenderTarget.h"




class GrGLTexID : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrGLTexID)

    GrGLTexID(const GrGLInterface* gl, GrGLuint texID, bool ownsID)
        : fGL(gl)
        , fTexID(texID)
        , fOwnsID(ownsID) {
    }

    virtual ~GrGLTexID() {
        if (0 != fTexID && fOwnsID) {
            GR_GL_CALL(fGL, DeleteTextures(1, &fTexID));
        }
    }

    void abandon() { fTexID = 0; }
    GrGLuint id() const { return fTexID; }

private:
    const GrGLInterface* fGL;
    GrGLuint             fTexID;
    bool                 fOwnsID;

    typedef GrRefCnt INHERITED;
};




class GrGLTexture : public GrTexture {

public:
    enum Orientation {
        kBottomUp_Orientation,
        kTopDown_Orientation,
    };

    struct TexParams {
        GrGLenum fFilter;
        GrGLenum fWrapS;
        GrGLenum fWrapT;
        GrGLenum fSwizzleRGBA[4];
        void invalidate() { memset(this, 0xff, sizeof(TexParams)); }
    };

    struct Desc : public GrTextureDesc {
        GrGLuint        fTextureID;
        bool            fOwnsID;
        Orientation     fOrientation;
    };

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc,
                const GrGLRenderTarget::Desc& rtDesc);

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc);


    virtual ~GrGLTexture() { this->release(); }

    virtual intptr_t getTextureHandle() const SK_OVERRIDE;

    virtual void invalidateCachedState() SK_OVERRIDE { fTexParams.invalidate(); }

    
    const TexParams& getCachedTexParams(GrGpu::ResetTimestamp* timestamp) const {
        *timestamp = fTexParamsTimestamp;
        return fTexParams;
    }
    void setCachedTexParams(const TexParams& texParams,
                            GrGpu::ResetTimestamp timestamp) {
        fTexParams = texParams;
        fTexParamsTimestamp = timestamp;
    }
    GrGLuint textureID() const { return fTexIDObj->id(); }

    
    
    
    
    
    
    
    
    Orientation orientation() const { return fOrientation; }

protected:

    
    virtual void onAbandon() SK_OVERRIDE;
    virtual void onRelease() SK_OVERRIDE;

private:
    TexParams                       fTexParams;
    GrGpu::ResetTimestamp           fTexParamsTimestamp;
    GrGLTexID*                      fTexIDObj;
    Orientation                     fOrientation;

    void init(GrGpuGL* gpu,
              const Desc& textureDesc,
              const GrGLRenderTarget::Desc* rtDesc);

    typedef GrTexture INHERITED;
};

#endif
