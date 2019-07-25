








#ifndef SkColor_DEFINED
#define SkColor_DEFINED

#include "SkScalar.h"








typedef uint8_t SkAlpha;





typedef uint32_t SkColor;



static inline SkColor SkColorSetARGBInline(U8CPU a, U8CPU r, U8CPU g, U8CPU b)
{
    SkASSERT(a <= 255 && r <= 255 && g <= 255 && b <= 255);

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

#define SkColorSetARGBMacro(a, r, g, b) \
    static_cast<SkColor>( \
        (static_cast<U8CPU>(a) << 24) | \
        (static_cast<U8CPU>(r) << 16) | \
        (static_cast<U8CPU>(g) << 8) | \
        (static_cast<U8CPU>(b) << 0))





#if defined(NDEBUG)
#define SkColorSetARGB(a, r, g, b) SkColorSetARGBMacro(a, r, g, b)
#else
#define SkColorSetARGB(a, r, g, b) SkColorSetARGBInline(a, r, g, b)
#endif




#define SkColorSetRGB(r, g, b)  SkColorSetARGB(0xFF, r, g, b)


#define SkColorGetA(color)      (((color) >> 24) & 0xFF)

#define SkColorGetR(color)      (((color) >> 16) & 0xFF)

#define SkColorGetG(color)      (((color) >>  8) & 0xFF)

#define SkColorGetB(color)      (((color) >>  0) & 0xFF)

static inline SkColor SkColorSetA(SkColor c, U8CPU a) {
    return (c & 0x00FFFFFF) | (a << 24);
}



#define SK_ColorBLACK   0xFF000000  //!< black SkColor value
#define SK_ColorDKGRAY  0xFF444444  //!< dark gray SkColor value
#define SK_ColorGRAY    0xFF888888  //!< gray SkColor value
#define SK_ColorLTGRAY  0xFFCCCCCC  //!< light gray SkColor value
#define SK_ColorWHITE   0xFFFFFFFF  //!< white SkColor value

#define SK_ColorRED     0xFFFF0000  //!< red SkColor value
#define SK_ColorGREEN   0xFF00FF00  //!< green SkColor value
#define SK_ColorBLUE    0xFF0000FF  //!< blue SkColor value
#define SK_ColorYELLOW  0xFFFFFF00  //!< yellow SkColor value
#define SK_ColorCYAN    0xFF00FFFF  //!< cyan SkColor value
#define SK_ColorMAGENTA 0xFFFF00FF  //!< magenta SkColor value












SK_API void SkRGBToHSV(U8CPU red, U8CPU green, U8CPU blue, SkScalar hsv[3]);








static inline void SkColorToHSV(SkColor color, SkScalar hsv[3])
{
    SkRGBToHSV(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color), hsv);
}










SK_API SkColor SkHSVToColor(U8CPU alpha, const SkScalar hsv[3]);









static inline SkColor SkHSVToColor(const SkScalar hsv[3])
{
    return SkHSVToColor(0xFF, hsv);
}







typedef uint32_t SkPMColor;



SK_API SkPMColor SkPreMultiplyARGB(U8CPU a, U8CPU r, U8CPU g, U8CPU b);




SK_API SkPMColor SkPreMultiplyColor(SkColor c);



typedef SkPMColor (*SkXfermodeProc)(SkPMColor src, SkPMColor dst);




typedef uint16_t (*SkXfermodeProc16)(SkPMColor src, uint16_t dst);

#endif

