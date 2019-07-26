






#ifndef GrSimpleTextureEffect_DEFINED
#define GrSimpleTextureEffect_DEFINED

#include "GrSingleTextureEffect.h"

class GrGLSimpleTextureEffect;









class GrSimpleTextureEffect : public GrSingleTextureEffect {
public:
    
    static GrEffectRef* Create(GrTexture* tex,
                               const SkMatrix& matrix,
                               CoordsType coordsType = kLocal_CoordsType) {
        GrAssert(kLocal_CoordsType == coordsType || kPosition_CoordsType == coordsType);
        AutoEffectUnref effect(SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, false, coordsType)));
        return CreateEffectRef(effect);
    }

    
    static GrEffectRef* Create(GrTexture* tex,
                               const SkMatrix& matrix,
                               bool bilerp,
                               CoordsType coordsType = kLocal_CoordsType) {
        GrAssert(kLocal_CoordsType == coordsType || kPosition_CoordsType == coordsType);
        AutoEffectUnref effect(
            SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, bilerp, coordsType)));
        return CreateEffectRef(effect);
    }

    static GrEffectRef* Create(GrTexture* tex,
                               const SkMatrix& matrix,
                               const GrTextureParams& p,
                               CoordsType coordsType = kLocal_CoordsType) {
        GrAssert(kLocal_CoordsType == coordsType || kPosition_CoordsType == coordsType);
        AutoEffectUnref effect(SkNEW_ARGS(GrSimpleTextureEffect, (tex, matrix, p, coordsType)));
        return CreateEffectRef(effect);
    }

    

    static GrEffectRef* CreateWithCustomCoords(GrTexture* tex, const GrTextureParams& p) {
        AutoEffectUnref effect(SkNEW_ARGS(GrSimpleTextureEffect, (tex,
                                                                  SkMatrix::I(),
                                                                  p,
                                                                  kCustom_CoordsType)));
        return CreateEffectRef(effect);
    }

    virtual ~GrSimpleTextureEffect() {}

    static const char* Name() { return "Texture"; }

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    typedef GrGLSimpleTextureEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrSimpleTextureEffect(GrTexture* texture,
                          const SkMatrix& matrix,
                          bool bilerp,
                          CoordsType coordsType)
        : GrSingleTextureEffect(texture, matrix, bilerp, coordsType) {
        GrAssert(kLocal_CoordsType == coordsType || kPosition_CoordsType == coordsType);
    }

    GrSimpleTextureEffect(GrTexture* texture,
                          const SkMatrix& matrix,
                          const GrTextureParams& params,
                          CoordsType coordsType)
        : GrSingleTextureEffect(texture, matrix, params, coordsType) {
        if (kCustom_CoordsType == coordsType) {
            GrAssert(matrix.isIdentity());
            this->addVertexAttrib(kVec2f_GrSLType);
        }
    }

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        const GrSimpleTextureEffect& ste = CastEffect<GrSimpleTextureEffect>(other);
        return this->hasSameTextureParamsMatrixAndCoordsType(ste);
    }

    GR_DECLARE_EFFECT_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
