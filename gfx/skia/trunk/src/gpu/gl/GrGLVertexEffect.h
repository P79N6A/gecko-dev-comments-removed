






#ifndef GrGLVertexEffect_DEFINED
#define GrGLVertexEffect_DEFINED

#include "GrGLEffect.h"






class GrGLVertexEffect : public GrGLEffect {
public:
    GrGLVertexEffect(const GrBackendEffectFactory& factory)
        : INHERITED(factory) { fIsVertexEffect = true; }

    



    virtual void emitCode(GrGLFullShaderBuilder* builder,
                          const GrDrawEffect& drawEffect,
                          const GrEffectKey& key,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray& coords,
                          const TextureSamplerArray& samplers) = 0;

    


    virtual void emitCode(GrGLShaderBuilder* builder,
                          const GrDrawEffect& drawEffect,
                          const GrEffectKey& key,
                          const char* outputColor,
                          const char* inputColor,
                          const TransformedCoordsArray& coords,
                          const TextureSamplerArray& samplers) SK_OVERRIDE {
        SkFAIL("GrGLVertexEffect requires GrGLFullShaderBuilder* overload for emitCode().");
    }

private:
    typedef GrGLEffect INHERITED;
};

#endif
