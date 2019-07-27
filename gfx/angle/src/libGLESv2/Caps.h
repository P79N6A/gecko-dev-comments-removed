#ifndef LIBGLESV2_CAPS_H
#define LIBGLESV2_CAPS_H







#include "angle_gl.h"

#include <set>
#include <unordered_map>
#include <vector>
#include <string>

namespace gl
{

struct TextureCaps
{
    TextureCaps();

    bool texture2D;
    bool textureCubeMap;
    bool texture3D;
    bool texture2DArray;
    bool filtering;
    bool colorRendering;
    bool depthRendering;
    bool stencilRendering;

    std::set<GLuint> sampleCounts;
};

class TextureCapsMap
{
  public:
    void insert(GLenum internalFormat, const TextureCaps &caps);
    void remove(GLenum internalFormat);

    const TextureCaps &get(GLenum internalFormat) const;

  private:
    typedef std::unordered_map<GLenum, TextureCaps> InternalFormatToCapsMap;
    InternalFormatToCapsMap mCapsMap;
};

struct Extensions
{
    Extensions();

    
    std::vector<std::string> getStrings(GLuint clientVersion) const;

    
    
    
    
    
    
    
    
    
    
    void setTextureExtensionSupport(const TextureCapsMap &textureCaps);

    

    
    bool elementIndexUint;

    
    bool packedDepthStencil;

    
    bool getProgramBinary;

    
    
    bool rgb8rgba8;

    
    
    bool textureFormatBGRA8888;

    
    bool readFormatBGRA;

    
    bool pixelBufferObject;

    
    bool mapBuffer;
    bool mapBufferRange;

    
    
    
    bool textureHalfFloat;
    bool textureHalfFloatLinear;

    
    
    
    bool textureFloat;
    bool textureFloatLinear;

    
    
    
    bool textureRG;

    
    
    
    bool textureCompressionDXT1;
    bool textureCompressionDXT3;
    bool textureCompressionDXT5;

    
    
    
    bool sRGB;

    
    bool depthTextures;

    
    bool textureStorage;

    
    bool textureNPOT;

    
    bool drawBuffers;

    
    bool textureFilterAnisotropic;
    GLfloat maxTextureAnisotropy;

    
    bool occlusionQueryBoolean;

    
    bool fence;

    
    bool timerQuery;

    
    bool robustness;

    
    bool blendMinMax;

    
    bool framebufferBlit;

    
    bool framebufferMultisample;

    
    bool instancedArrays;

    
    bool packReverseRowOrder;

    
    bool standardDerivatives;

    
    bool shaderTextureLOD;

    
    bool fragDepth;

    
    bool textureUsage;

    
    bool translatedShaderSource;

    

    
    bool colorBufferFloat;
};

struct Caps
{
    Caps();

    
    TextureCapsMap textureCaps;

    
    Extensions extensions;
};

}

#endif 
