




#ifndef GLCONTEXT_TYPES_H_
#define GLCONTEXT_TYPES_H_

#include "GLTypes.h"

namespace mozilla {
namespace gl {

class GLContext;

enum class GLContextType {
    Unknown,
    WGL,
    CGL,
    GLX,
    EGL
};

enum class OriginPos : uint8_t {
  TopLeft,
  BottomLeft
};

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
