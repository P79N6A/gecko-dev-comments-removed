








#ifndef GrPaint_DEFINED
#define GrPaint_DEFINED

#include "GrColor.h"
#include "GrEffectStage.h"

#include "SkXfermode.h"



























class GrPaint {
public:
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

    


    const GrEffect* addColorEffect(const GrEffect* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        if (!effect->willUseInputColor()) {
            fColorStages.reset();
        }
        SkNEW_APPEND_TO_TARRAY(&fColorStages, GrEffectStage, (effect, attr0, attr1));
        return effect;
    }

    


    const GrEffect* addCoverageEffect(const GrEffect* effect, int attr0 = -1, int attr1 = -1) {
        SkASSERT(NULL != effect);
        if (!effect->willUseInputColor()) {
            fCoverageStages.reset();
        }
        SkNEW_APPEND_TO_TARRAY(&fCoverageStages, GrEffectStage, (effect, attr0, attr1));
        return effect;
    }

    



    void addColorTextureEffect(GrTexture* texture, const SkMatrix& matrix);
    void addCoverageTextureEffect(GrTexture* texture, const SkMatrix& matrix);

    void addColorTextureEffect(GrTexture* texture,
                               const SkMatrix& matrix,
                               const GrTextureParams& params);
    void addCoverageTextureEffect(GrTexture* texture,
                                  const SkMatrix& matrix,
                                  const GrTextureParams& params);

    int numColorStages() const { return fColorStages.count(); }
    int numCoverageStages() const { return fCoverageStages.count(); }
    int numTotalStages() const { return this->numColorStages() + this->numCoverageStages(); }

    const GrEffectStage& getColorStage(int s) const { return fColorStages[s]; }
    const GrEffectStage& getCoverageStage(int s) const { return fCoverageStages[s]; }

    GrPaint& operator=(const GrPaint& paint) {
        fSrcBlendCoeff = paint.fSrcBlendCoeff;
        fDstBlendCoeff = paint.fDstBlendCoeff;
        fAntiAlias = paint.fAntiAlias;
        fDither = paint.fDither;

        fColor = paint.fColor;
        fCoverage = paint.fCoverage;

        fColorStages = paint.fColorStages;
        fCoverageStages = paint.fCoverageStages;

        return *this;
    }

    


    void reset() {
        this->resetBlend();
        this->resetOptions();
        this->resetColor();
        this->resetCoverage();
        this->resetStages();
    }

    






    bool isOpaque() const;

    



    bool isOpaqueAndConstantColor(GrColor* constantColor) const;

private:

    


    bool getOpaqueAndKnownColor(GrColor* solidColor, uint32_t* solidColorKnownComponents) const;

    




    void localCoordChange(const SkMatrix& oldToNew) {
        for (int i = 0; i < fColorStages.count(); ++i) {
            fColorStages[i].localCoordChange(oldToNew);
        }
        for (int i = 0; i < fCoverageStages.count(); ++i) {
            fCoverageStages[i].localCoordChange(oldToNew);
        }
    }

    bool localCoordChangeInverse(const SkMatrix& newToOld) {
        SkMatrix oldToNew;
        bool computed = false;
        for (int i = 0; i < fColorStages.count(); ++i) {
            if (!computed && !newToOld.invert(&oldToNew)) {
                return false;
            } else {
                computed = true;
            }
            fColorStages[i].localCoordChange(oldToNew);
        }
        for (int i = 0; i < fCoverageStages.count(); ++i) {
            if (!computed && !newToOld.invert(&oldToNew)) {
                return false;
            } else {
                computed = true;
            }
            fCoverageStages[i].localCoordChange(oldToNew);
        }
        return true;
    }

    friend class GrContext; 
    friend class GrStencilAndCoverTextContext;  

    SkSTArray<4, GrEffectStage> fColorStages;
    SkSTArray<2, GrEffectStage> fCoverageStages;

    GrBlendCoeff                fSrcBlendCoeff;
    GrBlendCoeff                fDstBlendCoeff;
    bool                        fAntiAlias;
    bool                        fDither;

    GrColor                     fColor;
    uint8_t                     fCoverage;

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
        fColorStages.reset();
        fCoverageStages.reset();
    }
};

#endif
