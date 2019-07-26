






#ifndef GrGLEffect_DEFINED
#define GrGLEffect_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrGLProgramEffects.h"
#include "GrGLShaderBuilder.h"
#include "GrGLShaderVar.h"
#include "GrGLSL.h"


















class GrDrawEffect;
class GrGLTexture;
class GrGLVertexEffect;

class GrGLEffect {

public:
    typedef GrBackendEffectFactory::EffectKey EffectKey;
    typedef GrGLProgramEffects::TransformedCoordsArray TransformedCoordsArray;
    typedef GrGLProgramEffects::TextureSampler TextureSampler;
    typedef GrGLProgramEffects::TextureSamplerArray TextureSamplerArray;

    enum {
        kNoEffectKey = GrBackendEffectFactory::kNoEffectKey,
        
        kEffectKeyBits = GrBackendEffectFactory::kEffectKeyBits,
    };

    GrGLEffect(const GrBackendEffectFactory& factory)
        : fFactory(factory)
        , fIsVertexEffect(false) {
    }

    virtual ~GrGLEffect() {}

    



















    virtual void emitCode(GrGLShaderBuilder* builder,
                          const GrDrawEffect& drawEffect,
                          EffectKey key,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray& coords,
                          const TextureSamplerArray& samplers) = 0;

    






    virtual void setData(const GrGLUniformManager&, const GrDrawEffect&) {}

    const char* name() const { return fFactory.name(); }

    static inline EffectKey GenKey(const GrDrawEffect&, const GrGLCaps&) { return 0; }

    

    bool isVertexEffect() const { return fIsVertexEffect; }

protected:
    const GrBackendEffectFactory& fFactory;

private:
    friend class GrGLVertexEffect; 

    bool fIsVertexEffect;
};

#endif
