






#ifndef GrGLEffect_DEFINED
#define GrGLEffect_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrGLShaderBuilder.h"
#include "GrGLShaderVar.h"
#include "GrGLSL.h"

class GrGLTexture;


















class GrDrawEffect;

class GrGLEffect {

public:
    typedef GrBackendEffectFactory::EffectKey EffectKey;

    enum {
        kNoEffectKey = GrBackendEffectFactory::kNoEffectKey,
        
        kEffectKeyBits = GrBackendEffectFactory::kEffectKeyBits,
    };

    typedef GrGLShaderBuilder::TextureSamplerArray TextureSamplerArray;

    GrGLEffect(const GrBackendEffectFactory&);

    virtual ~GrGLEffect();

    



















    virtual void emitCode(GrGLShaderBuilder* builder,
                          const GrDrawEffect& drawEffect,
                          EffectKey key,
                          const char* outputColor,
                          const char* inputColor,
                          const TextureSamplerArray& samplers) = 0;

    






    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&);

    const char* name() const { return fFactory.name(); }

    static EffectKey GenTextureKey(const GrDrawEffect&, const GrGLCaps&);
    static EffectKey GenAttribKey(const GrDrawEffect& stage);

protected:
    const GrBackendEffectFactory& fFactory;
};

#endif
