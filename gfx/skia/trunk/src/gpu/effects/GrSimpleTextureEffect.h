






#ifndef GrSimpleTextureEffect_DEFINED
#define GrSimpleTextureEffect_DEFINED

#include "GrSingleTextureEffect.h"

class GrGLSimpleTextureEffect;









class GrSimpleTextureEffect : public GrSingleTextureEffect {
public:
    
    static GrEffect* Create(GrTexture* tex,
                            const SkMatrix& matrix,
                            GrCoordSet coordSet = kLocal_GrCoordSet) {
        return SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, GrTextureParams::kNone_FilterMode,
                                                  coordSet));
    }

    
    static GrEffect* Create(GrTexture* tex,
                            const SkMatrix& matrix,
                            GrTextureParams::FilterMode filterMode,
                            GrCoordSet coordSet = kLocal_GrCoordSet) {
        return SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, filterMode, coordSet));
    }

    static GrEffect* Create(GrTexture* tex,
                            const SkMatrix& matrix,
                            const GrTextureParams& p,
                            GrCoordSet coordSet = kLocal_GrCoordSet) {
        return SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, p, coordSet));
    }

    virtual ~GrSimpleTextureEffect() {}

    static const char* Name() { return "Texture"; }

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    typedef GrGLSimpleTextureEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrSimpleTextureEffect(GrTexture* texture,
                          const SkMatrix& matrix,
                          GrTextureParams::FilterMode filterMode,
                          GrCoordSet coordSet)
        : GrSingleTextureEffect(texture, matrix, filterMode, coordSet) {
    }

    GrSimpleTextureEffect(GrTexture* texture,
                          const SkMatrix& matrix,
                          const GrTextureParams& params,
                          GrCoordSet coordSet)
        : GrSingleTextureEffect(texture, matrix, params, coordSet) {
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        const GrSimpleTextureEffect& ste = CastEffect<GrSimpleTextureEffect>(other);
        return this->hasSameTextureParamsMatrixAndSourceCoords(ste);
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
