




#ifndef GLCONTEXT_TYPES_H_
#define GLCONTEXT_TYPES_H_

typedef unsigned int GLenum;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;

namespace mozilla {
namespace gl {

enum ShaderProgramType {
    RGBALayerProgramType,
    RGBALayerExternalProgramType,
    BGRALayerProgramType,
    RGBXLayerProgramType,
    BGRXLayerProgramType,
    RGBARectLayerProgramType,
    RGBAExternalLayerProgramType,
    ColorLayerProgramType,
    YCbCrLayerProgramType,
    ComponentAlphaPass1ProgramType,
    ComponentAlphaPass2ProgramType,
    Copy2DProgramType,
    Copy2DRectProgramType,
    NumProgramTypes
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
