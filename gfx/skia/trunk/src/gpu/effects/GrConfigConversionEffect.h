






#ifndef GrConfigConversionEffect_DEFINED
#define GrConfigConversionEffect_DEFINED

#include "GrSingleTextureEffect.h"

class GrEffectStage;
class GrGLConfigConversionEffect;







class GrConfigConversionEffect : public GrSingleTextureEffect {
public:
    


    enum PMConversion {
        kNone_PMConversion = 0,
        kMulByAlpha_RoundUp_PMConversion,
        kMulByAlpha_RoundDown_PMConversion,
        kDivByAlpha_RoundUp_PMConversion,
        kDivByAlpha_RoundDown_PMConversion,

        kPMConversionCnt
    };

    
    static const GrEffect* Create(GrTexture*,
                                  bool swapRedAndBlue,
                                  PMConversion pmConversion,
                                  const SkMatrix& matrix);

    static const char* Name() { return "Config Conversion"; }
    typedef GrGLConfigConversionEffect GLEffect;

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE;

    virtual void getConstantColorComponents(GrColor* color, uint32_t* validFlags) const SK_OVERRIDE;

    bool swapsRedAndBlue() const { return fSwapRedAndBlue; }
    PMConversion  pmConversion() const { return fPMConversion; }

    
    
    
    
    
    static void TestForPreservingPMConversions(GrContext* context,
                                               PMConversion* PMToUPMRule,
                                               PMConversion* UPMToPMRule);

private:
    GrConfigConversionEffect(GrTexture*,
                            bool swapRedAndBlue,
                            PMConversion pmConversion,
                            const SkMatrix& matrix);

    virtual bool onIsEqual(const GrEffect&) const SK_OVERRIDE;

    bool            fSwapRedAndBlue;
    PMConversion    fPMConversion;

    GR_DECLARE_EFFECT_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
