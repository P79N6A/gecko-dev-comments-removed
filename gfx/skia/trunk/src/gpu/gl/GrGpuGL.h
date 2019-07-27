






#ifndef GrGpuGL_DEFINED
#define GrGpuGL_DEFINED

#include "GrBinHashKey.h"
#include "GrDrawState.h"
#include "GrGLContext.h"
#include "GrGLIRect.h"
#include "GrGLIndexBuffer.h"
#include "GrGLProgram.h"
#include "GrGLStencilBuffer.h"
#include "GrGLTexture.h"
#include "GrGLVertexArray.h"
#include "GrGLVertexBuffer.h"
#include "GrGpu.h"
#include "SkTypes.h"

#ifdef SK_DEVELOPER
#define PROGRAM_CACHE_STATS
#endif

class GrGLNameAllocator;

class GrGpuGL : public GrGpu {
public:
    GrGpuGL(const GrGLContext& ctx, GrContext* context);
    virtual ~GrGpuGL();

    const GrGLContext& glContext() const { return fGLContext; }

    const GrGLInterface* glInterface() const { return fGLContext.interface(); }
    const GrGLContextInfo& ctxInfo() const { return fGLContext; }
    GrGLStandard glStandard() const { return fGLContext.standard(); }
    GrGLVersion glVersion() const { return fGLContext.version(); }
    GrGLSLGeneration glslGeneration() const { return fGLContext.glslGeneration(); }
    const GrGLCaps& glCaps() const { return *fGLContext.caps(); }

    virtual void discard(GrRenderTarget*) SK_OVERRIDE;

    
    
    void bindTexture(int unitIdx, const GrTextureParams& params, GrGLTexture* texture);
    void setProjectionMatrix(const SkMatrix& matrix,
                             const SkISize& renderTargetSize,
                             GrSurfaceOrigin renderTargetOrigin);
    enum PathTexGenComponents {
        kS_PathTexGenComponents = 1,
        kST_PathTexGenComponents = 2,
        kSTR_PathTexGenComponents = 3
    };
    void enablePathTexGen(int unitIdx, PathTexGenComponents, const GrGLfloat* coefficients);
    void enablePathTexGen(int unitIdx, PathTexGenComponents, const SkMatrix& matrix);
    void flushPathTexGenSettings(int numUsedTexCoordSets);
    bool shouldUseFixedFunctionTexturing() const {
        return this->glCaps().pathRenderingSupport();
    }

    bool programUnitTest(int maxStages);

    
    virtual GrPixelConfig preferredReadPixelsConfig(GrPixelConfig readConfig,
                                                    GrPixelConfig surfaceConfig) const SK_OVERRIDE;
    virtual GrPixelConfig preferredWritePixelsConfig(GrPixelConfig writeConfig,
                                                     GrPixelConfig surfaceConfig) const SK_OVERRIDE;
    virtual bool canWriteTexturePixels(const GrTexture*, GrPixelConfig srcConfig) const SK_OVERRIDE;
    virtual bool readPixelsWillPayForYFlip(
                                    GrRenderTarget* renderTarget,
                                    int left, int top,
                                    int width, int height,
                                    GrPixelConfig config,
                                    size_t rowBytes) const SK_OVERRIDE;
    virtual bool fullReadPixelsIsFasterThanPartial() const SK_OVERRIDE;

    virtual void initCopySurfaceDstDesc(const GrSurface* src, GrTextureDesc* desc) SK_OVERRIDE;

    virtual void abandonResources() SK_OVERRIDE;

    
    
    void bindVertexArray(GrGLuint id) {
        fHWGeometryState.setVertexArrayID(this, id);
    }
    void bindIndexBufferAndDefaultVertexArray(GrGLuint id) {
        fHWGeometryState.setIndexBufferIDOnDefaultVertexArray(this, id);
    }
    void bindVertexBuffer(GrGLuint id) {
        fHWGeometryState.setVertexBufferID(this, id);
    }

    
    
    void notifyVertexArrayDelete(GrGLuint id) {
        fHWGeometryState.notifyVertexArrayDelete(id);
    }
    void notifyVertexBufferDelete(GrGLuint id) {
        fHWGeometryState.notifyVertexBufferDelete(id);
    }
    void notifyIndexBufferDelete(GrGLuint id) {
        fHWGeometryState.notifyIndexBufferDelete(id);
    }

    
    
    GrGLuint createGLPathObject();
    void deleteGLPathObject(GrGLuint);

protected:
    virtual bool onCopySurface(GrSurface* dst,
                               GrSurface* src,
                               const SkIRect& srcRect,
                               const SkIPoint& dstPoint) SK_OVERRIDE;

    virtual bool onCanCopySurface(GrSurface* dst,
                                  GrSurface* src,
                                  const SkIRect& srcRect,
                                  const SkIPoint& dstPoint) SK_OVERRIDE;

private:
    
    virtual void onResetContext(uint32_t resetBits) SK_OVERRIDE;

    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) SK_OVERRIDE;
    virtual GrTexture* onCreateCompressedTexture(const GrTextureDesc& desc,
                                                 const void* srcData) SK_OVERRIDE;
    virtual GrVertexBuffer* onCreateVertexBuffer(size_t size, bool dynamic) SK_OVERRIDE;
    virtual GrIndexBuffer* onCreateIndexBuffer(size_t size, bool dynamic) SK_OVERRIDE;
    virtual GrPath* onCreatePath(const SkPath&, const SkStrokeRec&) SK_OVERRIDE;
    virtual GrPathRange* onCreatePathRange(size_t size, const SkStrokeRec&) SK_OVERRIDE;
    virtual GrTexture* onWrapBackendTexture(const GrBackendTextureDesc&) SK_OVERRIDE;
    virtual GrRenderTarget* onWrapBackendRenderTarget(const GrBackendRenderTargetDesc&) SK_OVERRIDE;
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget* rt,
                                                    int width,
                                                    int height) SK_OVERRIDE;
    virtual bool attachStencilBufferToRenderTarget(
        GrStencilBuffer* sb,
        GrRenderTarget* rt) SK_OVERRIDE;

    virtual void onClear(const SkIRect* rect, GrColor color, bool canIgnoreRect) SK_OVERRIDE;

    virtual bool onReadPixels(GrRenderTarget* target,
                              int left, int top,
                              int width, int height,
                              GrPixelConfig,
                              void* buffer,
                              size_t rowBytes) SK_OVERRIDE;

    virtual bool onWriteTexturePixels(GrTexture* texture,
                                      int left, int top, int width, int height,
                                      GrPixelConfig config, const void* buffer,
                                      size_t rowBytes) SK_OVERRIDE;

    virtual void onResolveRenderTarget(GrRenderTarget* target) SK_OVERRIDE;

    virtual void onGpuDraw(const DrawInfo&) SK_OVERRIDE;

    virtual void onGpuStencilPath(const GrPath*, SkPath::FillType) SK_OVERRIDE;
    virtual void onGpuDrawPath(const GrPath*, SkPath::FillType) SK_OVERRIDE;
    virtual void onGpuDrawPaths(const GrPathRange*,
                                const uint32_t indices[], int count,
                                const float transforms[], PathTransformType,
                                SkPath::FillType) SK_OVERRIDE;

    virtual void clearStencil() SK_OVERRIDE;
    virtual void clearStencilClip(const SkIRect& rect,
                                  bool insideClip) SK_OVERRIDE;
    virtual bool flushGraphicsState(DrawType, const GrDeviceCoordTexture* dstCopy) SK_OVERRIDE;

    
    virtual void didAddGpuTraceMarker() SK_OVERRIDE;
    virtual void didRemoveGpuTraceMarker() SK_OVERRIDE;

    
    void setTextureUnit(int unitIdx);

    
    
    
    void setupGeometry(const DrawInfo& info, size_t* indexOffsetInBytes);

    
    
    
    
    void flushBlend(bool isLines, GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff);

    bool hasExtension(const char* ext) const { return fGLContext.hasExtension(ext); }

    static bool BlendCoeffReferencesConstant(GrBlendCoeff coeff);

    class ProgramCache : public ::SkNoncopyable {
    public:
        ProgramCache(GrGpuGL* gpu);
        ~ProgramCache();

        void abandon();
        GrGLProgram* getProgram(const GrGLProgramDesc& desc,
                                const GrEffectStage* colorStages[],
                                const GrEffectStage* coverageStages[]);

    private:
        enum {
            
            
            kMaxEntries = 128,
            kHashBits = 6,
        };

        struct Entry;

        struct ProgDescLess;

        
        
        int search(const GrGLProgramDesc& desc) const;

        
        Entry*                      fEntries[kMaxEntries];
        
        
        Entry*                      fHashTable[1 << kHashBits];

        int                         fCount;
        unsigned int                fCurrLRUStamp;
        GrGpuGL*                    fGpu;
#ifdef PROGRAM_CACHE_STATS
        int                         fTotalRequests;
        int                         fCacheMisses;
        int                         fHashMisses; 
#endif
    };

    
    void flushMiscFixedFunctionState();

    
    
    void flushScissor();

    void initFSAASupport();

    
    void initStencilFormats();

    
    
    void setScratchTextureUnit();

    
    
    void flushRenderTarget(const SkIRect* bound);
    void flushStencil(DrawType);
    void flushAAState(DrawType);
    void flushPathStencilSettings(SkPath::FillType fill);

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

    
    
    
    
    
    
    bool uploadCompressedTexData(const GrGLTexture::Desc& desc,
                                 const void* data,
                                 bool isNewTexture = true,
                                 int left = 0, int top = 0,
                                 int width = -1, int height = -1);

    bool createRenderTargetObjects(int width, int height,
                                   GrGLuint texID,
                                   GrGLRenderTarget::Desc* desc);

    GrGLContext fGLContext;

    
    ProgramCache*               fProgramCache;
    SkAutoTUnref<GrGLProgram>   fCurrentProgram;

    
    
    
    int                         fHWActiveTextureUnitIdx;
    GrGLuint                    fHWProgramID;

    GrGLProgram::SharedGLState  fSharedGLProgramState;

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

    


    class HWGeometryState {
    public:
        HWGeometryState() { fVBOVertexArray = NULL; this->invalidate(); }

        ~HWGeometryState() { SkSafeUnref(fVBOVertexArray); }

        void invalidate() {
            fBoundVertexArrayIDIsValid = false;
            fBoundVertexBufferIDIsValid = false;
            fDefaultVertexArrayBoundIndexBufferID = false;
            fDefaultVertexArrayBoundIndexBufferIDIsValid = false;
            fDefaultVertexArrayAttribState.invalidate();
            if (NULL != fVBOVertexArray) {
                fVBOVertexArray->invalidateCachedState();
            }
        }

        void notifyVertexArrayDelete(GrGLuint id) {
            if (fBoundVertexArrayIDIsValid && fBoundVertexArrayID == id) {
                
                fBoundVertexArrayID = 0;
            }
        }

        void setVertexArrayID(GrGpuGL* gpu, GrGLuint arrayID) {
            if (!gpu->glCaps().vertexArrayObjectSupport()) {
                SkASSERT(0 == arrayID);
                return;
            }
            if (!fBoundVertexArrayIDIsValid || arrayID != fBoundVertexArrayID) {
                GR_GL_CALL(gpu->glInterface(), BindVertexArray(arrayID));
                fBoundVertexArrayIDIsValid = true;
                fBoundVertexArrayID = arrayID;
            }
        }

        void notifyVertexBufferDelete(GrGLuint id) {
            if (fBoundVertexBufferIDIsValid && id == fBoundVertexBufferID) {
                fBoundVertexBufferID = 0;
            }
            if (NULL != fVBOVertexArray) {
                fVBOVertexArray->notifyVertexBufferDelete(id);
            }
            fDefaultVertexArrayAttribState.notifyVertexBufferDelete(id);
        }

        void notifyIndexBufferDelete(GrGLuint id) {
            if (fDefaultVertexArrayBoundIndexBufferIDIsValid &&
                id == fDefaultVertexArrayBoundIndexBufferID) {
                fDefaultVertexArrayBoundIndexBufferID = 0;
            }
            if (NULL != fVBOVertexArray) {
                fVBOVertexArray->notifyIndexBufferDelete(id);
            }
        }

        void setVertexBufferID(GrGpuGL* gpu, GrGLuint id) {
            if (!fBoundVertexBufferIDIsValid || id != fBoundVertexBufferID) {
                GR_GL_CALL(gpu->glInterface(), BindBuffer(GR_GL_ARRAY_BUFFER, id));
                fBoundVertexBufferIDIsValid = true;
                fBoundVertexBufferID = id;
            }
        }

        



        void setIndexBufferIDOnDefaultVertexArray(GrGpuGL* gpu, GrGLuint id) {
            this->setVertexArrayID(gpu, 0);
            if (!fDefaultVertexArrayBoundIndexBufferIDIsValid ||
                id != fDefaultVertexArrayBoundIndexBufferID) {
                GR_GL_CALL(gpu->glInterface(), BindBuffer(GR_GL_ELEMENT_ARRAY_BUFFER, id));
                fDefaultVertexArrayBoundIndexBufferIDIsValid = true;
                fDefaultVertexArrayBoundIndexBufferID = id;
            }
        }

        





        GrGLAttribArrayState* bindArrayAndBuffersToDraw(GrGpuGL* gpu,
                                                        const GrGLVertexBuffer* vbuffer,
                                                        const GrGLIndexBuffer* ibuffer);

    private:
        GrGLuint                fBoundVertexArrayID;
        GrGLuint                fBoundVertexBufferID;
        bool                    fBoundVertexArrayIDIsValid;
        bool                    fBoundVertexBufferIDIsValid;

        GrGLuint                fDefaultVertexArrayBoundIndexBufferID;
        bool                    fDefaultVertexArrayBoundIndexBufferIDIsValid;
        
        
        
        GrGLAttribArrayState    fDefaultVertexArrayAttribState;

        
        GrGLVertexArray*        fVBOVertexArray;
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


    GrGLProgram::MatrixState    fHWProjectionMatrixState;

    GrStencilSettings           fHWStencilSettings;
    TriState                    fHWStencilTestEnabled;
    GrStencilSettings           fHWPathStencilSettings;

    GrDrawState::DrawFace       fHWDrawFace;
    TriState                    fHWWriteToColor;
    TriState                    fHWDitherEnabled;
    uint32_t                    fHWBoundRenderTargetUniqueID;
    SkTArray<uint32_t, true>    fHWBoundTextureUniqueIDs;

    struct PathTexGenData {
        GrGLenum  fMode;
        GrGLint   fNumComponents;
        GrGLfloat fCoefficients[3 * 3];
    };
    int                         fHWActivePathTexGenSets;
    SkTArray<PathTexGenData, true>  fHWPathTexGenSettings;
    

    
    
    int fLastSuccessfulStencilFmtIdx;

    SkAutoTDelete<GrGLNameAllocator> fPathNameAllocator;

    typedef GrGpu INHERITED;
};

#endif
