






#ifndef GrBicubicTextureEffect_DEFINED
#define GrBicubicTextureEffect_DEFINED

#include "GrSingleTextureEffect.h"
#include "GrTextureDomain.h"
#include "GrDrawEffect.h"
#include "gl/GrGLEffect.h"
#include "GrTBackendEffectFactory.h"

class GrGLBicubicEffect;

class GrBicubicEffect : public GrSingleTextureEffect {
public:
    enum {
        kFilterTexelPad = 2, 
                             
    };
    virtual ~GrBicubicEffect();

    static const char* Name() { return "Bicubic"; }
    const float* coefficients() const { return fCoefficients; }

    typedef GrGLBicubicEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;
    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    const GrTextureDomain& domain() const { return fDomain; }

    


    static GrEffect* Create(GrTexture* tex, const SkScalar coefficients[16],
                            const SkRect* domain = NULL) {
        if (NULL == domain) {
            static const SkShader::TileMode kTileModes[] = { SkShader::kClamp_TileMode,
                                                             SkShader::kClamp_TileMode };
            return Create(tex, coefficients, MakeDivByTextureWHMatrix(tex), kTileModes);
        } else {
            return SkNEW_ARGS(GrBicubicEffect, (tex, coefficients,
                                                MakeDivByTextureWHMatrix(tex), *domain));
        }
    }

    


    static GrEffect* Create(GrTexture* tex, const SkMatrix& matrix,
                            SkShader::TileMode tileModes[2]) {
        return Create(tex, gMitchellCoefficients, matrix, tileModes);
    }

    



    static GrEffect* Create(GrTexture* tex, const SkScalar coefficients[16],
                            const SkMatrix& matrix, const SkShader::TileMode tileModes[2]) {
        return SkNEW_ARGS(GrBicubicEffect, (tex, coefficients, matrix, tileModes));
    }

    


    static GrEffect* Create(GrTexture* tex, const SkMatrix& matrix, const SkRect& domain) {
        return SkNEW_ARGS(GrBicubicEffect, (tex, gMitchellCoefficients, matrix, domain));
    }

    






    static bool ShouldUseBicubic(const SkMatrix& localCoordsToDevice,
                                 GrTextureParams::FilterMode* filterMode);

private:
    GrBicubicEffect(GrTexture*, const SkScalar coefficients[16],
                    const SkMatrix &matrix, const SkShader::TileMode tileModes[2]);
    GrBicubicEffect(GrTexture*, const SkScalar coefficients[16],
                    const SkMatrix &matrix, const SkRect& domain);
    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    float           fCoefficients[16];
    GrTextureDomain fDomain;

    GR_DECLARE_EFFECT_TEST;

    static const SkScalar gMitchellCoefficients[16];

    typedef GrSingleTextureEffect INHERITED;
};

#endif
