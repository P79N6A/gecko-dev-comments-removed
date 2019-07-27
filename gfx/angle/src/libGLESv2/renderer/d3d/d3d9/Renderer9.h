







#ifndef LIBGLESV2_RENDERER_RENDERER9_H_
#define LIBGLESV2_RENDERER_RENDERER9_H_

#include "common/angleutils.h"
#include "common/mathutil.h"
#include "libGLESv2/renderer/d3d/HLSLCompiler.h"
#include "libGLESv2/renderer/d3d/d3d9/ShaderCache.h"
#include "libGLESv2/renderer/d3d/d3d9/VertexDeclarationCache.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/RenderTarget.h"

namespace gl
{
class FramebufferAttachment;
}

namespace rx
{
class VertexDataManager;
class IndexDataManager;
class StreamingIndexBufferInterface;
struct TranslatedAttribute;
class Blit9;

class Renderer9 : public Renderer
{
  public:
    Renderer9(egl::Display *display, EGLNativeDisplayType hDc, EGLint requestedDisplay);
    virtual ~Renderer9();

    static Renderer9 *makeRenderer9(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    void startScene();
    void endScene();

    virtual void sync(bool block);

    virtual SwapChain *createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

    IDirect3DQuery9* allocateEventQuery();
    void freeEventQuery(IDirect3DQuery9* query);

    
    IDirect3DVertexShader9 *createVertexShader(const DWORD *function, size_t length);
    IDirect3DPixelShader9 *createPixelShader(const DWORD *function, size_t length);
    HRESULT createVertexBuffer(UINT Length, DWORD Usage, IDirect3DVertexBuffer9 **ppVertexBuffer);
    HRESULT createIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, IDirect3DIndexBuffer9 **ppIndexBuffer);
    virtual void generateSwizzle(gl::Texture *texture);
    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler);
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    virtual bool setUniformBuffers(const gl::Buffer *vertexUniformBuffers[], const gl::Buffer *fragmentUniformBuffers[]);

    virtual void setRasterizerState(const gl::RasterizerState &rasterState);
    virtual void setBlendState(gl::Framebuffer *framebuffer, const gl::BlendState &blendState, const gl::ColorF &blendColor,
                               unsigned int sampleMask);
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW);

    virtual void setScissorRectangle(const gl::Rectangle &scissor, bool enabled);
    virtual bool setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace,
                             bool ignoreViewport);

    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer);
    virtual void applyShaders(gl::ProgramBinary *programBinary, const gl::VertexFormat inputLayout[], const gl::Framebuffer *framebuffer,
                              bool rasterizerDiscard, bool transformFeedbackActive);
    virtual void applyUniforms(const gl::ProgramBinary &programBinary);
    virtual bool applyPrimitiveType(GLenum primitiveType, GLsizei elementCount);
    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, const gl::VertexAttribute vertexAttributes[], const gl::VertexAttribCurrentValueData currentValues[],
                                     GLint first, GLsizei count, GLsizei instances);
    virtual GLenum applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo);

    virtual void applyTransformFeedbackBuffers(gl::Buffer *transformFeedbackBuffers[], GLintptr offsets[]);

    virtual void drawArrays(GLenum mode, GLsizei count, GLsizei instances, bool transformFeedbackActive);
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices,
                              gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo, GLsizei instances);

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer);

    virtual void markAllStateDirty();

    
    void notifyDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost(bool notify);
    virtual bool testDeviceResettable();

    IDirect3DDevice9 *getDevice() { return mDevice; }
    virtual DWORD getAdapterVendor() const;
    virtual std::string getRendererDescription() const;
    virtual GUID getAdapterIdentifier() const;

    virtual unsigned int getReservedVertexUniformVectors() const;
    virtual unsigned int getReservedFragmentUniformVectors() const;
    virtual unsigned int getReservedVertexUniformBuffers() const;
    virtual unsigned int getReservedFragmentUniformBuffers() const;
    virtual bool getShareHandleSupport() const;
    virtual bool getPostSubBufferSupport() const;

    virtual int getMajorShaderModel() const;
    DWORD getCapsDeclTypes() const;
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    
    virtual bool copyToRenderTarget(TextureStorageInterface2D *dest, TextureStorageInterface2D *source);
    virtual bool copyToRenderTarget(TextureStorageInterfaceCube *dest, TextureStorageInterfaceCube *source);
    virtual bool copyToRenderTarget(TextureStorageInterface3D *dest, TextureStorageInterface3D *source);
    virtual bool copyToRenderTarget(TextureStorageInterface2DArray *dest, TextureStorageInterface2DArray *source);

    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, GLint zOffset, TextureStorageInterface3D *storage, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, GLint zOffset, TextureStorageInterface2DArray *storage, GLint level);

    virtual bool blitRect(gl::Framebuffer *readTarget, const gl::Rectangle &readRect, gl::Framebuffer *drawTarget, const gl::Rectangle &drawRect,
                          const gl::Rectangle *scissor, bool blitRenderTarget, bool blitDepth, bool blitStencil, GLenum filter);
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format,
                            GLenum type, GLuint outputPitch, const gl::PixelPackState &pack, uint8_t *pixels);

    
    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth);
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples);

    
    virtual ShaderImpl *createShader(GLenum type);

    
    virtual void releaseShaderCompiler();
    virtual ShaderExecutable *loadExecutable(const void *function, size_t length, rx::ShaderType type,
                                             const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                             bool separatedOutputBuffers);
    virtual ShaderExecutable *compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, rx::ShaderType type,
                                                  const std::vector<gl::LinkedVarying> &transformFeedbackVaryings,
                                                  bool separatedOutputBuffers, D3DWorkaroundType workaround);
    virtual UniformStorage *createUniformStorage(size_t storageSize);

    
    virtual Image *createImage();
    virtual void generateMipmap(Image *dest, Image *source);
    virtual TextureStorage *createTextureStorage2D(SwapChain *swapChain);
    virtual TextureStorage *createTextureStorage2D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, int levels);
    virtual TextureStorage *createTextureStorageCube(GLenum internalformat, bool renderTarget, int size, int levels);
    virtual TextureStorage *createTextureStorage3D(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels);
    virtual TextureStorage *createTextureStorage2DArray(GLenum internalformat, bool renderTarget, GLsizei width, GLsizei height, GLsizei depth, int levels);

    
    virtual TextureImpl *createTexture(GLenum target);

    
    virtual BufferImpl *createBuffer();
    virtual VertexBuffer *createVertexBuffer();
    virtual IndexBuffer *createIndexBuffer();

    
    virtual VertexArrayImpl *createVertexArray();

    
    virtual QueryImpl *createQuery(GLenum type);
    virtual FenceImpl *createFence();

    
    virtual TransformFeedbackImpl* createTransformFeedback();

    
    virtual bool supportsFastCopyBufferToTexture(GLenum internalFormat) const;
    virtual bool fastCopyBufferToTexture(const gl::PixelUnpackState &unpack, unsigned int offset, RenderTarget *destRenderTarget,
                                         GLenum destinationFormat, GLenum sourcePixelsType, const gl::Box &destArea);

    
    bool boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest);

    D3DPOOL getTexturePool(DWORD usage) const;

    virtual bool getLUID(LUID *adapterLuid) const;
    virtual rx::VertexConversionType getVertexConversionType(const gl::VertexFormat &vertexFormat) const;
    virtual GLenum getVertexComponentType(const gl::VertexFormat &vertexFormat) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer9);

    virtual void generateCaps(gl::Caps *outCaps, gl::TextureCapsMap *outTextureCaps, gl::Extensions *outExtensions) const;

    void release();

    void applyUniformnfv(gl::LinkedUniform *targetUniform, const GLfloat *v);
    void applyUniformniv(gl::LinkedUniform *targetUniform, const GLint *v);
    void applyUniformnbv(gl::LinkedUniform *targetUniform, const GLint *v);

    void drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);
    void drawIndexedPoints(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer);

    bool copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged);
    gl::FramebufferAttachment *getNullColorbuffer(gl::FramebufferAttachment *depthbuffer);

    D3DPOOL getBufferPool(DWORD usage) const;

    HMODULE mD3d9Module;
    HDC mDc;

    void initializeDevice();
    D3DPRESENT_PARAMETERS getDefaultPresentParameters();
    void releaseDeviceResources();

    HRESULT getDeviceStatusCode();
    bool isRemovedDeviceResettable() const;
    bool resetRemovedDevice();

    UINT mAdapter;
    D3DDEVTYPE mDeviceType;
    IDirect3D9 *mD3d9;  
    IDirect3D9Ex *mD3d9Ex;  
    IDirect3DDevice9 *mDevice;
    IDirect3DDevice9Ex *mDeviceEx;  

    HLSLCompiler mCompiler;

    Blit9 *mBlit;

    HWND mDeviceWindow;

    bool mDeviceLost;
    D3DCAPS9 mDeviceCaps;
    D3DADAPTER_IDENTIFIER9 mAdapterIdentifier;

    D3DPRIMITIVETYPE mPrimitiveType;
    int mPrimitiveCount;
    GLsizei mRepeatDraw;

    bool mSceneStarted;
    int mMinSwapInterval;
    int mMaxSwapInterval;

    bool mVertexTextureSupport;

    
    unsigned int mAppliedRenderTargetSerial;
    unsigned int mAppliedDepthbufferSerial;
    unsigned int mAppliedStencilbufferSerial;
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    rx::RenderTarget::Desc mRenderTargetDesc;
    unsigned int mCurStencilSize;
    unsigned int mCurDepthSize;

    IDirect3DStateBlock9 *mMaskedClearSavedState;

    
    bool mForceSetDepthStencilState;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;
    bool mCurFrontFaceCCW;

    bool mForceSetRasterState;
    gl::RasterizerState mCurRasterState;

    bool mForceSetScissor;
    gl::Rectangle mCurScissor;
    bool mScissorEnabled;

    bool mForceSetViewport;
    gl::Rectangle mCurViewport;
    float mCurNear;
    float mCurFar;
    float mCurDepthFront;

    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    GLuint mCurSampleMask;

    
    bool mForceSetVertexSamplerStates[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    gl::SamplerState mCurVertexSamplerStates[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];

    bool mForceSetPixelSamplerStates[gl::MAX_TEXTURE_IMAGE_UNITS];
    gl::SamplerState mCurPixelSamplerStates[gl::MAX_TEXTURE_IMAGE_UNITS];

    
    unsigned int mCurVertexTextureSerials[gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    unsigned int mCurPixelTextureSerials[gl::MAX_TEXTURE_IMAGE_UNITS];

    unsigned int mAppliedIBSerial;
    IDirect3DVertexShader9 *mAppliedVertexShader;
    IDirect3DPixelShader9 *mAppliedPixelShader;
    unsigned int mAppliedProgramSerial;

    rx::dx_VertexConstants mVertexConstants;
    rx::dx_PixelConstants mPixelConstants;
    bool mDxUniformsDirty;

    
    std::vector<IDirect3DQuery9*> mEventQueryPool;
    VertexShaderCache mVertexShaderCache;
    PixelShaderCache mPixelShaderCache;

    VertexDataManager *mVertexDataManager;
    VertexDeclarationCache mVertexDeclarationCache;

    IndexDataManager *mIndexDataManager;
    StreamingIndexBufferInterface *mLineLoopIB;

    enum { NUM_NULL_COLORBUFFER_CACHE_ENTRIES = 12 };
    struct NullColorbufferCacheEntry
    {
        UINT lruCount;
        int width;
        int height;
        gl::FramebufferAttachment *buffer;
    } mNullColorbufferCache[NUM_NULL_COLORBUFFER_CACHE_ENTRIES];
    UINT mMaxNullColorbufferLRU;

};

}
#endif 
