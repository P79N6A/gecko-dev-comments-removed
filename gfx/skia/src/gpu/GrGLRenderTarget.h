








#ifndef GrGLRenderTarget_DEFINED
#define GrGLRenderTarget_DEFINED

#include "GrGLIRect.h"
#include "GrRenderTarget.h"
#include "GrScalar.h"

class GrGpuGL;
class GrGLTexture;
class GrGLTexID;

class GrGLRenderTarget : public GrRenderTarget {

public:
    
    
    enum { kUnresolvableFBOID = 0 };

    struct Desc {
        GrGLuint      fRTFBOID;
        GrGLuint      fTexFBOID;
        GrGLuint      fMSColorRenderbufferID;
        bool          fOwnIDs;
        GrPixelConfig fConfig;
        int           fSampleCnt;
    };

    
    GrGLRenderTarget(GrGpuGL*          gpu,
                     const Desc&       desc,
                     const GrGLIRect&  viewport,
                     GrGLTexID*        texID,
                     GrGLTexture*      texture);

    
    GrGLRenderTarget(GrGpuGL*          gpu,
                     const Desc&       desc,
                     const GrGLIRect&  viewport);

    virtual ~GrGLRenderTarget() { this->release(); }

    void setViewport(const GrGLIRect& rect) { fViewport = rect; }
    const GrGLIRect& getViewport() const { return fViewport; }

    
    
    
    
    GrGLuint renderFBOID() const { return fRTFBOID; }
    
    GrGLuint textureFBOID() const { return fTexFBOID; }

    
    virtual intptr_t getRenderTargetHandle() const {
        return this->renderFBOID(); 
    }
    virtual intptr_t getRenderTargetResolvedHandle() const {
        return this->textureFBOID();
    }
    virtual ResolveType getResolveType() const {
        if (fRTFBOID == fTexFBOID) {
            
            return kAutoResolves_ResolveType;
        } else if (kUnresolvableFBOID == fTexFBOID) {
            return kCantResolve_ResolveType;
        } else {
            return kCanResolve_ResolveType;
        }
    }

protected:
    
    virtual void onAbandon();
    virtual void onRelease();

private:
    GrGLuint      fRTFBOID;
    GrGLuint      fTexFBOID;

    GrGLuint      fMSColorRenderbufferID;

    
    
    bool        fOwnIDs;

    
    
    
    GrGLIRect fViewport;

    
    GrGLTexID* fTexIDObj;

    void init(const Desc& desc, const GrGLIRect& viewport, GrGLTexID* texID);

    typedef GrRenderTarget INHERITED;
};

#endif
