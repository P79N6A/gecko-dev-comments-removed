









#ifndef GrColor_DEFINED
#define GrColor_DEFINED

#include "GrTypes.h"





typedef uint32_t GrColor;





#ifdef SK_CPU_BENDIAN
    #define GrColor_SHIFT_R     24
    #define GrColor_SHIFT_G     16
    #define GrColor_SHIFT_B     8
    #define GrColor_SHIFT_A     0
#else
    #define GrColor_SHIFT_R     0
    #define GrColor_SHIFT_G     8
    #define GrColor_SHIFT_B     16
    #define GrColor_SHIFT_A     24
#endif




static inline GrColor GrColorPackRGBA(unsigned r, unsigned g,
                                      unsigned b, unsigned a) {
    SkASSERT((uint8_t)r == r);
    SkASSERT((uint8_t)g == g);
    SkASSERT((uint8_t)b == b);
    SkASSERT((uint8_t)a == a);
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




static inline void GrColorIsPMAssert(GrColor c) {
#ifdef SK_DEBUG
    unsigned a = GrColorUnpackA(c);
    unsigned r = GrColorUnpackR(c);
    unsigned g = GrColorUnpackG(c);
    unsigned b = GrColorUnpackB(c);

    SkASSERT(r <= a);
    SkASSERT(g <= a);
    SkASSERT(b <= a);
#endif
}


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

static inline char GrColorComponentFlagToChar(GrColorComponentFlags component) {
    SkASSERT(SkIsPow2(component));
    switch (component) {
        case kR_GrColorComponentFlag:
            return 'r';
        case kG_GrColorComponentFlag:
            return 'g';
        case kB_GrColorComponentFlag:
            return 'b';
        case kA_GrColorComponentFlag:
            return 'a';
        default:
            SkFAIL("Invalid color component flag.");
            return '\0';
    }
}

static inline uint32_t GrPixelConfigComponentMask(GrPixelConfig config) {
    SkASSERT(config >= 0 && config < kGrPixelConfigCnt);
    static const uint32_t kFlags[] = {
        0,                              
        kA_GrColorComponentFlag,        
        kRGBA_GrColorComponentFlags,    
        kRGB_GrColorComponentFlags,     
        kRGBA_GrColorComponentFlags,    
        kRGBA_GrColorComponentFlags,    
        kRGBA_GrColorComponentFlags,    
        kRGB_GrColorComponentFlags,     
        kA_GrColorComponentFlag,        
        kA_GrColorComponentFlag,        
        kRGBA_GrColorComponentFlags,    
        kRGBA_GrColorComponentFlags,    
    };
    return kFlags[config];

    GR_STATIC_ASSERT(0  == kUnknown_GrPixelConfig);
    GR_STATIC_ASSERT(1  == kAlpha_8_GrPixelConfig);
    GR_STATIC_ASSERT(2  == kIndex_8_GrPixelConfig);
    GR_STATIC_ASSERT(3  == kRGB_565_GrPixelConfig);
    GR_STATIC_ASSERT(4  == kRGBA_4444_GrPixelConfig);
    GR_STATIC_ASSERT(5  == kRGBA_8888_GrPixelConfig);
    GR_STATIC_ASSERT(6  == kBGRA_8888_GrPixelConfig);
    GR_STATIC_ASSERT(7  == kETC1_GrPixelConfig);
    GR_STATIC_ASSERT(8  == kLATC_GrPixelConfig);
    GR_STATIC_ASSERT(9  == kR11_EAC_GrPixelConfig);
    GR_STATIC_ASSERT(10 == kASTC_12x12_GrPixelConfig);
    GR_STATIC_ASSERT(11 == kRGBA_float_GrPixelConfig);
    GR_STATIC_ASSERT(SK_ARRAY_COUNT(kFlags) == kGrPixelConfigCnt);
}

#endif
