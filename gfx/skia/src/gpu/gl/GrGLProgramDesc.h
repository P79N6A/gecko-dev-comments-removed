






#ifndef GrGLProgramDesc_DEFINED
#define GrGLProgramDesc_DEFINED

#include "GrGLEffect.h"
#include "GrDrawState.h"
#include "GrGLShaderBuilder.h"

class GrGpuGL;



#define GR_GL_EXPERIMENTAL_GS GR_DEBUG






class GrGLProgramDesc {
public:
    GrGLProgramDesc() {
        
        memset(this, 0, sizeof(GrGLProgramDesc));
    }

    
    const uint32_t* asKey() const {
        return reinterpret_cast<const uint32_t*>(this);
    }

    
    void setRandom(SkMWCRandom*,
                   const GrGpuGL* gpu,
                   const GrTexture* dummyDstTexture,
                   const GrEffectStage stages[GrDrawState::kNumStages],
                   int currAttribIndex);

    



    static void Build(const GrDrawState&,
                      bool isPoints,
                      GrDrawState::BlendOptFlags,
                      GrBlendCoeff srcCoeff,
                      GrBlendCoeff dstCoeff,
                      const GrGpuGL* gpu,
                      const GrDeviceCoordTexture* dstCopy,
                      GrGLProgramDesc* outDesc);

private:
    
    enum ColorInput {
        kSolidWhite_ColorInput,
        kTransBlack_ColorInput,
        kAttribute_ColorInput,
        kUniform_ColorInput,

        kColorInputCnt
    };
    
    
    
    enum DualSrcOutput {
        kNone_DualSrcOutput,
        kCoverage_DualSrcOutput,
        kCoverageISA_DualSrcOutput,
        kCoverageISC_DualSrcOutput,

        kDualSrcOutputCnt
    };

    
    GrGLEffect::EffectKey       fEffectKeys[GrDrawState::kNumStages];

    
    
#if GR_GL_EXPERIMENTAL_GS
    bool                     fExperimentalGS;
#endif

    GrGLShaderBuilder::DstReadKey fDstRead;             
                                                        
                                                        

    
    SkBool8                     fDiscardIfZeroCoverage;

    uint8_t                     fColorInput;            
    uint8_t                     fCoverageInput;         
    uint8_t                     fDualSrcOutput;         

    int8_t                      fFirstCoverageStage;
    SkBool8                     fEmitsPointSize;
    uint8_t                     fColorFilterXfermode;   

    int8_t                      fPositionAttributeIndex;
    int8_t                      fLocalCoordAttributeIndex;
    int8_t                      fColorAttributeIndex;
    int8_t                      fCoverageAttributeIndex;

    
    
    friend class GrGLProgram;
    friend class GrGLShaderBuilder;
};

#endif
