








#include "GrGpu.h"

#include "GrBufferAllocPool.h"
#include "GrClipIterator.h"
#include "GrContext.h"
#include "GrIndexBuffer.h"
#include "GrPathRenderer.h"
#include "GrGLStencilBuffer.h"
#include "GrVertexBuffer.h"


static const size_t VERTEX_POOL_VB_SIZE = 1 << 18;
static const int VERTEX_POOL_VB_COUNT = 4;
static const size_t INDEX_POOL_IB_SIZE = 1 << 16;
static const int INDEX_POOL_IB_COUNT = 4;



extern void gr_run_unittests();

#define DEBUG_INVAL_BUFFER    0xdeadcafe
#define DEBUG_INVAL_START_IDX -1

GrGpu::GrGpu()
    : fContext(NULL)
    , fVertexPool(NULL)
    , fIndexPool(NULL)
    , fVertexPoolUseCnt(0)
    , fIndexPoolUseCnt(0)
    , fQuadIndexBuffer(NULL)
    , fUnitSquareVertexBuffer(NULL)
    , fPathRendererChain(NULL)
    , fContextIsDirty(true)
    , fResourceHead(NULL) {

#if GR_DEBUG
    
#endif
        
    fGeomPoolStateStack.push_back();
#if GR_DEBUG
    GeometryPoolState& poolState = fGeomPoolStateStack.back();
    poolState.fPoolVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
    poolState.fPoolStartVertex = DEBUG_INVAL_START_IDX;
    poolState.fPoolIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
    poolState.fPoolStartIndex = DEBUG_INVAL_START_IDX;
#endif
    resetStats();
}

GrGpu::~GrGpu() {
    this->releaseResources();
}

void GrGpu::abandonResources() {

    while (NULL != fResourceHead) {
        fResourceHead->abandon();
    }

    GrAssert(NULL == fQuadIndexBuffer || !fQuadIndexBuffer->isValid());
    GrAssert(NULL == fUnitSquareVertexBuffer ||
             !fUnitSquareVertexBuffer->isValid());
    GrSafeSetNull(fQuadIndexBuffer);
    GrSafeSetNull(fUnitSquareVertexBuffer);
    delete fVertexPool;
    fVertexPool = NULL;
    delete fIndexPool;
    fIndexPool = NULL;
    
    GrSafeSetNull(fPathRendererChain);
}

void GrGpu::releaseResources() {

    while (NULL != fResourceHead) {
        fResourceHead->release();
    }

    GrAssert(NULL == fQuadIndexBuffer || !fQuadIndexBuffer->isValid());
    GrAssert(NULL == fUnitSquareVertexBuffer ||
             !fUnitSquareVertexBuffer->isValid());
    GrSafeSetNull(fQuadIndexBuffer);
    GrSafeSetNull(fUnitSquareVertexBuffer);
    delete fVertexPool;
    fVertexPool = NULL;
    delete fIndexPool;
    fIndexPool = NULL;
    
    GrSafeSetNull(fPathRendererChain);
}

void GrGpu::insertResource(GrResource* resource) {
    GrAssert(NULL != resource);
    GrAssert(this == resource->getGpu());
    GrAssert(NULL == resource->fNext);
    GrAssert(NULL == resource->fPrevious);

    resource->fNext = fResourceHead;
    if (NULL != fResourceHead) {
        GrAssert(NULL == fResourceHead->fPrevious);
        fResourceHead->fPrevious = resource;
    }
    fResourceHead = resource;
}

void GrGpu::removeResource(GrResource* resource) {
    GrAssert(NULL != resource);
    GrAssert(NULL != fResourceHead);

    if (fResourceHead == resource) {
        GrAssert(NULL == resource->fPrevious);
        fResourceHead = resource->fNext;
    } else {
        GrAssert(NULL != fResourceHead);
        resource->fPrevious->fNext = resource->fNext;
    }
    if (NULL != resource->fNext) {
        resource->fNext->fPrevious = resource->fPrevious;
    }
    resource->fNext = NULL;
    resource->fPrevious = NULL;
}


void GrGpu::unimpl(const char msg[]) {
#if GR_DEBUG
    GrPrintf("--- GrGpu unimplemented(\"%s\")\n", msg);
#endif
}



GrTexture* GrGpu::createTexture(const GrTextureDesc& desc,
                                const void* srcData, size_t rowBytes) {
    this->handleDirtyContext();
    GrTexture* tex = this->onCreateTexture(desc, srcData, rowBytes);
    if (NULL != tex && 
        (kRenderTarget_GrTextureFlagBit & desc.fFlags) &&
        !(kNoStencil_GrTextureFlagBit & desc.fFlags)) {
        GrAssert(NULL != tex->asRenderTarget());
        
        if (!this->attachStencilBufferToRenderTarget(tex->asRenderTarget())) {
            tex->unref();
            return NULL;
        }
    }
    return tex;
}

bool GrGpu::attachStencilBufferToRenderTarget(GrRenderTarget* rt) {
    GrAssert(NULL == rt->getStencilBuffer());
    GrStencilBuffer* sb = 
        this->getContext()->findStencilBuffer(rt->allocatedWidth(),
                                              rt->allocatedHeight(),
                                              rt->numSamples());
    if (NULL != sb) {
        rt->setStencilBuffer(sb);
        bool attached = this->attachStencilBufferToRenderTarget(sb, rt);
        if (!attached) {
            rt->setStencilBuffer(NULL);
        }
        return attached;
    }
    if (this->createStencilBufferForRenderTarget(rt, rt->allocatedWidth(),
                                                 rt->allocatedHeight())) {
        rt->getStencilBuffer()->ref();
        rt->getStencilBuffer()->transferToCacheAndLock();

        
        
        
        
        
        
        
        
        GrRenderTarget* oldRT = fCurrDrawState.fRenderTarget;
        fCurrDrawState.fRenderTarget = rt;
        this->clearStencil();
        fCurrDrawState.fRenderTarget = oldRT;
        return true;
    } else {
        return false;
    }
}

GrResource* GrGpu::createPlatformSurface(const GrPlatformSurfaceDesc& desc) {
    this->handleDirtyContext();
    return this->onCreatePlatformSurface(desc);
}

GrVertexBuffer* GrGpu::createVertexBuffer(uint32_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateVertexBuffer(size, dynamic);
}

GrIndexBuffer* GrGpu::createIndexBuffer(uint32_t size, bool dynamic) {
    this->handleDirtyContext();
    return this->onCreateIndexBuffer(size, dynamic);
}

void GrGpu::clear(const GrIRect* rect, GrColor color) {
    this->handleDirtyContext();
    this->onClear(rect, color);
}

void GrGpu::forceRenderTargetFlush() {
    this->handleDirtyContext();
    this->onForceRenderTargetFlush();
}

bool GrGpu::readPixels(GrRenderTarget* target,
                       int left, int top, int width, int height,
                       GrPixelConfig config, void* buffer) {

    this->handleDirtyContext();
    return this->onReadPixels(target, left, top, width, height, config, buffer);
}



static const int MAX_QUADS = 1 << 12; 

GR_STATIC_ASSERT(4 * MAX_QUADS <= 65535);

static inline void fill_indices(uint16_t* indices, int quadCount) {
    for (int i = 0; i < quadCount; ++i) {
        indices[6 * i + 0] = 4 * i + 0;
        indices[6 * i + 1] = 4 * i + 1;
        indices[6 * i + 2] = 4 * i + 2;
        indices[6 * i + 3] = 4 * i + 0;
        indices[6 * i + 4] = 4 * i + 2;
        indices[6 * i + 5] = 4 * i + 3;
    }
}

const GrIndexBuffer* GrGpu::getQuadIndexBuffer() const {
    if (NULL == fQuadIndexBuffer) {
        static const int SIZE = sizeof(uint16_t) * 6 * MAX_QUADS;
        GrGpu* me = const_cast<GrGpu*>(this);
        fQuadIndexBuffer = me->createIndexBuffer(SIZE, false);
        if (NULL != fQuadIndexBuffer) {
            uint16_t* indices = (uint16_t*)fQuadIndexBuffer->lock();
            if (NULL != indices) {
                fill_indices(indices, MAX_QUADS);
                fQuadIndexBuffer->unlock();
            } else {
                indices = (uint16_t*)GrMalloc(SIZE);
                fill_indices(indices, MAX_QUADS);
                if (!fQuadIndexBuffer->updateData(indices, SIZE)) {
                    fQuadIndexBuffer->unref();
                    fQuadIndexBuffer = NULL;
                    GrCrash("Can't get indices into buffer!");
                }
                GrFree(indices);
            }
        }
    }

    return fQuadIndexBuffer;
}

const GrVertexBuffer* GrGpu::getUnitSquareVertexBuffer() const {
    if (NULL == fUnitSquareVertexBuffer) {

        static const GrPoint DATA[] = {
            { 0,            0 },
            { GR_Scalar1,   0 },
            { GR_Scalar1,   GR_Scalar1 },
            { 0,            GR_Scalar1 }
#if 0
            GrPoint(0,         0),
            GrPoint(GR_Scalar1,0),
            GrPoint(GR_Scalar1,GR_Scalar1),
            GrPoint(0,         GR_Scalar1)
#endif
        };
        static const size_t SIZE = sizeof(DATA);

        GrGpu* me = const_cast<GrGpu*>(this);
        fUnitSquareVertexBuffer = me->createVertexBuffer(SIZE, false);
        if (NULL != fUnitSquareVertexBuffer) {
            if (!fUnitSquareVertexBuffer->updateData(DATA, SIZE)) {
                fUnitSquareVertexBuffer->unref();
                fUnitSquareVertexBuffer = NULL;
                GrCrash("Can't get vertices into buffer!");
            }
        }
    }

    return fUnitSquareVertexBuffer;
}




const GrStencilSettings GrGpu::gClipStencilSettings = {
    kKeep_StencilOp,             kKeep_StencilOp,
    kKeep_StencilOp,             kKeep_StencilOp,
    kAlwaysIfInClip_StencilFunc, kAlwaysIfInClip_StencilFunc,
    0,                           0,
    0,                           0,
    0,                           0
};



static const GrStencilFunc gGrClipToNormalStencilFunc[2][kClipStencilFuncCount] = {
    {
        
        kAlways_StencilFunc,          
        kEqual_StencilFunc,           
        kLess_StencilFunc,            
        kLEqual_StencilFunc,          
        
        kNotEqual_StencilFunc,        
                                      
    },
    {
        
        kEqual_StencilFunc,           
                                      
                                      

        kEqual_StencilFunc,           
                                      

        kLess_StencilFunc,            
        kLEqual_StencilFunc,          
                                      
                                      
                                      
        
        kLess_StencilFunc,            
                                      
                                      
                                      
    }
};

GrStencilFunc GrGpu::ConvertStencilFunc(bool stencilInClip, GrStencilFunc func) {
    GrAssert(func >= 0);
    if (func >= kBasicStencilFuncCount) {
        GrAssert(func < kStencilFuncCount);
        func = gGrClipToNormalStencilFunc[stencilInClip ? 1 : 0][func - kBasicStencilFuncCount];
        GrAssert(func >= 0 && func < kBasicStencilFuncCount);
    }
    return func;
}

void GrGpu::ConvertStencilFuncAndMask(GrStencilFunc func,
                                      bool clipInStencil,
                                      unsigned int clipBit,
                                      unsigned int userBits,
                                      unsigned int* ref,
                                      unsigned int* mask) {
    if (func < kBasicStencilFuncCount) {
        *mask &= userBits;
        *ref &= userBits;
    } else {
        if (clipInStencil) {
            switch (func) {
                case kAlwaysIfInClip_StencilFunc:
                    *mask = clipBit;
                    *ref = clipBit;
                    break;
                case kEqualIfInClip_StencilFunc:
                case kLessIfInClip_StencilFunc:
                case kLEqualIfInClip_StencilFunc:
                    *mask = (*mask & userBits) | clipBit;
                    *ref = (*ref & userBits) | clipBit;
                    break;
                case kNonZeroIfInClip_StencilFunc:
                    *mask = (*mask & userBits) | clipBit;
                    *ref = clipBit;
                    break;
                default:
                    GrCrash("Unknown stencil func");
            }
        } else {
            *mask &= userBits;
            *ref &= userBits;
        }
    }
}



#define VISUALIZE_COMPLEX_CLIP 0

#if VISUALIZE_COMPLEX_CLIP
    #include "GrRandom.h"
    GrRandom gRandom;
    #define SET_RANDOM_COLOR this->setColor(0xff000000 | gRandom.nextU());
#else
    #define SET_RANDOM_COLOR
#endif

namespace {



int process_initial_clip_elements(const GrClip& clip,
                                  bool* clearToInside,
                                  GrSetOp* startOp) {

    
    
    
    
    
    int curr;
    bool done = false;
    *clearToInside = true;
    int count = clip.getElementCount();

    for (curr = 0; curr < count && !done; ++curr) {
        switch (clip.getOp(curr)) {
            case kReplace_SetOp:
                
                *startOp = kReplace_SetOp;
                *clearToInside = false;
                done = true;
                break;
            case kIntersect_SetOp:
                
                
                
                if (*clearToInside) {
                    *startOp = kReplace_SetOp;
                    *clearToInside = false;
                    done = true;
                }
                break;
                
            case kUnion_SetOp:
                
                
                
                if (!*clearToInside) {
                    *startOp = kReplace_SetOp;
                    done = true;
                }
                break;
            case kXor_SetOp:
                
                
                if (*clearToInside) {
                    *startOp = kDifference_SetOp;
                } else {
                    *startOp = kReplace_SetOp;
                }
                done = true;
                break;
            case kDifference_SetOp:
                
                
                
                if (*clearToInside) {
                    *startOp = kDifference_SetOp;
                    done = true;
                }
                break;
            case kReverseDifference_SetOp:
                
                
                if (*clearToInside) {
                    *clearToInside = false;
                } else {
                    *startOp = kReplace_SetOp;
                    done = true;
                }
                break;
            default:
                GrCrash("Unknown set op.");
        }
    }
    return done ? curr-1 : count;
}
}

bool GrGpu::setupClipAndFlushState(GrPrimitiveType type) {
    const GrIRect* r = NULL;
    GrIRect clipRect;

    
    
    
    if (NULL == fCurrDrawState.fRenderTarget) {
        GrAssert(!"No render target bound.");
        return false;
    }

    if (fCurrDrawState.fFlagBits & kClip_StateBit) {
        GrRenderTarget& rt = *fCurrDrawState.fRenderTarget;

        GrRect bounds;
        GrRect rtRect;
        rtRect.setLTRB(0, 0,
                       GrIntToScalar(rt.width()), GrIntToScalar(rt.height()));
        if (fClip.hasConservativeBounds()) {
            bounds = fClip.getConservativeBounds();
            if (!bounds.intersect(rtRect)) {
                bounds.setEmpty();
            }
        } else {
            bounds = rtRect;
        }

        bounds.roundOut(&clipRect);
        if  (clipRect.isEmpty()) {
            clipRect.setLTRB(0,0,0,0);
        }
        r = &clipRect;

        
        fClipInStencil = !fClip.isRect() && !fClip.isEmpty() && 
                         !bounds.isEmpty();

        
        GrStencilBuffer* stencilBuffer = rt.getStencilBuffer();
        if (fClipInStencil && NULL == stencilBuffer) {
            return false;
        }

        if (fClipInStencil &&
            stencilBuffer->mustRenderClip(fClip, rt.width(), rt.height())) {

            stencilBuffer->setLastClip(fClip, rt.width(), rt.height());

            
            
            
            
            const GrClip& clip = stencilBuffer->getLastClip();
            fClip.setFromRect(bounds);

            AutoStateRestore asr(this);
            AutoGeometryPush agp(this);

            this->setViewMatrix(GrMatrix::I());
            this->flushScissor(NULL);
#if !VISUALIZE_COMPLEX_CLIP
            this->enableState(kNoColorWrites_StateBit);
#else
            this->disableState(kNoColorWrites_StateBit);
#endif
            int count = clip.getElementCount();
            int clipBit = stencilBuffer->bits();
            clipBit = (1 << (clipBit-1));
            
            bool clearToInside;
            GrSetOp startOp = kReplace_SetOp; 
            int start = process_initial_clip_elements(clip, &clearToInside,
                                                      &startOp);

            this->clearStencilClip(clipRect, clearToInside);

            
            
            for (int c = start; c < count; ++c) {
                GrPathFill fill;
                bool fillInverted;
                
                this->disableState(kModifyStencilClip_StateBit);

                bool canRenderDirectToStencil; 
                                               
                                               
                                               
                                               

                GrPathRenderer* pr = NULL;
                const GrPath* clipPath = NULL;
                GrPathRenderer::AutoClearPath arp;
                if (kRect_ClipType == clip.getElementType(c)) {
                    canRenderDirectToStencil = true;
                    fill = kEvenOdd_PathFill;
                    fillInverted = false;
                } else {
                    fill = clip.getPathFill(c);
                    fillInverted = GrIsFillInverted(fill);
                    fill = GrNonInvertedFill(fill);
                    clipPath = &clip.getPath(c);
                    pr = this->getClipPathRenderer(*clipPath, fill);
                    if (NULL == pr) {
                        fClipInStencil = false;
                        fClip = clip;
                        return false;
                    }
                    canRenderDirectToStencil =
                        !pr->requiresStencilPass(this, *clipPath, fill);
                    arp.set(pr, this, clipPath, fill, false, NULL);
                }

                GrSetOp op = (c == start) ? startOp : clip.getOp(c);
                int passes;
                GrStencilSettings stencilSettings[GrStencilSettings::kMaxStencilClipPasses];

                bool canDrawDirectToClip; 
                                          
                                          
                                          
                canDrawDirectToClip =
                    GrStencilSettings::GetClipPasses(op,
                                                     canRenderDirectToStencil,
                                                     clipBit,
                                                     fillInverted,
                                                     &passes, stencilSettings);

                
                if (!canDrawDirectToClip) {
                    static const GrStencilSettings gDrawToStencil = {
                        kIncClamp_StencilOp, kIncClamp_StencilOp,
                        kIncClamp_StencilOp, kIncClamp_StencilOp,
                        kAlways_StencilFunc, kAlways_StencilFunc,
                        0xffffffff,          0xffffffff,
                        0x00000000,          0x00000000,
                        0xffffffff,          0xffffffff,
                    };
                    SET_RANDOM_COLOR
                    if (kRect_ClipType == clip.getElementType(c)) {
                        this->setStencil(gDrawToStencil);
                        this->drawSimpleRect(clip.getRect(c), NULL, 0);
                    } else {
                        if (canRenderDirectToStencil) {
                            this->setStencil(gDrawToStencil);
                            pr->drawPath(0);
                        } else {
                            pr->drawPathToStencil();
                        }
                    }
                }

                
                
                this->enableState(kModifyStencilClip_StateBit);
                for (int p = 0; p < passes; ++p) {
                    this->setStencil(stencilSettings[p]);
                    if (canDrawDirectToClip) {
                        if (kRect_ClipType == clip.getElementType(c)) {
                            SET_RANDOM_COLOR
                            this->drawSimpleRect(clip.getRect(c), NULL, 0);
                        } else {
                            SET_RANDOM_COLOR
                            pr->drawPath(0);
                        }
                    } else {
                        SET_RANDOM_COLOR
                        this->drawSimpleRect(bounds, NULL, 0);
                    }
                }
            }
            
            fClip = clip;
            
            
            fClipInStencil = true;
        }
    }

    
    if (!this->flushGraphicsState(type)) {
        return false;
    }
    this->flushScissor(r);
    return true;
}

GrPathRenderer* GrGpu::getClipPathRenderer(const GrPath& path,
                                           GrPathFill fill) {
    if (NULL == fPathRendererChain) {
        fPathRendererChain = 
            new GrPathRendererChain(this->getContext(),
                                    GrPathRendererChain::kNonAAOnly_UsageFlag);
    }
    return fPathRendererChain->getPathRenderer(this->getCaps(),
                                               path, fill, false);
}




void GrGpu::geometrySourceWillPush() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    if (kArray_GeometrySrcType == geoSrc.fVertexSrc ||
        kReserved_GeometrySrcType == geoSrc.fVertexSrc) {
        this->finalizeReservedVertices();
    }
    if (kArray_GeometrySrcType == geoSrc.fIndexSrc ||
        kReserved_GeometrySrcType == geoSrc.fIndexSrc) {
        this->finalizeReservedIndices();
    }
    GeometryPoolState& newState = fGeomPoolStateStack.push_back();
#if GR_DEBUG
    newState.fPoolVertexBuffer = (GrVertexBuffer*)DEBUG_INVAL_BUFFER;
    newState.fPoolStartVertex = DEBUG_INVAL_START_IDX;
    newState.fPoolIndexBuffer = (GrIndexBuffer*)DEBUG_INVAL_BUFFER;
    newState.fPoolStartIndex = DEBUG_INVAL_START_IDX;
#endif
}

void GrGpu::geometrySourceWillPop(const GeometrySrcState& restoredState) {
    
    GrAssert(fGeomPoolStateStack.count() > 1);
    fGeomPoolStateStack.pop_back();
}

void GrGpu::onDrawIndexed(GrPrimitiveType type,
                          int startVertex,
                          int startIndex,
                          int vertexCount,
                          int indexCount) {

    this->handleDirtyContext();

    if (!this->setupClipAndFlushState(type)) {
        return;
    }

#if GR_COLLECT_STATS
    fStats.fVertexCnt += vertexCount;
    fStats.fIndexCnt  += indexCount;
    fStats.fDrawCnt   += 1;
#endif

    int sVertex = startVertex;
    int sIndex = startIndex;
    setupGeometry(&sVertex, &sIndex, vertexCount, indexCount);

    this->onGpuDrawIndexed(type, sVertex, sIndex,
                           vertexCount, indexCount);
}

void GrGpu::onDrawNonIndexed(GrPrimitiveType type,
                           int startVertex,
                           int vertexCount) {
    this->handleDirtyContext();

    if (!this->setupClipAndFlushState(type)) {
        return;
    }
#if GR_COLLECT_STATS
    fStats.fVertexCnt += vertexCount;
    fStats.fDrawCnt   += 1;
#endif

    int sVertex = startVertex;
    setupGeometry(&sVertex, NULL, vertexCount, 0);

    this->onGpuDrawNonIndexed(type, sVertex, vertexCount);
}

void GrGpu::finalizeReservedVertices() {
    GrAssert(NULL != fVertexPool);
    fVertexPool->unlock();
}

void GrGpu::finalizeReservedIndices() {
    GrAssert(NULL != fIndexPool);
    fIndexPool->unlock();
}

void GrGpu::prepareVertexPool() {
    if (NULL == fVertexPool) {
        GrAssert(0 == fVertexPoolUseCnt);
        fVertexPool = new GrVertexBufferAllocPool(this, true,
                                                  VERTEX_POOL_VB_SIZE,
                                                  VERTEX_POOL_VB_COUNT);
        fVertexPool->releaseGpuRef();
    } else if (!fVertexPoolUseCnt) {
        
        fVertexPool->reset();
    }
}

void GrGpu::prepareIndexPool() {
    if (NULL == fIndexPool) {
        GrAssert(0 == fIndexPoolUseCnt);
        fIndexPool = new GrIndexBufferAllocPool(this, true,
                                                INDEX_POOL_IB_SIZE,
                                                INDEX_POOL_IB_COUNT);
        fIndexPool->releaseGpuRef();
    } else if (!fIndexPoolUseCnt) {
        
        fIndexPool->reset();
    }
}

bool GrGpu::onReserveVertexSpace(GrVertexLayout vertexLayout,
                                 int vertexCount,
                                 void** vertices) {
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
    
    GrAssert(vertexCount > 0);
    GrAssert(NULL != vertices);
    
    this->prepareVertexPool();
    
    *vertices = fVertexPool->makeSpace(vertexLayout,
                                       vertexCount,
                                       &geomPoolState.fPoolVertexBuffer,
                                       &geomPoolState.fPoolStartVertex);
    if (NULL == *vertices) {
        return false;
    }
    ++fVertexPoolUseCnt;
    return true;
}

bool GrGpu::onReserveIndexSpace(int indexCount, void** indices) {
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
    
    GrAssert(indexCount > 0);
    GrAssert(NULL != indices);

    this->prepareIndexPool();

    *indices = fIndexPool->makeSpace(indexCount,
                                     &geomPoolState.fPoolIndexBuffer,
                                     &geomPoolState.fPoolStartIndex);
    if (NULL == *indices) {
        return false;
    }
    ++fIndexPoolUseCnt;
    return true;
}

void GrGpu::releaseReservedVertexSpace() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kReserved_GeometrySrcType == geoSrc.fVertexSrc);
    size_t bytes = geoSrc.fVertexCount * VertexSize(geoSrc.fVertexLayout);
    fVertexPool->putBack(bytes);
    --fVertexPoolUseCnt;
}

void GrGpu::releaseReservedIndexSpace() {
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kReserved_GeometrySrcType == geoSrc.fIndexSrc);
    size_t bytes = geoSrc.fIndexCount * sizeof(uint16_t);
    fIndexPool->putBack(bytes);
    --fIndexPoolUseCnt;
}

void GrGpu::onSetVertexSourceToArray(const void* vertexArray, int vertexCount) {
    this->prepareVertexPool();
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
#if GR_DEBUG
    bool success =
#endif
    fVertexPool->appendVertices(this->getGeomSrc().fVertexLayout,
                                vertexCount,
                                vertexArray,
                                &geomPoolState.fPoolVertexBuffer,
                                &geomPoolState.fPoolStartVertex);
    ++fVertexPoolUseCnt;
    GR_DEBUGASSERT(success);
}

void GrGpu::onSetIndexSourceToArray(const void* indexArray, int indexCount) {
    this->prepareIndexPool();
    GeometryPoolState& geomPoolState = fGeomPoolStateStack.back();
#if GR_DEBUG
    bool success =
#endif
    fIndexPool->appendIndices(indexCount,
                              indexArray,
                              &geomPoolState.fPoolIndexBuffer,
                              &geomPoolState.fPoolStartIndex);
    ++fIndexPoolUseCnt;
    GR_DEBUGASSERT(success);
}

void GrGpu::releaseVertexArray() {
    
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kArray_GeometrySrcType == geoSrc.fVertexSrc);
    size_t bytes = geoSrc.fVertexCount * VertexSize(geoSrc.fVertexLayout);
    fVertexPool->putBack(bytes);
    --fVertexPoolUseCnt;
}

void GrGpu::releaseIndexArray() {
    
    const GeometrySrcState& geoSrc = this->getGeomSrc();
    GrAssert(kArray_GeometrySrcType == geoSrc.fIndexSrc);
    size_t bytes = geoSrc.fIndexCount * sizeof(uint16_t);
    fIndexPool->putBack(bytes);
    --fIndexPoolUseCnt;
}



const GrGpuStats& GrGpu::getStats() const {
    return fStats;
}

void GrGpu::resetStats() {
    memset(&fStats, 0, sizeof(fStats));
}

void GrGpu::printStats() const {
    if (GR_COLLECT_STATS) {
     GrPrintf(
     "-v-------------------------GPU STATS----------------------------v-\n"
     "Stats collection is: %s\n"
     "Draws: %04d, Verts: %04d, Indices: %04d\n"
     "ProgChanges: %04d, TexChanges: %04d, RTChanges: %04d\n"
     "TexCreates: %04d, RTCreates:%04d\n"
     "-^--------------------------------------------------------------^-\n",
     (GR_COLLECT_STATS ? "ON" : "OFF"),
    fStats.fDrawCnt, fStats.fVertexCnt, fStats.fIndexCnt,
    fStats.fProgChngCnt, fStats.fTextureChngCnt, fStats.fRenderTargetChngCnt,
    fStats.fTextureCreateCnt, fStats.fRenderTargetCreateCnt);
    }
}


const GrSamplerState GrSamplerState::gClampNoFilter(
    GrSamplerState::kClamp_WrapMode,
    GrSamplerState::kClamp_WrapMode,
    GrSamplerState::kNormal_SampleMode,
    GrMatrix::I(),
    GrSamplerState::kNearest_Filter);




