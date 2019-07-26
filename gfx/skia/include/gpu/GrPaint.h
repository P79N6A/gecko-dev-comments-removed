








#ifndef GrPaint_DEFINED
#define GrPaint_DEFINED

#include "GrTexture.h"
#include "GrColor.h"
#include "GrSamplerState.h"

#include "SkXfermode.h"






class GrPaint {
public:
    enum {
        kMaxTextures = 2,
        kMaxMasks    = 1,
    };

    
    GrBlendCoeff                fSrcBlendCoeff;
    GrBlendCoeff                fDstBlendCoeff;
    bool                        fAntiAlias;
    bool                        fDither;
    bool                        fColorMatrixEnabled;

    GrColor                     fColor;
    uint8_t                     fCoverage;

    GrColor                     fColorFilterColor;
    SkXfermode::Mode            fColorFilterXfermode;
    float                       fColorMatrix[20];

    GrSamplerState* textureSampler(int i) {
        GrAssert((unsigned)i < kMaxTextures);
        return fTextureSamplers + i;
    }

    const GrSamplerState& getTextureSampler(int i) const {
        GrAssert((unsigned)i < kMaxTextures);
        return fTextureSamplers[i];
    }

    bool isTextureStageEnabled(int i) const {
        GrAssert((unsigned)i < kMaxTextures);
        return (NULL != fTextureSamplers[i].getCustomStage());
    }


    
    
    GrSamplerState* maskSampler(int i) {
        GrAssert((unsigned)i < kMaxMasks);
        return fMaskSamplers + i;
    }

    const GrSamplerState& getMaskSampler(int i) const {
        GrAssert((unsigned)i < kMaxMasks);
        return fMaskSamplers[i];
    }

    bool isMaskStageEnabled(int i) const {
        GrAssert((unsigned)i < kMaxTextures);
        return (NULL != fMaskSamplers[i].getCustomStage());
    }

    bool hasMask() const {
        for (int i = 0; i < kMaxMasks; ++i) {
            if (this->isMaskStageEnabled(i)) {
                return true;
            }
        }
        return false;
    }

    bool hasTexture() const {
        for (int i = 0; i < kMaxTextures; ++i) {
            if (this->isTextureStageEnabled(i)) {
                return true;
            }
        }
        return false;
    }

    bool hasTextureOrMask() const { return this->hasTexture() || this->hasMask(); }

    




    bool preConcatSamplerMatricesWithInverse(const GrMatrix& matrix) {
        GrMatrix inv;
        bool computed = false;
        for (int i = 0; i < kMaxTextures; ++i) {
            if (this->isTextureStageEnabled(i)) {
                if (!computed && !matrix.invert(&inv)) {
                    return false;
                } else {
                    computed = true;
                }
                fTextureSamplers[i].preConcatMatrix(inv);
            }
        }
        for (int i = 0; i < kMaxMasks; ++i) {
            if (this->isMaskStageEnabled(i)) {
                if (!computed && !matrix.invert(&inv)) {
                    return false;
                } else {
                    computed = true;
                }
                fMaskSamplers[i].preConcatMatrix(inv);
            }
        }
        return true;
    }

    
    GrPaint() {
    }

    GrPaint(const GrPaint& paint) {
        *this = paint;
    }

    ~GrPaint() {}

    GrPaint& operator=(const GrPaint& paint) {
        fSrcBlendCoeff = paint.fSrcBlendCoeff;
        fDstBlendCoeff = paint.fDstBlendCoeff;
        fAntiAlias = paint.fAntiAlias;
        fDither = paint.fDither;

        fColor = paint.fColor;
        fCoverage = paint.fCoverage;

        fColorFilterColor = paint.fColorFilterColor;
        fColorFilterXfermode = paint.fColorFilterXfermode;
        fColorMatrixEnabled = paint.fColorMatrixEnabled;
        if (fColorMatrixEnabled) {
            memcpy(fColorMatrix, paint.fColorMatrix, sizeof(fColorMatrix));
        }

        for (int i = 0; i < kMaxTextures; ++i) {
            if (paint.isTextureStageEnabled(i)) {
                fTextureSamplers[i] = paint.fTextureSamplers[i];
            }
        }
        for (int i = 0; i < kMaxMasks; ++i) {
            if (paint.isMaskStageEnabled(i)) {
                fMaskSamplers[i] = paint.fMaskSamplers[i];
            }
        }
        return *this;
    }

    
    void reset() {
        this->resetBlend();
        this->resetOptions();
        this->resetColor();
        this->resetCoverage();
        this->resetTextures();
        this->resetColorFilter();
        this->resetMasks();
    }

    void resetColorFilter() {
        fColorFilterXfermode = SkXfermode::kDst_Mode;
        fColorFilterColor = GrColorPackRGBA(0xff, 0xff, 0xff, 0xff);
        fColorMatrixEnabled = false;
    }

    
    
    
    enum {
        kFirstTextureStage = 0,
        kFirstMaskStage = kMaxTextures,
        kTotalStages = kMaxTextures + kMaxMasks,
    };

private:

    GrSamplerState              fTextureSamplers[kMaxTextures];
    GrSamplerState              fMaskSamplers[kMaxMasks];

    void resetBlend() {
        fSrcBlendCoeff = kOne_GrBlendCoeff;
        fDstBlendCoeff = kZero_GrBlendCoeff;
    }

    void resetOptions() {
        fAntiAlias = false;
        fDither = false;
    }

    void resetColor() {
        fColor = GrColorPackRGBA(0xff, 0xff, 0xff, 0xff);
    }

    void resetCoverage() {
        fCoverage = 0xff;
    }

    void resetTextures() {
        for (int i = 0; i < kMaxTextures; ++i) {
            fTextureSamplers[i].reset();
        }
    }

    void resetMasks() {
        for (int i = 0; i < kMaxMasks; ++i) {
            fMaskSamplers[i].reset();
        }
    }
};

#endif
