




#ifndef GLCONTEXT_TYPES_H_
#define GLCONTEXT_TYPES_H_

#include "GLTypes.h"
#include "mozilla/TypedEnum.h"

namespace mozilla {
namespace gl {

class GLContext;

typedef uintptr_t SharedTextureHandle;

MOZ_BEGIN_ENUM_CLASS(SharedTextureShareType)
    SameProcess = 0,
    CrossProcess
MOZ_END_ENUM_CLASS(SharedTextureShareType)

MOZ_BEGIN_ENUM_CLASS(SharedTextureBufferType)
    TextureID,
    SurfaceTexture,
    IOSurface
MOZ_END_ENUM_CLASS(SharedTextureBufferType)

MOZ_BEGIN_ENUM_CLASS(GLContextType)
    Unknown,
    WGL,
    CGL,
    GLX,
    EGL
MOZ_END_ENUM_CLASS(GLContextType)

struct GLFormats
{
    
    GLFormats();

    GLenum color_texInternalFormat;
    GLenum color_texFormat;
    GLenum color_texType;
    GLenum color_rbFormat;

    GLenum depthStencil;
    GLenum depth;
    GLenum stencil;

    GLsizei samples;
};


struct PixelBufferFormat
{
    
    PixelBufferFormat();

    int red, green, blue;
    int alpha;
    int depth, stencil;
    int samples;

    int ColorBits() const { return red + green + blue; }
};


} 
} 

#endif 
