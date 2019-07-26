






#include "GrGLProgramDesc.h"
#include "GrBackendEffectFactory.h"
#include "GrDrawEffect.h"
#include "GrEffect.h"
#include "GrGLShaderBuilder.h"
#include "GrGpuGL.h"

void GrGLProgramDesc::Build(const GrDrawState& drawState,
                            bool isPoints,
                            GrDrawState::BlendOptFlags blendOpts,
                            GrBlendCoeff srcCoeff,
                            GrBlendCoeff dstCoeff,
                            const GrGpuGL* gpu,
                            const GrDeviceCoordTexture* dstCopy,
                            GrGLProgramDesc* desc) {

    
    GrAssert(!(GrDrawState::kSkipDraw_BlendOptFlag & blendOpts));

    bool skipCoverage = SkToBool(blendOpts & GrDrawState::kEmitTransBlack_BlendOptFlag);

    bool skipColor = SkToBool(blendOpts & (GrDrawState::kEmitTransBlack_BlendOptFlag |
                                           GrDrawState::kEmitCoverage_BlendOptFlag));

    
    
    
    


    desc->fEmitsPointSize = isPoints;

    bool requiresColorAttrib = !skipColor && drawState.hasColorVertexAttribute();
    bool requiresCoverageAttrib = !skipCoverage && drawState.hasCoverageVertexAttribute();
    
    bool requiresLocalCoordAttrib = !(skipCoverage  && skipColor) &&
                                    drawState.hasLocalCoordAttribute();

    
    
    

    desc->fColorFilterXfermode = skipColor ? SkXfermode::kDst_Mode : drawState.getColorFilterMode();


    bool colorIsTransBlack = SkToBool(blendOpts & GrDrawState::kEmitTransBlack_BlendOptFlag);
    bool colorIsSolidWhite = (blendOpts & GrDrawState::kEmitCoverage_BlendOptFlag) ||
                             (!requiresColorAttrib && 0xffffffff == drawState.getColor());
    if (colorIsTransBlack) {
        desc->fColorInput = kTransBlack_ColorInput;
    } else if (colorIsSolidWhite) {
        desc->fColorInput = kSolidWhite_ColorInput;
    } else if (GR_GL_NO_CONSTANT_ATTRIBUTES && !requiresColorAttrib) {
        desc->fColorInput = kUniform_ColorInput;
    } else {
        desc->fColorInput = kAttribute_ColorInput;
    }

    bool covIsSolidWhite = !requiresCoverageAttrib && 0xffffffff == drawState.getCoverage();

    if (skipCoverage) {
        desc->fCoverageInput = kTransBlack_ColorInput;
    } else if (covIsSolidWhite) {
        desc->fCoverageInput = kSolidWhite_ColorInput;
    } else if (GR_GL_NO_CONSTANT_ATTRIBUTES && !requiresCoverageAttrib) {
        desc->fCoverageInput = kUniform_ColorInput;
    } else {
        desc->fCoverageInput = kAttribute_ColorInput;
    }

    bool readsDst = false;
    int lastEnabledStage = -1;

    for (int s = 0; s < GrDrawState::kNumStages; ++s) {

        bool skip = s < drawState.getFirstCoverageStage() ? skipColor : skipCoverage;
        if (!skip && drawState.isStageEnabled(s)) {
            lastEnabledStage = s;
            const GrEffectRef& effect = *drawState.getStage(s).getEffect();
            const GrBackendEffectFactory& factory = effect->getFactory();
            GrDrawEffect drawEffect(drawState.getStage(s), requiresLocalCoordAttrib);
            desc->fEffectKeys[s] = factory.glEffectKey(drawEffect, gpu->glCaps());
            if (effect->willReadDst()) {
                readsDst = true;
            }
        } else {
            desc->fEffectKeys[s] = 0;
        }
    }

    if (readsDst) {
        GrAssert(NULL != dstCopy);
        desc->fDstRead = GrGLShaderBuilder::KeyForDstRead(dstCopy->texture(), gpu->glCaps());
        GrAssert(0 != desc->fDstRead);
    } else {
        desc->fDstRead = 0;
    }

    desc->fDualSrcOutput = kNone_DualSrcOutput;

    
    
#if GR_GL_EXPERIMENTAL_GS
#if 0
    desc->fExperimentalGS = gpu->caps().geometryShaderSupport();
#else
    desc->fExperimentalGS = false;
#endif
#endif

    
    
    
    desc->fFirstCoverageStage = GrDrawState::kNumStages;
    
    int firstCoverageStage = GrDrawState::kNumStages;
    desc->fDiscardIfZeroCoverage = false; 
    bool hasCoverage = false;
    
    if (!drawState.isCoverageDrawing()) {
        
        if (drawState.getFirstCoverageStage() <= lastEnabledStage) {
            firstCoverageStage = drawState.getFirstCoverageStage();
            hasCoverage = true;
        } else {
            hasCoverage = requiresCoverageAttrib;
        }
    }

    if (hasCoverage) {
        
        if (SkXfermode::kDst_Mode != desc->fColorFilterXfermode) {
            desc->fFirstCoverageStage = firstCoverageStage;
        }

        
        if (drawState.getStencil().doesWrite()) {
            desc->fDiscardIfZeroCoverage = true;
            desc->fFirstCoverageStage = firstCoverageStage;
        }

        if (gpu->caps()->dualSourceBlendingSupport() &&
            !(blendOpts & (GrDrawState::kEmitCoverage_BlendOptFlag |
                           GrDrawState::kCoverageAsAlpha_BlendOptFlag))) {
            if (kZero_GrBlendCoeff == dstCoeff) {
                
                desc->fDualSrcOutput =  kCoverage_DualSrcOutput;
                desc->fFirstCoverageStage = firstCoverageStage;
            } else if (kSA_GrBlendCoeff == dstCoeff) {
                
                desc->fDualSrcOutput = kCoverageISA_DualSrcOutput;
                desc->fFirstCoverageStage = firstCoverageStage;
            } else if (kSC_GrBlendCoeff == dstCoeff) {
                
                desc->fDualSrcOutput = kCoverageISC_DualSrcOutput;
                desc->fFirstCoverageStage = firstCoverageStage;
            }
        }
    }

    desc->fPositionAttributeIndex = drawState.positionAttributeIndex();
    desc->fLocalCoordAttributeIndex = drawState.localCoordAttributeIndex();

    
    int availableAttributeIndex = drawState.getVertexAttribCount();
    if (requiresColorAttrib) {
        desc->fColorAttributeIndex = drawState.colorVertexAttributeIndex();
    } else if (GrGLProgramDesc::kAttribute_ColorInput == desc->fColorInput) {
        GrAssert(availableAttributeIndex < GrDrawState::kMaxVertexAttribCnt);
        desc->fColorAttributeIndex = availableAttributeIndex;
        availableAttributeIndex++;
    } else {
        desc->fColorAttributeIndex = -1;
    }

    if (requiresCoverageAttrib) {
        desc->fCoverageAttributeIndex = drawState.coverageVertexAttributeIndex();
    } else if (GrGLProgramDesc::kAttribute_ColorInput == desc->fCoverageInput) {
        GrAssert(availableAttributeIndex < GrDrawState::kMaxVertexAttribCnt);
        desc->fCoverageAttributeIndex = availableAttributeIndex;
    } else {
        desc->fCoverageAttributeIndex = -1;
    }
}
