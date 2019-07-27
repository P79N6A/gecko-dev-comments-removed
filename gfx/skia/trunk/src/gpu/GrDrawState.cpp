






#include "GrDrawState.h"
#include "GrPaint.h"

bool GrDrawState::setIdentityViewMatrix()  {
    if (fColorStages.count() || fCoverageStages.count()) {
        SkMatrix invVM;
        if (!fViewMatrix.invert(&invVM)) {
            
            return false;
        }
        for (int s = 0; s < fColorStages.count(); ++s) {
            fColorStages[s].localCoordChange(invVM);
        }
        for (int s = 0; s < fCoverageStages.count(); ++s) {
            fCoverageStages[s].localCoordChange(invVM);
        }
    }
    fViewMatrix.reset();
    return true;
}

void GrDrawState::setFromPaint(const GrPaint& paint, const SkMatrix& vm, GrRenderTarget* rt) {
    SkASSERT(0 == fBlockEffectRemovalCnt || 0 == this->numTotalStages());

    fColorStages.reset();
    fCoverageStages.reset();

    for (int i = 0; i < paint.numColorStages(); ++i) {
        fColorStages.push_back(paint.getColorStage(i));
    }

    for (int i = 0; i < paint.numCoverageStages(); ++i) {
        fCoverageStages.push_back(paint.getCoverageStage(i));
    }

    this->setRenderTarget(rt);

    fViewMatrix = vm;

    
    fBlendConstant = 0x0;
    fDrawFace = kBoth_DrawFace;
    fStencilSettings.setDisabled();
    this->resetStateFlags();

    
    this->enableState(GrDrawState::kClip_StateBit);

    this->setColor(paint.getColor());
    this->setState(GrDrawState::kDither_StateBit, paint.isDither());
    this->setState(GrDrawState::kHWAntialias_StateBit, paint.isAntiAlias());

    this->setBlendFunc(paint.getSrcBlendCoeff(), paint.getDstBlendCoeff());
    this->setCoverage(paint.getCoverage());
    this->invalidateBlendOptFlags();
}



static size_t vertex_size(const GrVertexAttrib* attribs, int count) {
    
#ifdef SK_DEBUG
    uint32_t overlapCheck = 0;
#endif
    SkASSERT(count <= GrDrawState::kMaxVertexAttribCnt);
    size_t size = 0;
    for (int index = 0; index < count; ++index) {
        size_t attribSize = GrVertexAttribTypeSize(attribs[index].fType);
        size += attribSize;
#ifdef SK_DEBUG
        size_t dwordCount = attribSize >> 2;
        uint32_t mask = (1 << dwordCount)-1;
        size_t offsetShift = attribs[index].fOffset >> 2;
        SkASSERT(!(overlapCheck & (mask << offsetShift)));
        overlapCheck |= (mask << offsetShift);
#endif
    }
    return size;
}

size_t GrDrawState::getVertexSize() const {
    return vertex_size(fVAPtr, fVACount);
}



void GrDrawState::setVertexAttribs(const GrVertexAttrib* attribs, int count) {
    SkASSERT(count <= kMaxVertexAttribCnt);

    fVAPtr = attribs;
    fVACount = count;

    
    memset(fFixedFunctionVertexAttribIndices,
           0xff,
           sizeof(fFixedFunctionVertexAttribIndices));
#ifdef SK_DEBUG
    uint32_t overlapCheck = 0;
#endif
    for (int i = 0; i < count; ++i) {
        if (attribs[i].fBinding < kGrFixedFunctionVertexAttribBindingCnt) {
            
            SkASSERT(-1 == fFixedFunctionVertexAttribIndices[attribs[i].fBinding]);
            SkASSERT(GrFixedFunctionVertexAttribVectorCount(attribs[i].fBinding) ==
                     GrVertexAttribTypeVectorCount(attribs[i].fType));
            fFixedFunctionVertexAttribIndices[attribs[i].fBinding] = i;
        }
#ifdef SK_DEBUG
        size_t dwordCount = GrVertexAttribTypeSize(attribs[i].fType) >> 2;
        uint32_t mask = (1 << dwordCount)-1;
        size_t offsetShift = attribs[i].fOffset >> 2;
        SkASSERT(!(overlapCheck & (mask << offsetShift)));
        overlapCheck |= (mask << offsetShift);
#endif
    }
    this->invalidateBlendOptFlags();
    
    SkASSERT(-1 != fFixedFunctionVertexAttribIndices[kPosition_GrVertexAttribBinding]);
}



void GrDrawState::setDefaultVertexAttribs() {
    static const GrVertexAttrib kPositionAttrib =
        {kVec2f_GrVertexAttribType, 0, kPosition_GrVertexAttribBinding};

    fVAPtr = &kPositionAttrib;
    fVACount = 1;

    
    memset(fFixedFunctionVertexAttribIndices,
           0xff,
           sizeof(fFixedFunctionVertexAttribIndices));
    fFixedFunctionVertexAttribIndices[kPosition_GrVertexAttribBinding] = 0;
    this->invalidateBlendOptFlags();
}



bool GrDrawState::validateVertexAttribs() const {
    
    GrSLType slTypes[kMaxVertexAttribCnt];
    for (int i = 0; i < kMaxVertexAttribCnt; ++i) {
        slTypes[i] = static_cast<GrSLType>(-1);
    }
    int totalStages = fColorStages.count() + fCoverageStages.count();
    for (int s = 0; s < totalStages; ++s) {
        int covIdx = s - fColorStages.count();
        const GrEffectStage& stage = covIdx < 0 ? fColorStages[s] : fCoverageStages[covIdx];
        const GrEffect* effect = stage.getEffect();
        SkASSERT(NULL != effect);
        
        
        
        const int* attributeIndices = stage.getVertexAttribIndices();
        int numAttributes = stage.getVertexAttribIndexCount();
        for (int i = 0; i < numAttributes; ++i) {
            int attribIndex = attributeIndices[i];
            if (attribIndex >= fVACount ||
                kEffect_GrVertexAttribBinding != fVAPtr[attribIndex].fBinding) {
                return false;
            }

            GrSLType effectSLType = effect->vertexAttribType(i);
            GrVertexAttribType attribType = fVAPtr[attribIndex].fType;
            int slVecCount = GrSLTypeVectorCount(effectSLType);
            int attribVecCount = GrVertexAttribTypeVectorCount(attribType);
            if (slVecCount != attribVecCount ||
                (static_cast<GrSLType>(-1) != slTypes[attribIndex] &&
                    slTypes[attribIndex] != effectSLType)) {
                return false;
            }
            slTypes[attribIndex] = effectSLType;
        }
    }

    return true;
}

bool GrDrawState::willEffectReadDstColor() const {
    if (!this->isColorWriteDisabled()) {
        for (int s = 0; s < fColorStages.count(); ++s) {
            if (fColorStages[s].getEffect()->willReadDstColor()) {
                return true;
            }
        }
    }
    for (int s = 0; s < fCoverageStages.count(); ++s) {
        if (fCoverageStages[s].getEffect()->willReadDstColor()) {
            return true;
        }
    }
    return false;
}



bool GrDrawState::srcAlphaWillBeOne() const {
    uint32_t validComponentFlags;
    GrColor color;
    
    if (this->hasColorVertexAttribute()) {
        validComponentFlags = 0;
        color = 0; 
    } else {
        validComponentFlags = kRGBA_GrColorComponentFlags;
        color = this->getColor();
    }

    
    for (int s = 0; s < fColorStages.count(); ++s) {
        const GrEffect* effect = fColorStages[s].getEffect();
        effect->getConstantColorComponents(&color, &validComponentFlags);
    }

    
    if (this->isCoverageDrawing()) {
        GrColor coverageColor = this->getCoverageColor();
        GrColor oldColor = color;
        color = 0;
        for (int c = 0; c < 4; ++c) {
            if (validComponentFlags & (1 << c)) {
                U8CPU a = (oldColor >> (c * 8)) & 0xff;
                U8CPU b = (coverageColor >> (c * 8)) & 0xff;
                color |= (SkMulDiv255Round(a, b) << (c * 8));
            }
        }
        for (int s = 0; s < fCoverageStages.count(); ++s) {
            const GrEffect* effect = fCoverageStages[s].getEffect();
            effect->getConstantColorComponents(&color, &validComponentFlags);
        }
    }
    return (kA_GrColorComponentFlag & validComponentFlags) && 0xff == GrColorUnpackA(color);
}

bool GrDrawState::hasSolidCoverage() const {
    
    if (this->isCoverageDrawing()) {
        return true;
    }

    GrColor coverage;
    uint32_t validComponentFlags;
    
    if (this->hasCoverageVertexAttribute()) {
        validComponentFlags = 0;
    } else {
        coverage = fCoverage;
        validComponentFlags = kRGBA_GrColorComponentFlags;
    }

    
    for (int s = 0; s < fCoverageStages.count(); ++s) {
        const GrEffect* effect = fCoverageStages[s].getEffect();
        effect->getConstantColorComponents(&coverage, &validComponentFlags);
    }
    return (kRGBA_GrColorComponentFlags == validComponentFlags) && (0xffffffff == coverage);
}





bool GrDrawState::canTweakAlphaForCoverage() const {
    










    return kOne_GrBlendCoeff == fDstBlend ||
           kISA_GrBlendCoeff == fDstBlend ||
           kISC_GrBlendCoeff == fDstBlend ||
           this->isCoverageDrawing();
}

GrDrawState::BlendOptFlags GrDrawState::getBlendOpts(bool forceCoverage,
                                                     GrBlendCoeff* srcCoeff,
                                                     GrBlendCoeff* dstCoeff) const {
    GrBlendCoeff bogusSrcCoeff, bogusDstCoeff;
    if (NULL == srcCoeff) {
        srcCoeff = &bogusSrcCoeff;
    }
    if (NULL == dstCoeff) {
        dstCoeff = &bogusDstCoeff;
    }

    if (forceCoverage) {
        return this->calcBlendOpts(true, srcCoeff, dstCoeff);
    }

    if (0 == (fBlendOptFlags & kInvalid_BlendOptFlag)) {
        *srcCoeff = fOptSrcBlend;
        *dstCoeff = fOptDstBlend;
        return fBlendOptFlags;
    }

    fBlendOptFlags = this->calcBlendOpts(forceCoverage, srcCoeff, dstCoeff);
    fOptSrcBlend = *srcCoeff;
    fOptDstBlend = *dstCoeff;

    return fBlendOptFlags;
}

GrDrawState::BlendOptFlags GrDrawState::calcBlendOpts(bool forceCoverage,
                                                      GrBlendCoeff* srcCoeff,
                                                      GrBlendCoeff* dstCoeff) const {
    *srcCoeff = this->getSrcBlendCoeff();
    *dstCoeff = this->getDstBlendCoeff();

    if (this->isColorWriteDisabled()) {
        *srcCoeff = kZero_GrBlendCoeff;
        *dstCoeff = kOne_GrBlendCoeff;
    }

    bool srcAIsOne = this->srcAlphaWillBeOne();
    bool dstCoeffIsOne = kOne_GrBlendCoeff == *dstCoeff ||
                         (kSA_GrBlendCoeff == *dstCoeff && srcAIsOne);
    bool dstCoeffIsZero = kZero_GrBlendCoeff == *dstCoeff ||
                         (kISA_GrBlendCoeff == *dstCoeff && srcAIsOne);

    bool covIsZero = !this->isCoverageDrawing() &&
                     !this->hasCoverageVertexAttribute() &&
                     0 == this->getCoverageColor();
    
    
    
    if ((kZero_GrBlendCoeff == *srcCoeff && dstCoeffIsOne) || covIsZero) {
        if (this->getStencil().doesWrite()) {
            return kDisableBlend_BlendOptFlag |
                   kEmitCoverage_BlendOptFlag;
        } else {
            return kSkipDraw_BlendOptFlag;
        }
    }

    
    bool hasCoverage = forceCoverage ||
                       0xffffffff != this->getCoverageColor() ||
                       this->hasCoverageVertexAttribute() ||
                       fCoverageStages.count() > 0;

    
    
    if (!hasCoverage) {
        if (dstCoeffIsZero) {
            if (kOne_GrBlendCoeff == *srcCoeff) {
                
                
                return kDisableBlend_BlendOptFlag;
            } else if (kZero_GrBlendCoeff == *srcCoeff) {
                
                
                *srcCoeff = kOne_GrBlendCoeff;
                *dstCoeff = kZero_GrBlendCoeff;
                return kDisableBlend_BlendOptFlag | kEmitTransBlack_BlendOptFlag;
            }
        }
    } else if (this->isCoverageDrawing()) {
        
        return kCoverageAsAlpha_BlendOptFlag;
    } else {
        
        
        if (this->canTweakAlphaForCoverage()) {
            return kCoverageAsAlpha_BlendOptFlag;
        }
        if (dstCoeffIsZero) {
            if (kZero_GrBlendCoeff == *srcCoeff) {
                
                
                
                *dstCoeff = kISA_GrBlendCoeff;
                return  kEmitCoverage_BlendOptFlag;
            } else if (srcAIsOne) {
                
                
                
                
                *dstCoeff = kISA_GrBlendCoeff;
                return  kCoverageAsAlpha_BlendOptFlag;
            }
        } else if (dstCoeffIsOne) {
            
            
            *dstCoeff = kOne_GrBlendCoeff;
            return  kCoverageAsAlpha_BlendOptFlag;
        }
    }
    if (kOne_GrBlendCoeff == *srcCoeff &&
        kZero_GrBlendCoeff == *dstCoeff &&
        this->willEffectReadDstColor()) {
        
        
        return kDisableBlend_BlendOptFlag;
    }
    return kNone_BlendOpt;
}

bool GrDrawState::canIgnoreColorAttribute() const {
    if (fBlendOptFlags & kInvalid_BlendOptFlag) {
        this->getBlendOpts();
    }
    return SkToBool(fBlendOptFlags & (GrDrawState::kEmitTransBlack_BlendOptFlag |
                                      GrDrawState::kEmitCoverage_BlendOptFlag));
}




void GrDrawState::AutoViewMatrixRestore::restore() {
    if (NULL != fDrawState) {
        SkDEBUGCODE(--fDrawState->fBlockEffectRemovalCnt;)
        fDrawState->fViewMatrix = fViewMatrix;
        SkASSERT(fDrawState->numColorStages() >= fNumColorStages);
        int numCoverageStages = fSavedCoordChanges.count() - fNumColorStages;
        SkASSERT(fDrawState->numCoverageStages() >= numCoverageStages);

        int i = 0;
        for (int s = 0; s < fNumColorStages; ++s, ++i) {
            fDrawState->fColorStages[s].restoreCoordChange(fSavedCoordChanges[i]);
        }
        for (int s = 0; s < numCoverageStages; ++s, ++i) {
            fDrawState->fCoverageStages[s].restoreCoordChange(fSavedCoordChanges[i]);
        }
        fDrawState = NULL;
    }
}

void GrDrawState::AutoViewMatrixRestore::set(GrDrawState* drawState,
                                             const SkMatrix& preconcatMatrix) {
    this->restore();

    SkASSERT(NULL == fDrawState);
    if (NULL == drawState || preconcatMatrix.isIdentity()) {
        return;
    }
    fDrawState = drawState;

    fViewMatrix = drawState->getViewMatrix();
    drawState->fViewMatrix.preConcat(preconcatMatrix);

    this->doEffectCoordChanges(preconcatMatrix);
    SkDEBUGCODE(++fDrawState->fBlockEffectRemovalCnt;)
}

bool GrDrawState::AutoViewMatrixRestore::setIdentity(GrDrawState* drawState) {
    this->restore();

    if (NULL == drawState) {
        return false;
    }

    if (drawState->getViewMatrix().isIdentity()) {
        return true;
    }

    fViewMatrix = drawState->getViewMatrix();
    if (0 == drawState->numTotalStages()) {
        drawState->fViewMatrix.reset();
        fDrawState = drawState;
        fNumColorStages = 0;
        fSavedCoordChanges.reset(0);
        SkDEBUGCODE(++fDrawState->fBlockEffectRemovalCnt;)
        return true;
    } else {
        SkMatrix inv;
        if (!fViewMatrix.invert(&inv)) {
            return false;
        }
        drawState->fViewMatrix.reset();
        fDrawState = drawState;
        this->doEffectCoordChanges(inv);
        SkDEBUGCODE(++fDrawState->fBlockEffectRemovalCnt;)
        return true;
    }
}

void GrDrawState::AutoViewMatrixRestore::doEffectCoordChanges(const SkMatrix& coordChangeMatrix) {
    fSavedCoordChanges.reset(fDrawState->numTotalStages());
    int i = 0;

    fNumColorStages = fDrawState->numColorStages();
    for (int s = 0; s < fNumColorStages; ++s, ++i) {
        fDrawState->fColorStages[s].saveCoordChange(&fSavedCoordChanges[i]);
        fDrawState->fColorStages[s].localCoordChange(coordChangeMatrix);
    }

    int numCoverageStages = fDrawState->numCoverageStages();
    for (int s = 0; s < numCoverageStages; ++s, ++i) {
        fDrawState->fCoverageStages[s].saveCoordChange(&fSavedCoordChanges[i]);
        fDrawState->fCoverageStages[s].localCoordChange(coordChangeMatrix);
    }
}
