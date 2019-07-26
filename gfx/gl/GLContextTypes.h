




#ifndef GLCONTEXTSTUFF_H_
#define GLCONTEXTSTUFF_H_





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

} 
} 

#endif 
