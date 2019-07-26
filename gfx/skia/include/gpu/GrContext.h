








#ifndef GrContext_DEFINED
#define GrContext_DEFINED

#include "GrColor.h"
#include "GrAARectRenderer.h"
#include "GrClipData.h"
#include "SkMatrix.h"
#include "GrPaint.h"
#include "GrPathRendererChain.h"
#include "GrRenderTarget.h"
#include "GrRefCnt.h"
#include "GrTexture.h"

class GrAutoScratchTexture;
class GrDrawState;
class GrDrawTarget;
class GrEffect;
class GrFontCache;
class GrGpu;
class GrIndexBuffer;
class GrIndexBufferAllocPool;
class GrInOrderDrawBuffer;
class GrOvalRenderer;
class GrPathRenderer;
class GrResourceEntry;
class GrResourceCache;
class GrStencilBuffer;
class GrTextureParams;
class GrVertexBuffer;
class GrVertexBufferAllocPool;
class GrSoftwarePathRenderer;
class SkStrokeRec;

class GR_API GrContext : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrContext)

    


    static GrContext* Create(GrBackend, GrBackendContext);

    


    static int GetThreadInstanceCount();

    virtual ~GrContext();

    





    void resetContext();

    



    typedef void (*PFCleanUpFunc)(const GrContext* context, void* info);

    




    void addCleanUp(PFCleanUpFunc cleanUp, void* info) {
        CleanUpData* entry = fCleanUpData.push();

        entry->fFunc = cleanUp;
        entry->fInfo = info;
    }

    










    void contextLost();

    




    void contextDestroyed();

    



    void freeGpuResources();

    


    size_t getGpuTextureCacheBytes() const;

    
    

    













    GrTexture* createTexture(const GrTextureParams* params,
                             const GrTextureDesc& desc,
                             const GrCacheID& cacheID,
                             void* srcData, size_t rowBytes);

    










    GrTexture* findAndRefTexture(const GrTextureDesc& desc,
                                 const GrCacheID& cacheID,
                                 const GrTextureParams* params);
    




    bool isTextureInCache(const GrTextureDesc& desc,
                          const GrCacheID& cacheID,
                          const GrTextureParams* params) const;

    



    enum ScratchTexMatch {
        


        kExact_ScratchTexMatch,
        







        kApprox_ScratchTexMatch
    };

    












    GrTexture* lockAndRefScratchTexture(const GrTextureDesc&, ScratchTexMatch match);

    



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

    
    

    



    void setRenderTarget(GrRenderTarget* target);

    



    const GrRenderTarget* getRenderTarget() const;
    GrRenderTarget* getRenderTarget();

    GrAARectRenderer* getAARectRenderer() { return fAARectRenderer; }

    


    bool isConfigRenderable(GrPixelConfig config) const;

    



    int getMaxRenderTargetSize() const;

    



    int getMaxSampleCount() const;

    
    

    









    GrTexture* wrapBackendTexture(const GrBackendTextureDesc& desc);

    









     GrRenderTarget* wrapBackendRenderTarget(const GrBackendRenderTargetDesc& desc);

    
    

    



    const SkMatrix& getMatrix() const;

    



    void setMatrix(const SkMatrix& m);

    


    void setIdentityMatrix();

    




    void concatMatrix(const SkMatrix& m) const;


    
    
    



    const GrClipData* getClip() const;

    



    void setClip(const GrClipData* clipData);

    
    

    






    void clear(const GrIRect* rect, GrColor color,
               GrRenderTarget* target = NULL);

    


    void drawPaint(const GrPaint& paint);

    










    void drawRect(const GrPaint& paint,
                  const GrRect&,
                  SkScalar strokeWidth = -1,
                  const SkMatrix* matrix = NULL);

    












    void drawRectToRect(const GrPaint& paint,
                        const GrRect& dstRect,
                        const GrRect& localRect,
                        const SkMatrix* dstMatrix = NULL,
                        const SkMatrix* localMatrix = NULL);

    






    void drawPath(const GrPaint& paint, const SkPath& path, const SkStrokeRec& stroke);

    















    void drawVertices(const GrPaint& paint,
                      GrPrimitiveType primitiveType,
                      int vertexCount,
                      const GrPoint positions[],
                      const GrPoint texs[],
                      const GrColor colors[],
                      const uint16_t indices[],
                      int indexCount);

    






    void drawOval(const GrPaint& paint,
                  const GrRect& oval,
                  const SkStrokeRec& stroke);

    
    

    


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

    
















    bool writeRenderTargetPixels(GrRenderTarget* target,
                                 int left, int top, int width, int height,
                                 GrPixelConfig config, const void* buffer,
                                 size_t rowBytes = 0,
                                 uint32_t pixelOpsFlags = 0);

    















    bool readTexturePixels(GrTexture* texture,
                           int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer,
                           size_t rowBytes = 0,
                           uint32_t pixelOpsFlags = 0);

    














    bool writeTexturePixels(GrTexture* texture,
                            int left, int top, int width, int height,
                            GrPixelConfig config, const void* buffer,
                            size_t rowBytes,
                            uint32_t pixelOpsFlags = 0);


    








    void copyTexture(GrTexture* src, GrRenderTarget* dst, const SkIPoint* topLeft = NULL);

    










    void resolveRenderTarget(GrRenderTarget* target);

    










     GrTexture* gaussianBlur(GrTexture* srcTexture,
                             bool canClobberSrc,
                             const SkRect& rect,
                             float sigmaX, float sigmaY);

    
    

    class AutoRenderTarget : public ::GrNoncopyable {
    public:
        AutoRenderTarget(GrContext* context, GrRenderTarget* target) {
            fPrevTarget = context->getRenderTarget();
            GrSafeRef(fPrevTarget);
            context->setRenderTarget(target);
            fContext = context;
        }
        AutoRenderTarget(GrContext* context) {
            fPrevTarget = context->getRenderTarget();
            GrSafeRef(fPrevTarget);
            fContext = context;
        }
        ~AutoRenderTarget() {
            if (NULL != fContext) {
                fContext->setRenderTarget(fPrevTarget);
            }
            GrSafeUnref(fPrevTarget);
        }
    private:
        GrContext*      fContext;
        GrRenderTarget* fPrevTarget;
    };

    













    class AutoMatrix : GrNoncopyable {
    public:
        AutoMatrix() : fContext(NULL) {}

        ~AutoMatrix() { this->restore(); }

        


        void setPreConcat(GrContext* context, const SkMatrix& preConcat, GrPaint* paint = NULL) {
            GrAssert(NULL != context);

            this->restore();

            fContext = context;
            fMatrix = context->getMatrix();
            this->preConcat(preConcat, paint);
        }

        



        bool setIdentity(GrContext* context, GrPaint* paint = NULL) {
            GrAssert(NULL != context);

            this->restore();

            if (NULL != paint) {
                if (!paint->localCoordChangeInverse(context->getMatrix())) {
                    return false;
                }
            }
            fMatrix = context->getMatrix();
            fContext = context;
            context->setIdentityMatrix();
            return true;
        }

        



        bool set(GrContext* context, const SkMatrix& newMatrix, GrPaint* paint = NULL) {
            if (NULL != paint) {
                if (!this->setIdentity(context, paint)) {
                    return false;
                }
                this->preConcat(newMatrix, paint);
            } else {
                this->restore();
                fContext = context;
                fMatrix = context->getMatrix();
                context->setMatrix(newMatrix);
            }
            return true;
        }

        






        void preConcat(const SkMatrix& preConcat, GrPaint* paint = NULL) {
            if (NULL != paint) {
                paint->localCoordChange(preConcat);
            }
            fContext->concatMatrix(preConcat);
        }

        



        bool succeeded() const { return NULL != fContext; }

        


        void restore() {
            if (NULL != fContext) {
                fContext->setMatrix(fMatrix);
                fContext = NULL;
            }
        }

    private:
        GrContext*  fContext;
        SkMatrix    fMatrix;
    };

    class AutoClip : GrNoncopyable {
    public:
        
        
        
        enum InitialClip {
            kWideOpen_InitialClip,
        };

        AutoClip(GrContext* context, InitialClip initialState)
        : fContext(context) {
            GrAssert(kWideOpen_InitialClip == initialState);
            fNewClipData.fClipStack = &fNewClipStack;

            fOldClip = context->getClip();
            context->setClip(&fNewClipData);
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
            , fAutoRT(ctx, rt) {
            fAutoMatrix.setIdentity(ctx);
            
            GrAssert(fAutoMatrix.succeeded());
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

    GrPathRenderer* getPathRenderer(
                    const SkPath& path,
                    const SkStrokeRec& stroke,
                    const GrDrawTarget* target,
                    bool allowSW,
                    GrPathRendererChain::DrawType drawType = GrPathRendererChain::kColor_DrawType,
                    GrPathRendererChain::StencilSupport* stencilSupport = NULL);

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
    GrOvalRenderer*             fOvalRenderer;

    bool                        fDidTestPMConversions;
    int                         fPMToUPMConversion;
    int                         fUPMToPMConversion;

    struct CleanUpData {
        PFCleanUpFunc fFunc;
        void*         fInfo;
    };

    SkTDArray<CleanUpData>      fCleanUpData;

    GrContext(); 
    bool init(GrBackend, GrBackendContext);

    void setupDrawBuffer();

    void flushDrawBuffer();

    
    
    GrDrawTarget* prepareToDraw(const GrPaint*, BufferedDraw);

    void internalDrawPath(GrDrawTarget* target, const GrPaint& paint, const SkPath& path,
                          const SkStrokeRec& stroke);

    GrTexture* createResizedTexture(const GrTextureDesc& desc,
                                    const GrCacheID& cacheID,
                                    void* srcData,
                                    size_t rowBytes,
                                    bool needsFiltering);

    
    
    friend class GrTexture;

    
    
    void addExistingTextureToCache(GrTexture* texture);

    




    const GrEffectRef* createPMToUPMEffect(GrTexture* texture,
                                           bool swapRAndB,
                                           const SkMatrix& matrix);
    const GrEffectRef* createUPMToPMEffect(GrTexture* texture,
                                           bool swapRAndB,
                                           const SkMatrix& matrix);

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
                         GrContext::ScratchTexMatch match = GrContext::kApprox_ScratchTexMatch)
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
            fTexture->unref();
            fTexture = NULL;
        }
    }

    











    GrTexture* detach() {
        GrTexture* texture = fTexture;
        fTexture = NULL;

        
        
        
        
        GrAssert(texture->getRefCnt() > 1);
        texture->setFlag((GrTextureFlags) GrTexture::kReturnToCache_FlagBit);
        texture->unref();
        GrAssert(NULL != texture->getCacheEntry());

        return texture;
    }

    GrTexture* set(GrContext* context,
                   const GrTextureDesc& desc,
                   GrContext::ScratchTexMatch match = GrContext::kApprox_ScratchTexMatch) {
        this->reset();

        fContext = context;
        if (NULL != fContext) {
            fTexture = fContext->lockAndRefScratchTexture(desc, match);
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
