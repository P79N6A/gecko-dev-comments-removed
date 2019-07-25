









#ifndef GrDrawTarget_DEFINED
#define GrDrawTarget_DEFINED

#include "GrClip.h"
#include "GrColor.h"
#include "GrDrawState.h"
#include "GrIndexBuffer.h"
#include "GrMatrix.h"
#include "GrRefCnt.h"
#include "GrSamplerState.h"
#include "GrStencil.h"
#include "GrTexture.h"

#include "SkXfermode.h"
#include "SkTLazy.h"

class GrTexture;
class GrClipIterator;
class GrVertexBuffer;
class GrIndexBuffer;

class GrDrawTarget : public GrRefCnt {
public:
    


    struct Caps {
        Caps() { memset(this, 0, sizeof(Caps)); }
        Caps(const Caps& c) { *this = c; }
        Caps& operator= (const Caps& c) {
            memcpy(this, &c, sizeof(Caps));
            return *this;
        }
        void print() const;
        bool f8BitPaletteSupport        : 1;
        bool fNPOTTextureTileSupport    : 1;
        bool fTwoSidedStencilSupport    : 1;
        bool fStencilWrapOpsSupport     : 1;
        bool fHWAALineSupport           : 1;
        bool fShaderDerivativeSupport   : 1;
        bool fGeometryShaderSupport     : 1;
        bool fFSAASupport               : 1;
        bool fDualSourceBlendingSupport : 1;
        bool fBufferLockSupport         : 1;
        int fMaxRenderTargetSize;
        int fMaxTextureSize;
    };

    
    typedef GrDrawState::StageMask StageMask;

    

    GrDrawTarget();
    virtual ~GrDrawTarget();

    


    const Caps& getCaps() const { return fCaps; }

    







    void setClip(const GrClip& clip);

    




    const GrClip& getClip() const;

    





    void setDrawState(GrDrawState*  drawState);

    


    const GrDrawState& getDrawState() const { return *fDrawState; }

    



    GrDrawState* drawState() { return fDrawState; }

    





    void preConcatEnabledSamplerMatrices(const GrMatrix& matrix) {
        StageMask stageMask = this->enabledStages();
        this->drawState()->preConcatSamplerMatrices(stageMask, matrix);
    }

    

















    bool canApplyCoverage() const;

    






    bool canTweakAlphaForCoverage() const;

    





    bool willUseHWAALines() const;

    






















    







    static int StageTexCoordVertexLayoutBit(int stage, int texCoordIdx) {
        GrAssert(stage < GrDrawState::kNumStages);
        GrAssert(texCoordIdx < GrDrawState::kMaxTexCoords);
        return 1 << (stage + (texCoordIdx * GrDrawState::kNumStages));
    }

    virtual void postClipPush() {};
    virtual void preClipPop() {};

private:
    static const int TEX_COORD_BIT_CNT = GrDrawState::kNumStages *
                                         GrDrawState::kMaxTexCoords;

public:
    








    static int StagePosAsTexCoordVertexLayoutBit(int stage) {
        GrAssert(stage < GrDrawState::kNumStages);
        return (1 << (TEX_COORD_BIT_CNT + stage));
    }

    




    void addToVertexLayout(int flag) {
        GrAssert((1 << TEX_COORD_BIT_CNT) == flag ||
                 (1 << (TEX_COORD_BIT_CNT + 1)) == flag ||
                 (1 << (TEX_COORD_BIT_CNT + 2)) == flag ||
                 (1 << (TEX_COORD_BIT_CNT + 3)) == flag);
        fGeoSrcStateStack.back().fVertexLayout |= flag;
    }

private:
    static const int STAGE_BIT_CNT = TEX_COORD_BIT_CNT +
        GrDrawState::kNumStages;

public:

    


    enum VertexLayoutBits {
        
        kColor_VertexLayoutBit              = 1 << (STAGE_BIT_CNT + 0),
        

        kCoverage_VertexLayoutBit           = 1 << (STAGE_BIT_CNT + 1),
        


        kTextFormat_VertexLayoutBit         = 1 << (STAGE_BIT_CNT + 2),

        


        kEdge_VertexLayoutBit               = 1 << (STAGE_BIT_CNT + 3),
        
        kDummyVertexLayoutBit,
        kHighVertexLayoutBit = kDummyVertexLayoutBit - 1
    };
    
    GR_STATIC_ASSERT(kHighVertexLayoutBit < ((uint64_t)1 << 8*sizeof(GrVertexLayout)));

    











































    



























     bool reserveVertexAndIndexSpace(GrVertexLayout vertexLayout,
                                     int vertexCount,
                                     int indexCount,
                                     void** vertices,
                                     void** indices);

    





















    virtual bool geometryHints(GrVertexLayout vertexLayout,
                               int* vertexCount,
                               int* indexCount) const;

    







    void setVertexSourceToArray(GrVertexLayout vertexLayout,
                                const void* vertexArray,
                                int vertexCount);

    






    void setIndexSourceToArray(const void* indexArray, int indexCount);

    







    void setVertexSourceToBuffer(GrVertexLayout vertexLayout,
                                 const GrVertexBuffer* buffer);

    






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
                     int indexCount);

    








    void drawNonIndexed(GrPrimitiveType type,
                        int startVertex,
                        int vertexCount);

    






















    virtual void drawRect(const GrRect& rect,
                          const GrMatrix* matrix,
                          StageMask stageMask,
                          const GrRect* srcRects[],
                          const GrMatrix* srcMatrices[]);

    



























    virtual void drawIndexedInstances(GrPrimitiveType type,
                                      int instanceCount,
                                      int verticesPerInstance,
                                      int indicesPerInstance);

    



    void drawSimpleRect(const GrRect& rect,
                        const GrMatrix* matrix,
                        StageMask stageEnableBitfield) {
         drawRect(rect, matrix, stageEnableBitfield, NULL, NULL);
    }

    




    virtual void clear(const GrIRect* rect, GrColor color) = 0;

    



    virtual void purgeResources() {};

    

    


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

    

    



    class AutoDeviceCoordDraw : ::GrNoncopyable {
    public:
        AutoDeviceCoordDraw(GrDrawTarget* target, StageMask stageMask);
        ~AutoDeviceCoordDraw();
    private:
        GrDrawTarget*       fDrawTarget;
        GrMatrix            fViewMatrix;
        GrMatrix            fSamplerMatrices[GrDrawState::kNumStages];
        int                 fStageMask;
    };

    

    class AutoReleaseGeometry : ::GrNoncopyable {
    public:
        AutoReleaseGeometry(GrDrawTarget*  target,
                            GrVertexLayout vertexLayout,
                            int            vertexCount,
                            int            indexCount);
        AutoReleaseGeometry();
        ~AutoReleaseGeometry();
        bool set(GrDrawTarget*  target,
                 GrVertexLayout vertexLayout,
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

        ~AutoClipRestore() {
            fTarget->setClip(fClip);
        }
    private:
        GrDrawTarget* fTarget;
        GrClip        fClip;
    };
    
    
    
    class AutoGeometryPush : ::GrNoncopyable {
    public:
        AutoGeometryPush(GrDrawTarget* target) {
            GrAssert(NULL != target);
            fTarget = target;
            target->pushGeometrySource();
        }
        ~AutoGeometryPush() {
            fTarget->popGeometrySource();
        }
    private:
        GrDrawTarget* fTarget;
    };

    
    

    



    static size_t VertexSize(GrVertexLayout vertexLayout);

    











    static int VertexTexCoordsForStage(int stage, GrVertexLayout vertexLayout);

    





    static int VertexStageCoordOffset(int stage, GrVertexLayout vertexLayout);

    




    static int VertexColorOffset(GrVertexLayout vertexLayout);

    




    static int VertexCoverageOffset(GrVertexLayout vertexLayout);

     




     static int VertexEdgeOffset(GrVertexLayout vertexLayout);

    









    static bool VertexUsesTexCoordIdx(int coordIndex,
                                      GrVertexLayout vertexLayout);

    









    static bool VertexUsesStage(int stage, GrVertexLayout vertexLayout);

    






















    static int VertexSizeAndOffsetsByIdx(GrVertexLayout vertexLayout,
                   int texCoordOffsetsByIdx[GrDrawState::kMaxTexCoords],
                   int *colorOffset,
                   int *coverageOffset,
                   int* edgeOffset);

    
























    static int VertexSizeAndOffsetsByStage(GrVertexLayout vertexLayout,
                   int texCoordOffsetsByStage[GrDrawState::kNumStages],
                   int* colorOffset,
                   int* coverageOffset,
                   int* edgeOffset);

    





    









    static GrPoint* GetVertexPoint(void* vertices,
                                   int vertexIndex,
                                   int vertexSize,
                                   int offset = 0) {
        intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<GrPoint*>(start + offset +
                                 vertexIndex * vertexSize);
    }
    static const GrPoint* GetVertexPoint(const void* vertices,
                                         int vertexIndex,
                                         int vertexSize,
                                         int offset = 0) {
        intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<const GrPoint*>(start + offset +
                                       vertexIndex * vertexSize);
    }

    







    static GrColor* GetVertexColor(void* vertices,
                                   int vertexIndex,
                                   int vertexSize,
                                   int offset) {
        intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<GrColor*>(start + offset +
                                 vertexIndex * vertexSize);
    }
    static const GrColor* GetVertexColor(const void* vertices,
                                         int vertexIndex,
                                         int vertexSize,
                                         int offset) {
        const intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<const GrColor*>(start + offset +
                                       vertexIndex * vertexSize);
    }

    static void VertexLayoutUnitTest();

protected:

    





    enum BlendOptFlags {
        


        kNone_BlendOpt = 0,
        


        kSkipDraw_BlendOptFlag = 0x2,
        


        kDisableBlend_BlendOptFlag = 0x4,
        



        kCoverageAsAlpha_BlendOptFlag = 0x1,
        



        kEmitCoverage_BlendOptFlag = 0x10,
        



        kEmitTransBlack_BlendOptFlag = 0x8,
    };
    GR_DECL_BITFIELD_OPS_FRIENDS(BlendOptFlags);

    
    
    
    
    
    
    
    
    BlendOptFlags getBlendOpts(bool forceCoverage = false,
                               GrBlendCoeff* srcCoeff = NULL,
                               GrBlendCoeff* dstCoeff = NULL) const;

    
    bool srcAlphaWillBeOne(GrVertexLayout vertexLayout) const;

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
        
        GrVertexLayout          fVertexLayout;
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
    
    static bool StageWillBeUsed(int stage, GrVertexLayout layout, 
                                const GrDrawState& state) {
        return NULL != state.getTexture(stage) &&
               VertexUsesStage(stage, layout);
    }

    bool isStageEnabled(int stage) const {
        return StageWillBeUsed(stage, this->getVertexLayout(),
                               this->getDrawState());
    }

    StageMask enabledStages() const {
        StageMask mask = 0;
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            mask |= this->isStageEnabled(s) ? 1 : 0;
        }
        return mask;
    }

    
    
    virtual void willReserveVertexAndIndexSpace(GrVertexLayout vertexLayout,
                                                int vertexCount,
                                                int indexCount) {}
    

    
    virtual bool onReserveVertexSpace(GrVertexLayout vertexLayout,
                                      int vertexCount,
                                      void** vertices) = 0;
    virtual bool onReserveIndexSpace(int indexCount, void** indices) = 0;
    
    virtual void releaseReservedVertexSpace() = 0;
    virtual void releaseReservedIndexSpace() = 0;
    
    virtual void onSetVertexSourceToArray(const void* vertexArray,
                                          int vertexCount) = 0;
    virtual void onSetIndexSourceToArray(const void* indexArray,
                                         int indexCount) = 0;
    
    virtual void releaseVertexArray() = 0;
    virtual void releaseIndexArray() = 0;
    
    
    virtual void geometrySourceWillPush() = 0;
    virtual void geometrySourceWillPop(const GeometrySrcState& restoredState) = 0;
    
    virtual void onDrawIndexed(GrPrimitiveType type,
                               int startVertex,
                               int startIndex,
                               int vertexCount,
                               int indexCount) = 0;
    virtual void onDrawNonIndexed(GrPrimitiveType type,
                                  int startVertex,
                                  int vertexCount) = 0;
    
    
    virtual void clipWillBeSet(const GrClip& clip) {}

    
    
    static GrVertexLayout GetRectVertexLayout(StageMask stageEnableBitfield,
                                              const GrRect* srcRects[]);

    static void SetRectVertices(const GrRect& rect,
                                const GrMatrix* matrix,
                                const GrRect* srcRects[],
                                const GrMatrix* srcMatrices[],
                                GrVertexLayout layout,
                                void* vertices);

    
    const GeometrySrcState& getGeomSrc() const {
        return fGeoSrcStateStack.back();
    }
    
    
    GrVertexLayout getVertexLayout() const {
        
        
        GrAssert(this->getGeomSrc().fVertexSrc != kNone_GeometrySrcType);
        return this->getGeomSrc().fVertexLayout;
    }

    GrClip fClip;

    GrDrawState* fDrawState;
    GrDrawState fDefaultDrawState;

    Caps fCaps;

    
    
    
    void releaseGeometry();

private:
    
    bool reserveVertexSpace(GrVertexLayout vertexLayout,
                            int vertexCount,
                            void** vertices);
    bool reserveIndexSpace(int indexCount, void** indices);
    
    
    
    bool checkDraw(GrPrimitiveType type, int startVertex,
                   int startIndex, int vertexCount,
                   int indexCount) const;
    
    void releasePreviousVertexSource();
    void releasePreviousIndexSource();
    
    enum {
        kPreallocGeoSrcStateStackCnt = 4,
    };
    SkSTArray<kPreallocGeoSrcStateStackCnt, 
              GeometrySrcState, true>           fGeoSrcStateStack;
    
};

GR_MAKE_BITFIELD_OPS(GrDrawTarget::BlendOptFlags);

#endif
