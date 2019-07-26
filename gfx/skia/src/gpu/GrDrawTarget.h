








#ifndef GrDrawTarget_DEFINED
#define GrDrawTarget_DEFINED

#include "GrClipData.h"
#include "GrDrawState.h"
#include "GrIndexBuffer.h"
#include "SkMatrix.h"
#include "GrRefCnt.h"

#include "SkClipStack.h"
#include "SkPath.h"
#include "SkTLazy.h"
#include "SkTArray.h"
#include "SkXfermode.h"

class GrClipData;
class GrDrawTargetCaps;
class GrPath;
class GrVertexBuffer;
class SkStrokeRec;

class GrDrawTarget : public GrRefCnt {
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

    




    void stencilPath(const GrPath*, const SkStrokeRec& stroke, SkPath::FillType fill);

    




















    virtual void drawRect(const GrRect& rect,
                          const SkMatrix* matrix,
                          const GrRect* localRect,
                          const SkMatrix* localMatrix);

    


    void drawSimpleRect(const GrRect& rect, const SkMatrix* matrix = NULL) {
        drawRect(rect, matrix, NULL, NULL);
    }
    void drawSimpleRect(const GrIRect& irect, const SkMatrix* matrix = NULL) {
        SkRect rect = SkRect::MakeFromIRect(irect);
        this->drawRect(rect, matrix, NULL, NULL);
    }

    





























    void drawIndexedInstances(GrPrimitiveType type,
                              int instanceCount,
                              int verticesPerInstance,
                              int indicesPerInstance,
                              const SkRect* devBounds = NULL);

    




    virtual void clear(const GrIRect* rect,
                       GrColor color,
                       GrRenderTarget* renderTarget = NULL) = 0;

    



    virtual void purgeResources() {};

    


    void executeDraw(const DrawInfo& info) { this->onDraw(info); }

    

    


    enum ASRInit {
        kPreserve_ASRInit,
        kReset_ASRInit
    };

    














    class AutoStateRestore : ::GrNoncopyable {
    public:
        


        AutoStateRestore();

        






        AutoStateRestore(GrDrawTarget* target, ASRInit init);

        ~AutoStateRestore();

        








        void set(GrDrawTarget* target, ASRInit init);

    private:
        GrDrawTarget*        fDrawTarget;
        SkTLazy<GrDrawState> fTempState;
        GrDrawState*         fSavedState;
    };

    

    class AutoReleaseGeometry : ::GrNoncopyable {
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
        void* vertices() const { GrAssert(this->succeeded()); return fVertices; }
        void* indices() const { GrAssert(this->succeeded()); return fIndices; }
        GrPoint* positions() const {
            return static_cast<GrPoint*>(this->vertices());
        }

    private:
        void reset();

        GrDrawTarget* fTarget;
        void*         fVertices;
        void*         fIndices;
    };

    

    class AutoClipRestore : ::GrNoncopyable {
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

    

    class AutoGeometryAndStatePush : ::GrNoncopyable {
    public:
        AutoGeometryAndStatePush(GrDrawTarget* target, ASRInit init)
            : fState(target, init) {
            GrAssert(NULL != target);
            fTarget = target;
            target->pushGeometrySource();
        }
        ~AutoGeometryAndStatePush() {
            fTarget->popGeometrySource();
        }
    private:
        GrDrawTarget*    fTarget;
        AutoStateRestore fState;
    };

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
                return src.fIndexBuffer->sizeInBytes() / sizeof(uint16_t);
            default:
                GrCrash("Unexpected Index Source.");
                return 0;
        }
    }

    GrContext* getContext() { return fContext; }
    const GrContext* getContext() const { return fContext; }

    
    
    virtual void clipWillBeSet(const GrClipData* clipData);

    
    
    
    void releaseGeometry();

    
    const GeometrySrcState& getGeomSrc() const { return fGeoSrcStateStack.back(); }
    
    size_t getVertexSize() const {
        
        GrAssert(this->getGeomSrc().fVertexSrc != kNone_GeometrySrcType);
        return this->getGeomSrc().fVertexSize;
    }

    
    SkAutoTUnref<const GrDrawTargetCaps> fCaps;

    


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
#if GR_DEBUG
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

        bool getDevIBounds(SkIRect* bounds) const {
            if (NULL != fDevBounds) {
                fDevBounds->roundOut(bounds);
                return true;
            } else {
                return false;
            }
        }

        
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
    virtual void onStencilPath(const GrPath*, const SkStrokeRec& stroke, SkPath::FillType fill) = 0;

    
    bool reserveVertexSpace(size_t vertexSize,
                            int vertexCount,
                            void** vertices);
    bool reserveIndexSpace(int indexCount, void** indices);

    
    
    bool checkDraw(GrPrimitiveType type, int startVertex,
                   int startIndex, int vertexCount,
                   int indexCount) const;
    
    void releasePreviousVertexSource();
    void releasePreviousIndexSource();

    
    
    bool setupDstReadIfNecessary(DrawInfo* info);

    enum {
        kPreallocGeoSrcStateStackCnt = 4,
    };
    SkSTArray<kPreallocGeoSrcStateStackCnt, GeometrySrcState, true> fGeoSrcStateStack;
    const GrClipData*                                               fClip;
    GrDrawState*                                                    fDrawState;
    GrDrawState                                                     fDefaultDrawState;
    
    GrContext*                                                      fContext;

    typedef GrRefCnt INHERITED;
};

#endif
