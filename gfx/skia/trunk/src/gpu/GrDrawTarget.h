






#ifndef GrDrawTarget_DEFINED
#define GrDrawTarget_DEFINED

#include "GrClipData.h"
#include "GrContext.h"
#include "GrDrawState.h"
#include "GrIndexBuffer.h"
#include "GrTraceMarker.h"

#include "SkClipStack.h"
#include "SkMatrix.h"
#include "SkPath.h"
#include "SkStrokeRec.h"
#include "SkTArray.h"
#include "SkTLazy.h"
#include "SkTypes.h"
#include "SkXfermode.h"

class GrClipData;
class GrDrawTargetCaps;
class GrPath;
class GrPathRange;
class GrVertexBuffer;

class GrDrawTarget : public SkRefCnt {
protected:
    class DrawInfo;

public:
    SK_DECLARE_INST_COUNT(GrDrawTarget)

    

    
    
    GrDrawTarget(GrContext* context);
    virtual ~GrDrawTarget();

    


    const GrDrawTargetCaps* caps() const { return fCaps.get(); }

    







    void setClip(const GrClipData* clip);

    




    const GrClipData* getClip() const;

    





    void setDrawState(GrDrawState*  drawState);

    


    const GrDrawState& getDrawState() const { return *fDrawState; }

    



    GrDrawState* drawState() { return fDrawState; }

    
















    bool canApplyCoverage() const;

    

    bool shouldDisableCoverageAAForBlend() {
        
        
        
        return !this->canApplyCoverage();
    }

    



    bool willUseHWAALines() const;

    













































    


























     bool reserveVertexAndIndexSpace(int vertexCount,
                                     int indexCount,
                                     void** vertices,
                                     void** indices);

    





















    virtual bool geometryHints(int* vertexCount,
                               int* indexCount) const;

    







    void setVertexSourceToArray(const void* vertexArray, int vertexCount);

    






    void setIndexSourceToArray(const void* indexArray, int indexCount);

    







    void setVertexSourceToBuffer(const GrVertexBuffer* buffer);

    






    void setIndexSourceToBuffer(const GrIndexBuffer* buffer);

    





    void resetVertexSource();

    





    void resetIndexSource();

    


    bool hasReservedVerticesOrIndices() const {
        return kReserved_GeometrySrcType == this->getGeomSrc().fVertexSrc ||
        kReserved_GeometrySrcType == this->getGeomSrc().fIndexSrc;
    }

    




    void pushGeometrySource();

    


    void popGeometrySource();

    














    void drawIndexed(GrPrimitiveType type,
                     int startVertex,
                     int startIndex,
                     int vertexCount,
                     int indexCount,
                     const SkRect* devBounds = NULL);

    










    void drawNonIndexed(GrPrimitiveType type,
                        int startVertex,
                        int vertexCount,
                        const SkRect* devBounds = NULL);

    




    void stencilPath(const GrPath*, SkPath::FillType fill);

    



    void drawPath(const GrPath*, SkPath::FillType fill);

    











    enum PathTransformType {
        kNone_PathTransformType,        
        kTranslateX_PathTransformType,  
        kTranslateY_PathTransformType,  
        kTranslate_PathTransformType,   
        kAffine_PathTransformType,      

        kLast_PathTransformType = kAffine_PathTransformType
    };
    void drawPaths(const GrPathRange* pathRange,
                   const uint32_t indices[], int count,
                   const float transforms[], PathTransformType transformsType,
                   SkPath::FillType fill);

    static inline int PathTransformSize(PathTransformType type) {
        switch (type) {
            case kNone_PathTransformType:
                return 0;
            case kTranslateX_PathTransformType:
            case kTranslateY_PathTransformType:
                return 1;
            case kTranslate_PathTransformType:
                return 2;
            case kAffine_PathTransformType:
                return 6;

            default:
                SkFAIL("Unknown path transform type");
                return 0;
        }
    }

    












    void drawRect(const SkRect& rect,
                  const SkMatrix* matrix,
                  const SkRect* localRect,
                  const SkMatrix* localMatrix) {
        AutoGeometryPush agp(this);
        this->onDrawRect(rect, matrix, localRect, localMatrix);
    }

    


    void drawSimpleRect(const SkRect& rect, const SkMatrix* matrix = NULL) {
        this->drawRect(rect, matrix, NULL, NULL);
    }
    void drawSimpleRect(const SkIRect& irect, const SkMatrix* matrix = NULL) {
        SkRect rect = SkRect::Make(irect);
        this->drawRect(rect, matrix, NULL, NULL);
    }

    





























    void drawIndexedInstances(GrPrimitiveType type,
                              int instanceCount,
                              int verticesPerInstance,
                              int indicesPerInstance,
                              const SkRect* devBounds = NULL);

    





    virtual void clear(const SkIRect* rect,
                       GrColor color,
                       bool canIgnoreRect,
                       GrRenderTarget* renderTarget = NULL) = 0;

    



    virtual void discard(GrRenderTarget* = NULL) = 0;

    




    void addGpuTraceMarker(const GrGpuTraceMarker* marker);
    void removeGpuTraceMarker(const GrGpuTraceMarker* marker);

    







    void saveActiveTraceMarkers();
    void restoreActiveTraceMarkers();

    










    bool copySurface(GrSurface* dst,
                     GrSurface* src,
                     const SkIRect& srcRect,
                     const SkIPoint& dstPoint);
    



    bool canCopySurface(GrSurface* dst,
                        GrSurface* src,
                        const SkIRect& srcRect,
                        const SkIPoint& dstPoint);

    




    virtual void initCopySurfaceDstDesc(const GrSurface* src, GrTextureDesc* desc);


    



    virtual void purgeResources() {};

    


    void executeDraw(const DrawInfo& info) { this->onDraw(info); }

    


    void executeDrawPath(const GrPath* path, SkPath::FillType fill,
                         const GrDeviceCoordTexture* dstCopy) {
        this->onDrawPath(path, fill, dstCopy);
    }

    


    void executeDrawPaths(const GrPathRange* pathRange,
                          const uint32_t indices[], int count,
                          const float transforms[], PathTransformType transformsType,
                          SkPath::FillType fill,
                          const GrDeviceCoordTexture* dstCopy) {
        this->onDrawPaths(pathRange, indices, count, transforms, transformsType, fill, dstCopy);
    }

    inline bool isGpuTracingEnabled() const {
        return this->getContext()->isGpuTracingEnabled();
    }

    

    


    enum ASRInit {
        kPreserve_ASRInit,
        kReset_ASRInit
    };

    














    class AutoStateRestore : public ::SkNoncopyable {
    public:
        


        AutoStateRestore();

        










        AutoStateRestore(GrDrawTarget* target, ASRInit init, const SkMatrix* viewMatrix = NULL);

        ~AutoStateRestore();

        












        void set(GrDrawTarget* target, ASRInit init, const SkMatrix* viewMatrix = NULL);

        





        bool setIdentity(GrDrawTarget* target, ASRInit init);

    private:
        GrDrawTarget*                       fDrawTarget;
        SkTLazy<GrDrawState>                fTempState;
        GrDrawState*                        fSavedState;
    };

    

    class AutoReleaseGeometry : public ::SkNoncopyable {
    public:
        AutoReleaseGeometry(GrDrawTarget*  target,
                            int            vertexCount,
                            int            indexCount);
        AutoReleaseGeometry();
        ~AutoReleaseGeometry();
        bool set(GrDrawTarget*  target,
                 int            vertexCount,
                 int            indexCount);
        bool succeeded() const { return NULL != fTarget; }
        void* vertices() const { SkASSERT(this->succeeded()); return fVertices; }
        void* indices() const { SkASSERT(this->succeeded()); return fIndices; }
        SkPoint* positions() const {
            return static_cast<SkPoint*>(this->vertices());
        }

    private:
        void reset();

        GrDrawTarget* fTarget;
        void*         fVertices;
        void*         fIndices;
    };

    

    class AutoClipRestore : public ::SkNoncopyable {
    public:
        AutoClipRestore(GrDrawTarget* target) {
            fTarget = target;
            fClip = fTarget->getClip();
        }

        AutoClipRestore(GrDrawTarget* target, const SkIRect& newClip);

        ~AutoClipRestore() {
            fTarget->setClip(fClip);
        }
    private:
        GrDrawTarget*           fTarget;
        const GrClipData*       fClip;
        SkTLazy<SkClipStack>    fStack;
        GrClipData              fReplacementClip;
    };

    

    



    class AutoGeometryPush : public ::SkNoncopyable {
    public:
        AutoGeometryPush(GrDrawTarget* target)
            : fAttribRestore(target->drawState()) {
            SkASSERT(NULL != target);
            fTarget = target;
            target->pushGeometrySource();
        }

        ~AutoGeometryPush() { fTarget->popGeometrySource(); }

    private:
        GrDrawTarget*                           fTarget;
        GrDrawState::AutoVertexAttribRestore    fAttribRestore;
    };

    



    class AutoGeometryAndStatePush : public ::SkNoncopyable {
    public:
        AutoGeometryAndStatePush(GrDrawTarget* target,
                                 ASRInit init,
                                 const SkMatrix* viewMatrix = NULL)
            : fState(target, init, viewMatrix) {
            SkASSERT(NULL != target);
            fTarget = target;
            target->pushGeometrySource();
            if (kPreserve_ASRInit == init) {
                target->drawState()->setDefaultVertexAttribs();
            }
        }

        ~AutoGeometryAndStatePush() { fTarget->popGeometrySource(); }

    private:
        AutoStateRestore fState;
        GrDrawTarget*    fTarget;
    };

    
    
    class DrawToken {
    public:
        DrawToken(GrDrawTarget* drawTarget, uint32_t drawID) :
                  fDrawTarget(drawTarget), fDrawID(drawID) {}

        bool isIssued() { return NULL != fDrawTarget && fDrawTarget->isIssued(fDrawID); }

    private:
        GrDrawTarget*  fDrawTarget;
        uint32_t       fDrawID;   
                                  
    };

    virtual DrawToken getCurrentDrawToken() { return DrawToken(this, 0); }

protected:

    enum GeometrySrcType {
        kNone_GeometrySrcType,     
        kReserved_GeometrySrcType, 
        kArray_GeometrySrcType,    
        kBuffer_GeometrySrcType    
    };

    struct GeometrySrcState {
        GeometrySrcType         fVertexSrc;
        union {
            
            const GrVertexBuffer*   fVertexBuffer;
            
            int                     fVertexCount;
        };

        GeometrySrcType         fIndexSrc;
        union {
            
            const GrIndexBuffer*    fIndexBuffer;
            
            int                     fIndexCount;
        };

        size_t                  fVertexSize;
    };

    int indexCountInCurrentSource() const {
        const GeometrySrcState& src = this->getGeomSrc();
        switch (src.fIndexSrc) {
            case kNone_GeometrySrcType:
                return 0;
            case kReserved_GeometrySrcType:
            case kArray_GeometrySrcType:
                return src.fIndexCount;
            case kBuffer_GeometrySrcType:
                return static_cast<int>(src.fIndexBuffer->gpuMemorySize() / sizeof(uint16_t));
            default:
                SkFAIL("Unexpected Index Source.");
                return 0;
        }
    }

    
    
    
    
    virtual bool onCopySurface(GrSurface* dst,
                               GrSurface* src,
                               const SkIRect& srcRect,
                               const SkIPoint& dstPoint);

    
    
    
    
    
    virtual bool onCanCopySurface(GrSurface* dst,
                                  GrSurface* src,
                                  const SkIRect& srcRect,
                                  const SkIPoint& dstPoint);

    GrContext* getContext() { return fContext; }
    const GrContext* getContext() const { return fContext; }

    
    
    virtual void clipWillBeSet(const GrClipData* clipData);

    
    
    
    void releaseGeometry();

    
    const GeometrySrcState& getGeomSrc() const { return fGeoSrcStateStack.back(); }
    
    size_t getVertexSize() const {
        
        SkASSERT(this->getGeomSrc().fVertexSrc != kNone_GeometrySrcType);
        return this->getGeomSrc().fVertexSize;
    }

    
    SkAutoTUnref<const GrDrawTargetCaps> fCaps;

    const GrTraceMarkerSet& getActiveTraceMarkers() { return fActiveTraceMarkers; }

    


    class DrawInfo {
    public:
        DrawInfo(const DrawInfo& di) { (*this) = di; }
        DrawInfo& operator =(const DrawInfo& di);

        GrPrimitiveType primitiveType() const { return fPrimitiveType; }
        int startVertex() const { return fStartVertex; }
        int startIndex() const { return fStartIndex; }
        int vertexCount() const { return fVertexCount; }
        int indexCount() const { return fIndexCount; }
        int verticesPerInstance() const { return fVerticesPerInstance; }
        int indicesPerInstance() const { return fIndicesPerInstance; }
        int instanceCount() const { return fInstanceCount; }

        bool isIndexed() const { return fIndexCount > 0; }
#ifdef SK_DEBUG
        bool isInstanced() const; 
#else
        bool isInstanced() const { return fInstanceCount > 0; }
#endif

        
        void adjustInstanceCount(int instanceOffset);
        
        void adjustStartVertex(int vertexOffset);
        
        void adjustStartIndex(int indexOffset);

        void setDevBounds(const SkRect& bounds) {
            fDevBoundsStorage = bounds;
            fDevBounds = &fDevBoundsStorage;
        }
        const SkRect* getDevBounds() const { return fDevBounds; }

        
        const GrDeviceCoordTexture* getDstCopy() const {
            if (NULL != fDstCopy.texture()) {
                return &fDstCopy;
            } else {
                return NULL;
            }
        }

    private:
        DrawInfo() { fDevBounds = NULL; }

        friend class GrDrawTarget;

        GrPrimitiveType         fPrimitiveType;

        int                     fStartVertex;
        int                     fStartIndex;
        int                     fVertexCount;
        int                     fIndexCount;

        int                     fInstanceCount;
        int                     fVerticesPerInstance;
        int                     fIndicesPerInstance;

        SkRect                  fDevBoundsStorage;
        SkRect*                 fDevBounds;

        GrDeviceCoordTexture    fDstCopy;
    };

private:
    
    
    virtual void willReserveVertexAndIndexSpace(int vertexCount, int indexCount) {}

    
    virtual bool onReserveVertexSpace(size_t vertexSize, int vertexCount, void** vertices) = 0;
    virtual bool onReserveIndexSpace(int indexCount, void** indices) = 0;
    
    virtual void releaseReservedVertexSpace() = 0;
    virtual void releaseReservedIndexSpace() = 0;
    
    virtual void onSetVertexSourceToArray(const void* vertexArray, int vertexCount) = 0;
    virtual void onSetIndexSourceToArray(const void* indexArray, int indexCount) = 0;
    
    virtual void releaseVertexArray() = 0;
    virtual void releaseIndexArray() = 0;
    
    virtual void geometrySourceWillPush() = 0;
    virtual void geometrySourceWillPop(const GeometrySrcState& restoredState) = 0;
    
    virtual void onDraw(const DrawInfo&) = 0;
    
    
    
    
    
    
    virtual void onDrawRect(const SkRect& rect,
                            const SkMatrix* matrix,
                            const SkRect* localRect,
                            const SkMatrix* localMatrix);

    virtual void onStencilPath(const GrPath*, SkPath::FillType) = 0;
    virtual void onDrawPath(const GrPath*, SkPath::FillType,
                            const GrDeviceCoordTexture* dstCopy) = 0;
    virtual void onDrawPaths(const GrPathRange*,
                             const uint32_t indices[], int count,
                             const float transforms[], PathTransformType,
                             SkPath::FillType, const GrDeviceCoordTexture*) = 0;

    virtual void didAddGpuTraceMarker() = 0;
    virtual void didRemoveGpuTraceMarker() = 0;

    
    bool reserveVertexSpace(size_t vertexSize,
                            int vertexCount,
                            void** vertices);
    bool reserveIndexSpace(int indexCount, void** indices);

    
    
    bool checkDraw(GrPrimitiveType type, int startVertex,
                   int startIndex, int vertexCount,
                   int indexCount) const;
    
    void releasePreviousVertexSource();
    void releasePreviousIndexSource();

    
    
    bool setupDstReadIfNecessary(DrawInfo* info) {
        return this->setupDstReadIfNecessary(&info->fDstCopy, info->getDevBounds());
    }
    bool setupDstReadIfNecessary(GrDeviceCoordTexture* dstCopy, const SkRect* drawBounds);

    
    virtual bool       isIssued(uint32_t drawID) { return true; }

    enum {
        kPreallocGeoSrcStateStackCnt = 4,
    };
    SkSTArray<kPreallocGeoSrcStateStackCnt, GeometrySrcState, true> fGeoSrcStateStack;
    const GrClipData*                                               fClip;
    GrDrawState*                                                    fDrawState;
    GrDrawState                                                     fDefaultDrawState;
    
    GrContext*                                                      fContext;
    
    int                                                             fGpuTraceMarkerCount;
    GrTraceMarkerSet                                                fActiveTraceMarkers;
    GrTraceMarkerSet                                                fStoredTraceMarkers;

    typedef SkRefCnt INHERITED;
};

#endif
