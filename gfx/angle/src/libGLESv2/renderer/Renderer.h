








#ifndef LIBGLESV2_RENDERER_RENDERER_H_
#define LIBGLESV2_RENDERER_RENDERER_H_

#include "libGLESv2/Uniform.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Caps.h"

#include <cstdint>

#include <EGL/egl.h>

#if !defined(ANGLE_COMPILE_OPTIMIZATION_LEVEL)


#define ANGLE_COMPILE_OPTIMIZATION_LEVEL D3DCOMPILE_OPTIMIZATION_LEVEL1
#endif

namespace egl
{
class Display;
}

namespace gl
{
class InfoLog;
class ProgramBinary;
struct LinkedVarying;
struct VertexAttribute;
class Buffer;
class Texture;
class Framebuffer;
struct VertexAttribCurrentValueData;
}

namespace rx
{
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;
class TextureStorageInterface3D;
class TextureStorageInterface2DArray;
class VertexBuffer;
class IndexBuffer;
class QueryImpl;
class FenceImpl;
class BufferImpl;
class VertexArrayImpl;
class BufferStorage;
struct TranslatedIndexData;
class ShaderImpl;
class ShaderExecutable;
class SwapChain;
class RenderTarget;
class Image;
class TextureStorage;
class UniformStorage;
class TextureImpl;
class TransformFeedbackImpl;

struct ConfigDesc
{
    GLenum  renderTargetFormat;
    GLenum  depthStencilFormat;
    GLint   multiSample;
    bool    fastConfig;
    bool    es3Capable;
};

struct dx_VertexConstants
{
    float depthRange[4];
    float viewAdjust[4];
};

struct dx_PixelConstants
{
    float depthRange[4];
    float viewCoords[4];
    float depthFront[4];
};

enum ShaderType
{
    SHADER_VERTEX,
    SHADER_PIXEL,
    SHADER_GEOMETRY
};

class Renderer
{
  public:
    explicit Renderer(egl::Display *display);
    virtual ~Renderer();

    virtual EGLint initialize() = 0;
    virtual bool resetDevice() = 0;

    virtual int generateConfigs(ConfigDesc **configDescList) = 0;
    virtual void deleteConfigs(ConfigDesc *configDescList) = 0;

    virtual void sync(bool block) = 0;

    virtual SwapChain *createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat) = 0;

    virtual void generateSwizzle(gl::Texture *texture) = 0;
    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler) = 0;
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture) = 0;

    virtual bool setUniformBuffers(const gl::Buffer *vertexUniformBuffers[], const gl::Buffer *fragmentUniformBuffers[]) = 0;

    virtual void setRasterizerState(const gl::RasterizerState &rasterState) = 0;
    virtual void setBlendState(gl::Framebuffer *framebuffer, const gl::BlendState &blendState, const gl::ColorF &blendColor,
                               unsigned int sampleMask) = 0;
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW) = 0;

    virtual void setScissorRectangle(const gl::Rectangle &scissor, bool enabled) = 0;
    virtual bool setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                             bool ignoreViewport) = 0;

    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer) = 0;
    virtual void applyShaders(gl::ProgramBinary *programBinary, const gl::VertexFormat inputLayout[], const gl::Framebuffer *framebuffer,
                              bool rasterizerDiscard, bool transformFeedbackActive) = 0;
    virtual void applyUniforms(const gl::ProgramBinary &programBinary) = 0;
    virtual bool applyPrimitiveType(GLenum primitiveType, GLsizei elementCount) = 0;
    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, const gl::VertexAttribute vertexAttributes[], const gl::VertexAttribCurrentValueData currentValues[],
                                     GLint first, GLsizei count, GLsizei instances) = 0;
    virtual GLenum applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo) = 0;
    virtual void applyTransformFeedbackBuffers(gl::Buffer *transformFeedbackBuffers[], GLintptr offsets[]) = 0;

    virtual void drawArrays(GLenum mode, GLsizei count, GLsizei instances, bool transformFeedbackActive) = 0;
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices,
                              gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei instances) = 0;

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer) = 0;

    virtual void markAllStateDirty() = 0;

    
    virtual void notifyDeviceLost() = 0;
    virtual bool isDeviceLost() = 0;
    virtual bool testDeviceLost(bool notify) = 0;
    virtual bool testDeviceResettable() = 0;

    
    virtual const gl::Caps &getRendererCaps() const;
    virtual const gl::TextureCapsMap &getRendererTextureCaps() const;
    virtual const gl::Extensions &getRendererExtensions() const;

    virtual DWORD getAdapterVendor() const = 0;
    virtual std::string getRendererDescription() const = 0;
    virtual GUID getAdapterIdentifier() const = 0;

    virtual unsigned int getReservedVertexUniformVectors() const = 0;
    virtual unsigned int getReservedFragmentUniformVectors() const = 0;
    virtual unsigned int getReservedVertexUniformBuffers() const = 0;
    virtual unsigned int getReservedFragmentUniformBuffers() const = 0;
    virtual bool getShareHandleSupport() const = 0;
    virtual bool getPostSubBufferSupport() const = 0;

    virtual int getMajorShaderModel() const = 0;
    virtual int getMinSwapInterval() const = 0;
    virtual int getMaxSwapInterval() const = 0;

    
    virtual bool copyToRenderTarget(TextureStorageInterface2D *dest, TextureStorageInterface2D *source) = 0;
    virtual bool copyToRenderTarget(TextureStorageInterfaceCube *dest, TextureStorageInterfaceCube *source) = 0;
    virtual bool copyToRenderTarget(TextureStorageInterface3D *dest, TextureStorageInterface3D *source) = 0;
    virtual bool copyToRenderTarget(TextureStorageInterface2DArray *dest, TextureStorageInterface2DArray *source) = 0;

    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level) = 0;
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level) = 0;
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, GLint zOffset, TextureStorageInterface3D *storage, GLint level) = 0;
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, GLint zOffset, TextureStorageInterface2DArray *storage, GLint level) = 0;

    virtual bool blitRect(gl::Framebuffer *readTarget, const gl::Rectangle &readRect, gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                          const gl::Rectangle *scissor, bool blitRenderTarget, bool blitDepth, bool blitStencil, GLenum filter) = 0;
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                            GLenum type, GLuint outputPitch, const gl::PixelPackState &pack, uint8_t *pixels) = 0;

    
    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth) = 0;
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples) = 0;

    
    virtual ShaderImpl *createShader(GLenum type) = 0;

    
    virtual void releaseShaderCompiler() = 0;
    virtual ShaderExecutable *loadExecutable(const void *function, size_t length, rx::ShaderType type,
                                             const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                             bool separatedOutputBuffers) = 0;
    virtual ShaderExecutable *compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, rx::ShaderType type,
                                                  const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                                  bool separatedOutputBuffers, D3DWorkaroundType workaround) = 0;
    virtual UniformStorage *createUniformStorage(size_t storageSize) = 0;

    
    virtual Image *createImage() = 0;
    virtual void generateMipmap(Image *dest, Image *source) = 0;
    virtual TextureStorage *createTextureStorage2D(SwapChain *swapChain) = 0;
    virtual TextureStorage *createTextureStorage2D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels) = 0;
    virtual TextureStorage *createTextureStorageCube(GLenum internalformat, bool renderTarget, int size, int levels) = 0;
    virtual TextureStorage *createTextureStorage3D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels) = 0;
    virtual TextureStorage *createTextureStorage2DArray(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels) = 0;

    
    virtual TextureImpl *createTexture(GLenum target) = 0;

    
    virtual BufferImpl *createBuffer() = 0;
    virtual VertexBuffer *createVertexBuffer() = 0;
    virtual IndexBuffer *createIndexBuffer() = 0;

    
    virtual VertexArrayImpl *createVertexArray() = 0;

    
    virtual QueryImpl *createQuery(GLenum type) = 0;
    virtual FenceImpl *createFence() = 0;

    
    virtual TransformFeedbackImpl* createTransformFeedback() = 0;

    
    void setCurrentClientVersion(int clientVersion) { mCurrentClientVersion = clientVersion; }
    int getCurrentClientVersion() const { return mCurrentClientVersion; }

    
    virtual bool supportsFastCopyBufferToTexture(GLenum internalFormat) const = 0;
    virtual bool fastCopyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                         GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea) = 0;

    virtual bool getLUID(LUID *adapterLuid) const = 0;
    virtual rx::VertexConversionType getVertexConversionType(const gl::VertexFormat &vertexFormat) const = 0;
    virtual GLenum getVertexComponentType(const gl::VertexFormat &vertexFormat) const = 0;

  protected:
    egl::Display *mDisplay;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer);

    virtual void generateCaps(gl::Caps *outCaps, gl::TextureCapsMap* outTextureCaps, gl::Extensions *outExtensions) const = 0;

    mutable bool mCapsInitialized;
    mutable gl::Caps mCaps;
    mutable gl::TextureCapsMap mTextureCaps;
    mutable gl::Extensions mExtensions;

    int mCurrentClientVersion;
};

}
#endif 
