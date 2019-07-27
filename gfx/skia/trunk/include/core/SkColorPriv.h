






#ifndef SkColorPriv_DEFINED
#define SkColorPriv_DEFINED


#ifdef SK_DEBUG
    #define CHECK_FOR_565_OVERFLOW
#endif

#include "SkColor.h"
#include "SkMath.h"



#define SkASSERT_IS_BYTE(x)     SkASSERT(0 == ((x) & ~0xFF))













#ifdef SK_CPU_BENDIAN
    #define SK_RGBA_R32_SHIFT   24
    #define SK_RGBA_G32_SHIFT   16
    #define SK_RGBA_B32_SHIFT   8
    #define SK_RGBA_A32_SHIFT   0

    #define SK_BGRA_B32_SHIFT   24
    #define SK_BGRA_G32_SHIFT   16
    #define SK_BGRA_R32_SHIFT   8
    #define SK_BGRA_A32_SHIFT   0
#else
    #define SK_RGBA_R32_SHIFT   0
    #define SK_RGBA_G32_SHIFT   8
    #define SK_RGBA_B32_SHIFT   16
    #define SK_RGBA_A32_SHIFT   24

    #define SK_BGRA_B32_SHIFT   0
    #define SK_BGRA_G32_SHIFT   8
    #define SK_BGRA_R32_SHIFT   16
    #define SK_BGRA_A32_SHIFT   24
#endif

#if defined(SK_PMCOLOR_IS_RGBA) && defined(SK_PMCOLOR_IS_BGRA)
    #error "can't define PMCOLOR to be RGBA and BGRA"
#endif

#define LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_RGBA  \
    (SK_A32_SHIFT == SK_RGBA_A32_SHIFT &&    \
     SK_R32_SHIFT == SK_RGBA_R32_SHIFT &&    \
     SK_G32_SHIFT == SK_RGBA_G32_SHIFT &&    \
     SK_B32_SHIFT == SK_RGBA_B32_SHIFT)

#define LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_BGRA  \
    (SK_A32_SHIFT == SK_BGRA_A32_SHIFT &&    \
     SK_R32_SHIFT == SK_BGRA_R32_SHIFT &&    \
     SK_G32_SHIFT == SK_BGRA_G32_SHIFT &&    \
     SK_B32_SHIFT == SK_BGRA_B32_SHIFT)


#if defined(SK_PMCOLOR_IS_RGBA) && !LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_RGBA
    #error "SK_PMCOLOR_IS_RGBA does not match SK_*32_SHIFT values"
#endif

#if defined(SK_PMCOLOR_IS_BGRA) && !LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_BGRA
    #error "SK_PMCOLOR_IS_BGRA does not match SK_*32_SHIFT values"
#endif

#if !defined(SK_PMCOLOR_IS_RGBA) && !defined(SK_PMCOLOR_IS_BGRA)
    

    #if LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_RGBA
        #define SK_PMCOLOR_IS_RGBA
    #elif LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_BGRA
        #define SK_PMCOLOR_IS_BGRA
    #else
        #error "need 32bit packing to be either RGBA or BGRA"
    #endif
#endif


#undef LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_RGBA
#undef LOCAL_PMCOLOR_SHIFTS_EQUIVALENT_TO_BGRA






static inline uint32_t SkSwizzle_RB(uint32_t c) {
    static const uint32_t kRBMask = (0xFF << SK_R32_SHIFT) | (0xFF << SK_B32_SHIFT);

    unsigned c0 = (c >> SK_R32_SHIFT) & 0xFF;
    unsigned c1 = (c >> SK_B32_SHIFT) & 0xFF;
    return (c & ~kRBMask) | (c0 << SK_B32_SHIFT) | (c1 << SK_R32_SHIFT);
}

static inline uint32_t SkPackARGB_as_RGBA(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    SkASSERT_IS_BYTE(a);
    SkASSERT_IS_BYTE(r);
    SkASSERT_IS_BYTE(g);
    SkASSERT_IS_BYTE(b);
    return (a << SK_RGBA_A32_SHIFT) | (r << SK_RGBA_R32_SHIFT) |
           (g << SK_RGBA_G32_SHIFT) | (b << SK_RGBA_B32_SHIFT);
}

static inline uint32_t SkPackARGB_as_BGRA(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    SkASSERT_IS_BYTE(a);
    SkASSERT_IS_BYTE(r);
    SkASSERT_IS_BYTE(g);
    SkASSERT_IS_BYTE(b);
    return (a << SK_BGRA_A32_SHIFT) | (r << SK_BGRA_R32_SHIFT) |
           (g << SK_BGRA_G32_SHIFT) | (b << SK_BGRA_B32_SHIFT);
}

static inline SkPMColor SkSwizzle_RGBA_to_PMColor(uint32_t c) {
#ifdef SK_PMCOLOR_IS_RGBA
    return c;
#else
    return SkSwizzle_RB(c);
#endif
}

static inline SkPMColor SkSwizzle_BGRA_to_PMColor(uint32_t c) {
#ifdef SK_PMCOLOR_IS_BGRA
    return c;
#else
    return SkSwizzle_RB(c);
#endif
}





#define SK_ITU_BT709_LUM_COEFF_R (0.2126f)
#define SK_ITU_BT709_LUM_COEFF_G (0.7152f)
#define SK_ITU_BT709_LUM_COEFF_B (0.0722f)




#define SK_LUM_COEFF_R SK_ITU_BT709_LUM_COEFF_R
#define SK_LUM_COEFF_G SK_ITU_BT709_LUM_COEFF_G
#define SK_LUM_COEFF_B SK_ITU_BT709_LUM_COEFF_B





static inline U8CPU SkComputeLuminance(U8CPU r, U8CPU g, U8CPU b) {
    
    
    
    return (r * 54 + g * 183 + b * 19) >> 8;
}







static inline unsigned SkAlpha255To256(U8CPU alpha) {
    SkASSERT(SkToU8(alpha) == alpha);
    
    
    return alpha + 1;
}





static inline unsigned Sk255To256(U8CPU value) {
    SkASSERT(SkToU8(value) == value);
    return value + (value >> 7);
}




#define SkAlphaMul(value, alpha256)     (SkMulS16(value, alpha256) >> 8)




static inline int SkAlphaBlend(int src, int dst, int scale256) {
    SkASSERT((unsigned)scale256 <= 256);
    return dst + SkAlphaMul(src - dst, scale256);
}






static inline int SkAlphaBlend255(S16CPU src, S16CPU dst, U8CPU alpha) {
    SkASSERT((int16_t)src == src);
    SkASSERT((int16_t)dst == dst);
    SkASSERT((uint8_t)alpha == alpha);

    int prod = SkMulS16(src - dst, alpha) + 128;
    prod = (prod + (prod >> 8)) >> 8;
    return dst + prod;
}

#define SK_R16_BITS     5
#define SK_G16_BITS     6
#define SK_B16_BITS     5

#define SK_R16_SHIFT    (SK_B16_BITS + SK_G16_BITS)
#define SK_G16_SHIFT    (SK_B16_BITS)
#define SK_B16_SHIFT    0

#define SK_R16_MASK     ((1 << SK_R16_BITS) - 1)
#define SK_G16_MASK     ((1 << SK_G16_BITS) - 1)
#define SK_B16_MASK     ((1 << SK_B16_BITS) - 1)

#define SkGetPackedR16(color)   (((unsigned)(color) >> SK_R16_SHIFT) & SK_R16_MASK)
#define SkGetPackedG16(color)   (((unsigned)(color) >> SK_G16_SHIFT) & SK_G16_MASK)
#define SkGetPackedB16(color)   (((unsigned)(color) >> SK_B16_SHIFT) & SK_B16_MASK)

#define SkR16Assert(r)  SkASSERT((unsigned)(r) <= SK_R16_MASK)
#define SkG16Assert(g)  SkASSERT((unsigned)(g) <= SK_G16_MASK)
#define SkB16Assert(b)  SkASSERT((unsigned)(b) <= SK_B16_MASK)

static inline uint16_t SkPackRGB16(unsigned r, unsigned g, unsigned b) {
    SkASSERT(r <= SK_R16_MASK);
    SkASSERT(g <= SK_G16_MASK);
    SkASSERT(b <= SK_B16_MASK);

    return SkToU16((r << SK_R16_SHIFT) | (g << SK_G16_SHIFT) | (b << SK_B16_SHIFT));
}

#define SK_R16_MASK_IN_PLACE        (SK_R16_MASK << SK_R16_SHIFT)
#define SK_G16_MASK_IN_PLACE        (SK_G16_MASK << SK_G16_SHIFT)
#define SK_B16_MASK_IN_PLACE        (SK_B16_MASK << SK_B16_SHIFT)




static inline uint32_t SkExpand_rgb_16(U16CPU c) {
    SkASSERT(c == (uint16_t)c);

    return ((c & SK_G16_MASK_IN_PLACE) << 16) | (c & ~SK_G16_MASK_IN_PLACE);
}







static inline U16CPU SkCompact_rgb_16(uint32_t c) {
    return ((c >> 16) & SK_G16_MASK_IN_PLACE) | (c & ~SK_G16_MASK_IN_PLACE);
}






static inline U16CPU SkAlphaMulRGB16(U16CPU c, unsigned scale) {
    return SkCompact_rgb_16(SkExpand_rgb_16(c) * (scale >> 3) >> 5);
}


#define SkAlphaMulRGB16_ToU16(c, s)  (uint16_t)SkAlphaMulRGB16(c, s)






static inline U16CPU SkBlendRGB16(U16CPU src, U16CPU dst, int srcScale) {
    SkASSERT((unsigned)srcScale <= 256);

    srcScale >>= 3;

    uint32_t src32 = SkExpand_rgb_16(src);
    uint32_t dst32 = SkExpand_rgb_16(dst);
    return SkCompact_rgb_16(dst32 + ((src32 - dst32) * srcScale >> 5));
}

static inline void SkBlendRGB16(const uint16_t src[], uint16_t dst[],
                                int srcScale, int count) {
    SkASSERT(count > 0);
    SkASSERT((unsigned)srcScale <= 256);

    srcScale >>= 3;

    do {
        uint32_t src32 = SkExpand_rgb_16(*src++);
        uint32_t dst32 = SkExpand_rgb_16(*dst);
        *dst++ = SkCompact_rgb_16(dst32 + ((src32 - dst32) * srcScale >> 5));
    } while (--count > 0);
}

#ifdef SK_DEBUG
    static inline U16CPU SkRGB16Add(U16CPU a, U16CPU b) {
        SkASSERT(SkGetPackedR16(a) + SkGetPackedR16(b) <= SK_R16_MASK);
        SkASSERT(SkGetPackedG16(a) + SkGetPackedG16(b) <= SK_G16_MASK);
        SkASSERT(SkGetPackedB16(a) + SkGetPackedB16(b) <= SK_B16_MASK);

        return a + b;
    }
#else
    #define SkRGB16Add(a, b)  ((a) + (b))
#endif



#define SK_A32_BITS     8
#define SK_R32_BITS     8
#define SK_G32_BITS     8
#define SK_B32_BITS     8

#define SK_A32_MASK     ((1 << SK_A32_BITS) - 1)
#define SK_R32_MASK     ((1 << SK_R32_BITS) - 1)
#define SK_G32_MASK     ((1 << SK_G32_BITS) - 1)
#define SK_B32_MASK     ((1 << SK_B32_BITS) - 1)

#define SkGetPackedA32(packed)      ((uint32_t)((packed) << (24 - SK_A32_SHIFT)) >> 24)
#define SkGetPackedR32(packed)      ((uint32_t)((packed) << (24 - SK_R32_SHIFT)) >> 24)
#define SkGetPackedG32(packed)      ((uint32_t)((packed) << (24 - SK_G32_SHIFT)) >> 24)
#define SkGetPackedB32(packed)      ((uint32_t)((packed) << (24 - SK_B32_SHIFT)) >> 24)

#define SkA32Assert(a)  SkASSERT((unsigned)(a) <= SK_A32_MASK)
#define SkR32Assert(r)  SkASSERT((unsigned)(r) <= SK_R32_MASK)
#define SkG32Assert(g)  SkASSERT((unsigned)(g) <= SK_G32_MASK)
#define SkB32Assert(b)  SkASSERT((unsigned)(b) <= SK_B32_MASK)

#ifdef SK_DEBUG
    static inline void SkPMColorAssert(SkPMColor c) {
        unsigned a = SkGetPackedA32(c);
        unsigned r = SkGetPackedR32(c);
        unsigned g = SkGetPackedG32(c);
        unsigned b = SkGetPackedB32(c);

        SkA32Assert(a);
        SkASSERT(r <= a);
        SkASSERT(g <= a);
        SkASSERT(b <= a);
    }
#else
    #define SkPMColorAssert(c)
#endif





static inline SkPMColor SkPackARGB32(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    SkA32Assert(a);
    SkASSERT(r <= a);
    SkASSERT(g <= a);
    SkASSERT(b <= a);

    return (a << SK_A32_SHIFT) | (r << SK_R32_SHIFT) |
           (g << SK_G32_SHIFT) | (b << SK_B32_SHIFT);
}

static inline uint32_t SkPackPMColor_as_RGBA(SkPMColor c) {
    return SkPackARGB_as_RGBA(SkGetPackedA32(c), SkGetPackedR32(c),
                              SkGetPackedG32(c), SkGetPackedB32(c));
}

static inline uint32_t SkPackPMColor_as_BGRA(SkPMColor c) {
    return SkPackARGB_as_BGRA(SkGetPackedA32(c), SkGetPackedR32(c),
                              SkGetPackedG32(c), SkGetPackedB32(c));
}








static inline SkPMColor SkFourByteInterp256(SkPMColor src, SkPMColor dst,
                                         unsigned scale) {
    unsigned a = SkAlphaBlend(SkGetPackedA32(src), SkGetPackedA32(dst), scale);
    unsigned r = SkAlphaBlend(SkGetPackedR32(src), SkGetPackedR32(dst), scale);
    unsigned g = SkAlphaBlend(SkGetPackedG32(src), SkGetPackedG32(dst), scale);
    unsigned b = SkAlphaBlend(SkGetPackedB32(src), SkGetPackedB32(dst), scale);

    return SkPackARGB32(a, r, g, b);
}







static inline SkPMColor SkFourByteInterp(SkPMColor src, SkPMColor dst,
                                         U8CPU srcWeight) {
    unsigned scale = SkAlpha255To256(srcWeight);
    return SkFourByteInterp256(src, dst, scale);
}




static inline void SkSplay(uint32_t color, uint32_t* ag, uint32_t* rb) {
    const uint32_t mask = 0x00FF00FF;
    *ag = (color >> 8) & mask;
    *rb = color & mask;
}





static inline uint64_t SkSplay(uint32_t color) {
    const uint32_t mask = 0x00FF00FF;
    uint64_t agrb = (color >> 8) & mask;  
    agrb <<= 32;                          
    agrb |= color & mask;                 
    return agrb;
}




static inline uint32_t SkUnsplay(uint32_t ag, uint32_t rb) {
    const uint32_t mask = 0xFF00FF00;
    return (ag & mask) | ((rb & mask) >> 8);
}





static inline uint32_t SkUnsplay(uint64_t agrb) {
    const uint32_t mask = 0xFF00FF00;
    return SkPMColor(
        ((agrb & mask) >> 8) |   
        ((agrb >> 32) & mask));  
}

static inline SkPMColor SkFastFourByteInterp256_32(SkPMColor src, SkPMColor dst, unsigned scale) {
    SkASSERT(scale <= 256);

    
    uint32_t src_ag, src_rb, dst_ag, dst_rb;
    SkSplay(src, &src_ag, &src_rb);
    SkSplay(dst, &dst_ag, &dst_rb);

    const uint32_t ret_ag = src_ag * scale + (256 - scale) * dst_ag;
    const uint32_t ret_rb = src_rb * scale + (256 - scale) * dst_rb;

    return SkUnsplay(ret_ag, ret_rb);
}

static inline SkPMColor SkFastFourByteInterp256_64(SkPMColor src, SkPMColor dst, unsigned scale) {
    SkASSERT(scale <= 256);
    
    return SkUnsplay(SkSplay(src) * scale + (256-scale) * SkSplay(dst));
}






static inline SkPMColor SkFastFourByteInterp256(SkPMColor src, SkPMColor dst, unsigned scale) {
    
    if (sizeof(void*) == 4) {
        return SkFastFourByteInterp256_32(src, dst, scale);
    } else {
        return SkFastFourByteInterp256_64(src, dst, scale);
    }
}





static inline SkPMColor SkFastFourByteInterp(SkPMColor src,
                                             SkPMColor dst,
                                             U8CPU srcWeight) {
    SkASSERT(srcWeight <= 255);
    
    
    return SkFastFourByteInterp256(src, dst, srcWeight + (srcWeight >> 7));
}





static inline SkPMColor SkPackARGB32NoCheck(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    return (a << SK_A32_SHIFT) | (r << SK_R32_SHIFT) |
           (g << SK_G32_SHIFT) | (b << SK_B32_SHIFT);
}

static inline
SkPMColor SkPremultiplyARGBInline(U8CPU a, U8CPU r, U8CPU g, U8CPU b) {
    SkA32Assert(a);
    SkR32Assert(r);
    SkG32Assert(g);
    SkB32Assert(b);

    if (a != 255) {
        r = SkMulDiv255Round(r, a);
        g = SkMulDiv255Round(g, a);
        b = SkMulDiv255Round(b, a);
    }
    return SkPackARGB32(a, r, g, b);
}



static SK_ALWAYS_INLINE uint32_t SkAlphaMulQ(uint32_t c, unsigned scale) {
    uint32_t mask = 0xFF00FF;

    uint32_t rb = ((c & mask) * scale) >> 8;
    uint32_t ag = ((c >> 8) & mask) * scale;
    return (rb & mask) | (ag & ~mask);
}

static inline SkPMColor SkPMSrcOver(SkPMColor src, SkPMColor dst) {
    return src + SkAlphaMulQ(dst, SkAlpha255To256(255 - SkGetPackedA32(src)));
}

static inline SkPMColor SkBlendARGB32(SkPMColor src, SkPMColor dst, U8CPU aa) {
    SkASSERT((unsigned)aa <= 255);

    unsigned src_scale = SkAlpha255To256(aa);
    unsigned dst_scale = SkAlpha255To256(255 - SkAlphaMul(SkGetPackedA32(src), src_scale));

    return SkAlphaMulQ(src, src_scale) + SkAlphaMulQ(dst, dst_scale);
}




#define SkR32ToR16_MACRO(r)   ((unsigned)(r) >> (SK_R32_BITS - SK_R16_BITS))
#define SkG32ToG16_MACRO(g)   ((unsigned)(g) >> (SK_G32_BITS - SK_G16_BITS))
#define SkB32ToB16_MACRO(b)   ((unsigned)(b) >> (SK_B32_BITS - SK_B16_BITS))

#ifdef SK_DEBUG
    static inline unsigned SkR32ToR16(unsigned r) {
        SkR32Assert(r);
        return SkR32ToR16_MACRO(r);
    }
    static inline unsigned SkG32ToG16(unsigned g) {
        SkG32Assert(g);
        return SkG32ToG16_MACRO(g);
    }
    static inline unsigned SkB32ToB16(unsigned b) {
        SkB32Assert(b);
        return SkB32ToB16_MACRO(b);
    }
#else
    #define SkR32ToR16(r)   SkR32ToR16_MACRO(r)
    #define SkG32ToG16(g)   SkG32ToG16_MACRO(g)
    #define SkB32ToB16(b)   SkB32ToB16_MACRO(b)
#endif

#define SkPacked32ToR16(c)  (((unsigned)(c) >> (SK_R32_SHIFT + SK_R32_BITS - SK_R16_BITS)) & SK_R16_MASK)
#define SkPacked32ToG16(c)  (((unsigned)(c) >> (SK_G32_SHIFT + SK_G32_BITS - SK_G16_BITS)) & SK_G16_MASK)
#define SkPacked32ToB16(c)  (((unsigned)(c) >> (SK_B32_SHIFT + SK_B32_BITS - SK_B16_BITS)) & SK_B16_MASK)

static inline U16CPU SkPixel32ToPixel16(SkPMColor c) {
    unsigned r = ((c >> (SK_R32_SHIFT + (8 - SK_R16_BITS))) & SK_R16_MASK) << SK_R16_SHIFT;
    unsigned g = ((c >> (SK_G32_SHIFT + (8 - SK_G16_BITS))) & SK_G16_MASK) << SK_G16_SHIFT;
    unsigned b = ((c >> (SK_B32_SHIFT + (8 - SK_B16_BITS))) & SK_B16_MASK) << SK_B16_SHIFT;
    return r | g | b;
}

static inline U16CPU SkPack888ToRGB16(U8CPU r, U8CPU g, U8CPU b) {
    return  (SkR32ToR16(r) << SK_R16_SHIFT) |
            (SkG32ToG16(g) << SK_G16_SHIFT) |
            (SkB32ToB16(b) << SK_B16_SHIFT);
}

#define SkPixel32ToPixel16_ToU16(src)   SkToU16(SkPixel32ToPixel16(src))




#define SkShouldDitherXY(x, y)  (((x) ^ (y)) & 1)

static inline uint16_t SkDitherPack888ToRGB16(U8CPU r, U8CPU g, U8CPU b) {
    r = ((r << 1) - ((r >> (8 - SK_R16_BITS) << (8 - SK_R16_BITS)) | (r >> SK_R16_BITS))) >> (8 - SK_R16_BITS);
    g = ((g << 1) - ((g >> (8 - SK_G16_BITS) << (8 - SK_G16_BITS)) | (g >> SK_G16_BITS))) >> (8 - SK_G16_BITS);
    b = ((b << 1) - ((b >> (8 - SK_B16_BITS) << (8 - SK_B16_BITS)) | (b >> SK_B16_BITS))) >> (8 - SK_B16_BITS);

    return SkPackRGB16(r, g, b);
}

static inline uint16_t SkDitherPixel32ToPixel16(SkPMColor c) {
    return SkDitherPack888ToRGB16(SkGetPackedR32(c), SkGetPackedG32(c), SkGetPackedB32(c));
}








static inline uint32_t SkPMColorToExpanded16x5(SkPMColor c) {
    unsigned sr = SkPacked32ToR16(c);
    unsigned sg = SkPacked32ToG16(c);
    unsigned sb = SkPacked32ToB16(c);

    sr = (sr << 5) | sr;
    sg = (sg << 5) | (sg >> 1);
    sb = (sb << 5) | sb;
    return (sr << 11) | (sg << 21) | (sb << 0);
}




static inline U16CPU SkSrcOver32To16(SkPMColor src, uint16_t dst) {
    unsigned sr = SkGetPackedR32(src);
    unsigned sg = SkGetPackedG32(src);
    unsigned sb = SkGetPackedB32(src);

    unsigned dr = SkGetPackedR16(dst);
    unsigned dg = SkGetPackedG16(dst);
    unsigned db = SkGetPackedB16(dst);

    unsigned isa = 255 - SkGetPackedA32(src);

    dr = (sr + SkMul16ShiftRound(dr, isa, SK_R16_BITS)) >> (8 - SK_R16_BITS);
    dg = (sg + SkMul16ShiftRound(dg, isa, SK_G16_BITS)) >> (8 - SK_G16_BITS);
    db = (sb + SkMul16ShiftRound(db, isa, SK_B16_BITS)) >> (8 - SK_B16_BITS);

    return SkPackRGB16(dr, dg, db);
}




static inline unsigned SkR16ToR32(unsigned r) {
    return (r << (8 - SK_R16_BITS)) | (r >> (2 * SK_R16_BITS - 8));
}

static inline unsigned SkG16ToG32(unsigned g) {
    return (g << (8 - SK_G16_BITS)) | (g >> (2 * SK_G16_BITS - 8));
}

static inline unsigned SkB16ToB32(unsigned b) {
    return (b << (8 - SK_B16_BITS)) | (b >> (2 * SK_B16_BITS - 8));
}

#define SkPacked16ToR32(c)      SkR16ToR32(SkGetPackedR16(c))
#define SkPacked16ToG32(c)      SkG16ToG32(SkGetPackedG16(c))
#define SkPacked16ToB32(c)      SkB16ToB32(SkGetPackedB16(c))

static inline SkPMColor SkPixel16ToPixel32(U16CPU src) {
    SkASSERT(src == SkToU16(src));

    unsigned    r = SkPacked16ToR32(src);
    unsigned    g = SkPacked16ToG32(src);
    unsigned    b = SkPacked16ToB32(src);

    SkASSERT((r >> (8 - SK_R16_BITS)) == SkGetPackedR16(src));
    SkASSERT((g >> (8 - SK_G16_BITS)) == SkGetPackedG16(src));
    SkASSERT((b >> (8 - SK_B16_BITS)) == SkGetPackedB16(src));

    return SkPackARGB32(0xFF, r, g, b);
}


static inline SkColor SkPixel16ToColor(U16CPU src) {
    SkASSERT(src == SkToU16(src));

    unsigned    r = SkPacked16ToR32(src);
    unsigned    g = SkPacked16ToG32(src);
    unsigned    b = SkPacked16ToB32(src);

    SkASSERT((r >> (8 - SK_R16_BITS)) == SkGetPackedR16(src));
    SkASSERT((g >> (8 - SK_G16_BITS)) == SkGetPackedG16(src));
    SkASSERT((b >> (8 - SK_B16_BITS)) == SkGetPackedB16(src));

    return SkColorSetRGB(r, g, b);
}



typedef uint16_t SkPMColor16;


#define SK_A4444_SHIFT    0
#define SK_R4444_SHIFT    12
#define SK_G4444_SHIFT    8
#define SK_B4444_SHIFT    4

#define SkA32To4444(a)  ((unsigned)(a) >> 4)
#define SkR32To4444(r)  ((unsigned)(r) >> 4)
#define SkG32To4444(g)  ((unsigned)(g) >> 4)
#define SkB32To4444(b)  ((unsigned)(b) >> 4)

static inline U8CPU SkReplicateNibble(unsigned nib) {
    SkASSERT(nib <= 0xF);
    return (nib << 4) | nib;
}

#define SkA4444ToA32(a)     SkReplicateNibble(a)
#define SkR4444ToR32(r)     SkReplicateNibble(r)
#define SkG4444ToG32(g)     SkReplicateNibble(g)
#define SkB4444ToB32(b)     SkReplicateNibble(b)

#define SkGetPackedA4444(c)     (((unsigned)(c) >> SK_A4444_SHIFT) & 0xF)
#define SkGetPackedR4444(c)     (((unsigned)(c) >> SK_R4444_SHIFT) & 0xF)
#define SkGetPackedG4444(c)     (((unsigned)(c) >> SK_G4444_SHIFT) & 0xF)
#define SkGetPackedB4444(c)     (((unsigned)(c) >> SK_B4444_SHIFT) & 0xF)

#define SkPacked4444ToA32(c)    SkReplicateNibble(SkGetPackedA4444(c))
#define SkPacked4444ToR32(c)    SkReplicateNibble(SkGetPackedR4444(c))
#define SkPacked4444ToG32(c)    SkReplicateNibble(SkGetPackedG4444(c))
#define SkPacked4444ToB32(c)    SkReplicateNibble(SkGetPackedB4444(c))

#ifdef SK_DEBUG
static inline void SkPMColor16Assert(U16CPU c) {
    unsigned a = SkGetPackedA4444(c);
    unsigned r = SkGetPackedR4444(c);
    unsigned g = SkGetPackedG4444(c);
    unsigned b = SkGetPackedB4444(c);

    SkASSERT(a <= 0xF);
    SkASSERT(r <= a);
    SkASSERT(g <= a);
    SkASSERT(b <= a);
}
#else
#define SkPMColor16Assert(c)
#endif

static inline unsigned SkAlpha15To16(unsigned a) {
    SkASSERT(a <= 0xF);
    return a + (a >> 3);
}

#ifdef SK_DEBUG
    static inline int SkAlphaMul4(int value, int scale) {
        SkASSERT((unsigned)scale <= 0x10);
        return value * scale >> 4;
    }
#else
    #define SkAlphaMul4(value, scale)   ((value) * (scale) >> 4)
#endif

static inline unsigned SkR4444ToR565(unsigned r) {
    SkASSERT(r <= 0xF);
    return (r << (SK_R16_BITS - 4)) | (r >> (8 - SK_R16_BITS));
}

static inline unsigned SkG4444ToG565(unsigned g) {
    SkASSERT(g <= 0xF);
    return (g << (SK_G16_BITS - 4)) | (g >> (8 - SK_G16_BITS));
}

static inline unsigned SkB4444ToB565(unsigned b) {
    SkASSERT(b <= 0xF);
    return (b << (SK_B16_BITS - 4)) | (b >> (8 - SK_B16_BITS));
}

static inline SkPMColor16 SkPackARGB4444(unsigned a, unsigned r,
                                         unsigned g, unsigned b) {
    SkASSERT(a <= 0xF);
    SkASSERT(r <= a);
    SkASSERT(g <= a);
    SkASSERT(b <= a);

    return (SkPMColor16)((a << SK_A4444_SHIFT) | (r << SK_R4444_SHIFT) |
                         (g << SK_G4444_SHIFT) | (b << SK_B4444_SHIFT));
}

static inline U16CPU SkAlphaMulQ4(U16CPU c, unsigned scale) {
    SkASSERT(scale <= 16);

    const unsigned mask = 0xF0F;    

#if 0
    unsigned rb = ((c & mask) * scale) >> 4;
    unsigned ag = ((c >> 4) & mask) * scale;
    return (rb & mask) | (ag & ~mask);
#else
    c = (c & mask) | ((c & (mask << 4)) << 12);
    c = c * scale >> 4;
    return (c & mask) | ((c >> 12) & (mask << 4));
#endif
}




static inline uint32_t SkExpand_4444(U16CPU c) {
    SkASSERT(c == (uint16_t)c);

    const unsigned mask = 0xF0F;    
    return (c & mask) | ((c & ~mask) << 12);
}








static inline U16CPU SkCompact_4444(uint32_t c) {
    const unsigned mask = 0xF0F;    
    return (c & mask) | ((c >> 12) & ~mask);
}

static inline uint16_t SkSrcOver4444To16(SkPMColor16 s, uint16_t d) {
    unsigned sa = SkGetPackedA4444(s);
    unsigned sr = SkR4444ToR565(SkGetPackedR4444(s));
    unsigned sg = SkG4444ToG565(SkGetPackedG4444(s));
    unsigned sb = SkB4444ToB565(SkGetPackedB4444(s));

    
    
    
    
    sg &= ~(~(sa >> 3) & 1);

    unsigned scale = SkAlpha15To16(15 - sa);
    unsigned dr = SkAlphaMul4(SkGetPackedR16(d), scale);
    unsigned dg = SkAlphaMul4(SkGetPackedG16(d), scale);
    unsigned db = SkAlphaMul4(SkGetPackedB16(d), scale);

#if 0
    if (sg + dg > 63) {
        SkDebugf("---- SkSrcOver4444To16 src=%x dst=%x scale=%d, sg=%d dg=%d\n", s, d, scale, sg, dg);
    }
#endif
    return SkPackRGB16(sr + dr, sg + dg, sb + db);
}

static inline uint16_t SkBlend4444To16(SkPMColor16 src, uint16_t dst, int scale16) {
    SkASSERT((unsigned)scale16 <= 16);

    return SkSrcOver4444To16(SkAlphaMulQ4(src, scale16), dst);
}

static inline uint16_t SkBlend4444(SkPMColor16 src, SkPMColor16 dst, int scale16) {
    SkASSERT((unsigned)scale16 <= 16);

    uint32_t src32 = SkExpand_4444(src) * scale16;
    
#ifdef SK_DEBUG
    {
        unsigned srcA = SkGetPackedA4444(src) * scale16;
        SkASSERT(srcA == (src32 & 0xFF));
    }
#endif
    unsigned dstScale = SkAlpha255To256(255 - (src32 & 0xFF)) >> 4;
    uint32_t dst32 = SkExpand_4444(dst) * dstScale;
    return SkCompact_4444((src32 + dst32) >> 4);
}

static inline SkPMColor SkPixel4444ToPixel32(U16CPU c) {
    uint32_t d = (SkGetPackedA4444(c) << SK_A32_SHIFT) |
                 (SkGetPackedR4444(c) << SK_R32_SHIFT) |
                 (SkGetPackedG4444(c) << SK_G32_SHIFT) |
                 (SkGetPackedB4444(c) << SK_B32_SHIFT);
    return d | (d << 4);
}

static inline SkPMColor16 SkPixel32ToPixel4444(SkPMColor c) {
    return  (((c >> (SK_A32_SHIFT + 4)) & 0xF) << SK_A4444_SHIFT) |
    (((c >> (SK_R32_SHIFT + 4)) & 0xF) << SK_R4444_SHIFT) |
    (((c >> (SK_G32_SHIFT + 4)) & 0xF) << SK_G4444_SHIFT) |
    (((c >> (SK_B32_SHIFT + 4)) & 0xF) << SK_B4444_SHIFT);
}


static inline SkPMColor16 SkDitherARGB32To4444(U8CPU a, U8CPU r,
                                               U8CPU g, U8CPU b) {
    
    
    
    
    unsigned dithered_a = ((a << 1) - ((a >> 4 << 4) | (a >> 4))) >> 4;
    a = SkMax32(a >> 4, dithered_a);
    
    r = ((r << 1) - ((r >> 4 << 4) | (r >> 4))) >> 4;
    g = ((g << 1) - ((g >> 4 << 4) | (g >> 4))) >> 4;
    b = ((b << 1) - ((b >> 4 << 4) | (b >> 4))) >> 4;

    return SkPackARGB4444(a, r, g, b);
}

static inline SkPMColor16 SkDitherPixel32To4444(SkPMColor c) {
    return SkDitherARGB32To4444(SkGetPackedA32(c), SkGetPackedR32(c),
                                SkGetPackedG32(c), SkGetPackedB32(c));
}





static inline uint32_t SkExpand_8888(SkPMColor c) {
    return  (((c >> SK_R32_SHIFT) & 0xFF) << 24) |
            (((c >> SK_G32_SHIFT) & 0xFF) <<  8) |
            (((c >> SK_B32_SHIFT) & 0xFF) << 16) |
            (((c >> SK_A32_SHIFT) & 0xFF) <<  0);
}




static inline SkPMColor SkCompact_8888(uint32_t c) {
    return  (((c >> 24) & 0xFF) << SK_R32_SHIFT) |
            (((c >>  8) & 0xFF) << SK_G32_SHIFT) |
            (((c >> 16) & 0xFF) << SK_B32_SHIFT) |
            (((c >>  0) & 0xFF) << SK_A32_SHIFT);
}





static inline uint32_t SkExpand32_4444(SkPMColor c) {
    return  (((c >> (SK_R32_SHIFT + 4)) & 0xF) << 24) |
            (((c >> (SK_G32_SHIFT + 4)) & 0xF) <<  8) |
            (((c >> (SK_B32_SHIFT + 4)) & 0xF) << 16) |
            (((c >> (SK_A32_SHIFT + 4)) & 0xF) <<  0);
}



void sk_dither_memset16(uint16_t dst[], uint16_t value, uint16_t other, int n);



static inline int SkUpscale31To32(int value) {
    SkASSERT((unsigned)value <= 31);
    return value + (value >> 4);
}

static inline int SkBlend32(int src, int dst, int scale) {
    SkASSERT((unsigned)src <= 0xFF);
    SkASSERT((unsigned)dst <= 0xFF);
    SkASSERT((unsigned)scale <= 32);
    return dst + ((src - dst) * scale >> 5);
}

static inline SkPMColor SkBlendLCD16(int srcA, int srcR, int srcG, int srcB,
                                     SkPMColor dst, uint16_t mask) {
    if (mask == 0) {
        return dst;
    }

    


    int maskR = SkGetPackedR16(mask) >> (SK_R16_BITS - 5);
    int maskG = SkGetPackedG16(mask) >> (SK_G16_BITS - 5);
    int maskB = SkGetPackedB16(mask) >> (SK_B16_BITS - 5);

    
    maskR = SkUpscale31To32(maskR);
    maskG = SkUpscale31To32(maskG);
    maskB = SkUpscale31To32(maskB);

    
    maskR = maskR * srcA >> 8;
    maskG = maskG * srcA >> 8;
    maskB = maskB * srcA >> 8;

    int dstR = SkGetPackedR32(dst);
    int dstG = SkGetPackedG32(dst);
    int dstB = SkGetPackedB32(dst);

    
    
    return SkPackARGB32(0xFF,
                        SkBlend32(srcR, dstR, maskR),
                        SkBlend32(srcG, dstG, maskG),
                        SkBlend32(srcB, dstB, maskB));
}

static inline SkPMColor SkBlendLCD16Opaque(int srcR, int srcG, int srcB,
                                           SkPMColor dst, uint16_t mask,
                                           SkPMColor opaqueDst) {
    if (mask == 0) {
        return dst;
    }

    if (0xFFFF == mask) {
        return opaqueDst;
    }

    


    int maskR = SkGetPackedR16(mask) >> (SK_R16_BITS - 5);
    int maskG = SkGetPackedG16(mask) >> (SK_G16_BITS - 5);
    int maskB = SkGetPackedB16(mask) >> (SK_B16_BITS - 5);

    
    maskR = SkUpscale31To32(maskR);
    maskG = SkUpscale31To32(maskG);
    maskB = SkUpscale31To32(maskB);

    int dstR = SkGetPackedR32(dst);
    int dstG = SkGetPackedG32(dst);
    int dstB = SkGetPackedB32(dst);

    
    
    return SkPackARGB32(0xFF,
                        SkBlend32(srcR, dstR, maskR),
                        SkBlend32(srcG, dstG, maskG),
                        SkBlend32(srcB, dstB, maskB));
}

static inline void SkBlitLCD16Row(SkPMColor dst[], const uint16_t mask[],
                                  SkColor src, int width, SkPMColor) {
    int srcA = SkColorGetA(src);
    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);

    srcA = SkAlpha255To256(srcA);

    for (int i = 0; i < width; i++) {
        dst[i] = SkBlendLCD16(srcA, srcR, srcG, srcB, dst[i], mask[i]);
    }
}

static inline void SkBlitLCD16OpaqueRow(SkPMColor dst[], const uint16_t mask[],
                                        SkColor src, int width,
                                        SkPMColor opaqueDst) {
    int srcR = SkColorGetR(src);
    int srcG = SkColorGetG(src);
    int srcB = SkColorGetB(src);

    for (int i = 0; i < width; i++) {
        dst[i] = SkBlendLCD16Opaque(srcR, srcG, srcB, dst[i], mask[i],
                                    opaqueDst);
    }
}

#endif
