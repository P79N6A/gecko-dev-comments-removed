






#ifndef GrTBackendEffectFactory_DEFINED
#define GrTBackendEffectFactory_DEFINED

#include "GrBackendEffectFactory.h"
#include "GrDrawEffect.h"




template <typename EffectClass>
class GrTBackendEffectFactory : public GrBackendEffectFactory {

public:
    typedef typename EffectClass::GLEffect GLEffect;

    


    virtual const char* name() const SK_OVERRIDE { return EffectClass::Name(); }

    




    virtual EffectKey glEffectKey(const GrDrawEffect& drawEffect,
                                  const GrGLCaps& caps) const SK_OVERRIDE {
        GrAssert(kIllegalEffectClassID != fEffectClassID);
        EffectKey effectKey = GLEffect::GenKey(drawEffect, caps);
        EffectKey textureKey = GLEffect::GenTextureKey(drawEffect, caps);
        EffectKey attribKey = GLEffect::GenAttribKey(drawEffect);
#if GR_DEBUG
        static const EffectKey kIllegalIDMask = (uint16_t) (~((1U << kEffectKeyBits) - 1));
        GrAssert(!(kIllegalIDMask & effectKey));

        static const EffectKey kIllegalTextureKeyMask = (uint16_t) (~((1U << kTextureKeyBits) - 1));
        GrAssert(!(kIllegalTextureKeyMask & textureKey));

        static const EffectKey kIllegalAttribKeyMask = (uint16_t) (~((1U << kAttribKeyBits) - 1));
        GrAssert(!(kIllegalAttribKeyMask & textureKey));
#endif
        return fEffectClassID | (attribKey << (kEffectKeyBits+kTextureKeyBits)) |
               (textureKey << kEffectKeyBits) | effectKey;
    }

    


    virtual GLEffect* createGLInstance(const GrDrawEffect& drawEffect) const SK_OVERRIDE {
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
        fEffectClassID = GenID() << (kEffectKeyBits + kTextureKeyBits) ;
    }
};

#endif
