








#ifndef GrGpuGL_DEFINED
#define GrGpuGL_DEFINED

#include "GrBinHashKey.h"
#include "GrDrawState.h"
#include "GrGpu.h"
#include "GrGLContextInfo.h"
#include "GrGLIndexBuffer.h"
#include "GrGLIRect.h"
#include "GrGLProgram.h"
#include "GrGLStencilBuffer.h"
#include "GrGLTexture.h"
#include "GrGLVertexBuffer.h"
#include "../GrTHashCache.h"

class GrGpuGL : public GrGpu {
public:
    GrGpuGL(const GrGLContextInfo& ctxInfo);
    virtual ~GrGpuGL();

    const GrGLInterface* glInterface() const {
        return fGLContextInfo.interface();
    }
    GrGLBinding glBinding() const { return fGLContextInfo.binding(); }
    GrGLVersion glVersion() const { return fGLContextInfo.version(); }
    GrGLSLGeneration glslGeneration() const {
        return fGLContextInfo.glslGeneration();
    }

    
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

    virtual void abandonResources() SK_OVERRIDE;

    bool programUnitTest();


protected:
    
    virtual void onResetContext() SK_OVERRIDE;

    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) SK_OVERRIDE;
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size,
                                                 bool dynamic) SK_OVERRIDE;
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size,
                                               bool dynamic) SK_OVERRIDE;
    virtual GrPath* onCreatePath(const SkPath&) SK_OVERRIDE;
    virtual GrTexture* onCreatePlatformTexture(
        const GrPlatformTextureDesc& desc) SK_OVERRIDE;
    virtual GrRenderTarget* onCreatePlatformRenderTarget(
        const GrPlatformRenderTargetDesc& desc) SK_OVERRIDE;
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget* rt,
                                                    int width,
                                                    int height) SK_OVERRIDE;
    virtual bool attachStencilBufferToRenderTarget(
        GrStencilBuffer* sb,
        GrRenderTarget* rt) SK_OVERRIDE;

    virtual void onClear(const GrIRect* rect, GrColor color) SK_OVERRIDE;

    virtual void onForceRenderTargetFlush() SK_OVERRIDE;

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

    virtual void onResolveRenderTarget(GrRenderTarget* target) SK_OVERRIDE;

    virtual void onGpuDrawIndexed(GrPrimitiveType type,
                                  uint32_t startVertex,
                                  uint32_t startIndex,
                                  uint32_t vertexCount,
                                  uint32_t indexCount) SK_OVERRIDE;
    virtual void onGpuDrawNonIndexed(GrPrimitiveType type,
                                     uint32_t vertexCount,
                                     uint32_t numVertices) SK_OVERRIDE;

    virtual void setStencilPathSettings(const GrPath&,
                                        GrPathFill,
                                        GrStencilSettings* settings)
                                        SK_OVERRIDE;
    virtual void onGpuStencilPath(const GrPath*, GrPathFill) SK_OVERRIDE;

    virtual void clearStencil() SK_OVERRIDE;
    virtual void clearStencilClip(const GrIRect& rect,
                                  bool insideClip) SK_OVERRIDE;
    virtual bool flushGraphicsState(DrawType) SK_OVERRIDE;
    virtual void setupGeometry(int* startVertex,
                               int* startIndex,
                               int vertexCount,
                               int indexCount) SK_OVERRIDE;

private:

    const GrGLCaps& glCaps() const { return fGLContextInfo.caps(); }

    
    void setTextureUnit(int unitIdx);

    
    
    void setBuffers(bool indexed,
                    int* extraVertexOffset,
                    int* extraIndexOffset);

    
    
    
    
    void flushBlend(bool isLines,
                    GrBlendCoeff srcCoeff,
                    GrBlendCoeff dstCoeff);

    bool hasExtension(const char* ext) const {
        return fGLContextInfo.hasExtension(ext);
    }

    const GrGLContextInfo& glContextInfo() const { return fGLContextInfo; }

    
    static void AdjustTextureMatrix(const GrGLTexture* texture,
                                    GrMatrix* matrix);

    
    
    
    static bool TextureMatrixIsIdentity(const GrGLTexture* texture,
                                        const GrSamplerState& sampler);

    static bool BlendCoeffReferencesConstant(GrBlendCoeff coeff);

    
    typedef GrGLProgram::Desc        ProgramDesc;
    typedef ProgramDesc::StageDesc   StageDesc;

    class ProgramCache : public ::GrNoncopyable {
    public:
        ProgramCache(const GrGLContextInfo& gl);

        void abandon();
        GrGLProgram* getProgram(const GrGLProgram::Desc& desc, const GrCustomStage** stages);
    private:
        enum {
            kKeySize = sizeof(ProgramDesc),
            
            
            kMaxEntries = 32
        };

        class Entry;
        
        typedef GrTBinHashKey<Entry, kKeySize> ProgramHashKey;

        class Entry : public ::GrNoncopyable {
        public:
            Entry() : fProgram(NULL), fLRUStamp(0) {}
            Entry& operator = (const Entry& entry) {
                GrSafeRef(entry.fProgram.get());
                fProgram.reset(entry.fProgram.get());
                fKey = entry.fKey;
                fLRUStamp = entry.fLRUStamp;
                return *this;
            }
            int compare(const ProgramHashKey& key) const {
                return fKey.compare(key);
            }

        public:
            SkAutoTUnref<GrGLProgram>   fProgram;
            ProgramHashKey              fKey;
            unsigned int                fLRUStamp; 
        };

        GrTHashTable<Entry, ProgramHashKey, 8> fHashCache;

        Entry                       fEntries[kMaxEntries];
        int                         fCount;
        unsigned int                fCurrLRUStamp;
        const GrGLContextInfo&      fGL;
    };

    
    
    
    
    
    void flushBoundTextureAndParams(int stage);
    void flushBoundTextureAndParams(int stage,
                                    const GrTextureParams& params,
                                    GrGLTexture* nextTexture);

    
    void flushTextureMatrix(int stage);

    
    void flushColor(GrColor color);

    
    void flushCoverage(GrColor color);

    
    void flushViewMatrix(DrawType type);

    
    void flushRadial2(int stage);

    
    void flushConvolution(int stage);

    
    void flushColorMatrix();

    
    void flushMiscFixedFunctionState();

    
    
    void flushScissor();

    void buildProgram(bool isPoints,
                      BlendOptFlags blendOpts,
                      GrBlendCoeff dstCoeff,
                      const GrCustomStage** customStages,
                      ProgramDesc* desc);

    
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
    void flushStencil(DrawType);
    void flushAAState(DrawType);

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

    void fillInConfigRenderableTable();

    friend class GrGLVertexBuffer;
    friend class GrGLIndexBuffer;
    friend class GrGLTexture;
    friend class GrGLRenderTarget;

    GrGLContextInfo fGLContextInfo;

    
    ProgramCache*               fProgramCache;
    SkAutoTUnref<GrGLProgram>   fCurrentProgram;

    
    
    
    int                         fHWActiveTextureUnitIdx;
    GrGLuint                    fHWProgramID;
    GrColor                     fHWConstAttribColor;
    GrColor                     fHWConstAttribCoverage;

    enum TriState {
        kNo_TriState,
        kYes_TriState,
        kUnknown_TriState
    };

    
    struct {
        TriState    fEnabled;
        GrGLIRect   fRect;
        void invalidate() {
            fEnabled = kUnknown_TriState;
            fRect.invalidate();
        }
    } fHWScissorSettings;

    GrGLIRect   fHWViewport;

    struct {
        size_t                  fVertexOffset;
        GrVertexLayout          fVertexLayout;
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
        bool                    fArrayPtrsDirty;
    } fHWGeometryState;

    struct {
        GrBlendCoeff    fSrcCoeff;
        GrBlendCoeff    fDstCoeff;
        GrColor         fConstColor;
        bool            fConstColorValid;
        TriState        fEnabled;

        void invalidate() {
            fSrcCoeff = kInvalid_GrBlendCoeff;
            fDstCoeff = kInvalid_GrBlendCoeff;
            fConstColorValid = false;
            fEnabled = kUnknown_TriState;
        }
    } fHWBlendState;

    struct {
        TriState fMSAAEnabled;
        TriState fSmoothLineEnabled;
        void invalidate() {
            fMSAAEnabled = kUnknown_TriState;
            fSmoothLineEnabled = kUnknown_TriState;
        }
    } fHWAAState;

    struct {
        GrMatrix    fViewMatrix;
        SkISize     fRTSize;
        void invalidate() {
            fViewMatrix = GrMatrix::InvalidMatrix();
            fRTSize.fWidth = -1; 
        }
    } fHWPathMatrixState;

    GrStencilSettings       fHWStencilSettings;
    TriState                fHWStencilTestEnabled;

    GrDrawState::DrawFace   fHWDrawFace;
    TriState                fHWWriteToColor;
    TriState                fHWDitherEnabled;
    GrRenderTarget*         fHWBoundRenderTarget;
    GrTexture*              fHWBoundTextures[GrDrawState::kNumStages];
    

    
    
    int fLastSuccessfulStencilFmtIdx;

    bool fPrintedCaps;

    typedef GrGpu INHERITED;
};

#endif

