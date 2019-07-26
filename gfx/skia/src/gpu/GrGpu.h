








#ifndef GrGpu_DEFINED
#define GrGpu_DEFINED

#include "GrDrawTarget.h"
#include "GrRect.h"
#include "GrRefCnt.h"
#include "GrClipMaskManager.h"

#include "SkPath.h"

class GrContext;
class GrIndexBufferAllocPool;
class GrPath;
class GrPathRenderer;
class GrPathRendererChain;
class GrResource;
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

    





    void markContextDirty() { fContextIsDirty = true; }

    void unimpl(const char[]);

    

















    GrTexture* createTexture(const GrTextureDesc& desc,
                             const void* srcData, size_t rowBytes);

    


    GrTexture* wrapBackendTexture(const GrBackendTextureDesc&);

    


    GrRenderTarget* wrapBackendRenderTarget(const GrBackendRenderTargetDesc&);

    









    GrVertexBuffer* createVertexBuffer(uint32_t size, bool dynamic);

    









    GrIndexBuffer* createIndexBuffer(uint32_t size, bool dynamic);

    



    GrPath* createPath(const SkPath& path);

    






    const GrIndexBuffer* getQuadIndexBuffer() const;

    




    const GrVertexBuffer* getUnitSquareVertexBuffer() const;

    


    void resolveRenderTarget(GrRenderTarget* target);

    




    void forceRenderTargetFlush();

    



    virtual GrPixelConfig preferredReadPixelsConfig(GrPixelConfig config) const { return config; }
    virtual GrPixelConfig preferredWritePixelsConfig(GrPixelConfig config) const { return config; }

    



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

    



    void insertResource(GrResource* resource);

    




    void removeResource(GrResource* resource);

    
    virtual void clear(const GrIRect* rect,
                       GrColor color,
                       GrRenderTarget* renderTarget = NULL) SK_OVERRIDE;

    virtual void purgeResources() SK_OVERRIDE {
        
        
        fClipMaskManager.releaseResources();
    }

    
    
    
    
    
    
    typedef uint64_t ResetTimestamp;

    
    static const ResetTimestamp kExpiredTimestamp = 0;
    
    
    
    ResetTimestamp getResetTimestamp() const {
        return fResetTimestamp;
    }

    


    bool isConfigRenderable(GrPixelConfig config) const {
        GrAssert(kGrPixelConfigCnt > config);
        return fConfigRenderSupport[config];
    }

    





    void enableScissor(const GrIRect& rect) {
        fScissorState.fEnabled = true;
        fScissorState.fRect = rect;
    }
    void disableScissor() { fScissorState.fEnabled = false; }

    






    void setStencilSettings(const GrStencilSettings& settings) {
        fStencilSettings = settings;
    }
    void disableStencil() { fStencilSettings.setDisabled(); }

    
    
    
    virtual void clearStencilClip(const GrIRect& rect, bool insideClip) = 0;

    enum PrivateDrawStateStateBits {
        kFirstBit = (GrDrawState::kLastPublicStateBit << 1),

        kModifyStencilClip_StateBit = kFirstBit, 
                                                 
                                                 
    };

protected:
    enum DrawType {
        kDrawPoints_DrawType,
        kDrawLines_DrawType,
        kDrawTriangles_DrawType,
        kStencilPath_DrawType,
    };

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
                GrCrash("Unexpected primitive type");
                return kDrawTriangles_DrawType;
        }
    }

    
    bool setupClipAndFlushState(DrawType, const GrDeviceCoordTexture* dstCopy);

    
    
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
        GrIRect fRect;
    } fScissorState;

    
    GrStencilSettings fStencilSettings;

    
    
    bool    fConfigRenderSupport[kGrPixelConfigCnt];

    
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


    
    
    virtual void onResetContext() = 0;

    
    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) = 0;
    virtual GrTexture* onWrapBackendTexture(const GrBackendTextureDesc&) = 0;
    virtual GrRenderTarget* onWrapBackendRenderTarget(const GrBackendRenderTargetDesc&) = 0;
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size, bool dynamic) = 0;
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size, bool dynamic) = 0;
    virtual GrPath* onCreatePath(const SkPath& path) = 0;

    
    
    virtual void onClear(const GrIRect* rect, GrColor color) = 0;

    
    virtual void onGpuDraw(const DrawInfo&) = 0;
    
    
    
    
    virtual void setStencilPathSettings(const GrPath&,
                                        SkPath::FillType,
                                        GrStencilSettings* settings) = 0;
    
    virtual void onGpuStencilPath(const GrPath*, SkPath::FillType) = 0;

    
    virtual void onForceRenderTargetFlush() = 0;

    
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
    virtual void onStencilPath(const GrPath* path, const SkStrokeRec& stroke,
                               SkPath::FillType) SK_OVERRIDE;

    
    void prepareVertexPool();
    void prepareIndexPool();

    void resetContext() {
        
        
        
        fClipMaskManager.invalidateStencilMask();
        this->onResetContext();
        ++fResetTimestamp;
    }

    void handleDirtyContext() {
        if (fContextIsDirty) {
            this->resetContext();
            fContextIsDirty = false;
        }
    }

    enum {
        kPreallocGeomPoolStateStackCnt = 4,
    };
    typedef SkTInternalLList<GrResource> ResourceList;
    SkSTArray<kPreallocGeomPoolStateStackCnt, GeometryPoolState, true>  fGeomPoolStateStack;
    ResetTimestamp                                                      fResetTimestamp;
    GrVertexBufferAllocPool*                                            fVertexPool;
    GrIndexBufferAllocPool*                                             fIndexPool;
    
    int                                                                 fVertexPoolUseCnt;
    int                                                                 fIndexPoolUseCnt;
    
    mutable GrVertexBuffer*                                             fUnitSquareVertexBuffer;
    mutable GrIndexBuffer*                                              fQuadIndexBuffer;
    bool                                                                fContextIsDirty;
    ResourceList                                                        fResourceList;

    typedef GrDrawTarget INHERITED;
};

#endif
