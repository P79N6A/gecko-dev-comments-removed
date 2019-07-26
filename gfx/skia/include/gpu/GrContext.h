








#ifndef GrContext_DEFINED
#define GrContext_DEFINED

#include "GrConfig.h"
#include "GrPaint.h"
#include "GrAARectRenderer.h"
#include "GrClipData.h"


#include "GrRenderTarget.h"
#include "SkClipStack.h"

class GrAutoScratchTexture;
class GrCacheKey;
class GrDrawState;
class GrDrawTarget;
class GrFontCache;
class GrGpu;
class GrIndexBuffer;
class GrIndexBufferAllocPool;
class GrInOrderDrawBuffer;
class GrPathRenderer;
class GrPathRendererChain;
class GrResourceEntry;
class GrResourceCache;
class GrStencilBuffer;
class GrVertexBuffer;
class GrVertexBufferAllocPool;
class GrSoftwarePathRenderer;

class GR_API GrContext : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrContext)

    


    static GrContext* Create(GrEngine engine,
                             GrPlatform3DContext context3D);

    


    static int GetThreadInstanceCount();

    virtual ~GrContext();

    





    void resetContext();

    










    void contextLost();

    




    void contextDestroyed();

    



    void freeGpuResources();

    


    size_t getGpuTextureCacheBytes() const;

    
    

    













    GrTexture* createTexture(const GrTextureParams* params,
                             const GrTextureDesc& desc,
                             const GrCacheData& cacheData,
                             void* srcData, size_t rowBytes);

    



    GrTexture* findTexture(const GrCacheKey& key);

    










    GrTexture* findTexture(const GrTextureDesc& desc,
                           const GrCacheData& cacheData,
                           const GrTextureParams* params);
    




    bool isTextureInCache(const GrTextureDesc& desc,
                          const GrCacheData& cacheData,
                          const GrTextureParams* params) const;

    



    enum ScratchTexMatch {
        


        kExact_ScratchTexMatch,
        







        kApprox_ScratchTexMatch
    };

    











    GrTexture* lockScratchTexture(const GrTextureDesc& desc,
                                  ScratchTexMatch match);

    



    void unlockScratchTexture(GrTexture* texture);

    






    void purgeCache();

    



    GrTexture* createUncachedTexture(const GrTextureDesc& desc,
                                     void* srcData,
                                     size_t rowBytes);

    





    bool supportsIndex8PixelConfig(const GrTextureParams*,
                                   int width,
                                   int height) const;

    







    void getTextureCacheLimits(int* maxTextures, size_t* maxTextureBytes) const;

    








    void setTextureCacheLimits(int maxTextures, size_t maxTextureBytes);

    


    int getMaxTextureSize() const;

    



    int getMaxRenderTargetSize() const;

    
    

    



    void setRenderTarget(GrRenderTarget* target);

    



    const GrRenderTarget* getRenderTarget() const;
    GrRenderTarget* getRenderTarget();

    GrAARectRenderer* getAARectRenderer() { return fAARectRenderer; }

    


    bool isConfigRenderable(GrPixelConfig config) const;

    
    

    









    GrTexture* createPlatformTexture(const GrPlatformTextureDesc& desc);

    









     GrRenderTarget* createPlatformRenderTarget(
                                    const GrPlatformRenderTargetDesc& desc);

    
    

    



    const GrMatrix& getMatrix() const;

    



    void setMatrix(const GrMatrix& m);

    




    void concatMatrix(const GrMatrix& m) const;


    
    
    



    const GrClipData* getClip() const;

    



    void setClip(const GrClipData* clipData);

    
    

    






    void clear(const GrIRect* rect, GrColor color,
               GrRenderTarget* target = NULL);

    


    void drawPaint(const GrPaint& paint);

    










    void drawRect(const GrPaint& paint,
                  const GrRect&,
                  GrScalar strokeWidth = -1,
                  const GrMatrix* matrix = NULL);

    














    void drawRectToRect(const GrPaint& paint,
                        const GrRect& dstRect,
                        const GrRect& srcRect,
                        const GrMatrix* dstMatrix = NULL,
                        const GrMatrix* srcMatrix = NULL);

    








    void drawPath(const GrPaint& paint, const SkPath& path, GrPathFill fill,
                  const GrPoint* translate = NULL);

    















    void drawVertices(const GrPaint& paint,
                      GrPrimitiveType primitiveType,
                      int vertexCount,
                      const GrPoint positions[],
                      const GrPoint texs[],
                      const GrColor colors[],
                      const uint16_t indices[],
                      int indexCount);

    









    void drawOval(const GrPaint& paint,
                  const GrRect& rect,
                  SkScalar strokeWidth);

    
    

    


    enum FlushBits {
        






        kForceCurrentRenderTarget_FlushBit   = 0x1,
        





        kDiscard_FlushBit                    = 0x2,
    };

    





    void flush(int flagsBitfield = 0);

   


    enum PixelOpsFlags {
        

        kDontFlush_PixelOpsFlag = 0x1,
        

        kUnpremul_PixelOpsFlag  = 0x2,
    };

    
















    bool readRenderTargetPixels(GrRenderTarget* target,
                                int left, int top, int width, int height,
                                GrPixelConfig config, void* buffer,
                                size_t rowBytes = 0,
                                uint32_t pixelOpsFlags = 0);

    













    void writeRenderTargetPixels(GrRenderTarget* target,
                                 int left, int top, int width, int height,
                                 GrPixelConfig config, const void* buffer,
                                 size_t rowBytes = 0,
                                 uint32_t pixelOpsFlags = 0);

    















    bool readTexturePixels(GrTexture* texture,
                           int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer,
                           size_t rowBytes = 0,
                           uint32_t pixelOpsFlags = 0);

    












    void writeTexturePixels(GrTexture* texture,
                            int left, int top, int width, int height,
                            GrPixelConfig config, const void* buffer,
                            size_t rowBytes,
                            uint32_t pixelOpsFlags = 0);


    




    void copyTexture(GrTexture* src, GrRenderTarget* dst);

    










    void resolveRenderTarget(GrRenderTarget* target);

    










     GrTexture* gaussianBlur(GrTexture* srcTexture,
                             bool canClobberSrc,
                             const SkRect& rect,
                             float sigmaX, float sigmaY);

    












     GrTexture* zoom(GrTexture* srcTexture,
                     const SkRect& dstRect, const SkRect& srcRect, float inset);

    
    

    class AutoRenderTarget : ::GrNoncopyable {
    public:
        AutoRenderTarget(GrContext* context, GrRenderTarget* target) {
            fPrevTarget = context->getRenderTarget();
            context->setRenderTarget(target);
            fContext = context;
        }
        AutoRenderTarget(GrContext* context) {
            fPrevTarget = context->getRenderTarget();
            fContext = context;
        }
        ~AutoRenderTarget() {
            if (fContext) {
                fContext->setRenderTarget(fPrevTarget);
            }
        }
    private:
        GrContext*      fContext;
        GrRenderTarget* fPrevTarget;
    };

    


    class AutoMatrix : GrNoncopyable {
    public:
        enum InitialMatrix {
            kPreserve_InitialMatrix,
            kIdentity_InitialMatrix,
        };

        AutoMatrix() : fContext(NULL) {}

        AutoMatrix(GrContext* ctx, InitialMatrix initialState) : fContext(ctx) {
            fMatrix = ctx->getMatrix();
            switch (initialState) {
                case kPreserve_InitialMatrix:
                    break;
                case kIdentity_InitialMatrix:
                    ctx->setMatrix(GrMatrix::I());
                    break;
                default:
                    GrCrash("Unexpected initial matrix state");
            }
        }

        AutoMatrix(GrContext* ctx, const GrMatrix& matrix) : fContext(ctx) {
            fMatrix = ctx->getMatrix();
            ctx->setMatrix(matrix);
        }

        void set(GrContext* ctx) {
            if (NULL != fContext) {
                fContext->setMatrix(fMatrix);
            }
            fMatrix = ctx->getMatrix();
            fContext = ctx;
        }

        void set(GrContext* ctx, const GrMatrix& matrix) {
            if (NULL != fContext) {
                fContext->setMatrix(fMatrix);
            }
            fMatrix = ctx->getMatrix();
            ctx->setMatrix(matrix);
            fContext = ctx;
        }

        ~AutoMatrix() {
            if (NULL != fContext) {
                fContext->setMatrix(fMatrix);
            }
        }

    private:
        GrContext*  fContext;
        GrMatrix    fMatrix;
    };

    class AutoClip : GrNoncopyable {
    public:
        
        
        
        enum InitialClip {
            kWideOpen_InitialClip,
        };

        AutoClip(GrContext* context, InitialClip initialState) {
            GrAssert(kWideOpen_InitialClip == initialState);
            fOldClip = context->getClip();
            fNewClipData.fClipStack = &fNewClipStack;
            context->setClip(&fNewClipData);
            fContext = context;
        }

        AutoClip(GrContext* context, const GrRect& newClipRect)
        : fContext(context)
        , fNewClipStack(newClipRect) {
            fNewClipData.fClipStack = &fNewClipStack;

            fOldClip = fContext->getClip();
            fContext->setClip(&fNewClipData);
        }

        ~AutoClip() {
            if (NULL != fContext) {
                fContext->setClip(fOldClip);
            }
        }
    private:
        GrContext*        fContext;
        const GrClipData* fOldClip;

        SkClipStack       fNewClipStack;
        GrClipData        fNewClipData;
    };

    class AutoWideOpenIdentityDraw {
    public:
        AutoWideOpenIdentityDraw(GrContext* ctx, GrRenderTarget* rt)
            : fAutoClip(ctx, AutoClip::kWideOpen_InitialClip)
            , fAutoRT(ctx, rt)
            , fAutoMatrix(ctx, AutoMatrix::kIdentity_InitialMatrix) {
        }
    private:
        AutoClip fAutoClip;
        AutoRenderTarget fAutoRT;
        AutoMatrix fAutoMatrix;
    };

    
    
    GrGpu* getGpu() { return fGpu; }
    const GrGpu* getGpu() const { return fGpu; }
    GrFontCache* getFontCache() { return fFontCache; }
    GrDrawTarget* getTextTarget(const GrPaint& paint);
    const GrIndexBuffer* getQuadIndexBuffer() const;

    




    void addStencilBuffer(GrStencilBuffer* sb);
    GrStencilBuffer* findStencilBuffer(int width, int height, int sampleCnt);

    GrPathRenderer* getPathRenderer(const SkPath& path,
                                    GrPathFill fill,
                                    const GrDrawTarget* target,
                                    bool antiAlias,
                                    bool allowSW);

#if GR_CACHE_STATS
    void printCacheStats() const;
#endif

private:
    
    enum BufferedDraw {
        kYes_BufferedDraw,
        kNo_BufferedDraw,
    };
    BufferedDraw fLastDrawWasBuffered;

    GrGpu*              fGpu;
    GrDrawState*        fDrawState;

    GrResourceCache*    fTextureCache;
    GrFontCache*        fFontCache;

    GrPathRendererChain*        fPathRendererChain;
    GrSoftwarePathRenderer*     fSoftwarePathRenderer;

    GrVertexBufferAllocPool*    fDrawBufferVBAllocPool;
    GrIndexBufferAllocPool*     fDrawBufferIBAllocPool;
    GrInOrderDrawBuffer*        fDrawBuffer;

    GrAARectRenderer*           fAARectRenderer;

    bool                        fDidTestPMConversions;
    int                         fPMToUPMConversion;
    int                         fUPMToPMConversion;

    GrContext(GrGpu* gpu);

    void setupDrawBuffer();

    void flushDrawBuffer();

    void setPaint(const GrPaint& paint);

    
    
    GrDrawTarget* prepareToDraw(const GrPaint*, BufferedDraw);

    void internalDrawPath(const GrPaint& paint, const SkPath& path,
                          GrPathFill fill, const GrPoint* translate);

    GrTexture* createResizedTexture(const GrTextureDesc& desc,
                                    const GrCacheData& cacheData,
                                    void* srcData,
                                    size_t rowBytes,
                                    bool needsFiltering);

    
    
    friend class GrTexture;

    
    
    void addExistingTextureToCache(GrTexture* texture);

    GrCustomStage* createPMToUPMEffect(GrTexture* texture, bool swapRAndB);
    GrCustomStage* createUPMToPMEffect(GrTexture* texture, bool swapRAndB);

    typedef GrRefCnt INHERITED;
};






class GrAutoScratchTexture : ::GrNoncopyable {
public:
    GrAutoScratchTexture()
        : fContext(NULL)
        , fTexture(NULL) {
    }

    GrAutoScratchTexture(GrContext* context,
                         const GrTextureDesc& desc,
                         GrContext::ScratchTexMatch match =
                            GrContext::kApprox_ScratchTexMatch)
      : fContext(NULL)
      , fTexture(NULL) {
      this->set(context, desc, match);
    }

    ~GrAutoScratchTexture() {
        this->reset();
    }

    void reset() {
        if (NULL != fContext && NULL != fTexture) {
            fContext->unlockScratchTexture(fTexture);
            fTexture = NULL;
        }
    }

    











    GrTexture* detach() {
        GrTexture* temp = fTexture;

        
        
        GrAssert(NULL != temp->getCacheEntry());

        fTexture = NULL;

        temp->setFlag((GrTextureFlags) GrTexture::kReturnToCache_FlagBit);
        return temp;
    }

    GrTexture* set(GrContext* context,
                   const GrTextureDesc& desc,
                   GrContext::ScratchTexMatch match =
                        GrContext::kApprox_ScratchTexMatch) {
        this->reset();

        fContext = context;
        if (NULL != fContext) {
            fTexture = fContext->lockScratchTexture(desc, match);
            if (NULL == fTexture) {
                fContext = NULL;
            }
            return fTexture;
        } else {
            return NULL;
        }
    }

    GrTexture* texture() { return fTexture; }

private:
    GrContext*                    fContext;
    GrTexture*                    fTexture;
};

#endif
