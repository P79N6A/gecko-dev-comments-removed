






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrColor.h"
#include "GrEffectStage.h"
#include "GrPaint.h"
#include "GrRefCnt.h"
#include "GrRenderTarget.h"
#include "GrStencil.h"
#include "GrTemplates.h"
#include "GrTexture.h"
#include "GrTypesPriv.h"
#include "effects/GrSimpleTextureEffect.h"

#include "SkMatrix.h"
#include "SkXfermode.h"

class GrDrawState : public GrRefCnt {
public:
    SK_DECLARE_INST_COUNT(GrDrawState)

    
























    enum {
        kNumStages = GrPaint::kTotalStages + 2,
    };

    GrDrawState() {
        this->reset();
    }

    GrDrawState(const GrDrawState& state) {
        *this = state;
    }

    virtual ~GrDrawState() {
        this->disableStages();
    }

    



    void reset() {

        this->disableStages();

        fRenderTarget.reset(NULL);

        this->setDefaultVertexAttribs();

        fCommon.fColor = 0xffffffff;
        fCommon.fViewMatrix.reset();
        fCommon.fSrcBlend = kOne_GrBlendCoeff;
        fCommon.fDstBlend = kZero_GrBlendCoeff;
        fCommon.fBlendConstant = 0x0;
        fCommon.fFlagBits = 0x0;
        fCommon.fStencilSettings.setDisabled();
        fCommon.fFirstCoverageStage = kNumStages;
        fCommon.fCoverage = 0xffffffff;
        fCommon.fColorFilterMode = SkXfermode::kDst_Mode;
        fCommon.fColorFilterColor = 0x0;
        fCommon.fDrawFace = kBoth_DrawFace;
    }

    





    void setFromPaint(const GrPaint& paint);

    
    
    

    enum {
        kMaxVertexAttribCnt = kLast_GrVertexAttribBinding + 4,
    };

   








    





    void setVertexAttribs(const GrVertexAttrib attribs[], int count);

    const GrVertexAttrib* getVertexAttribs() const { return fCommon.fVertexAttribs.begin(); }
    int getVertexAttribCount() const { return fCommon.fVertexAttribs.count(); }

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

    



    void setColorFilter(GrColor c, SkXfermode::Mode mode) {
        fCommon.fColorFilterColor = c;
        fCommon.fColorFilterMode = mode;
    }

    GrColor getColorFilterColor() const { return fCommon.fColorFilterColor; }
    SkXfermode::Mode getColorFilterMode() const { return fCommon.fColorFilterMode; }

    


    class AutoColorRestore : public ::GrNoncopyable {
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

    



    void setCoverage4(GrColor coverage) {
        fCommon.fCoverage = coverage;
    }

    GrColor getCoverage() const {
        return fCommon.fCoverage;
    }

    

    
    
    

    const GrEffectRef* setEffect(int stageIdx, const GrEffectRef* effect) {
        fStages[stageIdx].setEffect(effect);
        return effect;
    }

    const GrEffectRef* setEffect(int stageIdx, const GrEffectRef* effect,
                                 int attr0, int attr1 = -1) {
        fStages[stageIdx].setEffect(effect, attr0, attr1);
        return effect;
    }

    


    void createTextureEffect(int stageIdx, GrTexture* texture, const SkMatrix& matrix) {
        GrAssert(!this->getStage(stageIdx).getEffect());
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix);
        this->setEffect(stageIdx, effect)->unref();
    }
    void createTextureEffect(int stageIdx,
                             GrTexture* texture,
                             const SkMatrix& matrix,
                             const GrTextureParams& params) {
        GrAssert(!this->getStage(stageIdx).getEffect());
        GrEffectRef* effect = GrSimpleTextureEffect::Create(texture, matrix, params);
        this->setEffect(stageIdx, effect)->unref();
    }

    bool stagesDisabled() {
        for (int i = 0; i < kNumStages; ++i) {
            if (NULL != fStages[i].getEffect()) {
                return false;
            }
        }
        return true;
    }

    void disableStage(int stageIdx) {
        this->setEffect(stageIdx, NULL);
    }

    


    void disableStages() {
        for (int i = 0; i < kNumStages; ++i) {
            this->disableStage(i);
        }
    }

    class AutoStageDisable : public ::GrNoncopyable {
    public:
        AutoStageDisable(GrDrawState* ds) : fDrawState(ds) {}
        ~AutoStageDisable() {
            if (NULL != fDrawState) {
                fDrawState->disableStages();
            }
        }
    private:
        GrDrawState* fDrawState;
    };

    


    const GrEffectStage& getStage(int stageIdx) const {
        GrAssert((unsigned)stageIdx < kNumStages);
        return fStages[stageIdx];
    }

    




    void localCoordChange(const SkMatrix& oldToNew) {
        for (int i = 0; i < kNumStages; ++i) {
            if (this->isStageEnabled(i)) {
                fStages[i].localCoordChange(oldToNew);
            }
        }
    }

    


    bool willEffectReadDst() const {
        for (int s = 0; s < kNumStages; ++s) {
            if (this->isStageEnabled(s) && (*this->getStage(s).getEffect())->willReadDst()) {
                return true;
            }
        }
        return false;
    }

    

    
    
    

    








    void setFirstCoverageStage(int firstCoverageStage) {
        GrAssert((unsigned)firstCoverageStage <= kNumStages);
        fCommon.fFirstCoverageStage = firstCoverageStage;
    }

    


    int getFirstCoverageStage() const {
        return fCommon.fFirstCoverageStage;
    }

    

    
    
    

    












    void setBlendFunc(GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff) {
        fCommon.fSrcBlend = srcCoeff;
        fCommon.fDstBlend = dstCoeff;
    #if GR_DEBUG
        switch (dstCoeff) {
        case kDC_GrBlendCoeff:
        case kIDC_GrBlendCoeff:
        case kDA_GrBlendCoeff:
        case kIDA_GrBlendCoeff:
            GrPrintf("Unexpected dst blend coeff. Won't work correctly with"
                     "coverage stages.\n");
            break;
        default:
            break;
        }
        switch (srcCoeff) {
        case kSC_GrBlendCoeff:
        case kISC_GrBlendCoeff:
        case kSA_GrBlendCoeff:
        case kISA_GrBlendCoeff:
            GrPrintf("Unexpected src blend coeff. Won't work correctly with"
                     "coverage stages.\n");
            break;
        default:
            break;
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

    

    
    
    

    






    void setViewMatrix(const SkMatrix& m) { fCommon.fViewMatrix = m; }

    


    SkMatrix* viewMatrix() { return &fCommon.fViewMatrix; }

    









    void preConcatViewMatrix(const SkMatrix& m) { fCommon.fViewMatrix.preConcat(m); }

    









    void postConcatViewMatrix(const SkMatrix& m) { fCommon.fViewMatrix.postConcat(m); }

    



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

    

    



    class AutoViewMatrixRestore : public ::GrNoncopyable {
    public:
        AutoViewMatrixRestore() : fDrawState(NULL) {}

        AutoViewMatrixRestore(GrDrawState* ds, const SkMatrix& preconcatMatrix) {
            fDrawState = NULL;
            this->set(ds, preconcatMatrix);
        }

        ~AutoViewMatrixRestore() { this->restore(); }

        


        void restore();

        void set(GrDrawState* drawState, const SkMatrix& preconcatMatrix);

        bool isSet() const { return NULL != fDrawState; }

    private:
        GrDrawState*                        fDrawState;
        SkMatrix                            fViewMatrix;
        GrEffectStage::SavedCoordChange     fSavedCoordChanges[GrDrawState::kNumStages];
        uint32_t                            fRestoreMask;
    };

    

    





    class AutoDeviceCoordDraw : ::GrNoncopyable {
    public:
        AutoDeviceCoordDraw() : fDrawState(NULL) {}
        




        AutoDeviceCoordDraw(GrDrawState* drawState) {
            fDrawState = NULL;
            this->set(drawState);
        }

        ~AutoDeviceCoordDraw() { this->restore(); }

        bool set(GrDrawState* drawState);

        




        bool succeeded() const { return NULL != fDrawState; }

        



        const SkMatrix& getOriginalMatrix() const {
            GrAssert(this->succeeded());
            return fViewMatrix;
        }

        


        void restore();

    private:
        GrDrawState*                        fDrawState;
        SkMatrix                            fViewMatrix;
        GrEffectStage::SavedCoordChange     fSavedCoordChanges[GrDrawState::kNumStages];
        uint32_t                            fRestoreMask;
    };

    

    
    
    

    




    void setRenderTarget(GrRenderTarget* target) {
        fRenderTarget.reset(SkSafeRef(target));
    }

    




    const GrRenderTarget* getRenderTarget() const { return fRenderTarget.get(); }
    GrRenderTarget* getRenderTarget() { return fRenderTarget.get(); }

    class AutoRenderTargetRestore : public ::GrNoncopyable {
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
            GrSafeSetNull(fSavedTarget);
        }

        void set(GrDrawState* ds, GrRenderTarget* newTarget) {
            this->restore();

            if (NULL != ds) {
                GrAssert(NULL == fSavedTarget);
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
        GrAssert(kInvalid_DrawFace != face);
        fCommon.fDrawFace = face;
    }

    




    DrawFace getDrawFace() const { return fCommon.fDrawFace; }

    

    

    bool isStageEnabled(int s) const {
        GrAssert((unsigned)s < kNumStages);
        return (NULL != fStages[s].getEffect());
    }

    bool operator ==(const GrDrawState& s) const {
        if (fRenderTarget.get() != s.fRenderTarget.get() || fCommon != s.fCommon) {
            return false;
        }
        for (int i = 0; i < kNumStages; i++) {
            bool enabled = this->isStageEnabled(i);
            if (enabled != s.isStageEnabled(i)) {
                return false;
            }
            if (enabled && this->fStages[i] != s.fStages[i]) {
                return false;
            }
        }
        return true;
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }

    GrDrawState& operator= (const GrDrawState& s) {
        this->setRenderTarget(s.fRenderTarget.get());
        fCommon = s.fCommon;
        for (int i = 0; i < kNumStages; i++) {
            if (s.isStageEnabled(i)) {
                this->fStages[i] = s.fStages[i];
            }
        }
        return *this;
    }

private:

    
    struct CommonState {
        
        GrColor                                  fColor;
        SkMatrix                                 fViewMatrix;
        GrBlendCoeff                             fSrcBlend;
        GrBlendCoeff                             fDstBlend;
        GrColor                                  fBlendConstant;
        uint32_t                                 fFlagBits;
        GrVertexAttribArray<kMaxVertexAttribCnt> fVertexAttribs;
        GrStencilSettings                        fStencilSettings;
        int                                      fFirstCoverageStage;
        GrColor                                  fCoverage;
        SkXfermode::Mode                         fColorFilterMode;
        GrColor                                  fColorFilterColor;
        DrawFace                                 fDrawFace;

        
        
        int fFixedFunctionVertexAttribIndices[kGrFixedFunctionVertexAttribBindingCnt];

        bool operator== (const CommonState& other) const {
            bool result = fColor == other.fColor &&
                          fViewMatrix.cheapEqualTo(other.fViewMatrix) &&
                          fSrcBlend == other.fSrcBlend &&
                          fDstBlend == other.fDstBlend &&
                          fBlendConstant == other.fBlendConstant &&
                          fFlagBits == other.fFlagBits &&
                          fVertexAttribs == other.fVertexAttribs &&
                          fStencilSettings == other.fStencilSettings &&
                          fFirstCoverageStage == other.fFirstCoverageStage &&
                          fCoverage == other.fCoverage &&
                          fColorFilterMode == other.fColorFilterMode &&
                          fColorFilterColor == other.fColorFilterColor &&
                          fDrawFace == other.fDrawFace;
            GrAssert(!result || 0 == memcmp(fFixedFunctionVertexAttribIndices,
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
            GR_DEBUGCODE(fInitialized = false;)
        }
        
        ~DeferredState() { SkSafeUnref(fRenderTarget); }

        void saveFrom(const GrDrawState& drawState) {
            fCommon = drawState.fCommon;
            
            fRenderTarget = drawState.fRenderTarget.get();
            SkSafeRef(fRenderTarget);
            
            
            
            for (int i = 0; i < kNumStages; ++i) {
                fStages[i].saveFrom(drawState.fStages[i]);
            }
            GR_DEBUGCODE(fInitialized = true;)
        }

        void restoreTo(GrDrawState* drawState) {
            GrAssert(fInitialized);
            drawState->fCommon = fCommon;
            drawState->setRenderTarget(fRenderTarget);
            for (int i = 0; i < kNumStages; ++i) {
                fStages[i].restoreTo(&drawState->fStages[i]);
            }
        }

        bool isEqual(const GrDrawState& state) const {
            if (fRenderTarget != state.fRenderTarget.get() || fCommon != state.fCommon) {
                return false;
            }
            for (int i = 0; i < kNumStages; ++i) {
                if (!fStages[i].isEqual(state.fStages[i])) {
                    return false;
                }
            }
            return true;
        }

    private:
        GrRenderTarget*                       fRenderTarget;
        CommonState                           fCommon;
        GrEffectStage::DeferredStage          fStages[kNumStages];

        GR_DEBUGCODE(bool fInitialized;)
    };

private:

    SkAutoTUnref<GrRenderTarget>           fRenderTarget;
    CommonState                            fCommon;
    GrEffectStage                          fStages[kNumStages];

    typedef GrRefCnt INHERITED;
};

GR_MAKE_BITFIELD_OPS(GrDrawState::BlendOptFlags);

#endif
