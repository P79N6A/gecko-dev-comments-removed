









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

    
    virtual GrPixelConfig preferredReadPixelsConfig(GrPixelConfig config)
                                                            const SK_OVERRIDE;
    virtual GrPixelConfig preferredWritePixelsConfig(GrPixelConfig config)
                                                            const SK_OVERRIDE;
    virtual bool readPixelsWillPayForYFlip(
                                    GrRenderTarget* renderTarget,
                                    int left, int top,
                                    int width, int height,
                                    GrPixelConfig config,
                                    size_t rowBytes) const SK_OVERRIDE;
    virtual bool fullReadPixelsIsFasterThanPartial() const SK_OVERRIDE;
protected:
    GrGpuGL(const GrGLInterface* glInterface, GrGLBinding glBinding);

    struct GLCaps {
        GLCaps()
            
            : fStencilFormats(8) 
            , fMSFBOType(kNone_MSFBO)
            , fMaxFragmentUniformVectors(0)
            , fRGBA8RenderbufferSupport(false)
            , fBGRAFormatSupport(false)
            , fBGRAIsInternalFormat(false)
            , fTextureSwizzleSupport(false)
            , fUnpackRowLengthSupport(false)
            , fUnpackFlipYSupport(false)
            , fPackRowLengthSupport(false)
            , fPackFlipYSupport(false)
            , fTextureUsageSupport(false)
            , fTexStorageSupport(false) {
            memset(fAASamples, 0, sizeof(fAASamples));
        }
        SkTArray<GrGLStencilBuffer::Format, true> fStencilFormats;

        enum {
            


            kNone_MSFBO = 0,  
            


            kDesktopARB_MSFBO,
            


            kDesktopEXT_MSFBO,
            


            kAppleES_MSFBO,
        } fMSFBOType;

        
        GrGLuint fAASamples[4];

        
        int fMaxFragmentUniformVectors;

        
        bool fRGBA8RenderbufferSupport;

        
        bool fBGRAFormatSupport;

        
        
        
        bool fBGRAIsInternalFormat;

        
        bool fTextureSwizzleSupport;
    
        
        bool fUnpackRowLengthSupport;

        
        bool fUnpackFlipYSupport;

        
        bool fPackRowLengthSupport;

        
        bool fPackFlipYSupport;

        
        bool fTextureUsageSupport;

        
        bool fTexStorageSupport;

        void print() const;
    } fGLCaps;
 
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

    const GLCaps& glCaps() const { return fGLCaps; }

    
    virtual void onResetContext() SK_OVERRIDE;

    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes);
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size,
                                                 bool dynamic);
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size,
                                               bool dynamic);
    virtual GrResource* onCreatePlatformSurface(const GrPlatformSurfaceDesc& desc);
    virtual GrTexture* onCreatePlatformTexture(const GrPlatformTextureDesc& desc) SK_OVERRIDE;
    virtual GrRenderTarget* onCreatePlatformRenderTarget(const GrPlatformRenderTargetDesc& desc) SK_OVERRIDE;
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget* rt,
                                                    int width, int height);
    virtual bool attachStencilBufferToRenderTarget(GrStencilBuffer* sb,
                                                   GrRenderTarget* rt);

    virtual void onClear(const GrIRect* rect, GrColor color);

    virtual void onForceRenderTargetFlush();

    virtual bool onReadPixels(GrRenderTarget* target,
                              int left, int top, 
                              int width, int height,
                              GrPixelConfig, 
                              void* buffer,
                              size_t rowBytes,
                              bool invertY) SK_OVERRIDE;

    virtual void onWriteTexturePixels(GrTexture* texture,
                                      int left, int top, int width, int height,
                                      GrPixelConfig config, const void* buffer,
                                      size_t rowBytes) SK_OVERRIDE;

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

    bool configToGLFormats(GrPixelConfig config,
                           bool getSizedInternal,
                           GrGLenum* internalFormat,
                           GrGLenum* externalFormat,
                           GrGLenum* externalType);
    
    bool uploadTexData(const GrGLTexture::Desc& desc,
                       bool isNewTexture,
                       int left, int top, int width, int height,
                       GrPixelConfig dataConfig,
                       const void* data,
                       size_t rowBytes);

    bool createRenderTargetObjects(int width, int height,
                                   GrGLuint texID,
                                   GrGLRenderTarget::Desc* desc);

    friend class GrGLVertexBuffer;
    friend class GrGLIndexBuffer;
    friend class GrGLTexture;
    friend class GrGLRenderTarget;

    
    SkString fExtensionString;
    GrGLVersion fGLVersion;

    
    
    
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

