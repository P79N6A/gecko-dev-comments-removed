









#ifndef GrColor_DEFINED
#define GrColor_DEFINED

#include "GrTypes.h"





typedef uint32_t GrColor;







#define GrColor_SHIFT_R     0
#define GrColor_SHIFT_G     8
#define GrColor_SHIFT_B     16
#define GrColor_SHIFT_A     24




static inline GrColor GrColorPackRGBA(unsigned r, unsigned g,
                                      unsigned b, unsigned a) {
    GrAssert((uint8_t)r == r);
    GrAssert((uint8_t)g == g);
    GrAssert((uint8_t)b == b);
    GrAssert((uint8_t)a == a);
    return  (r << GrColor_SHIFT_R) |
            (g << GrColor_SHIFT_G) |
            (b << GrColor_SHIFT_B) |
            (a << GrColor_SHIFT_A);
}



#define GrColorUnpackR(color)   (((color) >> GrColor_SHIFT_R) & 0xFF)
#define GrColorUnpackG(color)   (((color) >> GrColor_SHIFT_G) & 0xFF)
#define GrColorUnpackB(color)   (((color) >> GrColor_SHIFT_B) & 0xFF)
#define GrColorUnpackA(color)   (((color) >> GrColor_SHIFT_A) & 0xFF)





#define GrColor_ILLEGAL     (~(0xFF << GrColor_SHIFT_A))


static inline void GrColorToRGBAFloat(GrColor color, float rgba[4]) {
    static const float ONE_OVER_255 = 1.f / 255.f;
    rgba[0] = GrColorUnpackR(color) * ONE_OVER_255;
    rgba[1] = GrColorUnpackG(color) * ONE_OVER_255;
    rgba[2] = GrColorUnpackB(color) * ONE_OVER_255;
    rgba[3] = GrColorUnpackA(color) * ONE_OVER_255;
}





enum GrColorComponentFlags {
    kR_GrColorComponentFlag = 1 << (GrColor_SHIFT_R / 8),
    kG_GrColorComponentFlag = 1 << (GrColor_SHIFT_G / 8),
    kB_GrColorComponentFlag = 1 << (GrColor_SHIFT_B / 8),
    kA_GrColorComponentFlag = 1 << (GrColor_SHIFT_A / 8),

    kRGB_GrColorComponentFlags = (kR_GrColorComponentFlag | kG_GrColorComponentFlag |
                                  kB_GrColorComponentFlag),

    kRGBA_GrColorComponentFlags = (kR_GrColorComponentFlag | kG_GrColorComponentFlag |
                                   kB_GrColorComponentFlag | kA_GrColorComponentFlag)
};

static inline uint32_t GrPixelConfigComponentMask(GrPixelConfig config) {
    GrAssert(config >= 0 && config < kGrPixelConfigCnt);
    static const uint32_t kFlags[] = {
        0,                              
        kA_GrColorComponentFlag,        
        kRGBA_GrColorComponentFlags,    
        kRGB_GrColorComponentFlags,     
        kRGBA_GrColorComponentFlags,    
        kRGBA_GrColorComponentFlags,    
        kRGBA_GrColorComponentFlags,    
    };
    return kFlags[config];

    GR_STATIC_ASSERT(0 == kUnknown_GrPixelConfig);
    GR_STATIC_ASSERT(1 == kAlpha_8_GrPixelConfig);
    GR_STATIC_ASSERT(2 == kIndex_8_GrPixelConfig);
    GR_STATIC_ASSERT(3 == kRGB_565_GrPixelConfig);
    GR_STATIC_ASSERT(4 == kRGBA_4444_GrPixelConfig);
    GR_STATIC_ASSERT(5 == kRGBA_8888_GrPixelConfig);
    GR_STATIC_ASSERT(6 == kBGRA_8888_GrPixelConfig);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kFlags) == kGrPixelConfigCnt);
}

#endif
