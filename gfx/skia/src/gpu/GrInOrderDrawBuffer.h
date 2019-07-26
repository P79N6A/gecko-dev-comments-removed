









#ifndef GrInOrderDrawBuffer_DEFINED
#define GrInOrderDrawBuffer_DEFINED

#include "GrDrawTarget.h"
#include "GrAllocPool.h"
#include "GrAllocator.h"
#include "GrPath.h"

#include "SkClipStack.h"
#include "SkTemplates.h"

class GrGpu;
class GrIndexBufferAllocPool;
class GrVertexBufferAllocPool;













class GrInOrderDrawBuffer : public GrDrawTarget {
public:

    










    GrInOrderDrawBuffer(const GrGpu* gpu,
                        GrVertexBufferAllocPool* vertexPool,
                        GrIndexBufferAllocPool* indexPool);

    virtual ~GrInOrderDrawBuffer();

    




    void setQuadIndexBuffer(const GrIndexBuffer* indexBuffer);

    



    void reset();

    











    bool playback(GrDrawTarget* target);

    



    void flushTo(GrDrawTarget* target) {
        if (fFlushing) {
            
            
            
            
            return;
        }

        fFlushing = true;
        if (this->playback(target)) {
            this->reset();
        }
        fFlushing = false;
    }

    







    void setAutoFlushTarget(GrDrawTarget* target);

    
    virtual void drawRect(const GrRect& rect,
                          const GrMatrix* matrix = NULL,
                          const GrRect* srcRects[] = NULL,
                          const GrMatrix* srcMatrices[] = NULL) SK_OVERRIDE;

    virtual void drawIndexedInstances(GrPrimitiveType type,
                                      int instanceCount,
                                      int verticesPerInstance,
                                      int indicesPerInstance)
                                      SK_OVERRIDE;

    virtual bool geometryHints(GrVertexLayout vertexLayout,
                               int* vertexCount,
                               int* indexCount) const SK_OVERRIDE;

    virtual void clear(const GrIRect* rect,
                       GrColor color,
                       GrRenderTarget* renderTarget = NULL) SK_OVERRIDE;

protected:
    virtual void willReserveVertexAndIndexSpace(GrVertexLayout vertexLayout,
                                                int vertexCount,
                                                int indexCount) SK_OVERRIDE;
private:
    enum Cmd {
        kDraw_Cmd           = 1,
        kStencilPath_Cmd    = 2,
        kSetState_Cmd       = 3,
        kSetClip_Cmd        = 4,
        kClear_Cmd          = 5,
    };

    struct Draw {
        GrPrimitiveType         fPrimitiveType;
        int                     fStartVertex;
        int                     fStartIndex;
        int                     fVertexCount;
        int                     fIndexCount;
        GrVertexLayout          fVertexLayout;
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
    };

    struct StencilPath {
        SkAutoTUnref<const GrPath>  fPath;
        GrPathFill                  fFill;
    };

    struct Clear {
        Clear() : fRenderTarget(NULL) {}
        ~Clear() { GrSafeUnref(fRenderTarget); }

        GrIRect         fRect;
        GrColor         fColor;
        GrRenderTarget* fRenderTarget;
    };

    
    virtual void onDrawIndexed(GrPrimitiveType primitiveType,
                               int startVertex,
                               int startIndex,
                               int vertexCount,
                               int indexCount) SK_OVERRIDE;
    virtual void onDrawNonIndexed(GrPrimitiveType primitiveType,
                                  int startVertex,
                                  int vertexCount) SK_OVERRIDE;
    virtual void onStencilPath(const GrPath*, GrPathFill) SK_OVERRIDE;
    virtual bool onReserveVertexSpace(GrVertexLayout layout,
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
    virtual void geometrySourceWillPop(
        const GeometrySrcState& restoredState) SK_OVERRIDE;
    virtual void clipWillBeSet(const GrClipData* newClip) SK_OVERRIDE;

    
    
    bool needsNewState() const;
    bool needsNewClip() const;

    
    void            recordState();
    void            recordDefaultState();
    void            recordClip();
    void            recordDefaultClip();
    Draw*           recordDraw();
    StencilPath*    recordStencilPath();
    Clear*          recordClear();

    
    
    void resetDrawTracking();

    enum {
        kCmdPreallocCnt          = 32,
        kDrawPreallocCnt         = 8,
        kStencilPathPreallocCnt  = 8,
        kStatePreallocCnt        = 8,
        kClipPreallocCnt         = 8,
        kClearPreallocCnt        = 4,
        kGeoPoolStatePreAllocCnt = 4,
    };

    SkSTArray<kCmdPreallocCnt, uint8_t, true>           fCmds;
    GrSTAllocator<kDrawPreallocCnt, Draw>               fDraws;
    GrSTAllocator<kStatePreallocCnt, StencilPath>       fStencilPaths;
    GrSTAllocator<kStatePreallocCnt, GrDrawState>       fStates;
    GrSTAllocator<kClearPreallocCnt, Clear>             fClears;

    GrSTAllocator<kClipPreallocCnt, SkClipStack>        fClips;
    GrSTAllocator<kClipPreallocCnt, SkIPoint>           fClipOrigins;

    GrDrawTarget*                   fAutoFlushTarget;

    bool                            fClipSet;

    GrVertexBufferAllocPool&        fVertexPool;

    GrIndexBufferAllocPool&         fIndexPool;

    
    GrVertexLayout                  fLastRectVertexLayout;
    const GrIndexBuffer*            fQuadIndexBuffer;
    int                             fMaxQuads;
    int                             fCurrQuad;

    
    struct {
        int            fVerticesPerInstance;
        int            fIndicesPerInstance;
        void reset() {
            fVerticesPerInstance = 0;
            fIndicesPerInstance = 0;
        }
    } fInstancedDrawTracker;

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
