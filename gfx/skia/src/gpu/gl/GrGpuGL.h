








#ifndef GrGpuGL_DEFINED
#define GrGpuGL_DEFINED


#include "GrBinHashKey.h"
#include "GrDrawState.h"
#include "GrGpu.h"
#include "GrGLContext.h"
#include "GrGLIndexBuffer.h"
#include "GrGLIRect.h"
#include "GrGLProgram.h"
#include "GrGLStencilBuffer.h"
#include "GrGLTexture.h"
#include "GrGLVertexArray.h"
#include "GrGLVertexBuffer.h"
#include "../GrTHashCache.h"

#ifdef SK_DEVELOPER
#define PROGRAM_CACHE_STATS
#endif

class GrGpuGL : public GrGpu {
public:
    GrGpuGL(const GrGLContext& ctx, GrContext* context);
    virtual ~GrGpuGL();

    const GrGLInterface* glInterface() const { return fGLContext.interface(); }
    GrGLBinding glBinding() const { return fGLContext.info().binding(); }
    GrGLVersion glVersion() const { return fGLContext.info().version(); }
    GrGLSLGeneration glslGeneration() const { return fGLContext.info().glslGeneration(); }

    
    void bindTexture(int unitIdx, const GrTextureParams& params, GrGLTexture* texture);

    bool programUnitTest(int maxStages);

    
    virtual GrPixelConfig preferredReadPixelsConfig(GrPixelConfig config) const SK_OVERRIDE;
    virtual GrPixelConfig preferredWritePixelsConfig(GrPixelConfig config) const SK_OVERRIDE;
    virtual bool canWriteTexturePixels(const GrTexture*, GrPixelConfig srcConfig) const SK_OVERRIDE;
    virtual bool readPixelsWillPayForYFlip(
                                    GrRenderTarget* renderTarget,
                                    int left, int top,
                                    int width, int height,
                                    GrPixelConfig config,
                                    size_t rowBytes) const SK_OVERRIDE;
    virtual bool fullReadPixelsIsFasterThanPartial() const SK_OVERRIDE;

    virtual void abandonResources() SK_OVERRIDE;

    const GrGLCaps& glCaps() const { return *fGLContext.info().caps(); }

    
    
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
    void notifyTextureDelete(GrGLTexture* texture);
    void notifyRenderTargetDelete(GrRenderTarget* renderTarget);

private:
    
    virtual void onResetContext() SK_OVERRIDE;

    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) SK_OVERRIDE;
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size,
                                                 bool dynamic) SK_OVERRIDE;
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size,
                                               bool dynamic) SK_OVERRIDE;
    virtual GrPath* onCreatePath(const SkPath&) SK_OVERRIDE;
    virtual GrTexture* onWrapBackendTexture(const GrBackendTextureDesc&) SK_OVERRIDE;
    virtual GrRenderTarget* onWrapBackendRenderTarget(const GrBackendRenderTargetDesc&) SK_OVERRIDE;
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
                              size_t rowBytes) SK_OVERRIDE;

    virtual bool onWriteTexturePixels(GrTexture* texture,
                                      int left, int top, int width, int height,
                                      GrPixelConfig config, const void* buffer,
                                      size_t rowBytes) SK_OVERRIDE;

    virtual void onResolveRenderTarget(GrRenderTarget* target) SK_OVERRIDE;

    virtual void onGpuDraw(const DrawInfo&) SK_OVERRIDE;

    virtual void setStencilPathSettings(const GrPath&,
                                        SkPath::FillType,
                                        GrStencilSettings* settings)
                                        SK_OVERRIDE;
    virtual void onGpuStencilPath(const GrPath*, SkPath::FillType) SK_OVERRIDE;

    virtual void clearStencil() SK_OVERRIDE;
    virtual void clearStencilClip(const GrIRect& rect,
                                  bool insideClip) SK_OVERRIDE;
    virtual bool flushGraphicsState(DrawType, const GrDeviceCoordTexture* dstCopy) SK_OVERRIDE;

    
    void setTextureUnit(int unitIdx);

    
    
    
    void setupGeometry(const DrawInfo& info, size_t* indexOffsetInBytes);

    
    
    
    
    void flushBlend(bool isLines, GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff);

    bool hasExtension(const char* ext) const { return fGLContext.info().hasExtension(ext); }

    const GrGLContext& glContext() const { return fGLContext; }

    static bool BlendCoeffReferencesConstant(GrBlendCoeff coeff);

    class ProgramCache : public ::GrNoncopyable {
    public:
        ProgramCache(const GrGLContext& gl);
        ~ProgramCache();

        void abandon();
        GrGLProgram* getProgram(const GrGLProgramDesc& desc, const GrEffectStage* stages[]);
    private:
        enum {
            kKeySize = sizeof(GrGLProgramDesc),
            
            
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
        const GrGLContext&          fGL;
#ifdef PROGRAM_CACHE_STATS
        int                         fTotalRequests;
        int                         fCacheMisses;
#endif
    };

    
    void flushPathStencilMatrix();

    
    void flushMiscFixedFunctionState();

    
    
    void flushScissor();

    void initFSAASupport();

    
    void initStencilFormats();

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

    bool canReadBGRA() const;

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
        }

        void notifyVertexArrayDelete(GrGLuint id) {
            if (fBoundVertexArrayIDIsValid && fBoundVertexArrayID == id) {
                
                fBoundVertexArrayID = 0;
            }
        }

        void setVertexArrayID(GrGpuGL* gpu, GrGLuint arrayID) {
            if (!gpu->glCaps().vertexArrayObjectSupport()) {
                GrAssert(0 == arrayID);
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


    GrGLProgram::MatrixState    fHWPathStencilMatrixState;

    GrStencilSettings           fHWStencilSettings;
    TriState                    fHWStencilTestEnabled;

    GrDrawState::DrawFace       fHWDrawFace;
    TriState                    fHWWriteToColor;
    TriState                    fHWDitherEnabled;
    GrRenderTarget*             fHWBoundRenderTarget;
    GrTexture*                  fHWBoundTextures[GrDrawState::kNumStages];
    

    
    
    int fLastSuccessfulStencilFmtIdx;

    typedef GrGpu INHERITED;
};

#endif
