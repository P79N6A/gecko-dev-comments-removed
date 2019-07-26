






#ifndef GrTextureDomainEffect_DEFINED
#define GrTextureDomainEffect_DEFINED

#include "GrSingleTextureEffect.h"
#include "GrRect.h"

class GrGLTextureDomainEffect;








class GrTextureDomainEffect : public GrSingleTextureEffect {

public:
    





    enum WrapMode {
        kClamp_WrapMode,
        kDecal_WrapMode,
    };

    static GrEffectRef* Create(GrTexture*,
                               const SkMatrix&,
                               const SkRect& domain,
                               WrapMode,
                               bool bilerp,
                               CoordsType = kLocal_CoordsType);

    virtual ~GrTextureDomainEffect();

    static const char* Name() { return "TextureDomain"; }

    typedef GrGLTextureDomainEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;
    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    const SkRect& domain() const { return fTextureDomain; }
    WrapMode wrapMode() const { return fWrapMode; }

    

    static const SkRect MakeTexelDomain(const GrTexture* texture, const SkIRect& texelRect) {
        SkScalar wInv = SK_Scalar1 / texture->width();
        SkScalar hInv = SK_Scalar1 / texture->height();
        SkRect result = {
            texelRect.fLeft * wInv,
            texelRect.fTop * hInv,
            texelRect.fRight * wInv,
            texelRect.fBottom * hInv
        };
        return result;
    }

protected:
    WrapMode fWrapMode;
    SkRect   fTextureDomain;

private:
    GrTextureDomainEffect(GrTexture*,
                          const SkMatrix&,
                          const GrRect& domain,
                          WrapMode,
                          bool bilerp,
                          CoordsType type);

    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    GR_DECLARE_EFFECT_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
