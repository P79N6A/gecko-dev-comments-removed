








#ifndef GrGpu_DEFINED
#define GrGpu_DEFINED

#include "GrDrawTarget.h"
#include "GrRect.h"
#include "GrRefCnt.h"
#include "GrTexture.h"

class GrContext;
class GrIndexBufferAllocPool;
class GrPathRenderer;
class GrPathRendererChain;
class GrResource;
class GrStencilBuffer;
class GrVertexBufferAllocPool;




struct GrGpuStats {
    uint32_t fVertexCnt;  
    uint32_t fIndexCnt;   
    uint32_t fDrawCnt;    

    uint32_t fProgChngCnt;

    


    uint32_t fTextureChngCnt;
    


    uint32_t fRenderTargetChngCnt;
    


    uint32_t fTextureCreateCnt;
    


    uint32_t fRenderTargetCreateCnt;
};

class GrGpu : public GrDrawTarget {

public:

    



    enum ExtendedBlendCoeffs {
        
        
        kS2C_BlendCoeff = kPublicBlendCoeffCount,
        kIS2C_BlendCoeff,
        kS2A_BlendCoeff,
        kIS2A_BlendCoeff,

        kTotalBlendCoeffCount
    };

    




    static GrGpu* Create(GrEngine, GrPlatform3DContext context3D);

    

    GrGpu();
    virtual ~GrGpu();

    
    void setContext(GrContext* context) {
        GrAssert(NULL == fContext); 
        fContext = context;
    }
    GrContext* getContext() { return fContext; }
    const GrContext* getContext() const { return fContext; }

    





    void markContextDirty() { fContextIsDirty = true; }

    void unimpl(const char[]);

    

















    GrTexture* createTexture(const GrTextureDesc& desc,
                             const void* srcData, size_t rowBytes);

    GrResource* createPlatformSurface(const GrPlatformSurfaceDesc& desc);

    









    GrVertexBuffer* createVertexBuffer(uint32_t size, bool dynamic);

    









    GrIndexBuffer* createIndexBuffer(uint32_t size, bool dynamic);

    






    const GrIndexBuffer* getQuadIndexBuffer() const;

    




    const GrVertexBuffer* getUnitSquareVertexBuffer() const;

    




    void forceRenderTargetFlush();

    














    bool readPixels(GrRenderTarget* renderTarget,
                    int left, int top, int width, int height,
                    GrPixelConfig config, void* buffer);

    const GrGpuStats& getStats() const;
    void resetStats();
    void printStats() const;

    



    virtual void abandonResources();

    



    void releaseResources();

    



    void insertResource(GrResource* resource);

    




    void removeResource(GrResource* resource);

    
    virtual void clear(const GrIRect* rect, GrColor color);

protected:
    enum PrivateStateBits {
        kFirstBit = (kLastPublicStateBit << 1),

        kModifyStencilClip_StateBit = kFirstBit, 
                                                 
                                                 
    };

    
    
    bool    fClipInStencil;

    
    bool setupClipAndFlushState(GrPrimitiveType type);

    
    
    static GrStencilFunc ConvertStencilFunc(bool stencilInClip,
                                            GrStencilFunc func);
    static void ConvertStencilFuncAndMask(GrStencilFunc func,
                                          bool clipInStencil,
                                          unsigned int clipBit,
                                          unsigned int userBits,
                                          unsigned int* ref,
                                          unsigned int* mask);

    
    
    static const GrStencilSettings gClipStencilSettings;


    GrGpuStats fStats;

    struct GeometryPoolState {
        const GrVertexBuffer* fPoolVertexBuffer;
        int                   fPoolStartVertex;
        
        const GrIndexBuffer*  fPoolIndexBuffer;
        int                   fPoolStartIndex;
    };
    const GeometryPoolState& getGeomPoolState() { 
        return fGeomPoolStateStack.back(); 
    }

    
    virtual bool onReserveVertexSpace(GrVertexLayout vertexLayout,
                                      int vertexCount,
                                      void** vertices);
    virtual bool onReserveIndexSpace(int indexCount, void** indices);
    virtual void releaseReservedVertexSpace();
    virtual void releaseReservedIndexSpace();    
    virtual void onSetVertexSourceToArray(const void* vertexArray,
                                          int vertexCount);
    virtual void onSetIndexSourceToArray(const void* indexArray,
                                         int indexCount);
    virtual void releaseVertexArray();
    virtual void releaseIndexArray();
    virtual void geometrySourceWillPush();
    virtual void geometrySourceWillPop(const GeometrySrcState& restoredState);

    
    void finalizeReservedVertices();
    void finalizeReservedIndices();

    
    
    virtual void resetContext() = 0;

    
    virtual GrTexture* onCreateTexture(const GrTextureDesc& desc,
                                       const void* srcData,
                                       size_t rowBytes) = 0;
    virtual GrResource* onCreatePlatformSurface(const GrPlatformSurfaceDesc& desc) = 0;
    virtual GrVertexBuffer* onCreateVertexBuffer(uint32_t size,
                                                 bool dynamic) = 0;
    virtual GrIndexBuffer* onCreateIndexBuffer(uint32_t size,
                                               bool dynamic) = 0;

    
    
    virtual void onClear(const GrIRect* rect, GrColor color) = 0;

    
    virtual void onGpuDrawIndexed(GrPrimitiveType type,
                                  uint32_t startVertex,
                                  uint32_t startIndex,
                                  uint32_t vertexCount,
                                  uint32_t indexCount) = 0;

    virtual void onGpuDrawNonIndexed(GrPrimitiveType type,
                                     uint32_t vertexCount,
                                     uint32_t numVertices) = 0;

    
    virtual void onForceRenderTargetFlush() = 0;

    
    virtual bool onReadPixels(GrRenderTarget* target,
                              int left, int top, int width, int height,
                              GrPixelConfig, void* buffer) = 0;

    
    
    
    virtual void setupGeometry(int* startVertex,
                               int* startIndex,
                               int vertexCount,
                               int indexCount) = 0;

    
    
    
    virtual bool createStencilBufferForRenderTarget(GrRenderTarget* rt,
                                                    int width, 
                                                    int height) = 0;

    
    virtual bool attachStencilBufferToRenderTarget(GrStencilBuffer* sb,
                                                   GrRenderTarget* rt) = 0;

    
    
    
    
    virtual bool flushGraphicsState(GrPrimitiveType type) = 0;

    
    virtual void flushScissor(const GrIRect* rect) = 0;

    
    
    
    virtual void clearStencilClip(const GrIRect& rect, bool insideClip) = 0;

    
    virtual void clearStencil() = 0;

private:
    GrContext*                  fContext; 

    GrVertexBufferAllocPool*    fVertexPool;

    GrIndexBufferAllocPool*     fIndexPool;
    
    
    int                         fVertexPoolUseCnt;
    int                         fIndexPoolUseCnt;
    
    enum {
        kPreallocGeomPoolStateStackCnt = 4,
    };
    SkSTArray<kPreallocGeomPoolStateStackCnt,
              GeometryPoolState, true>              fGeomPoolStateStack;
    
    mutable GrIndexBuffer*      fQuadIndexBuffer; 
                                                  

    mutable GrVertexBuffer*     fUnitSquareVertexBuffer; 
                                                         

    
    
    GrPathRendererChain*        fPathRendererChain;

    bool                        fContextIsDirty;

    GrResource*                 fResourceHead;

    
    bool attachStencilBufferToRenderTarget(GrRenderTarget* target);

    
    virtual void onDrawIndexed(GrPrimitiveType type,
                               int startVertex,
                               int startIndex,
                               int vertexCount,
                               int indexCount);
    virtual void onDrawNonIndexed(GrPrimitiveType type,
                                  int startVertex,
                                  int vertexCount);

    
    void prepareVertexPool();
    void prepareIndexPool();

    
    GrPathRenderer* getClipPathRenderer(const SkPath& path, GrPathFill fill);

    void handleDirtyContext() {
        if (fContextIsDirty) {
            this->resetContext();
            fContextIsDirty = false;
        }
    }

    typedef GrDrawTarget INHERITED;
};

#endif
