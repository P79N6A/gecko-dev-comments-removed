








#ifndef GrGLTexture_DEFINED
#define GrGLTexture_DEFINED

#include "GrGLRenderTarget.h"
#include "GrScalar.h"
#include "GrTexture.h"




class GrGLTexID : public GrRefCnt {

public:
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
        void invalidate() { memset(this, 0xff, sizeof(TexParams)); }
    };

    struct Desc {
        int             fContentWidth;
        int             fContentHeight;
        int             fAllocWidth;
        int             fAllocHeight;
        GrPixelConfig   fFormat;
        GrGLuint        fTextureID;
        bool            fOwnsID;
        GrGLenum        fUploadFormat;
        GrGLenum        fUploadByteCount;
        GrGLenum        fUploadType;
        Orientation     fOrientation;
    };

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc,
                const GrGLRenderTarget::Desc& rtDesc,
                const TexParams& initialTexParams);

    
    GrGLTexture(GrGpuGL* gpu,
                const Desc& textureDesc,
                const TexParams& initialTexParams);


    virtual ~GrGLTexture() { this->release(); }

    
    virtual void uploadTextureData(int x,
                                   int y,
                                   int width,
                                   int height,
                                   const void* srcData,
                                   size_t rowBytes);
    virtual intptr_t getTextureHandle() const;

    const TexParams& getTexParams() const { return fTexParams; }
    void setTexParams(const TexParams& texParams) { fTexParams = texParams; }
    GrGLuint textureID() const { return fTexIDObj->id(); }

    GrGLenum uploadFormat() const { return fUploadFormat; }
    GrGLenum uploadByteCount() const { return fUploadByteCount; }
    GrGLenum uploadType() const { return fUploadType; }

    


    GrScalar contentScaleX() const { return fScaleX; }

    


    GrScalar contentScaleY() const { return fScaleY; }

    
    
    
    
    
    
    
    
    Orientation orientation() const { return fOrientation; }

    static const GrGLenum* WrapMode2GLWrap(GrGLBinding binding);

protected:

    
    virtual void onAbandon();
    virtual void onRelease();

private:
    TexParams           fTexParams;
    GrGLTexID*          fTexIDObj;
    GrGLenum            fUploadFormat;
    GrGLenum            fUploadByteCount;
    GrGLenum            fUploadType;
    
    GrScalar            fScaleX;
    GrScalar            fScaleY;
    Orientation         fOrientation;

    void init(GrGpuGL* gpu,
              const Desc& textureDesc,
              const GrGLRenderTarget::Desc* rtDesc,
              const TexParams& initialTexParams);

    typedef GrTexture INHERITED;
};

#endif
