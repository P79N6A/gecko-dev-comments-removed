








#ifndef GrGLStencilBuffer_DEFINED
#define GrGLStencilBuffer_DEFINED

#include "GrGLInterface.h"
#include "GrStencilBuffer.h"

class GrGLStencilBuffer : public GrStencilBuffer {
public:
    static const GrGLenum kUnknownInternalFormat = ~0;
    struct Format {
        GrGLenum  fInternalFormat;
        GrGLuint  fStencilBits;
        GrGLuint  fTotalBits;
        bool      fPacked;
    };

    GrGLStencilBuffer(GrGpu* gpu, GrGLint rbid, 
                      int width, int height,
                      int sampleCnt,
                      const Format& format) 
        : GrStencilBuffer(gpu, width, height, format.fStencilBits, sampleCnt)
        , fFormat(format)
        , fRenderbufferID(rbid) {
    }

    virtual ~GrGLStencilBuffer();

    virtual size_t sizeInBytes() const;

    GrGLuint renderbufferID() const {
        return fRenderbufferID;
    }

    const Format& format() const { return fFormat; }

protected:
    virtual void onRelease();

    virtual void onAbandon();

private:
    Format fFormat;
    
    
    
    GrGLuint fRenderbufferID;

    typedef GrStencilBuffer INHERITED;
};

#endif
