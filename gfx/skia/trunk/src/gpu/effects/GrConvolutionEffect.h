






#ifndef GrConvolutionEffect_DEFINED
#define GrConvolutionEffect_DEFINED

#include "Gr1DKernelEffect.h"

class GrGLConvolutionEffect;






class GrConvolutionEffect : public Gr1DKernelEffect {

public:

    
    static GrEffect* Create(GrTexture* tex,
                            Direction dir,
                            int halfWidth,
                            const float* kernel,
                            bool useBounds,
                            float bounds[2]) {
        return SkNEW_ARGS(GrConvolutionEffect, (tex,
                                                dir,
                                                halfWidth,
                                                kernel,
                                                useBounds,
                                                bounds));
    }

    
    static GrEffect* CreateGaussian(GrTexture* tex,
                                    Direction dir,
                                    int halfWidth,
                                    float gaussianSigma,
                                    bool useBounds,
                                    float bounds[2]) {
        return SkNEW_ARGS(GrConvolutionEffect, (tex,
                                                dir,
                                                halfWidth,
                                                gaussianSigma,
                                                useBounds,
                                                bounds));
    }

    virtual ~GrConvolutionEffect();

    const float* kernel() const { return fKernel; }

    const float* bounds() const { return fBounds; }
    bool useBounds() const { return fUseBounds; }

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
    bool fUseBounds;
    float fBounds[2];

private:
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth,
                        const float* kernel,
                        bool useBounds,
                        float bounds[2]);

    
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth,
                        float gaussianSigma,
                        bool useBounds,
                        float bounds[2]);

    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    GR_DECLARE_EFFECT_TEST;

    typedef Gr1DKernelEffect INHERITED;
};

#endif
