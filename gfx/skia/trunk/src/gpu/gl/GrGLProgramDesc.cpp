






#include "GrGLProgramDesc.h"
#include "GrBackendEffectFactory.h"
#include "GrDrawEffect.h"
#include "GrEffect.h"
#include "GrGLShaderBuilder.h"
#include "GrGpuGL.h"

#include "SkChecksum.h"

namespace {
inline GrGLEffect::EffectKey get_key_and_update_stats(const GrEffectStage& stage,
                                                      const GrGLCaps& caps,
                                                      bool useExplicitLocalCoords,
                                                      bool* setTrueIfReadsDst,
                                                      bool* setTrueIfReadsPos,
                                                      bool* setTrueIfHasVertexCode) {
    const GrEffectRef& effect = *stage.getEffect();
    const GrBackendEffectFactory& factory = effect->getFactory();
    GrDrawEffect drawEffect(stage, useExplicitLocalCoords);
    if (effect->willReadDstColor()) {
        *setTrueIfReadsDst = true;
    }
    if (effect->willReadFragmentPosition()) {
        *setTrueIfReadsPos = true;
    }
    if (effect->hasVertexCode()) {
        *setTrueIfHasVertexCode = true;
    }
    return factory.glEffectKey(drawEffect, caps);
}
}
void GrGLProgramDesc::Build(const GrDrawState& drawState,
                            bool isPoints,
                            GrDrawState::BlendOptFlags blendOpts,
                            GrBlendCoeff srcCoeff,
                            GrBlendCoeff dstCoeff,
                            const GrGpuGL* gpu,
                            const GrDeviceCoordTexture* dstCopy,
                            SkTArray<const GrEffectStage*, true>* colorStages,
                            SkTArray<const GrEffectStage*, true>* coverageStages,
                            GrGLProgramDesc* desc) {
    colorStages->reset();
    coverageStages->reset();

    
    SkASSERT(!(GrDrawState::kSkipDraw_BlendOptFlag & blendOpts));

    bool skipCoverage = SkToBool(blendOpts & GrDrawState::kEmitTransBlack_BlendOptFlag);

    bool skipColor = SkToBool(blendOpts & (GrDrawState::kEmitTransBlack_BlendOptFlag |
                                           GrDrawState::kEmitCoverage_BlendOptFlag));
    int firstEffectiveColorStage = 0;
    bool inputColorIsUsed = true;
    if (!skipColor) {
        firstEffectiveColorStage = drawState.numColorStages();
        while (firstEffectiveColorStage > 0 && inputColorIsUsed) {
            --firstEffectiveColorStage;
            const GrEffect* effect = drawState.getColorStage(firstEffectiveColorStage).getEffect()->get();
            inputColorIsUsed = effect->willUseInputColor();
        }
    }

    int firstEffectiveCoverageStage = 0;
    bool inputCoverageIsUsed = true;
    if (!skipCoverage) {
        firstEffectiveCoverageStage = drawState.numCoverageStages();
        while (firstEffectiveCoverageStage > 0 && inputCoverageIsUsed) {
            --firstEffectiveCoverageStage;
            const GrEffect* effect = drawState.getCoverageStage(firstEffectiveCoverageStage).getEffect()->get();
            inputCoverageIsUsed = effect->willUseInputColor();
        }
    }

    
    
    
    

    bool requiresColorAttrib = !skipColor && drawState.hasColorVertexAttribute();
    bool requiresCoverageAttrib = !skipCoverage && drawState.hasCoverageVertexAttribute();
    
    bool requiresLocalCoordAttrib = !(skipCoverage  && skipColor) &&
                                    drawState.hasLocalCoordAttribute();

    bool colorIsTransBlack = SkToBool(blendOpts & GrDrawState::kEmitTransBlack_BlendOptFlag);
    bool colorIsSolidWhite = (blendOpts & GrDrawState::kEmitCoverage_BlendOptFlag) ||
                             (!requiresColorAttrib && 0xffffffff == drawState.getColor()) ||
                             (!inputColorIsUsed);

    int numEffects = (skipColor ? 0 : (drawState.numColorStages() - firstEffectiveColorStage)) +
                     (skipCoverage ? 0 : (drawState.numCoverageStages() - firstEffectiveCoverageStage));

    size_t newKeyLength = KeyLength(numEffects);
    bool allocChanged;
    desc->fKey.reset(newKeyLength, SkAutoMalloc::kAlloc_OnShrink, &allocChanged);
    if (allocChanged || !desc->fInitialized) {
        
        memset(desc->header(), 0, kHeaderSize);
    }
    
    *desc->atOffset<uint32_t, kLengthOffset>() = SkToU32(newKeyLength);

    KeyHeader* header = desc->header();
    EffectKey* effectKeys = desc->effectKeys();

    int currEffectKey = 0;
    bool readsDst = false;
    bool readFragPosition = false;
    bool hasVertexCode = false;
    if (!skipColor) {
        for (int s = firstEffectiveColorStage; s < drawState.numColorStages(); ++s) {
            effectKeys[currEffectKey++] =
                get_key_and_update_stats(drawState.getColorStage(s), gpu->glCaps(),
                                         requiresLocalCoordAttrib, &readsDst, &readFragPosition,
                                         &hasVertexCode);
        }
    }
    if (!skipCoverage) {
        for (int s = firstEffectiveCoverageStage; s < drawState.numCoverageStages(); ++s) {
            effectKeys[currEffectKey++] =
                get_key_and_update_stats(drawState.getCoverageStage(s), gpu->glCaps(),
                                         requiresLocalCoordAttrib, &readsDst, &readFragPosition,
                                         &hasVertexCode);
        }
    }

    header->fHasVertexCode = hasVertexCode || requiresLocalCoordAttrib;
    header->fEmitsPointSize = isPoints;

    
    
#if GR_GL_EXPERIMENTAL_GS
#if 0
    header->fExperimentalGS = gpu->caps().geometryShaderSupport();
#else
    header->fExperimentalGS = false;
#endif
#endif
    bool defaultToUniformInputs = GR_GL_NO_CONSTANT_ATTRIBUTES || gpu->caps()->pathRenderingSupport();

    if (colorIsTransBlack) {
        header->fColorInput = kTransBlack_ColorInput;
    } else if (colorIsSolidWhite) {
        header->fColorInput = kSolidWhite_ColorInput;
    } else if (defaultToUniformInputs && !requiresColorAttrib) {
        header->fColorInput = kUniform_ColorInput;
    } else {
        header->fColorInput = kAttribute_ColorInput;
        header->fHasVertexCode = true;
    }

    bool covIsSolidWhite = !requiresCoverageAttrib && 0xffffffff == drawState.getCoverageColor();

    if (skipCoverage) {
        header->fCoverageInput = kTransBlack_ColorInput;
    } else if (covIsSolidWhite || !inputCoverageIsUsed) {
        header->fCoverageInput = kSolidWhite_ColorInput;
    } else if (defaultToUniformInputs && !requiresCoverageAttrib) {
        header->fCoverageInput = kUniform_ColorInput;
    } else {
        header->fCoverageInput = kAttribute_ColorInput;
        header->fHasVertexCode = true;
    }

    if (readsDst) {
        SkASSERT(NULL != dstCopy || gpu->caps()->dstReadInShaderSupport());
        const GrTexture* dstCopyTexture = NULL;
        if (NULL != dstCopy) {
            dstCopyTexture = dstCopy->texture();
        }
        header->fDstReadKey = GrGLShaderBuilder::KeyForDstRead(dstCopyTexture, gpu->glCaps());
        SkASSERT(0 != header->fDstReadKey);
    } else {
        header->fDstReadKey = 0;
    }

    if (readFragPosition) {
        header->fFragPosKey = GrGLShaderBuilder::KeyForFragmentPosition(drawState.getRenderTarget(),
                                                                      gpu->glCaps());
    } else {
        header->fFragPosKey = 0;
    }

    
    header->fPositionAttributeIndex = drawState.positionAttributeIndex();
    header->fLocalCoordAttributeIndex = drawState.localCoordAttributeIndex();

    
    int availableAttributeIndex = drawState.getVertexAttribCount();
    if (requiresColorAttrib) {
        header->fColorAttributeIndex = drawState.colorVertexAttributeIndex();
    } else if (GrGLProgramDesc::kAttribute_ColorInput == header->fColorInput) {
        SkASSERT(availableAttributeIndex < GrDrawState::kMaxVertexAttribCnt);
        header->fColorAttributeIndex = availableAttributeIndex;
        availableAttributeIndex++;
    } else {
        header->fColorAttributeIndex = -1;
    }

    if (requiresCoverageAttrib) {
        header->fCoverageAttributeIndex = drawState.coverageVertexAttributeIndex();
    } else if (GrGLProgramDesc::kAttribute_ColorInput == header->fCoverageInput) {
        SkASSERT(availableAttributeIndex < GrDrawState::kMaxVertexAttribCnt);
        header->fCoverageAttributeIndex = availableAttributeIndex;
    } else {
        header->fCoverageAttributeIndex = -1;
    }

    

    
    header->fCoverageOutput = kModulate_CoverageOutput;

    
    bool separateCoverageFromColor = false;
    if (!drawState.isCoverageDrawing() && !skipCoverage &&
        (drawState.numCoverageStages() > 0 || requiresCoverageAttrib)) {

        if (gpu->caps()->dualSourceBlendingSupport() &&
            !(blendOpts & (GrDrawState::kEmitCoverage_BlendOptFlag |
                           GrDrawState::kCoverageAsAlpha_BlendOptFlag))) {
            if (kZero_GrBlendCoeff == dstCoeff) {
                
                header->fCoverageOutput =  kSecondaryCoverage_CoverageOutput;
                separateCoverageFromColor = true;
            } else if (kSA_GrBlendCoeff == dstCoeff) {
                
                header->fCoverageOutput = kSecondaryCoverageISA_CoverageOutput;
                separateCoverageFromColor = true;
            } else if (kSC_GrBlendCoeff == dstCoeff) {
                
                header->fCoverageOutput = kSecondaryCoverageISC_CoverageOutput;
                separateCoverageFromColor = true;
            }
        } else if (readsDst &&
                   kOne_GrBlendCoeff == srcCoeff &&
                   kZero_GrBlendCoeff == dstCoeff) {
            header->fCoverageOutput = kCombineWithDst_CoverageOutput;
            separateCoverageFromColor = true;
        }
    }
    if (!skipColor) {
        for (int s = firstEffectiveColorStage; s < drawState.numColorStages(); ++s) {
            colorStages->push_back(&drawState.getColorStage(s));
        }
    }
    if (!skipCoverage) {
        SkTArray<const GrEffectStage*, true>* array;
        if (separateCoverageFromColor) {
            array = coverageStages;
        } else {
            array = colorStages;
        }
        for (int s = firstEffectiveCoverageStage; s < drawState.numCoverageStages(); ++s) {
            array->push_back(&drawState.getCoverageStage(s));
        }
    }
    header->fColorEffectCnt = colorStages->count();
    header->fCoverageEffectCnt = coverageStages->count();

    *desc->checksum() = 0;
    *desc->checksum() = SkChecksum::Compute(reinterpret_cast<uint32_t*>(desc->fKey.get()),
                                            newKeyLength);
    desc->fInitialized = true;
}

GrGLProgramDesc& GrGLProgramDesc::operator= (const GrGLProgramDesc& other) {
    fInitialized = other.fInitialized;
    if (fInitialized) {
        size_t keyLength = other.keyLength();
        fKey.reset(keyLength);
        memcpy(fKey.get(), other.fKey.get(), keyLength);
    }
    return *this;
}
