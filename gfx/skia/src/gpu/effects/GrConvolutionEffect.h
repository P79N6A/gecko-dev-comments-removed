






#ifndef GrConvolutionEffect_DEFINED
#define GrConvolutionEffect_DEFINED

#include "Gr1DKernelEffect.h"

class GrGLConvolutionEffect;






class GrConvolutionEffect : public Gr1DKernelEffect {

public:

    
    static GrEffectRef* Create(GrTexture* tex, Direction dir, int halfWidth, const float* kernel) {
        AutoEffectUnref effect(SkNEW_ARGS(GrConvolutionEffect, (tex,
                                                                dir,
                                                                halfWidth,
                                                                kernel)));
        return CreateEffectRef(effect);
    }

    
    static GrEffectRef* CreateGaussian(GrTexture* tex,
                                       Direction dir,
                                       int halfWidth,
                                       float gaussianSigma) {
        AutoEffectUnref effect(SkNEW_ARGS(GrConvolutionEffect, (tex,
                                                                dir,
                                                                halfWidth,
                                                                gaussianSigma)));
        return CreateEffectRef(effect);
    }

    virtual ~GrConvolutionEffect();

    const float* kernel() const { return fKernel; }

    static const char* Name() { return "Convolution"; }

    typedef GrGLConvolutionEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    virtual void getConstantColorComponents(GrColor*, uint32_t* validFlags) const {
        
        
        *validFlags = 0;
    }

    enum {
        
        
        
        
        kMaxKernelRadius = 12,
        
        
        kMaxKernelWidth = 2 * kMaxKernelRadius + 1,
    };

protected:

    float fKernel[kMaxKernelWidth];

private:
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth, const float* kernel);

    
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth,
                        float gaussianSigma);

    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    GR_DECLARE_EFFECT_TEST;

    typedef Gr1DKernelEffect INHERITED;
};

#endif
