






#ifndef GrConvolutionEffect_DEFINED
#define GrConvolutionEffect_DEFINED

#include "Gr1DKernelEffect.h"

class GrGLConvolutionEffect;






class GrConvolutionEffect : public Gr1DKernelEffect {

public:

    
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth, const float* kernel = NULL);

    
    GrConvolutionEffect(GrTexture*, Direction,
                        int halfWidth, float gaussianSigma);
    virtual ~GrConvolutionEffect();

    const float* kernel() const { return fKernel; }

    static const char* Name() { return "Convolution"; }

    typedef GrGLConvolutionEffect GLProgramStage;

    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE;
    virtual bool isEqual(const GrCustomStage&) const SK_OVERRIDE;

    enum {
        
        
        
        
        kMaxKernelRadius = 12,
        
        
        kMaxKernelWidth = 2 * kMaxKernelRadius + 1,
    };

protected:

    float fKernel[kMaxKernelWidth];

private:
    GR_DECLARE_CUSTOM_STAGE_TEST;

    typedef Gr1DKernelEffect INHERITED;
};

#endif
