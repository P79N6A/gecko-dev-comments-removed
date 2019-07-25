






#ifndef GrDrawState_DEFINED
#define GrDrawState_DEFINED

#include "GrColor.h"
#include "GrMatrix.h"
#include "GrNoncopyable.h"
#include "GrSamplerState.h"
#include "GrStencil.h"

#include "SkXfermode.h"

class GrRenderTarget;
class GrTexture;

struct GrDrawState {

    













    enum {
        kNumStages = 3,
        kMaxTexCoords = kNumStages
    };

    


    typedef uint32_t StageMask;
    GR_STATIC_ASSERT(sizeof(StageMask)*8 >= GrDrawState::kNumStages);

    GrDrawState() {
        
        
        
        memset(this, 0, sizeof(GrDrawState));
            
        
        fColorFilterMode = SkXfermode::kDstIn_Mode;
        fFirstCoverageStage = kNumStages;

        
        
        GrAssert((intptr_t)(void*)NULL == 0LL);

        GrAssert(fStencilSettings.isDisabled());
        fFirstCoverageStage = kNumStages;
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

    

    
    
    

    







    void setTexture(int stage, GrTexture* texture) {
        GrAssert((unsigned)stage < kNumStages);
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
        AutoRenderTargetRestore() : fDrawState(NULL) {}
        AutoRenderTargetRestore(GrDrawState* ds, GrRenderTarget* newTarget) {
            fDrawState = NULL;
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
        

        kHairQuad_EdgeType
    };

    




    void setVertexEdgeType(VertexEdgeType type) {
        fVertexEdgeType = type;
    }

    VertexEdgeType getVertexEdgeType() const {
        return fVertexEdgeType;
    }

    





    enum {
        
        
        kMaxEdges = 1
    };

    class Edge {
      public:
        Edge() {}
        Edge(float x, float y, float z) : fX(x), fY(y), fZ(z) {}
        GrPoint intersect(const Edge& other) {
            return GrPoint::Make(
                SkFloatToScalar((fY * other.fZ - other.fY * fZ) /
                                (fX * other.fY - other.fX * fY)),
                SkFloatToScalar((fX * other.fZ - other.fX * fZ) /
                                (other.fX * fY - fX * other.fY)));
        }
        float fX, fY, fZ;
    };

    





    void setEdgeAAData(const Edge* edges, int numEdges) {
        GrAssert(numEdges <= GrDrawState::kMaxEdges);
        memcpy(fEdgeAAEdges, edges, numEdges * sizeof(GrDrawState::Edge));
        fEdgeAANumEdges = numEdges;
    }

    int getNumAAEdges() const { return fEdgeAANumEdges; }

    const Edge* getAAEdges() const { return fEdgeAAEdges; }

    

    
    
    

    



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

    bool isConcaveEdgeAAState() const {
        return 0 != (fFlagBits & kEdgeAAConcave_StateBit);
    }

    bool isStateFlagEnabled(uint32_t stateBit) const {
        return 0 != (stateBit & fFlagBits);
    }

    void copyStateFlags(const GrDrawState& ds) {
        fFlagBits = ds.fFlagBits;
    }

    

    
    
    

    enum DrawFace {
        kBoth_DrawFace,
        kCCW_DrawFace,
        kCW_DrawFace,
    };

    



    void setDrawFace(DrawFace face) {
        fDrawFace = face;
    }

    




    DrawFace getDrawFace() const {
        return fDrawFace;
    }
    
    

    

    
    
    bool operator ==(const GrDrawState& s) const {
        if (memcmp(this, &s, this->leadingBytes())) return false;

        for (int i = 0; i < kNumStages; i++) {
            if (fTextures[i] &&
                memcmp(&this->fSamplerStates[i], &s.fSamplerStates[i],
                       sizeof(GrSamplerState))) {
                return false;
            }
        }

        return true;
    }
    bool operator !=(const GrDrawState& s) const { return !(*this == s); }

    
    
    GrDrawState& operator =(const GrDrawState& s) {
        memcpy(this, &s, this->leadingBytes());

        for (int i = 0; i < kNumStages; i++) {
            if (s.fTextures[i]) {
                memcpy(&this->fSamplerStates[i], &s.fSamplerStates[i],
                       sizeof(GrSamplerState));
            }
        }

        return *this;
    }

private:
    static const StageMask kIllegalStageMaskBits = ~((1 << kNumStages)-1);
    uint8_t                 fFlagBits;
    GrBlendCoeff            fSrcBlend : 8;
    GrBlendCoeff            fDstBlend : 8;
    DrawFace                fDrawFace : 8;
    uint8_t                 fFirstCoverageStage;
    SkXfermode::Mode        fColorFilterMode : 8;
    GrColor                 fBlendConstant;
    GrTexture*              fTextures[kNumStages];
    GrRenderTarget*         fRenderTarget;
    GrColor                 fColor;
    GrColor                 fColorFilterColor;
    float                   fColorMatrix[20];
    GrStencilSettings       fStencilSettings;
    GrMatrix                fViewMatrix;
    
    
    

    VertexEdgeType          fVertexEdgeType;
    int                     fEdgeAANumEdges;
    Edge                    fEdgeAAEdges[kMaxEdges];

    
    
    
    GrSamplerState          fSamplerStates[kNumStages];

    size_t leadingBytes() const {
        
        
        
        
        return (size_t) ((unsigned char*)&fEdgeAANumEdges -
                         (unsigned char*)&fFlagBits);
    }

};

#endif
