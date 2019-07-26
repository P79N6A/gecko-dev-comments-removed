







#ifndef GrGLTexture_DEFINED
#define GrGLTexture_DEFINED

#include "GrGpu.h"
#include "GrGLRenderTarget.h"




class GrGLTexID : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrGLTexID)

    GrGLTexID(const GrGLInterface* gl, GrGLuint texID, bool isWrapped)
        : fGL(gl)
        , fTexID(texID)
        , fIsWrapped(isWrapped) {
    }

    virtual ~GrGLTexID() {
        if (0 != fTexID && !fIsWrapped) {
            GR_GL_CALL(fGL, DeleteTextures(1, &fTexID));
        }
    }

    void abandon() { fTexID = 0; }
    GrGLuint id() const { return fTexID; }

private:
    const GrGLInterface* fGL;
    GrGLuint             fTexID;
    bool                 fIsWrapped;

    typedef GrRefCnt INHERITED;
};




class GrGLTexture : public GrTexture {

public:
    struct TexParams {
        GrGLenum fFilter;
        GrGLenum fWrapS;
        GrGLenum fWrapT;
        GrGLenum fSwizzleRGBA[4];
        void invalidate() { memset(this, 0xff, sizeof(TexParams)); }
    };

    struct Desc : public GrTextureDesc {
        GrGLuint        fTextureID;
        bool            fIsWrapped;
    };

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc,
                const GrGLRenderTarget::Desc& rtDesc);

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc);


    virtual ~GrGLTexture() { this->release(); }

    virtual GrBackendObject getTextureHandle() const SK_OVERRIDE;

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

protected:

    
    virtual void onAbandon() SK_OVERRIDE;
    virtual void onRelease() SK_OVERRIDE;

private:
    TexParams                       fTexParams;
    GrGpu::ResetTimestamp           fTexParamsTimestamp;
    GrGLTexID*                      fTexIDObj;

    void init(GrGpuGL* gpu,
              const Desc& textureDesc,
              const GrGLRenderTarget::Desc* rtDesc);

    typedef GrTexture INHERITED;
};

#endif
