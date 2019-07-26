






#ifndef SkDeviceProfile_DEFINED
#define SkDeviceProfile_DEFINED

#include "SkRefCnt.h"

class SkDeviceProfile : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkDeviceProfile)

    enum LCDConfig {
        kNone_LCDConfig,   
        kRGB_Horizontal_LCDConfig,
        kBGR_Horizontal_LCDConfig,
        kRGB_Vertical_LCDConfig,
        kBGR_Vertical_LCDConfig
    };

    enum FontHintLevel {
        kNone_FontHintLevel,
        kSlight_FontHintLevel,
        kNormal_FontHintLevel,
        kFull_FontHintLevel,
        kAuto_FontHintLevel
    };

    














    static SkDeviceProfile* Create(float gammaExp,
                                   float contrastScale,
                                   LCDConfig,
                                   FontHintLevel);

    





    static SkDeviceProfile* GetDefault();

    




    static SkDeviceProfile* RefGlobal();

    






    static void SetGlobal(SkDeviceProfile*);

    float getFontGammaExponent() const { return fGammaExponent; }
    float getFontContrastScale() const { return fContrastScale; }

    



    void generateTableForLuminanceByte(U8CPU lumByte, uint8_t table[256]) const;

private:
    SkDeviceProfile(float gammaExp, float contrastScale, LCDConfig,
                    FontHintLevel);

    float           fGammaExponent;
    float           fContrastScale;
    LCDConfig       fLCDConfig;
    FontHintLevel   fFontHintLevel;

    typedef SkRefCnt INHERITED;
};

#endif

