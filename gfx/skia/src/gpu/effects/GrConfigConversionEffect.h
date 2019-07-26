






#ifndef GrConfigConversionEffect_DEFINED
#define GrConfigConversionEffect_DEFINED

#include "GrSingleTextureEffect.h"

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

    
    static GrCustomStage* Create(GrTexture*,
                                 bool swapRedAndBlue,
                                 PMConversion pmConversion = kNone_PMConversion);

    static const char* Name() { return "Config Conversion"; }
    typedef GrGLConfigConversionEffect GLProgramStage;

    virtual const GrProgramStageFactory& getFactory() const SK_OVERRIDE;
    virtual bool isEqual(const GrCustomStage&) const SK_OVERRIDE;

    bool swapsRedAndBlue() const { return fSwapRedAndBlue; }
    PMConversion  pmConversion() const { return fPMConversion; }

    
    
    
    
    
    static void TestForPreservingPMConversions(GrContext* context,
                                               PMConversion* PMToUPMRule,
                                               PMConversion* UPMToPMRule);

private:
    GrConfigConversionEffect(GrTexture*,
                            bool swapRedAndBlue,
                            PMConversion pmConversion);

    bool            fSwapRedAndBlue;
    PMConversion    fPMConversion;

    GR_DECLARE_CUSTOM_STAGE_TEST;

    typedef GrSingleTextureEffect INHERITED;
};

#endif
