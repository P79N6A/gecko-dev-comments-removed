






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


    
    virtual void getGLEffectKey(const GrDrawEffect& drawEffect,
                                const GrGLCaps& caps,
                                GrEffectKeyBuilder* b) const SK_OVERRIDE {
        GLEffect::GenKey(drawEffect, caps, b);
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
    GrTBackendEffectFactory() {}
};

#endif
