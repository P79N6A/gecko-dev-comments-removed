






#ifndef GrGpu_DEFINED
#define GrGpu_DEFINED

#include "GrDrawTarget.h"
#include "GrClipMaskManager.h"
#include "SkPath.h"

class GrContext;
class GrIndexBufferAllocPool;
class GrPath;
class GrPathRange;
class GrPathRenderer;
class GrPathRendererChain;
class GrStencilBuffer;
class GrVertexBufferAllocPool;

class GrGpu : public GrDrawTarget {
public:

    



    enum ExtendedBlendCoeffs {
        
        
        kS2C_GrBlendCoeff = kPublicGrBlendCoeffCount,
        kIS2C_GrBlendCoeff,
        kS2A_GrBlendCoeff,
        kIS2A_GrBlendCoeff,

        kTotalGrBlendCoeffCount
    };

    




    static GrGpu* Create(GrBackend, GrBackendContext, GrContext* context);

    

    GrGpu(GrContext* context);
    virtual ~GrGpu();

    GrContext* getContext() { return this->INHERITED::getContext(); }
    const GrContext* getContext() const { return this->INHERITED::getContext(); }

    





    void markContextDirty(uint32_t state = kAll_GrBackendState) {
        fResetBits |= state;
    }

    void unimpl(const char[]);

    



























    GrTexture* createTexture(const GrTextureDesc& desc,
                             const void* srcData, size_t rowBytes);

    


    GrTexture* wrapBackendTexture(const GrBackendTextureDesc&);

    


    GrRenderTarget* wrapBackendRenderTarget(const GrBackendRenderTargetDesc&);

    









    GrVertexBuffer* createVertexBuffer(size_t size, bool dynamic);

    









    GrIndexBuffer* createIndexBuffer(size_t size, bool dynamic);

    



    GrPath* createPath(const SkPath& path, const SkStrokeRec& stroke);

    




    GrPathRange* createPathRange(size_t size, const SkStrokeRec&);

    






    const GrIndexBuffer* getQuadIndexBuffer() const;

    


    void resolveRenderTarget(GrRenderTarget* target);

    




    virtual GrPixelConfig preferredReadPixelsConfig(GrPixelConfig readConfig,
                                                    GrPixelConfig surfaceConfig) const {
        return readConfig;
    }
    virtual GrPixelConfig preferredWritePixelsConfig(GrPixelConfig writeConfig,
                                                     GrPixelConfig surfaceConfig) const {
        return writeConfig;
    }

    



    virtual bool canWriteTexturePixels(const GrTexture*, GrPixelConfig srcConfig) const = 0;

    
















     virtual bool readPixelsWillPayForYFlip(GrRenderTarget* renderTarget,
                                            int left, int top,
                                            int width, int height,
                                            GrPixelConfig config,
                                            size_t rowBytes) const = 0;
     




     virtual bool fullReadPixelsIsFasterThanPartial() const { return false; };

    



















    bool readPixels(GrRenderTarget* renderTarget,
                    int left, int top, int width, int height,
                    GrPixelConfig config, void* buffer, size_t rowBytes);

    











    bool writeTexturePixels(GrTexture* texture,
                            int left, int top, int width, int height,
                            GrPixelConfig config, const void* buffer,
                            size_t rowBytes);

    



    virtual void abandonResources();

    



    void releaseResources();

    



    void insertObject(GrGpuResource* object);

    



    void removeObject(GrGpuResource* object);

    
    virtual void clear(const SkIRect* rect,
                       GrColor color,
                       bool canIgnoreRect,
                       GrRenderTarget* renderTarget = NULL) SK_OVERRIDE;

    virtual void purgeResources() SK_OVERRIDE {
        
        
        fClipMaskManager.releaseResources();
    }

    
    
    
    
    
    
    typedef uint64_t ResetTimestamp;

    
    static const ResetTimestamp kExpiredTimestamp = 0;
    
    
    
    ResetTimestamp getResetTimestamp() const {
        return fResetTimestamp;
    }

    





    void enableScissor(const SkIRect& rect) {
        fScissorState.fEnabled = true;
        fScissorState.fRect = rect;
    }
    void disableScissor() { fScissorState.fEnabled = false; }

    






    void setStencilSettings(const GrStencilSettings& settings) {
        fStencilSettings = settings;
    }
    void disableStencil() { fStencilSettings.setDisabled(); }

    
    
    
    virtual void clearStencilClip(const SkIRect& rect, bool insideClip) = 0;

    enum PrivateDrawStateStateBits {
        kFirstBit = (GrDrawState::kLastPublicStateBit << 1),

        kModifyStencilClip_StateBit = kFirstBit, 
                                                 
                                                 
    };

    void getPathStencilSettingsForFillType(SkPath::FillType fill, GrStencilSettings* outStencilSettings);

    enum DrawType {
        kDrawPoints_DrawType,
        kDrawLines_DrawType,
        kDrawTriangles_DrawType,
        kStencilPath_DrawType,
        kDrawPath_DrawType,
        kDrawPaths_DrawType,
    };

protected:
    DrawType PrimTypeToDrawType(GrPrimitiveType type) {
        switch (type) {
            case kTriangles_GrPrimitiveType:
            case kTriangleStrip_GrPrimitiveType:
            case kTriangleFan_GrPrimitiveType:
                return kDrawTriangles_DrawType;
            case kPoints_GrPrimitiveType:
                return kDrawPoints_DrawType;
            case kLines_GrPrimitiveType:
            case kLineStrip_GrPrimitiveType:
                return kDrawLines_DrawType;
            default:
                SkFAIL("Unexpected primitive type");
                return kDrawTriangles_DrawType;
        }
    }

    
    bool setupClipAndFlushState(DrawType,
                                const GrDeviceCoordTexture* dstCopy,
                                GrDrawState::AutoRestoreEffects* are,
                                const SkRect* devBounds);

    
    
    static GrStencilFunc ConvertStencilFunc(bool stencilInClip,
                                            GrStencilFunc func);
    static void ConvertStencilFuncAndMask(GrStencilFunc func,
                                          bool clipInStencil,
                                          unsigned int clipBit,
                                          unsigned int userBits,
                                          unsigned int* ref,
                                          unsigned int* mask);

    GrClipMaskManager           fClipMaskManager;

    struct GeometryPoolState {
        const GrVertexBuffer* fPoolVertexBuffer;
        int                   fPoolStartVertex;

        const GrIndexBuffer*  fPoolIndexBuffer;
        int                   fPoolStartIndex;
    };
    const GeometryPoolState& getGeomPoolState() {
        return fGeomPoolStateStack.back();
    }

    
    struct ScissorState {
        bool    fEnabled;
        SkIRect fRect;
    } fScissorState;

    
    GrStencilSettings fStencilSettings;

    
    void finalizeReservedVertices();
    void finalizeReservedIndices();

private:
    
    virtual bool onReserveVertexSpace(size_t vertexSize, int vertexCount, void** vertices) SK_OVERRIDE;
    virtual bool onReserveIndexSpace(int indexCount, void** indices) SK_OVERRIDE;
    virtual void releaseReservedVertexSpace() SK_OVERRIDE;
    virtual void releaseReservedIndexSpace() SK_OVERRIDE;
    virtual void onSetVertexSourceToArray(const void* vertexArray, int vertexCount) SK_OVERRIDE;
    virtual void onSetIndexSourceToArray(const void* indexArray, int indexCount) SK_OVERRIDE;
    virtual void releaseVertexArray() SK_OVERRIDE;
    virtual void releaseIndexArray() SK_OVERRIDE;
    virtual void geometrySourceWillPush() SK_OVERRIDE;
    virtual void geometrySourceWillPop(const GeometrySrcState& restoredState) SK_OVERRIDE;


    
    
    virtual void onResetContext(uint32_t resetBits) = 0;

    
    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) = 0;
    virtual GrTexture* onCreateCompressedTexture(const GrTextureDesc& desc,
                                                 const void* srcData) = 0;
    virtual GrTexture* onWrapBackendTexture(const GrBackendTextureDesc&) = 0;
    virtual GrRenderTarget* onWrapBackendRenderTarget(const GrBackendRenderTargetDesc&) = 0;
    virtual GrVertexBuffer* onCreateVertexBuffer(size_t size, bool dynamic) = 0;
    virtual GrIndexBuffer* onCreateIndexBuffer(size_t size, bool dynamic) = 0;
    virtual GrPath* onCreatePath(const SkPath& path, const SkStrokeRec&) = 0;
    virtual GrPathRange* onCreatePathRange(size_t size, const SkStrokeRec&) = 0;

    
    
    
    virtual void onClear(const SkIRect* rect, GrColor color, bool canIgnoreRect) = 0;

    
    virtual void onGpuDraw(const DrawInfo&) = 0;

    
    virtual void onGpuStencilPath(const GrPath*, SkPath::FillType) = 0;
    virtual void onGpuDrawPath(const GrPath*, SkPath::FillType) = 0;
    virtual void onGpuDrawPaths(const GrPathRange*,
                                const uint32_t indices[], int count,
                                const float transforms[], PathTransformType,
                                SkPath::FillType) = 0;

    
    virtual bool onReadPixels(GrRenderTarget* target,
                              int left, int top, int width, int height,
                              GrPixelConfig,
                              void* buffer,
                              size_t rowBytes) = 0;

    
    virtual bool onWriteTexturePixels(GrTexture* texture,
                                      int left, int top, int width, int height,
                                      GrPixelConfig config, const void* buffer,
                                      size_t rowBytes) = 0;

    
    virtual void onResolveRenderTarget(GrRenderTarget* target) = 0;

    
    
    
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget*, int width, int height) = 0;

    
    virtual bool attachStencilBufferToRenderTarget(GrStencilBuffer*, GrRenderTarget*) = 0;

    
    
    
    
    virtual bool flushGraphicsState(DrawType, const GrDeviceCoordTexture* dstCopy) = 0;

    
    virtual void clearStencil() = 0;

    
    bool attachStencilBufferToRenderTarget(GrRenderTarget* target);

    
    virtual void onDraw(const DrawInfo&) SK_OVERRIDE;
    virtual void onStencilPath(const GrPath*, SkPath::FillType) SK_OVERRIDE;
    virtual void onDrawPath(const GrPath*, SkPath::FillType,
                            const GrDeviceCoordTexture* dstCopy) SK_OVERRIDE;
    virtual void onDrawPaths(const GrPathRange*,
                             const uint32_t indices[], int count,
                             const float transforms[], PathTransformType,
                             SkPath::FillType, const GrDeviceCoordTexture*) SK_OVERRIDE;

    
    void prepareVertexPool();
    void prepareIndexPool();

    void resetContext() {
        
        
        
        fClipMaskManager.invalidateStencilMask();
        this->onResetContext(fResetBits);
        fResetBits = 0;
        ++fResetTimestamp;
    }

    void handleDirtyContext() {
        if (fResetBits) {
            this->resetContext();
        }
    }

    enum {
        kPreallocGeomPoolStateStackCnt = 4,
    };
    typedef SkTInternalLList<GrGpuResource> ObjectList;
    SkSTArray<kPreallocGeomPoolStateStackCnt, GeometryPoolState, true>  fGeomPoolStateStack;
    ResetTimestamp                                                      fResetTimestamp;
    uint32_t                                                            fResetBits;
    GrVertexBufferAllocPool*                                            fVertexPool;
    GrIndexBufferAllocPool*                                             fIndexPool;
    
    int                                                                 fVertexPoolUseCnt;
    int                                                                 fIndexPoolUseCnt;
    
    mutable GrIndexBuffer*                                              fQuadIndexBuffer;
    
    
    ObjectList                                                          fObjectList;

    typedef GrDrawTarget INHERITED;
};

#endif
