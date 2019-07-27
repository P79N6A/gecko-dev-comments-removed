






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrBlend.h"
#include "GrColor.h"
#include "GrEffectStage.h"
#include "GrPaint.h"
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
            this->invalidateBlendOptFlags();
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

    const GrVertexAttrib* getVertexAttribs() const { return fVAPtr; }
    int getVertexAttribCount() const { return fVACount; }

    size_t getVertexSize() const;

    



    void setDefaultVertexAttribs();

    





    int positionAttributeIndex() const {
        return fFixedFunctionVertexAttribIndices[kPosition_GrVertexAttribBinding];
    }
    int localCoordAttributeIndex() const {
        return fFixedFunctionVertexAttribIndices[kLocalCoord_GrVertexAttribBinding];
    }
    int colorVertexAttributeIndex() const {
        return fFixedFunctionVertexAttribIndices[kColor_GrVertexAttribBinding];
    }
    int coverageVertexAttributeIndex() const {
        return fFixedFunctionVertexAttribIndices[kCoverage_GrVertexAttribBinding];
    }

    bool hasLocalCoordAttribute() const {
        return -1 != fFixedFunctionVertexAttribIndices[kLocalCoord_GrVertexAttribBinding];
    }
    bool hasColorVertexAttribute() const {
        return -1 != fFixedFunctionVertexAttribIndices[kColor_GrVertexAttribBinding];
    }
    bool hasCoverageVertexAttribute() const {
        return -1 != fFixedFunctionVertexAttribIndices[kCoverage_GrVertexAttribBinding];
    }

    bool validateVertexAttribs() const;

    


     class AutoVertexAttribRestore {
     public:
         AutoVertexAttribRestore(GrDrawState* drawState) {
             SkASSERT(NULL != drawState);
             fDrawState = drawState;
             fVAPtr = drawState->fVAPtr;
             fVACount = drawState->fVACount;
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

    





    









    static SkPoint* GetVertexPoint(void* vertices,
                                   int vertexIndex,
                                   int vertexSize,
                                   int offset = 0) {
        intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<SkPoint*>(start + offset +
                                 vertexIndex * vertexSize);
    }
    static const SkPoint* GetVertexPoint(const void* vertices,
                                         int vertexIndex,
                                         int vertexSize,
                                         int offset = 0) {
        intptr_t start = GrTCast<intptr_t>(vertices);
        return GrTCast<const SkPoint*>(start + offset +
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

    

    
    
    

    




    void setColor(GrColor color) {
        fColor = color;
        this->invalidateBlendOptFlags();
    }

    GrColor getColor() const { return fColor; }

    





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
        fCoverage = GrColorPackRGBA(coverage, coverage, coverage, coverage);
        this->invalidateBlendOptFlags();
    }

    uint8_t getCoverage() const {
        return GrColorUnpackR(fCoverage);
    }

    GrColor getCoverageColor() const {
        return fCoverage;
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    const GrEffect* addColorEffect(const GrEffect* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        SkNEW_APPEND_TO_TARRAY(&fColorStages, GrEffectStage, (effect, attr0, attr1));
        this->invalidateBlendOptFlags();
        return effect;
    }

    const GrEffect* addCoverageEffect(const GrEffect* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        SkNEW_APPEND_TO_TARRAY(&fCoverageStages, GrEffectStage, (effect, attr0, attr1));
        this->invalidateBlendOptFlags();
        return effect;
    }

    


    void addColorTextureEffect(GrTexture* texture, const SkMatrix& matrix) {
        this->addColorEffect(GrSimpleTextureEffect::Create(texture, matrix))->unref();
    }

    void addCoverageTextureEffect(GrTexture* texture, const SkMatrix& matrix) {
        this->addCoverageEffect(GrSimpleTextureEffect::Create(texture, matrix))->unref();
    }

    void addColorTextureEffect(GrTexture* texture,
                               const SkMatrix& matrix,
                               const GrTextureParams& params) {
        this->addColorEffect(GrSimpleTextureEffect::Create(texture, matrix, params))->unref();
    }

    void addCoverageTextureEffect(GrTexture* texture,
                                  const SkMatrix& matrix,
                                  const GrTextureParams& params) {
        this->addCoverageEffect(GrSimpleTextureEffect::Create(texture, matrix, params))->unref();
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
                int m = fDrawState->fColorStages.count() - fColorEffectCnt;
                SkASSERT(m >= 0);
                fDrawState->fColorStages.pop_back_n(m);

                int n = fDrawState->fCoverageStages.count() - fCoverageEffectCnt;
                SkASSERT(n >= 0);
                fDrawState->fCoverageStages.pop_back_n(n);
                if (m + n > 0) {
                    fDrawState->invalidateBlendOptFlags();
                }
                SkDEBUGCODE(--fDrawState->fBlockEffectRemovalCnt;)
            }
            fDrawState = ds;
            if (NULL != ds) {
                fColorEffectCnt = ds->fColorStages.count();
                fCoverageEffectCnt = ds->fCoverageStages.count();
                SkDEBUGCODE(++ds->fBlockEffectRemovalCnt;)
            }
        }

        bool isSet() const { return NULL != fDrawState; }

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
        fSrcBlend = srcCoeff;
        fDstBlend = dstCoeff;
        this->invalidateBlendOptFlags();
    #ifdef SK_DEBUG
        if (GrBlendCoeffRefsDst(dstCoeff)) {
            GrPrintf("Unexpected dst blend coeff. Won't work correctly with coverage stages.\n");
        }
        if (GrBlendCoeffRefsSrc(srcCoeff)) {
            GrPrintf("Unexpected src blend coeff. Won't work correctly with coverage stages.\n");
        }
    #endif
    }

    GrBlendCoeff getSrcBlendCoeff() const { return fSrcBlend; }
    GrBlendCoeff getDstBlendCoeff() const { return fDstBlend; }

    void getDstBlendCoeff(GrBlendCoeff* srcBlendCoeff,
                          GrBlendCoeff* dstBlendCoeff) const {
        *srcBlendCoeff = fSrcBlend;
        *dstBlendCoeff = fDstBlend;
    }

    









    void setBlendConstant(GrColor constant) {
        fBlendConstant = constant;
        this->invalidateBlendOptFlags();
    }

    



    GrColor getBlendConstant() const { return fBlendConstant; }

    




    bool canTweakAlphaForCoverage() const;

    


    enum BlendOptFlags {
        


        kNone_BlendOpt                  = 0,
        


        kSkipDraw_BlendOptFlag          = 0x1,
        


        kDisableBlend_BlendOptFlag      = 0x2,
        



        kCoverageAsAlpha_BlendOptFlag   = 0x4,
        



        kEmitCoverage_BlendOptFlag      = 0x8,
        


        kEmitTransBlack_BlendOptFlag    = 0x10,
        



        kInvalid_BlendOptFlag        = 0x20,
    };
    GR_DECL_BITFIELD_OPS_FRIENDS(BlendOptFlags);

    void invalidateBlendOptFlags() {
        fBlendOptFlags = kInvalid_BlendOptFlag;
    }

    



    bool canIgnoreColorAttribute() const;

    












    BlendOptFlags getBlendOpts(bool forceCoverage = false,
                               GrBlendCoeff* srcCoeff = NULL,
                               GrBlendCoeff* dstCoeff = NULL) const;

    

    
    
    

    



    bool setIdentityViewMatrix();

    



    const SkMatrix& getViewMatrix() const { return fViewMatrix; }

    








    bool getViewInverse(SkMatrix* matrix) const {
        
        
        SkMatrix inverse;
        if (fViewMatrix.invert(&inverse)) {
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
        fStencilSettings = settings;
        this->invalidateBlendOptFlags();
    }

    


    void disableStencil() {
        fStencilSettings.setDisabled();
        this->invalidateBlendOptFlags();
    }

    const GrStencilSettings& getStencil() const { return fStencilSettings; }

    GrStencilSettings* stencil() { return &fStencilSettings; }

    

    
    
    

    



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
        fFlagBits = 0;
        this->invalidateBlendOptFlags();
    }

    




    void enableState(uint32_t stateBits) {
        fFlagBits |= stateBits;
        this->invalidateBlendOptFlags();
    }

    




    void disableState(uint32_t stateBits) {
        fFlagBits &= ~(stateBits);
        this->invalidateBlendOptFlags();
    }

    





    void setState(uint32_t stateBits, bool enable) {
        if (enable) {
            this->enableState(stateBits);
        } else {
            this->disableState(stateBits);
        }
    }

    bool isDitherState() const {
        return 0 != (fFlagBits & kDither_StateBit);
    }

    bool isHWAntialiasState() const {
        return 0 != (fFlagBits & kHWAntialias_StateBit);
    }

    bool isClipState() const {
        return 0 != (fFlagBits & kClip_StateBit);
    }

    bool isColorWriteDisabled() const {
        return 0 != (fFlagBits & kNoColorWrites_StateBit);
    }

    bool isCoverageDrawing() const {
        return 0 != (fFlagBits & kCoverageDrawing_StateBit);
    }

    bool isStateFlagEnabled(uint32_t stateBit) const {
        return 0 != (stateBit & fFlagBits);
    }

    

    
    
    

    enum DrawFace {
        kInvalid_DrawFace = -1,

        kBoth_DrawFace,
        kCCW_DrawFace,
        kCW_DrawFace,
    };

    



    void setDrawFace(DrawFace face) {
        SkASSERT(kInvalid_DrawFace != face);
        fDrawFace = face;
    }

    




    DrawFace getDrawFace() const { return fDrawFace; }

    

    

    bool operator ==(const GrDrawState& that) const {
        if (fRenderTarget.get() != that.fRenderTarget.get() ||
            fColorStages.count() != that.fColorStages.count() ||
            fCoverageStages.count() != that.fCoverageStages.count() ||
            fColor != that.fColor ||
            !fViewMatrix.cheapEqualTo(that.fViewMatrix) ||
            fSrcBlend != that.fSrcBlend ||
            fDstBlend != that.fDstBlend ||
            fBlendConstant != that.fBlendConstant ||
            fFlagBits != that.fFlagBits ||
            fVACount != that.fVACount ||
            memcmp(fVAPtr, that.fVAPtr, fVACount * sizeof(GrVertexAttrib)) ||
            fStencilSettings != that.fStencilSettings ||
            fCoverage != that.fCoverage ||
            fDrawFace != that.fDrawFace) {
            return false;
        }
        for (int i = 0; i < fColorStages.count(); i++) {
            if (fColorStages[i] != that.fColorStages[i]) {
                return false;
            }
        }
        for (int i = 0; i < fCoverageStages.count(); i++) {
            if (fCoverageStages[i] != that.fCoverageStages[i]) {
                return false;
            }
        }
        SkASSERT(0 == memcmp(fFixedFunctionVertexAttribIndices,
                             that.fFixedFunctionVertexAttribIndices,
                             sizeof(fFixedFunctionVertexAttribIndices)));
        return true;
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }

    GrDrawState& operator= (const GrDrawState& that) {
        SkASSERT(0 == fBlockEffectRemovalCnt || 0 == this->numTotalStages());
        this->setRenderTarget(that.fRenderTarget.get());
        fColor = that.fColor;
        fViewMatrix = that.fViewMatrix;
        fSrcBlend = that.fSrcBlend;
        fDstBlend = that.fDstBlend;
        fBlendConstant = that.fBlendConstant;
        fFlagBits = that.fFlagBits;
        fVACount = that.fVACount;
        fVAPtr = that.fVAPtr;
        fStencilSettings = that.fStencilSettings;
        fCoverage = that.fCoverage;
        fDrawFace = that.fDrawFace;
        fColorStages = that.fColorStages;
        fCoverageStages = that.fCoverageStages;
        fOptSrcBlend = that.fOptSrcBlend;
        fOptDstBlend = that.fOptDstBlend;
        fBlendOptFlags = that.fBlendOptFlags;

        memcpy(fFixedFunctionVertexAttribIndices,
               that.fFixedFunctionVertexAttribIndices,
               sizeof(fFixedFunctionVertexAttribIndices));
        return *this;
    }

private:

    void onReset(const SkMatrix* initialViewMatrix) {
        SkASSERT(0 == fBlockEffectRemovalCnt || 0 == this->numTotalStages());
        fColorStages.reset();
        fCoverageStages.reset();

        fRenderTarget.reset(NULL);

        this->setDefaultVertexAttribs();

        fColor = 0xffffffff;
        if (NULL == initialViewMatrix) {
            fViewMatrix.reset();
        } else {
            fViewMatrix = *initialViewMatrix;
        }
        fSrcBlend = kOne_GrBlendCoeff;
        fDstBlend = kZero_GrBlendCoeff;
        fBlendConstant = 0x0;
        fFlagBits = 0x0;
        fStencilSettings.setDisabled();
        fCoverage = 0xffffffff;
        fDrawFace = kBoth_DrawFace;

        this->invalidateBlendOptFlags();
    }

    BlendOptFlags calcBlendOpts(bool forceCoverage = false,
                               GrBlendCoeff* srcCoeff = NULL,
                               GrBlendCoeff* dstCoeff = NULL) const;

    
    SkAutoTUnref<GrRenderTarget>        fRenderTarget;
    GrColor                             fColor;
    SkMatrix                            fViewMatrix;
    GrBlendCoeff                        fSrcBlend;
    GrBlendCoeff                        fDstBlend;
    GrColor                             fBlendConstant;
    uint32_t                            fFlagBits;
    const GrVertexAttrib*               fVAPtr;
    int                                 fVACount;
    GrStencilSettings                   fStencilSettings;
    GrColor                             fCoverage;
    DrawFace                            fDrawFace;

    typedef SkSTArray<4, GrEffectStage> EffectStageArray;
    EffectStageArray                    fColorStages;
    EffectStageArray                    fCoverageStages;
    
    mutable GrBlendCoeff                        fOptSrcBlend;
    mutable GrBlendCoeff                        fOptDstBlend;
    mutable BlendOptFlags                       fBlendOptFlags;

    
    
    int fFixedFunctionVertexAttribIndices[kGrFixedFunctionVertexAttribBindingCnt];

    
    
    SkDEBUGCODE(int fBlockEffectRemovalCnt;)

    





    void setVertexAttribs(const GrVertexAttrib attribs[], int count);

    typedef SkRefCnt INHERITED;
};

GR_MAKE_BITFIELD_OPS(GrDrawState::BlendOptFlags);

#endif
