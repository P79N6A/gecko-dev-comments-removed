









#ifndef GrGpuGL_DEFINED
#define GrGpuGL_DEFINED

#include "GrDrawState.h"
#include "GrGpu.h"
#include "GrGLIndexBuffer.h"
#include "GrGLIRect.h"
#include "GrGLStencilBuffer.h"
#include "GrGLTexture.h"
#include "GrGLVertexBuffer.h"

#include "SkString.h"

class GrGpuGL : public GrGpu {
public:
    virtual ~GrGpuGL();

    const GrGLInterface* glInterface() const { return fGL; }
    GrGLBinding glBinding() const { return fGLBinding; }
    GrGLVersion glVersion() const { return fGLVersion; }

protected:
    GrGpuGL(const GrGLInterface* glInterface, GrGLBinding glBinding);

    struct {
        size_t                  fVertexOffset;
        GrVertexLayout          fVertexLayout;
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
        bool                    fArrayPtrsDirty;
    } fHWGeometryState;

    struct AAState {
        bool fMSAAEnabled;
        bool fSmoothLineEnabled;
    } fHWAAState;

    GrDrawState fHWDrawState;
    bool        fHWStencilClip;

    
    
    
    
    
    struct {
        bool fRenderTargetChanged : 1;
        int  fTextureChangedMask;
    } fDirtyFlags;
    GR_STATIC_ASSERT(8 * sizeof(int) >= GrDrawState::kNumStages);

    
    void resetDirtyFlags();

    
    struct {
        bool        fScissorEnabled;
        GrGLIRect   fScissorRect;
        GrGLIRect   fViewportRect;
    } fHWBounds;

    
    virtual void resetContext();

    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes);
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size,
                                                 bool dynamic);
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size,
                                               bool dynamic);
    virtual GrResource* onCreatePlatformSurface(const GrPlatformSurfaceDesc& desc);
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget* rt,
                                                    int width, int height);
    virtual bool attachStencilBufferToRenderTarget(GrStencilBuffer* sb,
                                                   GrRenderTarget* rt);

    virtual void onClear(const GrIRect* rect, GrColor color);

    virtual void onForceRenderTargetFlush();

    virtual bool onReadPixels(GrRenderTarget* target,
                              int left, int top, int width, int height,
                              GrPixelConfig, void* buffer);

    virtual void onGpuDrawIndexed(GrPrimitiveType type,
                                  uint32_t startVertex,
                                  uint32_t startIndex,
                                  uint32_t vertexCount,
                                  uint32_t indexCount);
    virtual void onGpuDrawNonIndexed(GrPrimitiveType type,
                                     uint32_t vertexCount,
                                     uint32_t numVertices);
    virtual void flushScissor(const GrIRect* rect);
    virtual void clearStencil();
    virtual void clearStencilClip(const GrIRect& rect, bool insideClip);
    virtual int getMaxEdges() const;

    
    void setTextureUnit(int unitIdx);

    
    
    void setBuffers(bool indexed,
                    int* extraVertexOffset,
                    int* extraIndexOffset);

    
    
    
    
    
    
    
    bool flushGLStateCommon(GrPrimitiveType type);

    
    
    
    
    void flushBlend(GrPrimitiveType type,
                    GrBlendCoeff srcCoeff,
                    GrBlendCoeff dstCoeff);

    bool hasExtension(const char* ext) {
        return GrGLHasExtensionFromString(ext, fExtensionString.c_str());
    }

    
    static void AdjustTextureMatrix(const GrGLTexture* texture,
                                    GrSamplerState::SampleMode mode,
                                    GrMatrix* matrix);

    
    
    
    static bool TextureMatrixIsIdentity(const GrGLTexture* texture,
                                        const GrSamplerState& sampler);

    static bool BlendCoeffReferencesConstant(GrBlendCoeff coeff);

private:
    
    
    void initCaps();

    void initFSAASupport();

    
    void initStencilFormats();

    
    
    void notifyVertexBufferBind(const GrGLVertexBuffer* buffer);
    void notifyVertexBufferDelete(const GrGLVertexBuffer* buffer);
    void notifyIndexBufferBind(const GrGLIndexBuffer* buffer);
    void notifyIndexBufferDelete(const GrGLIndexBuffer* buffer);
    void notifyTextureDelete(GrGLTexture* texture);
    void notifyRenderTargetDelete(GrRenderTarget* renderTarget);

    void setSpareTextureUnit();

    
    
    void flushRenderTarget(const GrIRect* bound);
    void flushStencil();
    void flushAAState(GrPrimitiveType type);

    void resolveRenderTarget(GrGLRenderTarget* texture);

    bool canBeTexture(GrPixelConfig config,
                      GrGLenum* internalFormat,
                      GrGLenum* format,
                      GrGLenum* type);
    
    void allocateAndUploadTexData(const GrGLTexture::Desc& desc,
                                  GrGLenum internalFormat,
                                  const void* data,
                                  size_t rowBytes);

    bool createRenderTargetObjects(int width, int height,
                                   GrGLuint texID,
                                   GrGLRenderTarget::Desc* desc);

    bool fboInternalFormat(GrPixelConfig config, GrGLenum* format);

    friend class GrGLVertexBuffer;
    friend class GrGLIndexBuffer;
    friend class GrGLTexture;
    friend class GrGLRenderTarget;

    
    SkString fExtensionString;
    GrGLVersion fGLVersion;

    struct GLCaps {
        
        GLCaps() : fStencilFormats(8) {}
        SkTArray<GrGLStencilBuffer::Format, true> fStencilFormats;

        enum {
            


            kNone_MSFBO = 0,  
            


            kDesktopARB_MSFBO,
            


            kDesktopEXT_MSFBO,
            


            kAppleES_MSFBO,
        } fMSFBOType;

        
        GrGLuint fAASamples[4];

        
        int fMaxFragmentUniformVectors;

        
        bool fRGBA8Renderbuffer;

        void print() const;
    } fGLCaps;


    
    
    
    GrGLuint fStencilClearFBO;

    bool fHWBlendDisabled;

    int fActiveTextureUnitIdx;

    
    
    int fLastSuccessfulStencilFmtIdx;

    const GrGLInterface* fGL;
    GrGLBinding fGLBinding;

    bool fPrintedCaps;

    typedef GrGpu INHERITED;
};

#endif

