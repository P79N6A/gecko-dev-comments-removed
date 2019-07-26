




#ifndef SURFACE_TYPES_H_
#define SURFACE_TYPES_H_

#include "mozilla/TypedEnum.h"
#include "mozilla/StandardInteger.h"

#include <cstring>

namespace mozilla {
namespace gfx {

typedef uintptr_t SurfaceStreamHandle;

struct SurfaceCaps
{
    bool any;
    bool color, alpha;
    bool bpp16;
    bool depth, stencil;
    bool antialias;
    bool preserve;

    SurfaceCaps() {
        Clear();
    }

    void Clear() {
        std::memset(this, 0, sizeof(SurfaceCaps));
    }

    
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

MOZ_BEGIN_ENUM_CLASS(SharedSurfaceType, uint8_t)
    Unknown = 0,

    Basic,
    GLTextureShare,
    EGLImageShare,
    EGLSurfaceANGLE,
    DXGLInterop,
    DXGLInterop2,
    Gralloc,

    Max
MOZ_END_ENUM_CLASS(SharedSurfaceType)


MOZ_BEGIN_ENUM_CLASS(SurfaceStreamType, uint8_t)
    SingleBuffer,
    TripleBuffer_Copy,
    TripleBuffer,
    Max
MOZ_END_ENUM_CLASS(SurfaceStreamType)


MOZ_BEGIN_ENUM_CLASS(APITypeT, uint8_t)
    Generic = 0,

    OpenGL,

    Max
MOZ_END_ENUM_CLASS(APITypeT)


MOZ_BEGIN_ENUM_CLASS(AttachmentType, uint8_t)
    Screen = 0,

    GLTexture,
    GLRenderbuffer,

    Max
MOZ_END_ENUM_CLASS(AttachmentType)

} 
} 

#endif 
