









#ifndef GrInOrderDrawBuffer_DEFINED
#define GrInOrderDrawBuffer_DEFINED

#include "GrDrawTarget.h"
#include "GrAllocPool.h"
#include "GrAllocator.h"
#include "GrClip.h"

class GrGpu;
class GrIndexBufferAllocPool;
class GrVertexBufferAllocPool;













class GrInOrderDrawBuffer : public GrDrawTarget {
public:

    










    GrInOrderDrawBuffer(const GrGpu* gpu,
                        GrVertexBufferAllocPool* vertexPool,
                        GrIndexBufferAllocPool* indexPool);

    virtual ~GrInOrderDrawBuffer();

    




    void initializeDrawStateAndClip(const GrDrawTarget& target);

    




    void setQuadIndexBuffer(const GrIndexBuffer* indexBuffer);

    


    void reset();

    




    void playback(GrDrawTarget* target);
    
    
    virtual void drawRect(const GrRect& rect, 
                          const GrMatrix* matrix = NULL,
                          int stageEnableMask = 0,
                          const GrRect* srcRects[] = NULL,
                          const GrMatrix* srcMatrices[] = NULL);

    virtual bool geometryHints(GrVertexLayout vertexLayout,
                               int* vertexCount,
                               int* indexCount) const;

    virtual void clear(const GrIRect* rect, GrColor color);

private:

    struct Draw {
        GrPrimitiveType         fPrimitiveType;
        int                     fStartVertex;
        int                     fStartIndex;
        int                     fVertexCount;
        int                     fIndexCount;
        bool                    fStateChanged;
        bool                    fClipChanged;
        GrVertexLayout          fVertexLayout;
        const GrVertexBuffer*   fVertexBuffer;
        const GrIndexBuffer*    fIndexBuffer;
    };

    struct Clear {
        int fBeforeDrawIdx;
        GrIRect fRect;
        GrColor fColor;
    };

    
    virtual void onDrawIndexed(GrPrimitiveType primitiveType,
                               int startVertex,
                               int startIndex,
                               int vertexCount,
                               int indexCount);
    virtual void onDrawNonIndexed(GrPrimitiveType primitiveType,
                                  int startVertex,
                                  int vertexCount);
    virtual bool onReserveVertexSpace(GrVertexLayout layout, 
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
    virtual void clipWillBeSet(const GrClip& newClip);

    bool needsNewState() const;
    bool needsNewClip() const;

    void pushState();
    void pushClip();
    
    enum {
        kDrawPreallocCnt         = 8,
        kStatePreallocCnt        = 8,
        kClipPreallocCnt         = 8,
        kClearPreallocCnt        = 4,
        kGeoPoolStatePreAllocCnt = 4,
    };

    const GrGpu*                    fGpu;

    GrSTAllocator<kDrawPreallocCnt, Draw>               fDraws;
    GrSTAllocator<kStatePreallocCnt, SavedDrawState>    fStates;
    GrSTAllocator<kClearPreallocCnt, Clear>             fClears;
    GrSTAllocator<kClipPreallocCnt, GrClip>             fClips;
    
    bool                            fClipSet;

    GrVertexLayout                  fLastRectVertexLayout;
    const GrIndexBuffer*            fQuadIndexBuffer;
    int                             fMaxQuads;
    int                             fCurrQuad;

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

    typedef GrDrawTarget INHERITED;
};

#endif
