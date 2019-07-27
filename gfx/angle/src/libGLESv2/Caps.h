#ifndef LIBGLESV2_CAPS_H
#define LIBGLESV2_CAPS_H







#include "angle_gl.h"

#include <set>
#include <unordered_map>
#include <vector>
#include <string>

namespace gl
{

typedef std::set<GLuint> SupportedSampleSet;

struct TextureCaps
{
    TextureCaps();

    
    bool texturable;

    
    bool filterable;

    
    bool renderable;

    SupportedSampleSet sampleCounts;

    
    GLuint getMaxSamples() const;

    
    
    GLuint getNearestSamples(GLuint requestedSamples) const;
};

class TextureCapsMap
{
  public:
    typedef std::unordered_map<GLenum, TextureCaps>::const_iterator const_iterator;

    void insert(GLenum internalFormat, const TextureCaps &caps);
    void remove(GLenum internalFormat);

    const TextureCaps &get(GLenum internalFormat) const;

    const_iterator begin() const;
    const_iterator end() const;

    size_t size() const;

  private:
    typedef std::unordered_map<GLenum, TextureCaps> InternalFormatToCapsMap;
    InternalFormatToCapsMap mCapsMap;
};

struct Extensions
{
    Extensions();

    
    std::vector<std::string> getStrings() const;

    
    
    
    
    
    
    
    
    
    
    
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
    GLuint maxSamples;

    
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

    
    GLuint64 maxElementIndex;
    GLuint max3DTextureSize;
    GLuint max2DTextureSize;
    GLuint maxArrayTextureLayers;
    GLfloat maxLODBias;
    GLuint maxCubeMapTextureSize;
    GLuint maxRenderbufferSize;
    GLuint maxDrawBuffers;
    GLuint maxColorAttachments;
    GLuint maxViewportWidth;
    GLuint maxViewportHeight;
    GLfloat minAliasedPointSize;
    GLfloat maxAliasedPointSize;
    GLfloat minAliasedLineWidth;
    GLfloat maxAliasedLineWidth;

    
    GLuint maxElementsIndices;
    GLuint maxElementsVertices;
    std::vector<GLenum> compressedTextureFormats;
    std::vector<GLenum> programBinaryFormats;
    std::vector<GLenum> shaderBinaryFormats;
    GLuint64 maxServerWaitTimeout;

    
    GLuint maxVertexAttributes;
    GLuint maxVertexUniformComponents;
    GLuint maxVertexUniformVectors;
    GLuint maxVertexUniformBlocks;
    GLuint maxVertexOutputComponents;
    GLuint maxVertexTextureImageUnits;

    
    GLuint maxFragmentUniformComponents;
    GLuint maxFragmentUniformVectors;
    GLuint maxFragmentUniformBlocks;
    GLuint maxFragmentInputComponents;
    GLuint maxTextureImageUnits;
    GLint minProgramTexelOffset;
    GLint maxProgramTexelOffset;

    
    GLuint maxUniformBufferBindings;
    GLuint64 maxUniformBlockSize;
    GLuint uniformBufferOffsetAlignment;
    GLuint maxCombinedUniformBlocks;
    GLuint64 maxCombinedVertexUniformComponents;
    GLuint64 maxCombinedFragmentUniformComponents;
    GLuint maxVaryingComponents;
    GLuint maxVaryingVectors;
    GLuint maxCombinedTextureImageUnits;

    
    GLuint maxTransformFeedbackInterleavedComponents;
    GLuint maxTransformFeedbackSeparateAttributes;
    GLuint maxTransformFeedbackSeparateComponents;
};

}

#endif 
