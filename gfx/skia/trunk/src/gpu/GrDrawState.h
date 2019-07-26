






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrBlend.h"
#include "GrColor.h"
#include "GrEffectStage.h"
#include "GrPaint.h"
#include "GrPoint.h"
#include "GrRenderTarget.h"
#include "GrStencil.h"
#include "GrTemplates.h"
#include "GrTexture.h"
#include "GrTypesPriv.h"
#include "effects/GrSimpleTextureEffect.h"

#include "SkMatrix.h"
#include "SkTypes.h"
#include "SkXfermode.h"

class GrDrawState : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrDrawState)

    GrDrawState() {
        SkDEBUGCODE(fBlockEffectRemovalCnt = 0;)
        this->reset();
    }

    GrDrawState(const SkMatrix& initialViewMatrix) {
        SkDEBUGCODE(fBlockEffectRemovalCnt = 0;)
        this->reset(initialViewMatrix);
    }

    


    GrDrawState(const GrDrawState& state) : INHERITED() {
        SkDEBUGCODE(fBlockEffectRemovalCnt = 0;)
        *this = state;
    }

    


    GrDrawState(const GrDrawState& state, const SkMatrix& preConcatMatrix) {
        SkDEBUGCODE(fBlockEffectRemovalCnt = 0;)
        *this = state;
        if (!preConcatMatrix.isIdentity()) {
            for (int i = 0; i < fColorStages.count(); ++i) {
                fColorStages[i].localCoordChange(preConcatMatrix);
            }
            for (int i = 0; i < fCoverageStages.count(); ++i) {
                fCoverageStages[i].localCoordChange(preConcatMatrix);
            }
        }
    }

    virtual ~GrDrawState() { SkASSERT(0 == fBlockEffectRemovalCnt); }

    


    void reset() { this->onReset(NULL); }

    void reset(const SkMatrix& initialViewMatrix) { this->onReset(&initialViewMatrix); }

    




    void setFromPaint(const GrPaint& , const SkMatrix& viewMatrix, GrRenderTarget*);

    
    
    

    enum {
        kMaxVertexAttribCnt = kLast_GrVertexAttribBinding + 4,
    };

   








    



    template <const GrVertexAttrib A[]> void setVertexAttribs(int count) {
        this->setVertexAttribs(A, count);
    }

    const GrVertexAttrib* getVertexAttribs() const { return fCommon.fVAPtr; }
    int getVertexAttribCount() const { return fCommon.fVACount; }

    size_t getVertexSize() const;

    



    void setDefaultVertexAttribs();

    





    int positionAttributeIndex() const {
        return fCommon.fFixedFunctionVertexAttribIndices[kPosition_GrVertexAttribBinding];
    }
    int localCoordAttributeIndex() const {
        return fCommon.fFixedFunctionVertexAttribIndices[kLocalCoord_GrVertexAttribBinding];
    }
    int colorVertexAttributeIndex() const {
        return fCommon.fFixedFunctionVertexAttribIndices[kColor_GrVertexAttribBinding];
    }
    int coverageVertexAttributeIndex() const {
        return fCommon.fFixedFunctionVertexAttribIndices[kCoverage_GrVertexAttribBinding];
    }

    bool hasLocalCoordAttribute() const {
        return -1 != fCommon.fFixedFunctionVertexAttribIndices[kLocalCoord_GrVertexAttribBinding];
    }
    bool hasColorVertexAttribute() const {
        return -1 != fCommon.fFixedFunctionVertexAttribIndices[kColor_GrVertexAttribBinding];
    }
    bool hasCoverageVertexAttribute() const {
        return -1 != fCommon.fFixedFunctionVertexAttribIndices[kCoverage_GrVertexAttribBinding];
    }

    bool validateVertexAttribs() const;

    


     class AutoVertexAttribRestore {
     public:
         AutoVertexAttribRestore(GrDrawState* drawState) {
             SkASSERT(NULL != drawState);
             fDrawState = drawState;
             fVAPtr = drawState->fCommon.fVAPtr;
             fVACount = drawState->fCommon.fVACount;
             fDrawState->setDefaultVertexAttribs();
         }

         ~AutoVertexAttribRestore(){
             fDrawState->setVertexAttribs(fVAPtr, fVACount);
         }

     private:
         GrDrawState*          fDrawState;
         const GrVertexAttrib* fVAPtr;
         int                   fVACount;
     };

    





    









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

    

    


    bool srcAlphaWillBeOne() const;

    


    bool hasSolidCoverage() const;

    

    
    
    

    




    void setColor(GrColor color) { fCommon.fColor = color; }

    GrColor getColor() const { return fCommon.fColor; }

    





    void setAlpha(uint8_t a) {
        this->setColor((a << 24) | (a << 16) | (a << 8) | a);
    }

    


    class AutoColorRestore : public ::SkNoncopyable {
    public:
        AutoColorRestore() : fDrawState(NULL), fOldColor(0) {}

        AutoColorRestore(GrDrawState* drawState, GrColor color) {
            fDrawState = NULL;
            this->set(drawState, color);
        }

        void reset() {
            if (NULL != fDrawState) {
                fDrawState->setColor(fOldColor);
                fDrawState = NULL;
            }
        }

        void set(GrDrawState* drawState, GrColor color) {
            this->reset();
            fDrawState = drawState;
            fOldColor = fDrawState->getColor();
            fDrawState->setColor(color);
        }

        ~AutoColorRestore() { this->reset(); }
    private:
        GrDrawState*    fDrawState;
        GrColor         fOldColor;
    };

    

    
    
    

    




    void setCoverage(uint8_t coverage) {
        fCommon.fCoverage = GrColorPackRGBA(coverage, coverage, coverage, coverage);
    }

    uint8_t getCoverage() const {
        return GrColorUnpackR(fCommon.fCoverage);
    }

    GrColor getCoverageColor() const {
        return fCommon.fCoverage;
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    const GrEffectRef* addColorEffect(const GrEffectRef* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        SkNEW_APPEND_TO_TARRAY(&fColorStages, GrEffectStage, (effect, attr0, attr1));
        return effect;
    }

    const GrEffectRef* addCoverageEffect(const GrEffectRef* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        SkNEW_APPEND_TO_TARRAY(&fCoverageStages, GrEffectStage, (effect, attr0, attr1));
        return effect;
    }

    


    void addColorTextureEffect(GrTexture* texture, const SkMatrix& matrix) {
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix);
        this->addColorEffect(effect)->unref();
    }

    void addCoverageTextureEffect(GrTexture* texture, const SkMatrix& matrix) {
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix);
        this->addCoverageEffect(effect)->unref();
    }

    void addColorTextureEffect(GrTexture* texture,
                               const SkMatrix& matrix,
                               const GrTextureParams& params) {
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix, params);
        this->addColorEffect(effect)->unref();
    }

    void addCoverageTextureEffect(GrTexture* texture,
                                  const SkMatrix& matrix,
                                  const GrTextureParams& params) {
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix, params);
        this->addCoverageEffect(effect)->unref();
    }

    



    class AutoRestoreEffects : public ::SkNoncopyable {
    public:
        AutoRestoreEffects() : fDrawState(NULL), fColorEffectCnt(0), fCoverageEffectCnt(0) {}

        AutoRestoreEffects(GrDrawState* ds) : fDrawState(NULL), fColorEffectCnt(0), fCoverageEffectCnt(0) {
            this->set(ds);
        }

        ~AutoRestoreEffects() { this->set(NULL); }

        void set(GrDrawState* ds) {
            if (NULL != fDrawState) {
                int n = fDrawState->fColorStages.count() - fColorEffectCnt;
                SkASSERT(n >= 0);
                fDrawState->fColorStages.pop_back_n(n);
                n = fDrawState->fCoverageStages.count() - fCoverageEffectCnt;
                SkASSERT(n >= 0);
                fDrawState->fCoverageStages.pop_back_n(n);
                SkDEBUGCODE(--fDrawState->fBlockEffectRemovalCnt;)
            }
            fDrawState = ds;
            if (NULL != ds) {
                fColorEffectCnt = ds->fColorStages.count();
                fCoverageEffectCnt = ds->fCoverageStages.count();
                SkDEBUGCODE(++ds->fBlockEffectRemovalCnt;)
            }
        }

    private:
        GrDrawState* fDrawState;
        int fColorEffectCnt;
        int fCoverageEffectCnt;
    };

    int numColorStages() const { return fColorStages.count(); }
    int numCoverageStages() const { return fCoverageStages.count(); }
    int numTotalStages() const { return this->numColorStages() + this->numCoverageStages(); }

    const GrEffectStage& getColorStage(int stageIdx) const { return fColorStages[stageIdx]; }
    const GrEffectStage& getCoverageStage(int stageIdx) const { return fCoverageStages[stageIdx]; }

    


    bool willEffectReadDstColor() const;

    

    
    
    

    












    void setBlendFunc(GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff) {
        fCommon.fSrcBlend = srcCoeff;
        fCommon.fDstBlend = dstCoeff;
    #ifdef SK_DEBUG
        if (GrBlendCoeffRefsDst(dstCoeff)) {
            GrPrintf("Unexpected dst blend coeff. Won't work correctly with coverage stages.\n");
        }
        if (GrBlendCoeffRefsSrc(srcCoeff)) {
            GrPrintf("Unexpected src blend coeff. Won't work correctly with coverage stages.\n");
        }
    #endif
    }

    GrBlendCoeff getSrcBlendCoeff() const { return fCommon.fSrcBlend; }
    GrBlendCoeff getDstBlendCoeff() const { return fCommon.fDstBlend; }

    void getDstBlendCoeff(GrBlendCoeff* srcBlendCoeff,
                          GrBlendCoeff* dstBlendCoeff) const {
        *srcBlendCoeff = fCommon.fSrcBlend;
        *dstBlendCoeff = fCommon.fDstBlend;
    }

    









    void setBlendConstant(GrColor constant) { fCommon.fBlendConstant = constant; }

    



    GrColor getBlendConstant() const { return fCommon.fBlendConstant; }

    




    bool canTweakAlphaForCoverage() const;

    


    enum BlendOptFlags {
        


        kNone_BlendOpt                  = 0,
        


        kSkipDraw_BlendOptFlag          = 0x1,
        


        kDisableBlend_BlendOptFlag      = 0x2,
        



        kCoverageAsAlpha_BlendOptFlag   = 0x4,
        



        kEmitCoverage_BlendOptFlag      = 0x8,
        


        kEmitTransBlack_BlendOptFlag    = 0x10,
    };
    GR_DECL_BITFIELD_OPS_FRIENDS(BlendOptFlags);

    









    BlendOptFlags getBlendOpts(bool forceCoverage = false,
                               GrBlendCoeff* srcCoeff = NULL,
                               GrBlendCoeff* dstCoeff = NULL) const;

    

    
    
    

    



    bool setIdentityViewMatrix();

    



    const SkMatrix& getViewMatrix() const { return fCommon.fViewMatrix; }

    








    bool getViewInverse(SkMatrix* matrix) const {
        
        
        SkMatrix inverse;
        if (fCommon.fViewMatrix.invert(&inverse)) {
            if (matrix) {
                *matrix = inverse;
            }
            return true;
        }
        return false;
    }

    

    



    class AutoViewMatrixRestore : public ::SkNoncopyable {
    public:
        AutoViewMatrixRestore() : fDrawState(NULL) {}

        AutoViewMatrixRestore(GrDrawState* ds, const SkMatrix& preconcatMatrix) {
            fDrawState = NULL;
            this->set(ds, preconcatMatrix);
        }

        ~AutoViewMatrixRestore() { this->restore(); }

        


        void restore();

        void set(GrDrawState* drawState, const SkMatrix& preconcatMatrix);

        

        bool setIdentity(GrDrawState* drawState);

    private:
        void doEffectCoordChanges(const SkMatrix& coordChangeMatrix);

        GrDrawState*                                        fDrawState;
        SkMatrix                                            fViewMatrix;
        int                                                 fNumColorStages;
        SkAutoSTArray<8, GrEffectStage::SavedCoordChange>   fSavedCoordChanges;
    };

    

    
    
    

    




    void setRenderTarget(GrRenderTarget* target) {
        fRenderTarget.reset(SkSafeRef(target));
    }

    




    const GrRenderTarget* getRenderTarget() const { return fRenderTarget.get(); }
    GrRenderTarget* getRenderTarget() { return fRenderTarget.get(); }

    class AutoRenderTargetRestore : public ::SkNoncopyable {
    public:
        AutoRenderTargetRestore() : fDrawState(NULL), fSavedTarget(NULL) {}
        AutoRenderTargetRestore(GrDrawState* ds, GrRenderTarget* newTarget) {
            fDrawState = NULL;
            fSavedTarget = NULL;
            this->set(ds, newTarget);
        }
        ~AutoRenderTargetRestore() { this->restore(); }

        void restore() {
            if (NULL != fDrawState) {
                fDrawState->setRenderTarget(fSavedTarget);
                fDrawState = NULL;
            }
            SkSafeSetNull(fSavedTarget);
        }

        void set(GrDrawState* ds, GrRenderTarget* newTarget) {
            this->restore();

            if (NULL != ds) {
                SkASSERT(NULL == fSavedTarget);
                fSavedTarget = ds->getRenderTarget();
                SkSafeRef(fSavedTarget);
                ds->setRenderTarget(newTarget);
                fDrawState = ds;
            }
        }
    private:
        GrDrawState* fDrawState;
        GrRenderTarget* fSavedTarget;
    };

    

    
    
    

    






    void setStencil(const GrStencilSettings& settings) {
        fCommon.fStencilSettings = settings;
    }

    


    void disableStencil() {
        fCommon.fStencilSettings.setDisabled();
    }

    const GrStencilSettings& getStencil() const { return fCommon.fStencilSettings; }

    GrStencilSettings* stencil() { return &fCommon.fStencilSettings; }

    

    
    
    

    



    enum StateBits {
        


        kDither_StateBit        = 0x01,
        




        kHWAntialias_StateBit   = 0x02,
        


        kClip_StateBit          = 0x04,
        



        kNoColorWrites_StateBit = 0x08,

        







         kCoverageDrawing_StateBit = 0x10,

        
        kDummyStateBit,
        kLastPublicStateBit = kDummyStateBit-1,
    };

    void resetStateFlags() {
        fCommon.fFlagBits = 0;
    }

    




    void enableState(uint32_t stateBits) {
        fCommon.fFlagBits |= stateBits;
    }

    




    void disableState(uint32_t stateBits) {
        fCommon.fFlagBits &= ~(stateBits);
    }

    





    void setState(uint32_t stateBits, bool enable) {
        if (enable) {
            this->enableState(stateBits);
        } else {
            this->disableState(stateBits);
        }
    }

    bool isDitherState() const {
        return 0 != (fCommon.fFlagBits & kDither_StateBit);
    }

    bool isHWAntialiasState() const {
        return 0 != (fCommon.fFlagBits & kHWAntialias_StateBit);
    }

    bool isClipState() const {
        return 0 != (fCommon.fFlagBits & kClip_StateBit);
    }

    bool isColorWriteDisabled() const {
        return 0 != (fCommon.fFlagBits & kNoColorWrites_StateBit);
    }

    bool isCoverageDrawing() const {
        return 0 != (fCommon.fFlagBits & kCoverageDrawing_StateBit);
    }

    bool isStateFlagEnabled(uint32_t stateBit) const {
        return 0 != (stateBit & fCommon.fFlagBits);
    }

    

    
    
    

    enum DrawFace {
        kInvalid_DrawFace = -1,

        kBoth_DrawFace,
        kCCW_DrawFace,
        kCW_DrawFace,
    };

    



    void setDrawFace(DrawFace face) {
        SkASSERT(kInvalid_DrawFace != face);
        fCommon.fDrawFace = face;
    }

    




    DrawFace getDrawFace() const { return fCommon.fDrawFace; }

    

    

    bool operator ==(const GrDrawState& s) const {
        if (fRenderTarget.get() != s.fRenderTarget.get() ||
            fColorStages.count() != s.fColorStages.count() ||
            fCoverageStages.count() != s.fCoverageStages.count() ||
            fCommon != s.fCommon) {
            return false;
        }
        for (int i = 0; i < fColorStages.count(); i++) {
            if (fColorStages[i] != s.fColorStages[i]) {
                return false;
            }
        }
        for (int i = 0; i < fCoverageStages.count(); i++) {
            if (fCoverageStages[i] != s.fCoverageStages[i]) {
                return false;
            }
        }
        return true;
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }

    GrDrawState& operator= (const GrDrawState& s) {
        SkASSERT(0 == fBlockEffectRemovalCnt || 0 == this->numTotalStages());
        this->setRenderTarget(s.fRenderTarget.get());
        fCommon = s.fCommon;
        fColorStages = s.fColorStages;
        fCoverageStages = s.fCoverageStages;
        return *this;
    }

private:

    void onReset(const SkMatrix* initialViewMatrix) {
        SkASSERT(0 == fBlockEffectRemovalCnt || 0 == this->numTotalStages());
        fColorStages.reset();
        fCoverageStages.reset();

        fRenderTarget.reset(NULL);

        this->setDefaultVertexAttribs();

        fCommon.fColor = 0xffffffff;
        if (NULL == initialViewMatrix) {
            fCommon.fViewMatrix.reset();
        } else {
            fCommon.fViewMatrix = *initialViewMatrix;
        }
        fCommon.fSrcBlend = kOne_GrBlendCoeff;
        fCommon.fDstBlend = kZero_GrBlendCoeff;
        fCommon.fBlendConstant = 0x0;
        fCommon.fFlagBits = 0x0;
        fCommon.fStencilSettings.setDisabled();
        fCommon.fCoverage = 0xffffffff;
        fCommon.fDrawFace = kBoth_DrawFace;
    }

    
    struct CommonState {
        
        GrColor               fColor;
        SkMatrix              fViewMatrix;
        GrBlendCoeff          fSrcBlend;
        GrBlendCoeff          fDstBlend;
        GrColor               fBlendConstant;
        uint32_t              fFlagBits;
        const GrVertexAttrib* fVAPtr;
        int                   fVACount;
        GrStencilSettings     fStencilSettings;
        GrColor               fCoverage;
        DrawFace              fDrawFace;

        
        
        int fFixedFunctionVertexAttribIndices[kGrFixedFunctionVertexAttribBindingCnt];

        bool operator== (const CommonState& other) const {
            bool result = fColor == other.fColor &&
                          fViewMatrix.cheapEqualTo(other.fViewMatrix) &&
                          fSrcBlend == other.fSrcBlend &&
                          fDstBlend == other.fDstBlend &&
                          fBlendConstant == other.fBlendConstant &&
                          fFlagBits == other.fFlagBits &&
                          fVACount == other.fVACount &&
                          !memcmp(fVAPtr, other.fVAPtr, fVACount * sizeof(GrVertexAttrib)) &&
                          fStencilSettings == other.fStencilSettings &&
                          fCoverage == other.fCoverage &&
                          fDrawFace == other.fDrawFace;
            SkASSERT(!result || 0 == memcmp(fFixedFunctionVertexAttribIndices,
                                            other.fFixedFunctionVertexAttribIndices,
                                            sizeof(fFixedFunctionVertexAttribIndices)));
            return result;
        }
        bool operator!= (const CommonState& other) const { return !(*this == other); }
    };

    

    struct SavedEffectStage {
        SavedEffectStage() : fEffect(NULL) {}
        const GrEffect*                    fEffect;
        GrEffectStage::SavedCoordChange    fCoordChange;
    };

public:
    





    class DeferredState {
    public:
        DeferredState() : fRenderTarget(NULL) {
            SkDEBUGCODE(fInitialized = false;)
        }
        
        ~DeferredState() { SkSafeUnref(fRenderTarget); }

        void saveFrom(const GrDrawState& drawState) {
            fCommon = drawState.fCommon;
            
            fRenderTarget = drawState.fRenderTarget.get();
            SkSafeRef(fRenderTarget);
            
            
            
            fStages.reset(drawState.fColorStages.count() + drawState.fCoverageStages.count());
            fColorStageCnt = drawState.fColorStages.count();
            for (int i = 0; i < fColorStageCnt; ++i) {
                fStages[i].saveFrom(drawState.fColorStages[i]);
            }
            for (int i = 0; i < drawState.fCoverageStages.count(); ++i) {
                fStages[i + fColorStageCnt].saveFrom(drawState.fCoverageStages[i]);
            }
            SkDEBUGCODE(fInitialized = true;)
        }

        void restoreTo(GrDrawState* drawState) {
            SkASSERT(fInitialized);
            drawState->fCommon = fCommon;
            drawState->setRenderTarget(fRenderTarget);
            
            drawState->fColorStages.reset();
            for (int i = 0; i < fColorStageCnt; ++i) {
                SkNEW_APPEND_TO_TARRAY(&drawState->fColorStages, GrEffectStage, (fStages[i]));
            }
            int coverageStageCnt = fStages.count() - fColorStageCnt;
            drawState->fCoverageStages.reset();
            for (int i = 0; i < coverageStageCnt; ++i) {
                SkNEW_APPEND_TO_TARRAY(&drawState->fCoverageStages,
                                        GrEffectStage, (fStages[i + fColorStageCnt]));
            }
        }

        bool isEqual(const GrDrawState& state) const {
            int numCoverageStages = fStages.count() - fColorStageCnt;
            if (fRenderTarget != state.fRenderTarget.get() ||
                fColorStageCnt != state.fColorStages.count() ||
                numCoverageStages != state.fCoverageStages.count() ||
                fCommon != state.fCommon) {
                return false;
            }
            bool explicitLocalCoords = state.hasLocalCoordAttribute();
            for (int i = 0; i < fColorStageCnt; ++i) {
                if (!fStages[i].isEqual(state.fColorStages[i], explicitLocalCoords)) {
                    return false;
                }
            }
            for (int i = 0; i < numCoverageStages; ++i) {
                int s = fColorStageCnt + i;
                if (!fStages[s].isEqual(state.fCoverageStages[i], explicitLocalCoords)) {
                    return false;
                }
            }
            return true;
        }

    private:
        typedef SkAutoSTArray<8, GrEffectStage::DeferredStage> DeferredStageArray;

        GrRenderTarget*                       fRenderTarget;
        CommonState                           fCommon;
        int                                   fColorStageCnt;
        DeferredStageArray                    fStages;

        SkDEBUGCODE(bool fInitialized;)
    };

private:

    SkAutoTUnref<GrRenderTarget>        fRenderTarget;
    CommonState                         fCommon;

    typedef SkSTArray<4, GrEffectStage> EffectStageArray;
    EffectStageArray                    fColorStages;
    EffectStageArray                    fCoverageStages;

    
    
    SkDEBUGCODE(int fBlockEffectRemovalCnt;)

    





    void setVertexAttribs(const GrVertexAttrib attribs[], int count);

    typedef SkRefCnt INHERITED;
};

GR_MAKE_BITFIELD_OPS(GrDrawState::BlendOptFlags);

#endif
