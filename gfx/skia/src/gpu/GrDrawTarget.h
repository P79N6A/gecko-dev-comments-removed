









#ifndef GrDrawTarget_DEFINED
#define GrDrawTarget_DEFINED

#include "GrClip.h"
#include "GrColor.h"
#include "GrDrawState.h"
#include "GrMatrix.h"
#include "GrRefCnt.h"
#include "GrRenderTarget.h"
#include "GrSamplerState.h"
#include "GrStencil.h"
#include "GrTexture.h"

#include "SkXfermode.h"

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
        bool fNPOTTextureSupport        : 1;
        bool fNPOTTextureTileSupport    : 1;
        bool fNPOTRenderTargetSupport   : 1;
        bool fTwoSidedStencilSupport    : 1;
        bool fStencilWrapOpsSupport     : 1;
        bool fHWAALineSupport           : 1;
        bool fShaderSupport             : 1;
        bool fShaderDerivativeSupport   : 1;
        bool fGeometryShaderSupport     : 1;
        bool fFSAASupport               : 1;
        bool fDualSourceBlendingSupport : 1;
        bool fBufferLockSupport         : 1;
        bool fSupportPerVertexCoverage  : 1;
        int fMinRenderTargetWidth;
        int fMinRenderTargetHeight;
        int fMaxRenderTargetSize;
        int fMaxTextureSize;
    };

    


    typedef int StageBitfield;
    GR_STATIC_ASSERT(sizeof(StageBitfield)*8 >= GrDrawState::kNumStages);

    



    enum StateBits {
        


        kDither_StateBit        = 0x01,
        




        kHWAntialias_StateBit   = 0x02,
        


        kClip_StateBit          = 0x04,
        



        kNoColorWrites_StateBit = 0x08,
        




        kEdgeAAConcave_StateBit =  0x10,
        
        kDummyStateBit,
        kLastPublicStateBit = kDummyStateBit-1
    };

    






    void setStencil(const GrStencilSettings& settings) {
        fCurrDrawState.fStencilSettings = settings;
    }

    


    void disableStencil() {
        fCurrDrawState.fStencilSettings.setDisabled();
    }

public:
    

    GrDrawTarget();
    virtual ~GrDrawTarget();

    


    const Caps& getCaps() const { return fCaps; }

    







    void setClip(const GrClip& clip);

    




    const GrClip& getClip() const;

    







    void setTexture(int stage, GrTexture* texture);

    






    const GrTexture* getTexture(int stage) const;
    GrTexture* getTexture(int stage);

    




    void setRenderTarget(GrRenderTarget* target);

    




    const GrRenderTarget* getRenderTarget() const;
    GrRenderTarget* getRenderTarget();

    








    void setSamplerState(int stage, const GrSamplerState& samplerState);

    





    void preConcatSamplerMatrix(int stage, const GrMatrix& matrix)  {
        GrAssert(stage >= 0 && stage < GrDrawState::kNumStages);
        fCurrDrawState.fSamplerStates[stage].preConcatMatrix(matrix);
    }

    



    void preConcatSamplerMatrices(int stageMask, const GrMatrix& matrix) {
        for (int i = 0; i < GrDrawState::kNumStages; ++i) {
            if ((1 << i) & stageMask) {
                this->preConcatSamplerMatrix(i, matrix);
            }
        }
    }

    






    void preConcatEnabledSamplerMatrices(const GrMatrix& matrix) {
        StageBitfield stageMask = this->enabledStages();
        this->preConcatSamplerMatrices(stageMask, matrix);
    }

    





    const GrMatrix& getSamplerMatrix(int stage) const {
        return fCurrDrawState.fSamplerStates[stage].getMatrix();
    }

    





    void setSamplerMatrix(int stage, const GrMatrix& matrix) {
        fCurrDrawState.fSamplerStates[stage].setMatrix(matrix);
    }

    








    void setViewMatrix(const GrMatrix& m);

    









    void preConcatViewMatrix(const GrMatrix& m);

    









    void postConcatViewMatrix(const GrMatrix& m);

    



    const GrMatrix& getViewMatrix() const;

    








    bool getViewInverse(GrMatrix* matrix) const;

    




    void setColor(GrColor);

    



    GrColor getColor() const { return fCurrDrawState.fColor; }

    


    void setColorFilter(GrColor, SkXfermode::Mode);

    





    void setAlpha(uint8_t alpha);

    



    void setDrawFace(GrDrawState::DrawFace face) {
        fCurrDrawState.fDrawFace = face;
    }

    








    void setFirstCoverageStage(int firstCoverageStage) { 
        fCurrDrawState.fFirstCoverageStage = firstCoverageStage; 
    }

    


    int getFirstCoverageStage() const { 
        return fCurrDrawState.fFirstCoverageStage; 
    }

    




    GrDrawState::DrawFace getDrawFace() const {
        return fCurrDrawState.fDrawFace;
    }

    




    void enableState(uint32_t stateBits);

    




    void disableState(uint32_t stateBits);

    bool isDitherState() const {
        return 0 != (fCurrDrawState.fFlagBits & kDither_StateBit);
    }

    bool isHWAntialiasState() const {
        return 0 != (fCurrDrawState.fFlagBits & kHWAntialias_StateBit);
    }

    bool isClipState() const {
        return 0 != (fCurrDrawState.fFlagBits & kClip_StateBit);
    }

    bool isColorWriteDisabled() const {
        return 0 != (fCurrDrawState.fFlagBits & kNoColorWrites_StateBit);
    }

    












    void setBlendFunc(GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff);

    









    void setBlendConstant(GrColor constant) { fCurrDrawState.fBlendConstant = constant; }

    



    GrColor getBlendConstant() const { return fCurrDrawState.fBlendConstant; }

    






    bool drawWillReadDst() const;

    










    bool canApplyCoverage() const;

    




    bool canTweakAlphaForCoverage() const;

     




    void setVertexEdgeType(GrDrawState::VertexEdgeType type) {
        fCurrDrawState.fVertexEdgeType = type;
    }

    




    bool willUseHWAALines() const;

    





    void setEdgeAAData(const GrDrawState::Edge* edges, int numEdges);

    


    struct SavedDrawState {
    private:
        GrDrawState fState;
        friend class GrDrawTarget;
    };

    







    void saveCurrentDrawState(SavedDrawState* state) const;

    








    void restoreDrawState(const SavedDrawState& state);

    




    void copyDrawState(const GrDrawTarget& srcTarget);

    






















    







    static int StageTexCoordVertexLayoutBit(int stage, int texCoordIdx) {
        GrAssert(stage < GrDrawState::kNumStages);
        GrAssert(texCoordIdx < GrDrawState::kMaxTexCoords);
        return 1 << (stage + (texCoordIdx * GrDrawState::kNumStages));
    }

private:
    static const int TEX_COORD_BIT_CNT = GrDrawState::kNumStages *
                                         GrDrawState::kMaxTexCoords;

public:
    








    static int StagePosAsTexCoordVertexLayoutBit(int stage) {
        GrAssert(stage < GrDrawState::kNumStages);
        return (1 << (TEX_COORD_BIT_CNT + stage));
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

    































    


























    bool reserveVertexSpace(GrVertexLayout vertexLayout,
                            int vertexCount,
                            void** vertices);
    























    bool reserveIndexSpace(int indexCount, void** indices);
    





















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
                          StageBitfield stageEnableBitfield,
                          const GrRect* srcRects[],
                          const GrMatrix* srcMatrices[]);

    



    void drawSimpleRect(const GrRect& rect,
                        const GrMatrix* matrix,
                        StageBitfield stageEnableBitfield) {
         drawRect(rect, matrix, stageEnableBitfield, NULL, NULL);
    }

    




    virtual void clear(const GrIRect* rect, GrColor color) = 0;

    






    virtual int getMaxEdges() const { return 6; }

    

    class AutoStateRestore : ::GrNoncopyable {
    public:
        AutoStateRestore();
        AutoStateRestore(GrDrawTarget* target);
        ~AutoStateRestore();

        




        void set(GrDrawTarget* target);

    private:
        GrDrawTarget*       fDrawTarget;
        SavedDrawState      fDrawState;
    };

    

    class AutoViewMatrixRestore : ::GrNoncopyable {
    public:
        AutoViewMatrixRestore() {
            fDrawTarget = NULL;
        }

        AutoViewMatrixRestore(GrDrawTarget* target)
            : fDrawTarget(target), fMatrix(fDrawTarget->getViewMatrix()) {
            GrAssert(NULL != target);
        }

        void set(GrDrawTarget* target) {
            GrAssert(NULL != target);
            if (NULL != fDrawTarget) {
                fDrawTarget->setViewMatrix(fMatrix);
            }
            fDrawTarget = target;
            fMatrix = target->getViewMatrix();
        }

        ~AutoViewMatrixRestore() {
            if (NULL != fDrawTarget) {
                fDrawTarget->setViewMatrix(fMatrix);
            }
        }

    private:
        GrDrawTarget*       fDrawTarget;
        GrMatrix            fMatrix;
    };

    

    



    class AutoDeviceCoordDraw : ::GrNoncopyable {
    public:
        AutoDeviceCoordDraw(GrDrawTarget* target, int stageMask);
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

    
    bool srcAlphaWillBeOne() const;

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
    
    
    static bool StageWillBeUsed(int stage, GrVertexLayout layout, 
                                const GrDrawState& state) {
        return NULL != state.fTextures[stage] && VertexUsesStage(stage, layout);
    }

    bool isStageEnabled(int stage) const {
        return StageWillBeUsed(stage, this->getGeomSrc().fVertexLayout, 
                               fCurrDrawState);
    }

    StageBitfield enabledStages() const {
        StageBitfield mask = 0;
        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            mask |= this->isStageEnabled(s) ? 1 : 0;
        }
        return mask;
    }

    
    
    static GrDrawState& accessSavedDrawState(SavedDrawState& sds)
                                                        { return sds.fState; }
    static const GrDrawState& accessSavedDrawState(const SavedDrawState& sds)
                                                        { return sds.fState; }

    
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
    
    
    virtual void clipWillBeSet(const GrClip& clip);

    
    
    static GrVertexLayout GetRectVertexLayout(StageBitfield stageEnableBitfield,
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

    GrClip fClip;

    GrDrawState fCurrDrawState;

    Caps fCaps;

private:
    
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
