






#ifndef GrCustomCoordsTextureEffect_DEFINED
#define GrCustomCoordsTextureEffect_DEFINED

#include "GrEffect.h"
#include "GrVertexEffect.h"

class GrGLCustomCoordsTextureEffect;






class GrCustomCoordsTextureEffect : public GrVertexEffect {
public:
    static GrEffect* Create(GrTexture* tex, const GrTextureParams& p) {
        return SkNEW_ARGS(GrCustomCoordsTextureEffect, (tex, p));
    }

    virtual ~GrCustomCoordsTextureEffect() {}

    static const char* Name() { return "Texture"; }

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    typedef GrGLCustomCoordsTextureEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

private:
    GrCustomCoordsTextureEffect(GrTexture* texture, const GrTextureParams& params);

    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE;

    GrTextureAccess fTextureAccess;

    GR_DECLARE_EFFECT_TEST;

    typedef GrVertexEffect INHERITED;
};

#endif
