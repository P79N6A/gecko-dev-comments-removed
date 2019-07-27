






#ifndef GrDistanceFieldTextureEffect_DEFINED
#define GrDistanceFieldTextureEffect_DEFINED

#include "GrEffect.h"
#include "GrVertexEffect.h"

class GrGLDistanceFieldTextureEffect;
class GrGLDistanceFieldLCDTextureEffect;







class GrDistanceFieldTextureEffect : public GrVertexEffect {
public:
#ifdef SK_GAMMA_APPLY_TO_A8
    static GrEffect* Create(GrTexture* tex, const GrTextureParams& params,
                            GrTexture* gamma, const GrTextureParams& gammaParams, float lum,
                            bool similarity) {
       return SkNEW_ARGS(GrDistanceFieldTextureEffect, (tex, params, gamma, gammaParams, lum,
                                                        similarity));
    }
#else
    static GrEffect* Create(GrTexture* tex, const GrTextureParams& params,
                            bool similarity) {
        return  SkNEW_ARGS(GrDistanceFieldTextureEffect, (tex, params, similarity));
    }
#endif

    virtual ~GrDistanceFieldTextureEffect() {}

    static const char* Name() { return "DistanceFieldTexture"; }

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;
#ifdef SK_GAMMA_APPLY_TO_A8
    float getLuminance() const { return fLuminance; }
#endif
    bool isSimilarity() const { return fIsSimilarity; }

    typedef GrGLDistanceFieldTextureEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrDistanceFieldTextureEffect(GrTexture* texture, const GrTextureParams& params,
#ifdef SK_GAMMA_APPLY_TO_A8
                                 GrTexture* gamma, const GrTextureParams& gammaParams, float lum,
#endif
                                 bool uniformScale);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrTextureAccess fTextureAccess;
#ifdef SK_GAMMA_APPLY_TO_A8
    GrTextureAccess fGammaTextureAccess;
    float           fLuminance;
#endif
    bool            fIsSimilarity;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};







class GrDistanceFieldLCDTextureEffect : public GrVertexEffect {
public:
    static GrEffect* Create(GrTexture* tex, const GrTextureParams& params,
                            GrTexture* gamma, const GrTextureParams& gammaParams, 
                            SkColor textColor, bool uniformScale, bool useBGR) {
        return SkNEW_ARGS(GrDistanceFieldLCDTextureEffect,
                          (tex, params, gamma, gammaParams, textColor, uniformScale, useBGR));
    }

    virtual ~GrDistanceFieldLCDTextureEffect() {}

    static const char* Name() { return "DistanceFieldLCDTexture"; }

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;
    GrColor getTextColor() const { return fTextColor; }
    bool isUniformScale() const { return fUniformScale; }
    bool useBGR() const { return fUseBGR; }

    typedef GrGLDistanceFieldLCDTextureEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrDistanceFieldLCDTextureEffect(GrTexture* texture, const GrTextureParams& params,
                                    GrTexture* gamma, const GrTextureParams& gammaParams,
                                    SkColor textColor,
                                    bool uniformScale, bool useBGR);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrTextureAccess fTextureAccess;
    GrTextureAccess fGammaTextureAccess;
    GrColor         fTextColor;
    bool            fUniformScale;
    bool            fUseBGR;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};

#endif
