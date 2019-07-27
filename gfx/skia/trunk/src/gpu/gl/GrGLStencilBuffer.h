







#ifndef GrGLStencilBuffer_DEFINED
#define GrGLStencilBuffer_DEFINED

#include "gl/GrGLInterface.h"
#include "GrStencilBuffer.h"

class GrGLStencilBuffer : public GrStencilBuffer {
public:
    static const GrGLenum kUnknownInternalFormat = ~0U;
    static const GrGLuint kUnknownBitCount = ~0U;
    struct Format {
        GrGLenum  fInternalFormat;
        GrGLuint  fStencilBits;
        GrGLuint  fTotalBits;
        bool      fPacked;
    };

    GrGLStencilBuffer(GrGpu* gpu,
                      bool isWrapped,
                      GrGLint rbid,
                      int width, int height,
                      int sampleCnt,
                      const Format& format)
        : GrStencilBuffer(gpu, isWrapped, width, height, format.fStencilBits, sampleCnt)
        , fFormat(format)
        , fRenderbufferID(rbid) {
    }

    virtual ~GrGLStencilBuffer();

    virtual size_t gpuMemorySize() const SK_OVERRIDE;

    GrGLuint renderbufferID() const {
        return fRenderbufferID;
    }

    const Format& format() const { return fFormat; }

protected:
    
    virtual void onRelease() SK_OVERRIDE;
    virtual void onAbandon() SK_OVERRIDE;

private:
    Format fFormat;
    
    
    
    GrGLuint fRenderbufferID;

    typedef GrStencilBuffer INHERITED;
};

#endif
