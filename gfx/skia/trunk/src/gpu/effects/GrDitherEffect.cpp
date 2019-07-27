






#include "GrDitherEffect.h"

#include "gl/GrGLEffect.h"
#include "gl/GrGLShaderBuilder.h"
#include "gl/GrGLSL.h"
#include "GrTBackendEffectFactory.h"

#include "SkRect.h"



class GLDitherEffect;

class DitherEffect : public GrEffect {
public:
    static GrEffect* Create() {
        GR_CREATE_STATIC_EFFECT(gDitherEffect, DitherEffect, ())
        return SkRef(gDitherEffect);
    }

    virtual ~DitherEffect() {};
    static const char* Name() { return "Dither"; }

    typedef GLDitherEffect GLEffect;

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<DitherEffect>::getInstance();
    }

private:
    DitherEffect() {
        this->setWillReadFragmentPosition();
    }

    
    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE { return true; }

    GR_DECLARE_EFFECT_TEST;

    typedef GrEffect INHERITED;
};

void DitherEffect::getConstantColorComponents(GrColor* color, uint32_t* validFlags) const {
    *validFlags = 0;
}



GR_DEFINE_EFFECT_TEST(DitherEffect);

GrEffect* DitherEffect::TestCreate(SkRandom*,
                                   GrContext*,
                                   const GrDrawTargetCaps&,
                                   GrTexture*[]) {
    return DitherEffect::Create();
}



class GLDitherEffect : public GrGLEffect {
public:
    GLDitherEffect(const GrBackendEffectFactory&, const GrDrawEffect&);

    virtual void emitCode(GrGLShaderBuilder* builder,
                          const GrDrawEffect& drawEffect,
                          const GrEffectKey& key,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray&,
                          const TextureSamplerArray&) SK_OVERRIDE;

private:
    typedef GrGLEffect INHERITED;
};

GLDitherEffect::GLDitherEffect(const GrBackendEffectFactory& factory,
                               const GrDrawEffect& drawEffect)
    : INHERITED (factory) {
}

void GLDitherEffect::emitCode(GrGLShaderBuilder* builder,
                              const GrDrawEffect& drawEffect,
                              const GrEffectKey& key,
                              const char* outputColor,
                              const char* inputColor,
                              const TransformedCoordsArray&,
                              const TextureSamplerArray& samplers) {
    
    
    
    
    
    

    
    
    builder->fsCodeAppendf("\t\tfloat r = "
                           "fract(sin(dot(%s.xy ,vec2(12.9898,78.233))) * 43758.5453);\n",
                           builder->fragmentPosition());
    builder->fsCodeAppendf("\t\t%s = (1.0/255.0) * vec4(r, r, r, r) + %s;\n",
                           outputColor, GrGLSLExpr4(inputColor).c_str());
}



GrEffect* GrDitherEffect::Create() { return DitherEffect::Create(); }
