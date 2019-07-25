








#ifndef GrContext_DEFINED
#define GrContext_DEFINED

#include "GrClip.h"
#include "GrPaint.h"


#include "GrRenderTarget.h" 

class GrDrawTarget;
class GrFontCache;
class GrGpu;
struct GrGpuStats;
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

class GR_API GrContext : public GrRefCnt {
public:
    


    static GrContext* Create(GrEngine engine,
                             GrPlatform3DContext context3D);

    


    static GrContext* CreateGLShaderContext();

    virtual ~GrContext();

    





    void resetContext();

    










    void contextLost();

    




    void contextDestroyed();

    



    void freeGpuResources();

    
    

    



    class TextureCacheEntry {
    public:
        TextureCacheEntry() : fEntry(NULL) {}
        TextureCacheEntry(const TextureCacheEntry& e) : fEntry(e.fEntry) {}
        TextureCacheEntry& operator= (const TextureCacheEntry& e) {
            fEntry = e.fEntry;
            return *this;
        }
        GrTexture* texture() const;
        void reset() { fEntry = NULL; }
    private:
        explicit TextureCacheEntry(GrResourceEntry* entry) { fEntry = entry; }
        void set(GrResourceEntry* entry) { fEntry = entry; }
        GrResourceEntry* cacheEntry() { return fEntry; }
        GrResourceEntry* fEntry;
        friend class GrContext;
    };

    




    typedef uint64_t TextureKey;

    




    TextureCacheEntry findAndLockTexture(TextureKey key,
                                         int width,
                                         int height,
                                         const GrSamplerState&);

    



    TextureCacheEntry createAndLockTexture(TextureKey key,
                                           const GrSamplerState&,
                                           const GrTextureDesc&,
                                           void* srcData, size_t rowBytes);

    



    enum ScratchTexMatch {
        


        kExact_ScratchTexMatch,
        







        kApprox_ScratchTexMatch
    };

    











    TextureCacheEntry lockScratchTexture(const GrTextureDesc& desc, ScratchTexMatch match);

    



    void unlockTexture(TextureCacheEntry entry);

    



    GrTexture* createUncachedTexture(const GrTextureDesc&,
                                     void* srcData,
                                     size_t rowBytes);

    


    bool supportsIndex8PixelConfig(const GrSamplerState&,
                                   int width,
                                   int height) const;

    







    void getTextureCacheLimits(int* maxTextures, size_t* maxTextureBytes) const;

    








    void setTextureCacheLimits(int maxTextures, size_t maxTextureBytes);

    


    int getMaxTextureSize() const;

    



    int getMaxRenderTargetSize() const;

    
    

    



    void setRenderTarget(GrRenderTarget* target);

    



    const GrRenderTarget* getRenderTarget() const;
    GrRenderTarget* getRenderTarget();

    
    

    













    GrResource* createPlatformSurface(const GrPlatformSurfaceDesc& desc);

    
    

    



    const GrMatrix& getMatrix() const;

    



    void setMatrix(const GrMatrix& m);

    




    void concatMatrix(const GrMatrix& m) const;


    
    
    



    const GrClip& getClip() const;

    



    void setClip(const GrClip& clip);

    



    void setClip(const GrIRect& rect);

    
    

    




    void clear(const GrIRect* rect, GrColor color);

    


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

    








    void drawPath(const GrPaint& paint, const GrPath& path, GrPathFill fill,
                  const GrPoint* translate = NULL);

    















    void drawVertices(const GrPaint& paint,
                      GrPrimitiveType primitiveType,
                      int vertexCount,
                      const GrPoint positions[],
                      const GrPoint texs[],
                      const GrColor colors[],
                      const uint16_t indices[],
                      int indexCount);

    
    

    


    bool supportsShaders() const;

    


    enum FlushBits {
        






        kForceCurrentRenderTarget_FlushBit   = 0x1,
        





        kDiscard_FlushBit                    = 0x2,
    };

    





    void flush(int flagsBitfield = 0);
    
    














    bool readRenderTargetPixels(GrRenderTarget* target,
                                int left, int top, int width, int height,
                                GrPixelConfig config, void* buffer);

    












    bool readTexturePixels(GrTexture* target,
                           int left, int top, int width, int height,
                           GrPixelConfig config, void* buffer);

    



    void writePixels(int left, int top, int width, int height,
                     GrPixelConfig, const void* buffer, size_t stride);

    







    void convolveInX(GrTexture* texture,
                     const SkRect& rect,
                     const float* kernel,
                     int kernelWidth);
    








    void convolveInY(GrTexture* texture,
                     const SkRect& rect,
                     const float* kernel,
                     int kernelWidth);
    
    

    class AutoRenderTarget : ::GrNoncopyable {
    public:
        AutoRenderTarget(GrContext* context, GrRenderTarget* target) {
            fContext = NULL;
            fPrevTarget = context->getRenderTarget();
            if (fPrevTarget != target) {
                context->setRenderTarget(target);
                fContext = context;
            }
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


    
    
    GrGpu* getGpu() { return fGpu; }
    const GrGpu* getGpu() const { return fGpu; }
    GrFontCache* getFontCache() { return fFontCache; }
    GrDrawTarget* getTextTarget(const GrPaint& paint);
    void flushText();
    const GrIndexBuffer* getQuadIndexBuffer() const;
    void resetStats();
    const GrGpuStats& getStats() const;
    void printStats() const;
    







    GrResourceEntry* addAndLockStencilBuffer(GrStencilBuffer* sb);
    void unlockStencilBuffer(GrResourceEntry* sbEntry);
    GrStencilBuffer* findStencilBuffer(int width, int height, int sampleCnt);

private:
    
    enum DrawCategory {
        kBuffered_DrawCategory,      
        kUnbuffered_DrawCategory,    
        kText_DrawCategory           
    };
    DrawCategory fLastDrawCategory;

    GrGpu*              fGpu;
    GrResourceCache*    fTextureCache;
    GrFontCache*        fFontCache;

    GrPathRendererChain*        fPathRendererChain;

    GrVertexBufferAllocPool*    fDrawBufferVBAllocPool;
    GrIndexBufferAllocPool*     fDrawBufferIBAllocPool;
    GrInOrderDrawBuffer*        fDrawBuffer;

    GrIndexBuffer*              fAAFillRectIndexBuffer;
    GrIndexBuffer*              fAAStrokeRectIndexBuffer;
    int                         fMaxOffscreenAASize;

    GrContext(GrGpu* gpu);

    void fillAARect(GrDrawTarget* target,
                    const GrRect& devRect,
                    bool useVertexCoverage);

    void strokeAARect(GrDrawTarget* target,
                      const GrRect& devRect,
                      const GrVec& devStrokeSize,
                      bool useVertexCoverage);

    inline int aaFillRectIndexCount() const;
    GrIndexBuffer* aaFillRectIndexBuffer();

    inline int aaStrokeRectIndexCount() const;
    GrIndexBuffer* aaStrokeRectIndexBuffer();

    void setupDrawBuffer();

    void flushDrawBuffer();

    static void SetPaint(const GrPaint& paint, GrDrawTarget* target);

    GrDrawTarget* prepareToDraw(const GrPaint& paint, DrawCategory drawType);

    GrPathRenderer* getPathRenderer(const GrPath& path,
                                    GrPathFill fill,
                                    bool antiAlias);

    struct OffscreenRecord;

    
    bool doOffscreenAA(GrDrawTarget* target,
                       bool isHairLines) const;

    
    
    bool prepareForOffscreenAA(GrDrawTarget* target,
                               bool requireStencil,
                               const GrIRect& boundRect,
                               GrPathRenderer* pr,
                               OffscreenRecord* record);

    
    void setupOffscreenAAPass1(GrDrawTarget* target,
                               const GrIRect& boundRect,
                               int tileX, int tileY,
                               OffscreenRecord* record);

    
    
    void doOffscreenAAPass2(GrDrawTarget* target,
                            const GrPaint& paint,
                            const GrIRect& boundRect,
                            int tileX, int tileY,
                            OffscreenRecord* record);

    
    void cleanupOffscreenAA(GrDrawTarget* target,
                            GrPathRenderer* pr,
                            OffscreenRecord* record);

    void convolve(GrTexture* texture,
                  const SkRect& rect,
                  float imageIncrement[2],
                  const float* kernel,
                  int kernelWidth);
    
    
    
    
    
    
    static int PaintStageVertexLayoutBits(
                                    const GrPaint& paint,
                                    const bool hasTexCoords[GrPaint::kTotalStages]);
    
};




class GrAutoMatrix : GrNoncopyable {
public:
    GrAutoMatrix() : fContext(NULL) {}
    GrAutoMatrix(GrContext* ctx) : fContext(ctx) {
        fMatrix = ctx->getMatrix();
    }
    GrAutoMatrix(GrContext* ctx, const GrMatrix& matrix) : fContext(ctx) {
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
    ~GrAutoMatrix() {
        if (NULL != fContext) {
            fContext->setMatrix(fMatrix);
        }
    }

private:
    GrContext*  fContext;
    GrMatrix    fMatrix;
};






class GrAutoScratchTexture : ::GrNoncopyable {
public:
    GrAutoScratchTexture()
        : fContext(NULL) {
    }

    GrAutoScratchTexture(GrContext* context,
                         const GrTextureDesc& desc,
                         GrContext::ScratchTexMatch match =
                            GrContext::kApprox_ScratchTexMatch)
      : fContext(NULL) {
      this->set(context, desc, match);
    }
    
    ~GrAutoScratchTexture() {
        if (NULL != fContext) {
            fContext->unlockTexture(fEntry);
        }
    }

    GrTexture* set(GrContext* context,
                   const GrTextureDesc& desc,
                   GrContext::ScratchTexMatch match =
                        GrContext::kApprox_ScratchTexMatch) {
        if (NULL != fContext) {
            fContext->unlockTexture(fEntry);
        }
        fContext = context;
        if (NULL != fContext) {
            fEntry = fContext->lockScratchTexture(desc, match);
            GrTexture* ret = fEntry.texture();
            if (NULL == ret) {
                fContext = NULL;
            }
            return ret;
        } else {
            return NULL;
        }
    }

    GrTexture* texture() { return fEntry.texture(); }
private:
    GrContext*                    fContext;
    GrContext::TextureCacheEntry  fEntry;
};

#endif

