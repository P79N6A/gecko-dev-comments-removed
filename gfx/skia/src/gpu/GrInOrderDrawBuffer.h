









#ifndef GrInOrderDrawBuffer_DEFINED
#define GrInOrderDrawBuffer_DEFINED

#include "GrDrawTarget.h"
#include "GrAllocPool.h"
#include "GrAllocator.h"
#include "GrPath.h"

#include "SkClipStack.h"
#include "SkStrokeRec.h"
#include "SkTemplates.h"

class GrGpu;
class GrIndexBufferAllocPool;
class GrVertexBufferAllocPool;











class GrInOrderDrawBuffer : public GrDrawTarget {
public:

    








    GrInOrderDrawBuffer(GrGpu* gpu,
                        GrVertexBufferAllocPool* vertexPool,
                        GrIndexBufferAllocPool* indexPool);

    virtual ~GrInOrderDrawBuffer();

    



    void reset();

    







    bool flush();

    bool isFlushing() const { return fFlushing; }

    
    virtual bool geometryHints(int* vertexCount,
                               int* indexCount) const SK_OVERRIDE;
    virtual void clear(const GrIRect* rect,
                       GrColor color,
                       GrRenderTarget* renderTarget = NULL) SK_OVERRIDE;
    virtual void drawRect(const GrRect& rect,
                          const SkMatrix* matrix,
                          const GrRect* localRect,
                          const SkMatrix* localMatrix) SK_OVERRIDE;

protected:
    virtual void clipWillBeSet(const GrClipData* newClip) SK_OVERRIDE;

private:
    enum Cmd {
        kDraw_Cmd           = 1,
        kStencilPath_Cmd    = 2,
        kSetState_Cmd       = 3,
        kSetClip_Cmd        = 4,
        kClear_Cmd          = 5,
    };

    class DrawRecord : public DrawInfo {
    public:
        DrawRecord(const DrawInfo& info) : DrawInfo(info) {}
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
    };

    struct StencilPath {
        StencilPath();

        SkAutoTUnref<const GrPath>  fPath;
        SkStrokeRec                 fStroke;
        SkPath::FillType            fFill;
    };

    struct Clear {
        Clear() : fRenderTarget(NULL) {}
        ~Clear() { GrSafeUnref(fRenderTarget); }

        GrIRect         fRect;
        GrColor         fColor;
        GrRenderTarget* fRenderTarget;
    };

    
    virtual void onDraw(const DrawInfo&) SK_OVERRIDE;
    virtual void onStencilPath(const GrPath*, const SkStrokeRec& stroke, SkPath::FillType) SK_OVERRIDE;
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
    bool quickInsideClip(const SkRect& devBounds);

    
    
    int concatInstancedDraw(const DrawInfo& info);

    
    
    bool needsNewState() const;
    bool needsNewClip() const;

    
    void            recordState();
    void            recordClip();
    DrawRecord*     recordDraw(const DrawInfo&);
    StencilPath*    recordStencilPath();
    Clear*          recordClear();

    enum {
        kCmdPreallocCnt          = 32,
        kDrawPreallocCnt         = 8,
        kStencilPathPreallocCnt  = 8,
        kStatePreallocCnt        = 8,
        kClipPreallocCnt         = 8,
        kClearPreallocCnt        = 4,
        kGeoPoolStatePreAllocCnt = 4,
    };

    SkSTArray<kCmdPreallocCnt, uint8_t, true>                          fCmds;
    GrSTAllocator<kDrawPreallocCnt, DrawRecord>                        fDraws;
    GrSTAllocator<kStatePreallocCnt, StencilPath>                      fStencilPaths;
    GrSTAllocator<kStatePreallocCnt, GrDrawState::DeferredState>       fStates;
    GrSTAllocator<kClearPreallocCnt, Clear>                            fClears;

    GrSTAllocator<kClipPreallocCnt, SkClipStack>        fClips;
    GrSTAllocator<kClipPreallocCnt, SkIPoint>           fClipOrigins;

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

    bool                            fFlushing;

    typedef GrDrawTarget INHERITED;
};

#endif
