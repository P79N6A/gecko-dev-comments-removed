




#ifndef GLCONTEXT_TYPES_H_
#define GLCONTEXT_TYPES_H_

#include "GLTypes.h"

namespace mozilla {
namespace gl {

class GLContext;

typedef uintptr_t SharedTextureHandle;

enum SharedTextureShareType {
    SameProcess = 0,
    CrossProcess
};

enum SharedTextureBufferType {
    TextureID
#ifdef MOZ_WIDGET_ANDROID
    , SurfaceTexture
#endif
#ifdef XP_MACOSX
    , IOSurface
#endif
};

enum ContextFlags {
    ContextFlagsNone = 0x0,
    ContextFlagsGlobal = 0x1,
    ContextFlagsMesaLLVMPipe = 0x2
};

enum GLContextType {
    ContextTypeUnknown,
    ContextTypeWGL,
    ContextTypeCGL,
    ContextTypeGLX,
    ContextTypeEGL
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
