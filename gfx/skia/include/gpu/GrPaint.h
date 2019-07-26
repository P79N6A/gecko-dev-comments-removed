








#ifndef GrPaint_DEFINED
#define GrPaint_DEFINED

#include "GrColor.h"
#include "GrEffectStage.h"

#include "SkXfermode.h"



























class GrPaint {
public:
    enum {
        kMaxColorStages     = 3,
        kMaxCoverageStages  = 1,
    };

    GrPaint() { this->reset(); }

    GrPaint(const GrPaint& paint) { *this = paint; }

    ~GrPaint() {}

    



    void setBlendFunc(GrBlendCoeff srcCoeff, GrBlendCoeff dstCoeff) {
        fSrcBlendCoeff = srcCoeff;
        fDstBlendCoeff = dstCoeff;
    }
    GrBlendCoeff getSrcBlendCoeff() const { return fSrcBlendCoeff; }
    GrBlendCoeff getDstBlendCoeff() const { return fDstBlendCoeff; }

    


    void setColor(GrColor color) { fColor = color; }
    GrColor getColor() const { return fColor; }

    


    void setCoverage(uint8_t coverage) { fCoverage = coverage; }
    uint8_t getCoverage() const { return fCoverage; }

    


    void setAntiAlias(bool aa) { fAntiAlias = aa; }
    bool isAntiAlias() const { return fAntiAlias; }

    


    void setDither(bool dither) { fDither = dither; }
    bool isDither() const { return fDither; }

    





    void setXfermodeColorFilter(SkXfermode::Mode mode, GrColor color) {
        fColorFilterColor = color;
        fColorFilterXfermode = mode;
    }
    SkXfermode::Mode getColorFilterMode() const { return fColorFilterXfermode; }
    GrColor getColorFilterColor() const { return fColorFilterColor; }

    


    void resetColorFilter() {
        fColorFilterXfermode = SkXfermode::kDst_Mode;
        fColorFilterColor = GrColorPackRGBA(0xff, 0xff, 0xff, 0xff);
    }

    




    GrEffectStage* colorStage(int i) {
        GrAssert((unsigned)i < kMaxColorStages);
        return fColorStages + i;
    }

    const GrEffectStage& getColorStage(int i) const {
        GrAssert((unsigned)i < kMaxColorStages);
        return fColorStages[i];
    }

    bool isColorStageEnabled(int i) const {
        GrAssert((unsigned)i < kMaxColorStages);
        return (NULL != fColorStages[i].getEffect());
    }

    



    GrEffectStage* coverageStage(int i) {
        GrAssert((unsigned)i < kMaxCoverageStages);
        return fCoverageStages + i;
    }

    const GrEffectStage& getCoverageStage(int i) const {
        GrAssert((unsigned)i < kMaxCoverageStages);
        return fCoverageStages[i];
    }

    bool isCoverageStageEnabled(int i) const {
        GrAssert((unsigned)i < kMaxCoverageStages);
        return (NULL != fCoverageStages[i].getEffect());
    }

    bool hasCoverageStage() const {
        for (int i = 0; i < kMaxCoverageStages; ++i) {
            if (this->isCoverageStageEnabled(i)) {
                return true;
            }
        }
        return false;
    }

    bool hasColorStage() const {
        for (int i = 0; i < kMaxColorStages; ++i) {
            if (this->isColorStageEnabled(i)) {
                return true;
            }
        }
        return false;
    }

    bool hasStage() const { return this->hasColorStage() || this->hasCoverageStage(); }

    GrPaint& operator=(const GrPaint& paint) {
        fSrcBlendCoeff = paint.fSrcBlendCoeff;
        fDstBlendCoeff = paint.fDstBlendCoeff;
        fAntiAlias = paint.fAntiAlias;
        fDither = paint.fDither;

        fColor = paint.fColor;
        fCoverage = paint.fCoverage;

        fColorFilterColor = paint.fColorFilterColor;
        fColorFilterXfermode = paint.fColorFilterXfermode;

        for (int i = 0; i < kMaxColorStages; ++i) {
            if (paint.isColorStageEnabled(i)) {
                fColorStages[i] = paint.fColorStages[i];
            }
        }
        for (int i = 0; i < kMaxCoverageStages; ++i) {
            if (paint.isCoverageStageEnabled(i)) {
                fCoverageStages[i] = paint.fCoverageStages[i];
            }
        }
        return *this;
    }

    


    void reset() {
        this->resetBlend();
        this->resetOptions();
        this->resetColor();
        this->resetCoverage();
        this->resetStages();
        this->resetColorFilter();
    }

    
    
    
    enum {
        kFirstColorStage = 0,
        kFirstCoverageStage = kMaxColorStages,
        kTotalStages = kFirstColorStage + kMaxColorStages + kMaxCoverageStages,
    };

private:
    




    void localCoordChange(const SkMatrix& oldToNew) {
        for (int i = 0; i < kMaxColorStages; ++i) {
            if (this->isColorStageEnabled(i)) {
                fColorStages[i].localCoordChange(oldToNew);
            }
        }
        for (int i = 0; i < kMaxCoverageStages; ++i) {
            if (this->isCoverageStageEnabled(i)) {
                fCoverageStages[i].localCoordChange(oldToNew);
            }
        }
    }

    bool localCoordChangeInverse(const SkMatrix& newToOld) {
        SkMatrix oldToNew;
        bool computed = false;
        for (int i = 0; i < kMaxColorStages; ++i) {
            if (this->isColorStageEnabled(i)) {
                if (!computed && !newToOld.invert(&oldToNew)) {
                    return false;
                } else {
                    computed = true;
                }
                fColorStages[i].localCoordChange(oldToNew);
            }
        }
        for (int i = 0; i < kMaxCoverageStages; ++i) {
            if (this->isCoverageStageEnabled(i)) {
                if (!computed && !newToOld.invert(&oldToNew)) {
                    return false;
                } else {
                    computed = true;
                }
                fCoverageStages[i].localCoordChange(oldToNew);
            }
        }
        return true;
    }

    friend class GrContext; 

    GrEffectStage               fColorStages[kMaxColorStages];
    GrEffectStage               fCoverageStages[kMaxCoverageStages];

    GrBlendCoeff                fSrcBlendCoeff;
    GrBlendCoeff                fDstBlendCoeff;
    bool                        fAntiAlias;
    bool                        fDither;

    GrColor                     fColor;
    uint8_t                     fCoverage;

    GrColor                     fColorFilterColor;
    SkXfermode::Mode            fColorFilterXfermode;

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

    void resetStages() {
        for (int i = 0; i < kMaxColorStages; ++i) {
            fColorStages[i].reset();
        }
        for (int i = 0; i < kMaxCoverageStages; ++i) {
            fCoverageStages[i].reset();
        }
    }
};

#endif
