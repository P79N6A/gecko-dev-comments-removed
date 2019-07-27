






#ifndef GrInOrderDrawBuffer_DEFINED
#define GrInOrderDrawBuffer_DEFINED

#include "GrDrawTarget.h"
#include "GrAllocPool.h"
#include "GrAllocator.h"
#include "GrPath.h"

#include "SkClipStack.h"
#include "SkTemplates.h"
#include "SkTypes.h"

class GrGpu;
class GrIndexBufferAllocPool;
class GrPathRange;
class GrVertexBufferAllocPool;











class GrInOrderDrawBuffer : public GrDrawTarget {
public:

    








    GrInOrderDrawBuffer(GrGpu* gpu,
                        GrVertexBufferAllocPool* vertexPool,
                        GrIndexBufferAllocPool* indexPool);

    virtual ~GrInOrderDrawBuffer();

    



    void reset();

    





    void flush();

    
    virtual DrawToken getCurrentDrawToken() { return DrawToken(this, fDrawID); }

    
    virtual bool geometryHints(int* vertexCount,
                               int* indexCount) const SK_OVERRIDE;
    virtual void clear(const SkIRect* rect,
                       GrColor color,
                       bool canIgnoreRect,
                       GrRenderTarget* renderTarget) SK_OVERRIDE;

    virtual void discard(GrRenderTarget*) SK_OVERRIDE;

    virtual void initCopySurfaceDstDesc(const GrSurface* src, GrTextureDesc* desc) SK_OVERRIDE;

protected:
    virtual void clipWillBeSet(const GrClipData* newClip) SK_OVERRIDE;

private:
    enum Cmd {
        kDraw_Cmd           = 1,
        kStencilPath_Cmd    = 2,
        kSetState_Cmd       = 3,
        kSetClip_Cmd        = 4,
        kClear_Cmd          = 5,
        kCopySurface_Cmd    = 6,
        kDrawPath_Cmd       = 7,
        kDrawPaths_Cmd      = 8,
    };

    class DrawRecord : public DrawInfo {
    public:
        DrawRecord(const DrawInfo& info) : DrawInfo(info) {}
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
    };

    struct StencilPath : public ::SkNoncopyable {
        StencilPath();

        SkAutoTUnref<const GrPath>  fPath;
        SkPath::FillType            fFill;
    };

    struct DrawPath : public ::SkNoncopyable {
        DrawPath();

        SkAutoTUnref<const GrPath>  fPath;
        SkPath::FillType            fFill;
        GrDeviceCoordTexture        fDstCopy;
    };

    struct DrawPaths : public ::SkNoncopyable {
        DrawPaths();
        ~DrawPaths();

        SkAutoTUnref<const GrPathRange> fPathRange;
        uint32_t* fIndices;
        size_t fCount;
        float* fTransforms;
        PathTransformType fTransformsType;
        SkPath::FillType fFill;
        GrDeviceCoordTexture fDstCopy;
    };

    
    struct Clear : public ::SkNoncopyable {
        Clear() : fRenderTarget(NULL) {}
        ~Clear() { SkSafeUnref(fRenderTarget); }

        SkIRect         fRect;
        GrColor         fColor;
        bool            fCanIgnoreRect;
        GrRenderTarget* fRenderTarget;
    };

    struct CopySurface : public ::SkNoncopyable {
        SkAutoTUnref<GrSurface> fDst;
        SkAutoTUnref<GrSurface> fSrc;
        SkIRect                 fSrcRect;
        SkIPoint                fDstPoint;
    };

    struct Clip : public ::SkNoncopyable {
        SkClipStack fStack;
        SkIPoint    fOrigin;
    };

    
    virtual void onDraw(const DrawInfo&) SK_OVERRIDE;
    virtual void onDrawRect(const SkRect& rect,
                            const SkMatrix* matrix,
                            const SkRect* localRect,
                            const SkMatrix* localMatrix) SK_OVERRIDE;

    virtual void onStencilPath(const GrPath*, SkPath::FillType) SK_OVERRIDE;
    virtual void onDrawPath(const GrPath*, SkPath::FillType,
                            const GrDeviceCoordTexture* dstCopy) SK_OVERRIDE;
    virtual void onDrawPaths(const GrPathRange*,
                             const uint32_t indices[], int count,
                             const float transforms[], PathTransformType,
                             SkPath::FillType, const GrDeviceCoordTexture*) SK_OVERRIDE;

    virtual bool onReserveVertexSpace(size_t vertexSize,
                                      int vertexCount,
                                      void** vertices) SK_OVERRIDE;
    virtual bool onReserveIndexSpace(int indexCount,
                                     void** indices) SK_OVERRIDE;
    virtual void releaseReservedVertexSpace() SK_OVERRIDE;
    virtual void releaseReservedIndexSpace() SK_OVERRIDE;
    virtual void onSetVertexSourceToArray(const void* vertexArray,
                                          int vertexCount) SK_OVERRIDE;
    virtual void onSetIndexSourceToArray(const void* indexArray,
                                         int indexCount) SK_OVERRIDE;
    virtual void releaseVertexArray() SK_OVERRIDE;
    virtual void releaseIndexArray() SK_OVERRIDE;
    virtual void geometrySourceWillPush() SK_OVERRIDE;
    virtual void geometrySourceWillPop(const GeometrySrcState& restoredState) SK_OVERRIDE;
    virtual void willReserveVertexAndIndexSpace(int vertexCount,
                                                int indexCount) SK_OVERRIDE;
    virtual bool onCopySurface(GrSurface* dst,
                               GrSurface* src,
                               const SkIRect& srcRect,
                               const SkIPoint& dstPoint)  SK_OVERRIDE;
    virtual bool onCanCopySurface(GrSurface* dst,
                                  GrSurface* src,
                                  const SkIRect& srcRect,
                                  const SkIPoint& dstPoint) SK_OVERRIDE;

    bool quickInsideClip(const SkRect& devBounds);

    virtual void didAddGpuTraceMarker() SK_OVERRIDE {}
    virtual void didRemoveGpuTraceMarker() SK_OVERRIDE {}

    
    
    int concatInstancedDraw(const DrawInfo& info);

    
    
    bool needsNewState() const;
    bool needsNewClip() const;

    
    void            recordState();
    void            recordClip();
    DrawRecord*     recordDraw(const DrawInfo&);
    StencilPath*    recordStencilPath();
    DrawPath*       recordDrawPath();
    DrawPaths*      recordDrawPaths();
    Clear*          recordClear();
    CopySurface*    recordCopySurface();

    
    enum {
        kCmdPreallocCnt          = 32,
        kDrawPreallocCnt         = 16,
        kStencilPathPreallocCnt  = 8,
        kDrawPathPreallocCnt     = 8,
        kDrawPathsPreallocCnt    = 8,
        kStatePreallocCnt        = 8,
        kClipPreallocCnt         = 8,
        kClearPreallocCnt        = 8,
        kGeoPoolStatePreAllocCnt = 4,
        kCopySurfacePreallocCnt  = 4,
    };

    typedef GrTAllocator<DrawRecord>                        DrawAllocator;
    typedef GrTAllocator<StencilPath>                       StencilPathAllocator;
    typedef GrTAllocator<DrawPath>                          DrawPathAllocator;
    typedef GrTAllocator<DrawPaths>                         DrawPathsAllocator;
    typedef GrTAllocator<GrDrawState>                       StateAllocator;
    typedef GrTAllocator<Clear>                             ClearAllocator;
    typedef GrTAllocator<CopySurface>                       CopySurfaceAllocator;
    typedef GrTAllocator<Clip>                              ClipAllocator;

    GrSTAllocator<kDrawPreallocCnt, DrawRecord>                        fDraws;
    GrSTAllocator<kStencilPathPreallocCnt, StencilPath>                fStencilPaths;
    GrSTAllocator<kDrawPathPreallocCnt, DrawPath>                      fDrawPath;
    GrSTAllocator<kDrawPathsPreallocCnt, DrawPaths>                    fDrawPaths;
    GrSTAllocator<kStatePreallocCnt, GrDrawState>                      fStates;
    GrSTAllocator<kClearPreallocCnt, Clear>                            fClears;
    GrSTAllocator<kCopySurfacePreallocCnt, CopySurface>                fCopySurfaces;
    GrSTAllocator<kClipPreallocCnt, Clip>                              fClips;

    SkTArray<GrTraceMarkerSet, false>                                  fGpuCmdMarkers;

    SkSTArray<kCmdPreallocCnt, uint8_t, true>                          fCmds;

    GrDrawTarget*                   fDstGpu;

    bool                            fClipSet;

    enum ClipProxyState {
        kUnknown_ClipProxyState,
        kValid_ClipProxyState,
        kInvalid_ClipProxyState
    };
    ClipProxyState                  fClipProxyState;
    SkRect                          fClipProxy;

    GrVertexBufferAllocPool&        fVertexPool;

    GrIndexBufferAllocPool&         fIndexPool;

    struct GeometryPoolState {
        const GrVertexBuffer*           fPoolVertexBuffer;
        int                             fPoolStartVertex;
        const GrIndexBuffer*            fPoolIndexBuffer;
        int                             fPoolStartIndex;
        
        
        
        size_t                          fUsedPoolVertexBytes;
        size_t                          fUsedPoolIndexBytes;
    };
    SkSTArray<kGeoPoolStatePreAllocCnt, GeometryPoolState> fGeoPoolStateStack;

    virtual bool       isIssued(uint32_t drawID) { return drawID != fDrawID; }

    void addToCmdBuffer(uint8_t cmd);

    bool                            fFlushing;
    uint32_t                        fDrawID;

    typedef GrDrawTarget INHERITED;
};

#endif
