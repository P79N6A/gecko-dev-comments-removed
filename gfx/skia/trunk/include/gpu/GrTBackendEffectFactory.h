






#ifndef GrTBackendEffectFactory_DEFINED
#define GrTBackendEffectFactory_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrDrawEffect.h"
#include "gl/GrGLProgramEffects.h"




template <typename EffectClass>
class GrTBackendEffectFactory : public GrBackendEffectFactory {

public:
    typedef typename EffectClass::GLEffect GLEffect;

    


    virtual const char* name() const SK_OVERRIDE { return EffectClass::Name(); }

    




    virtual EffectKey glEffectKey(const GrDrawEffect& drawEffect,
                                  const GrGLCaps& caps) const SK_OVERRIDE {
        SkASSERT(kIllegalEffectClassID != fEffectClassID);
        EffectKey effectKey = GLEffect::GenKey(drawEffect, caps);
        EffectKey textureKey = GrGLProgramEffects::GenTextureKey(drawEffect, caps);
        EffectKey transformKey = GrGLProgramEffects::GenTransformKey(drawEffect);
        EffectKey attribKey = GrGLProgramEffects::GenAttribKey(drawEffect);
#ifdef SK_DEBUG
        static const EffectKey kIllegalEffectKeyMask = (uint16_t) (~((1U << kEffectKeyBits) - 1));
        SkASSERT(!(kIllegalEffectKeyMask & effectKey));

        static const EffectKey kIllegalTextureKeyMask = (uint16_t) (~((1U << kTextureKeyBits) - 1));
        SkASSERT(!(kIllegalTextureKeyMask & textureKey));

        static const EffectKey kIllegalTransformKeyMask = (uint16_t) (~((1U << kTransformKeyBits) - 1));
        SkASSERT(!(kIllegalTransformKeyMask & transformKey));

        static const EffectKey kIllegalAttribKeyMask = (uint16_t) (~((1U << kAttribKeyBits) - 1));
        SkASSERT(!(kIllegalAttribKeyMask & textureKey));

        static const EffectKey kIllegalClassIDMask = (uint16_t) (~((1U << kClassIDBits) - 1));
        SkASSERT(!(kIllegalClassIDMask & fEffectClassID));
#endif
        return (fEffectClassID << (kEffectKeyBits+kTextureKeyBits+kTransformKeyBits+kAttribKeyBits)) |
               (attribKey << (kEffectKeyBits+kTextureKeyBits+kTransformKeyBits)) |
               (transformKey << (kEffectKeyBits+kTextureKeyBits)) |
               (textureKey << kEffectKeyBits) |
               (effectKey);
    }

    


    virtual GrGLEffect* createGLInstance(const GrDrawEffect& drawEffect) const SK_OVERRIDE {
        return SkNEW_ARGS(GLEffect, (*this, drawEffect));
    }

    

    static const GrBackendEffectFactory& getInstance() {
        static SkAlignedSTStorage<1, GrTBackendEffectFactory> gInstanceMem;
        static const GrTBackendEffectFactory* gInstance;
        if (!gInstance) {
            gInstance = SkNEW_PLACEMENT(gInstanceMem.get(),
                                        GrTBackendEffectFactory);
        }
        return *gInstance;
    }

protected:
    GrTBackendEffectFactory() {
        fEffectClassID = GenID();
    }
};

#endif
