






#include "GrGLSL.h"
#include "GrGLEffect.h"
#include "GrDrawEffect.h"

GrGLEffect::GrGLEffect(const GrBackendEffectFactory& factory)
    : fFactory(factory) {
}

GrGLEffect::~GrGLEffect() {
}



void GrGLEffect::setData(const GrGLUniformManager&, const GrDrawEffect&) {
}

GrGLEffect::EffectKey GrGLEffect::GenTextureKey(const GrDrawEffect& drawEffect,
                                                const GrGLCaps& caps) {
    EffectKey key = 0;
    int numTextures = (*drawEffect.effect())->numTextures();
    for (int index = 0; index < numTextures; ++index) {
        const GrTextureAccess& access = (*drawEffect.effect())->textureAccess(index);
        EffectKey value = GrGLShaderBuilder::KeyForTextureAccess(access, caps) << index;
        GrAssert(0 == (value & key)); 
        key |= value;
    }
    return key;
}

GrGLEffect::EffectKey GrGLEffect::GenAttribKey(const GrDrawEffect& drawEffect) {
    EffectKey key = 0;

    int numAttributes = drawEffect.getVertexAttribIndexCount();
    GrAssert(numAttributes <= 2);
    const int* attributeIndices = drawEffect.getVertexAttribIndices();
    for (int index = 0; index < numAttributes; ++index) {
        EffectKey value = attributeIndices[index] << 3*index;
        GrAssert(0 == (value & key)); 
        key |= value;
    }

    return key;
}
