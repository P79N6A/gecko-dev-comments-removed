






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrColor.h"
#include "GrMatrix.h"
#include "GrNoncopyable.h"
#include "GrRefCnt.h"
#include "GrSamplerState.h"
#include "GrStencil.h"

#include "SkXfermode.h"

class GrRenderTarget;
class GrTexture;

class GrDrawState : public GrRefCnt {

public:
    


















    enum {
        kNumStages = 4,
        kMaxTexCoords = kNumStages
    };

    


    typedef uint32_t StageMask;
    GR_STATIC_ASSERT(sizeof(StageMask)*8 >= GrDrawState::kNumStages);

    GrDrawState() {
        this->reset();
    }

    GrDrawState(const GrDrawState& state) {
        *this = state;
    }

    

 
    void reset() {
        
        
        
        
        memset(this->podStart(), 0, this->memsetSize());

        
        
        GrAssert((intptr_t)(void*)NULL == 0LL);
        GR_STATIC_ASSERT(0 == kBoth_DrawFace);
        GrAssert(fStencilSettings.isDisabled());

        
        fColor = 0xffffffff;
        fCoverage = 0xffffffff;
        fFirstCoverageStage = kNumStages;
        fColorFilterMode = SkXfermode::kDst_Mode;
        fSrcBlend = kOne_BlendCoeff;
        fDstBlend = kZero_BlendCoeff;
        fViewMatrix.reset();
        fBehaviorBits = 0;

        
        
        GrAssert(this->memsetSize() +  sizeof(fColor) + sizeof(fCoverage) +
                 sizeof(fFirstCoverageStage) + sizeof(fColorFilterMode) +
                 sizeof(fSrcBlend) + sizeof(fDstBlend) ==
                 this->podSize());
    }

    
    
    

    




    void setColor(GrColor color) { fColor = color; }

    GrColor getColor() const { return fColor; }

    





    void setAlpha(uint8_t a) {
        this->setColor((a << 24) | (a << 16) | (a << 8) | a);
    }

    



    void setColorFilter(GrColor c, SkXfermode::Mode mode) {
        fColorFilterColor = c;
        fColorFilterMode = mode;
    }

    GrColor getColorFilterColor() const { return fColorFilterColor; }
    SkXfermode::Mode getColorFilterMode() const { return fColorFilterMode; }

    

    
    
    

    




    void setCoverage(uint8_t coverage) {
        fCoverage = GrColorPackRGBA(coverage, coverage, coverage, coverage);
    }

    



    void setCoverage4(GrColor coverage) {
        fCoverage = coverage;
    }

    GrColor getCoverage() const {
        return fCoverage;
    }

    

    
    
    

    







    void setTexture(int stage, GrTexture* texture) {
        GrAssert((unsigned)stage < kNumStages);

        if (isBehaviorEnabled(kTexturesNeedRef_BehaviorBit)) {
            
            
            
            GrTexture* temp = fTextures[stage];
            fTextures[stage] = NULL;

            SkSafeRef(texture);
            SkSafeUnref(temp);
        }

        fTextures[stage] = texture;
    }

    






    const GrTexture* getTexture(int stage) const {
        GrAssert((unsigned)stage < kNumStages);
        return fTextures[stage];
    }
    GrTexture* getTexture(int stage) {
        GrAssert((unsigned)stage < kNumStages);
        return fTextures[stage];
    }

    

    
    
    

    


    const GrSamplerState& getSampler(int stage) const {
        GrAssert((unsigned)stage < kNumStages);
        return fSamplerStates[stage];
    }

    


    GrSamplerState* sampler(int stage) {
        GrAssert((unsigned)stage < kNumStages);
        return fSamplerStates + stage;
    }

    


    void preConcatSamplerMatrices(StageMask stageMask, const GrMatrix& matrix) {
        GrAssert(!(stageMask & kIllegalStageMaskBits));
        for (int i = 0; i < kNumStages; ++i) {
            if ((1 << i) & stageMask) {
                fSamplerStates[i].preConcatMatrix(matrix);
            }
        }
    }

    

    
    
    

    








    void setFirstCoverageStage(int firstCoverageStage) {
        GrAssert((unsigned)firstCoverageStage <= kNumStages);
        fFirstCoverageStage = firstCoverageStage; 
    }

    


    int getFirstCoverageStage() const {
        return fFirstCoverageStage; 
    }

    

    
    
    

    












    void setBlendFunc(GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff) {
        fSrcBlend = srcCoeff;
        fDstBlend = dstCoeff;
    #if GR_DEBUG
        switch (dstCoeff) {
        case kDC_BlendCoeff:
        case kIDC_BlendCoeff:
        case kDA_BlendCoeff:
        case kIDA_BlendCoeff:
            GrPrintf("Unexpected dst blend coeff. Won't work correctly with"
                     "coverage stages.\n");
            break;
        default:
            break;
        }
        switch (srcCoeff) {
        case kSC_BlendCoeff:
        case kISC_BlendCoeff:
        case kSA_BlendCoeff:
        case kISA_BlendCoeff:
            GrPrintf("Unexpected src blend coeff. Won't work correctly with"
                     "coverage stages.\n");
            break;
        default:
            break;
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

    









    void setBlendConstant(GrColor constant) { fBlendConstant = constant; }

    



    GrColor getBlendConstant() const { return fBlendConstant; }

    

    
    
    

    






    void setViewMatrix(const GrMatrix& m) { fViewMatrix = m; }

    


    GrMatrix* viewMatrix() { return &fViewMatrix; }

    









    void preConcatViewMatrix(const GrMatrix& m) { fViewMatrix.preConcat(m); }

    









    void postConcatViewMatrix(const GrMatrix& m) { fViewMatrix.postConcat(m); }

    



    const GrMatrix& getViewMatrix() const { return fViewMatrix; }

    








    bool getViewInverse(GrMatrix* matrix) const {
        
        
        GrMatrix inverse;
        if (fViewMatrix.invert(&inverse)) {
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
        AutoViewMatrixRestore(GrDrawState* ds, const GrMatrix& newMatrix) {
            fDrawState = NULL;
            this->set(ds, newMatrix);
        }
        AutoViewMatrixRestore(GrDrawState* ds) {
            fDrawState = NULL;
            this->set(ds);
        }
        ~AutoViewMatrixRestore() {
            this->set(NULL, GrMatrix::I());
        }
        void set(GrDrawState* ds, const GrMatrix& newMatrix) {
            if (NULL != fDrawState) {
                fDrawState->setViewMatrix(fSavedMatrix);
            }
            if (NULL != ds) {
                fSavedMatrix = ds->getViewMatrix();
                ds->setViewMatrix(newMatrix);
            }
            fDrawState = ds;
        }
        void set(GrDrawState* ds) {
            if (NULL != fDrawState) {
                fDrawState->setViewMatrix(fSavedMatrix);
            }
            if (NULL != ds) {
                fSavedMatrix = ds->getViewMatrix();
            }
            fDrawState = ds;
        }
    private:
        GrDrawState* fDrawState;
        GrMatrix fSavedMatrix;
    };

    

    
    
    

    




    void setRenderTarget(GrRenderTarget* target) { fRenderTarget = target; }

    




    const GrRenderTarget* getRenderTarget() const { return fRenderTarget; }
    GrRenderTarget* getRenderTarget() { return fRenderTarget; }

    class AutoRenderTargetRestore : public ::GrNoncopyable {
    public:
        AutoRenderTargetRestore() : fDrawState(NULL), fSavedTarget(NULL) {}
        AutoRenderTargetRestore(GrDrawState* ds, GrRenderTarget* newTarget) {
            fDrawState = NULL;
            fSavedTarget = NULL;
            this->set(ds, newTarget);
        }
        ~AutoRenderTargetRestore() { this->set(NULL, NULL); }
        void set(GrDrawState* ds, GrRenderTarget* newTarget) {
            if (NULL != fDrawState) {
                fDrawState->setRenderTarget(fSavedTarget);
            }
            if (NULL != ds) {
                fSavedTarget = ds->getRenderTarget();
                ds->setRenderTarget(newTarget);
            }
            fDrawState = ds;
        }
    private:
        GrDrawState* fDrawState;
        GrRenderTarget* fSavedTarget;
    };

    

    
    
    

    






    void setStencil(const GrStencilSettings& settings) {
        fStencilSettings = settings;
    }

    


    void disableStencil() {
        fStencilSettings.setDisabled();
    }

    const GrStencilSettings& getStencil() const { return fStencilSettings; }

    GrStencilSettings* stencil() { return &fStencilSettings; }

    

    
    
    

    



    void setColorMatrix(const float matrix[20]) {
        memcpy(fColorMatrix, matrix, sizeof(fColorMatrix));
    }

    const float* getColorMatrix() const { return fColorMatrix; }

    

    
    
    
    
    
    
    

    







    enum VertexEdgeType {
        

        kHairLine_EdgeType,
        



        kQuad_EdgeType,
        

        kHairQuad_EdgeType,
        

        kCircle_EdgeType,

        kVertexEdgeTypeCnt
    };

    




    void setVertexEdgeType(VertexEdgeType type) {
        GrAssert(type >=0 && type < kVertexEdgeTypeCnt);
        fVertexEdgeType = type;
    }

    VertexEdgeType getVertexEdgeType() const { return fVertexEdgeType; }

    

    
    
    

    



    enum StateBits {
        


        kDither_StateBit        = 0x01,
        




        kHWAntialias_StateBit   = 0x02,
        


        kClip_StateBit          = 0x04,
        



        kNoColorWrites_StateBit = 0x08,
        




        kEdgeAAConcave_StateBit = 0x10,
        



        kColorMatrix_StateBit   = 0x20,

        
        kDummyStateBit,
        kLastPublicStateBit = kDummyStateBit-1,
    };

    void resetStateFlags() {
        fFlagBits = 0;
    }

    




    void enableState(uint32_t stateBits) {
        fFlagBits |= stateBits;
    }

    




    void disableState(uint32_t stateBits) {
        fFlagBits &= ~(stateBits);
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

    bool isStateFlagEnabled(uint32_t stateBit) const {
        return 0 != (stateBit & fFlagBits);
    }

    void copyStateFlags(const GrDrawState& ds) {
        fFlagBits = ds.fFlagBits;
    }

    


    enum GrBehaviorBits {
        


        kTexturesNeedRef_BehaviorBit = 0x01,
    };

    void enableBehavior(uint32_t behaviorBits) {
        fBehaviorBits |= behaviorBits;
    }

    void disableBehavior(uint32_t behaviorBits) {
        fBehaviorBits &= ~(behaviorBits);
    }

    bool isBehaviorEnabled(uint32_t behaviorBits) const {
        return 0 != (behaviorBits & fBehaviorBits);
    }

    

    
    
    

    enum DrawFace {
        kInvalid_DrawFace = -1,

        kBoth_DrawFace,
        kCCW_DrawFace,
        kCW_DrawFace,
    };

    



    void setDrawFace(DrawFace face) {
        GrAssert(kInvalid_DrawFace != face);
        fDrawFace = face;
    }

    




    DrawFace getDrawFace() const { return fDrawFace; }
    
    

    

    
    
    bool operator ==(const GrDrawState& s) const {
        if (memcmp(this->podStart(), s.podStart(), this->podSize())) {
            return false;
        }

        if (!s.fViewMatrix.cheapEqualTo(fViewMatrix)) {
            return false;
        }

        
        
        
        if ((fBehaviorBits & ~kTexturesNeedRef_BehaviorBit) != 
            (s.fBehaviorBits & ~kTexturesNeedRef_BehaviorBit)) {
            return false;
        }

        for (int i = 0; i < kNumStages; i++) {
            if (fTextures[i] &&
                this->fSamplerStates[i] != s.fSamplerStates[i]) {
                return false;
            }
        }
        if (kColorMatrix_StateBit & s.fFlagBits) {
            if (memcmp(fColorMatrix,
                        s.fColorMatrix,
                        sizeof(fColorMatrix))) {
                return false;
            }
        }

        return true;
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }

    
    
    GrDrawState& operator =(const GrDrawState& s) {
        memcpy(this->podStart(), s.podStart(), this->podSize());

        fViewMatrix = s.fViewMatrix;
        fBehaviorBits = s.fBehaviorBits;

        for (int i = 0; i < kNumStages; i++) {
            if (s.fTextures[i]) {
                this->fSamplerStates[i] = s.fSamplerStates[i];
            }
        }
        if (kColorMatrix_StateBit & s.fFlagBits) {
            memcpy(this->fColorMatrix, s.fColorMatrix, sizeof(fColorMatrix));
        }

        return *this;
    }

private:

    const void* podStart() const {
        return reinterpret_cast<const void*>(&fPodStartMarker);
    }
    void* podStart() {
        return reinterpret_cast<void*>(&fPodStartMarker);
    }
    size_t memsetSize() const {
        return reinterpret_cast<size_t>(&fMemsetEndMarker) -
               reinterpret_cast<size_t>(&fPodStartMarker) +
               sizeof(fMemsetEndMarker);
    }
    size_t podSize() const {
        
        return reinterpret_cast<size_t>(&fPodEndMarker) -
               reinterpret_cast<size_t>(&fPodStartMarker) +
               sizeof(fPodEndMarker);
    }

    static const StageMask kIllegalStageMaskBits = ~((1 << kNumStages)-1);
    
    union {
        GrColor             fBlendConstant;
        GrColor             fPodStartMarker;
    };
    GrTexture*          fTextures[kNumStages];
    GrColor             fColorFilterColor;
    uint32_t            fFlagBits;
    DrawFace            fDrawFace; 
    VertexEdgeType      fVertexEdgeType;
    GrStencilSettings   fStencilSettings;
    union {
        GrRenderTarget* fRenderTarget;
        GrRenderTarget* fMemsetEndMarker;
    };
    

    
    
    GrColor             fColor;
    GrColor             fCoverage;
    int                 fFirstCoverageStage;
    SkXfermode::Mode    fColorFilterMode;
    GrBlendCoeff        fSrcBlend;
    union {
        GrBlendCoeff    fDstBlend;
        GrBlendCoeff    fPodEndMarker;
    };
    

    uint32_t            fBehaviorBits;
    GrMatrix            fViewMatrix;

    
    
    GrSamplerState      fSamplerStates[kNumStages];
    
    float               fColorMatrix[20];       

};

#endif
