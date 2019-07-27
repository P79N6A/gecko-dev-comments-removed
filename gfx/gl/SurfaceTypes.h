




#ifndef SURFACE_TYPES_H_
#define SURFACE_TYPES_H_

#include "mozilla/RefPtr.h"
#include "mozilla/Attributes.h"
#include <stdint.h>

namespace mozilla {
namespace layers {
class ISurfaceAllocator;
}

namespace gl {

struct SurfaceCaps final
{
    bool any;
    bool color, alpha;
    bool bpp16;
    bool depth, stencil;
    bool antialias;
    bool premultAlpha;
    bool preserve;

    
    
    RefPtr<layers::ISurfaceAllocator> surfaceAllocator;

    SurfaceCaps();
    SurfaceCaps(const SurfaceCaps& other);
    ~SurfaceCaps();

    void Clear();

    SurfaceCaps& operator=(const SurfaceCaps& other);

    
    static SurfaceCaps ForRGB() {
        SurfaceCaps caps;

        caps.color = true;

        return caps;
    }

    static SurfaceCaps ForRGBA() {
        SurfaceCaps caps;

        caps.color = true;
        caps.alpha = true;

        return caps;
    }

    static SurfaceCaps Any() {
        SurfaceCaps caps;

        caps.any = true;

        return caps;
    }
};

enum class SharedSurfaceType : uint8_t {
    Unknown = 0,

    Basic,
    GLTextureShare,
    EGLImageShare,
    EGLSurfaceANGLE,
    DXGLInterop,
    DXGLInterop2,
    Gralloc,
    IOSurface,

    Max
};

enum class AttachmentType : uint8_t {
    Screen = 0,

    GLTexture,
    GLRenderbuffer,

    Max
};

} 
} 

#endif
