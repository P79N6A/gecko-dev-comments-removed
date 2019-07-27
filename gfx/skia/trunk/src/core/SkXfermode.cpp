








#include "SkXfermode.h"
#include "SkXfermode_opts_SSE2.h"
#include "SkXfermode_proccoeff.h"
#include "SkColorPriv.h"
#include "SkLazyPtr.h"
#include "SkMathPriv.h"
#include "SkReadBuffer.h"
#include "SkString.h"
#include "SkUtilsArm.h"
#include "SkWriteBuffer.h"

#if !SK_ARM_NEON_IS_NONE
#include "SkXfermode_opts_arm_neon.h"
#endif

#define SkAlphaMulAlpha(a, b)   SkMulDiv255Round(a, b)

#if 0


static U8CPU mulmuldiv255round(U8CPU a, U8CPU b, U8CPU c, U8CPU d) {
    SkASSERT(a <= 255);
    SkASSERT(b <= 255);
    SkASSERT(c <= 255);
    SkASSERT(d <= 255);
    unsigned prod = SkMulS16(a, b) + SkMulS16(c, d) + 128;
    unsigned result = (prod + (prod >> 8)) >> 8;
    SkASSERT(result <= 255);
    return result;
}
#endif

static inline unsigned saturated_add(unsigned a, unsigned b) {
    SkASSERT(a <= 255);
    SkASSERT(b <= 255);
    unsigned sum = a + b;
    if (sum > 255) {
        sum = 255;
    }
    return sum;
}

static inline int clamp_signed_byte(int n) {
    if (n < 0) {
        n = 0;
    } else if (n > 255) {
        n = 255;
    }
    return n;
}

static inline int clamp_div255round(int prod) {
    if (prod <= 0) {
        return 0;
    } else if (prod >= 255*255) {
        return 255;
    } else {
        return SkDiv255Round(prod);
    }
}




static SkPMColor clear_modeproc(SkPMColor src, SkPMColor dst) {
    return 0;
}


static SkPMColor src_modeproc(SkPMColor src, SkPMColor dst) {
    return src;
}


static SkPMColor dst_modeproc(SkPMColor src, SkPMColor dst) {
    return dst;
}


static SkPMColor srcover_modeproc(SkPMColor src, SkPMColor dst) {
#if 0
    
    
    return src + SkAlphaMulQ(dst, SkAlpha255To256(255 - SkGetPackedA32(src)));
#else
    
    
    return src + SkAlphaMulQ(dst, 256 - SkGetPackedA32(src));
#endif
}


static SkPMColor dstover_modeproc(SkPMColor src, SkPMColor dst) {
    
    
    return dst + SkAlphaMulQ(src, 256 - SkGetPackedA32(dst));
}


static SkPMColor srcin_modeproc(SkPMColor src, SkPMColor dst) {
    return SkAlphaMulQ(src, SkAlpha255To256(SkGetPackedA32(dst)));
}


static SkPMColor dstin_modeproc(SkPMColor src, SkPMColor dst) {
    return SkAlphaMulQ(dst, SkAlpha255To256(SkGetPackedA32(src)));
}


static SkPMColor srcout_modeproc(SkPMColor src, SkPMColor dst) {
    return SkAlphaMulQ(src, SkAlpha255To256(255 - SkGetPackedA32(dst)));
}


static SkPMColor dstout_modeproc(SkPMColor src, SkPMColor dst) {
    return SkAlphaMulQ(dst, SkAlpha255To256(255 - SkGetPackedA32(src)));
}


static SkPMColor srcatop_modeproc(SkPMColor src, SkPMColor dst) {
    unsigned sa = SkGetPackedA32(src);
    unsigned da = SkGetPackedA32(dst);
    unsigned isa = 255 - sa;

    return SkPackARGB32(da,
                        SkAlphaMulAlpha(da, SkGetPackedR32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedR32(dst)),
                        SkAlphaMulAlpha(da, SkGetPackedG32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedG32(dst)),
                        SkAlphaMulAlpha(da, SkGetPackedB32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedB32(dst)));
}


static SkPMColor dstatop_modeproc(SkPMColor src, SkPMColor dst) {
    unsigned sa = SkGetPackedA32(src);
    unsigned da = SkGetPackedA32(dst);
    unsigned ida = 255 - da;

    return SkPackARGB32(sa,
                        SkAlphaMulAlpha(ida, SkGetPackedR32(src)) +
                            SkAlphaMulAlpha(sa, SkGetPackedR32(dst)),
                        SkAlphaMulAlpha(ida, SkGetPackedG32(src)) +
                            SkAlphaMulAlpha(sa, SkGetPackedG32(dst)),
                        SkAlphaMulAlpha(ida, SkGetPackedB32(src)) +
                            SkAlphaMulAlpha(sa, SkGetPackedB32(dst)));
}


static SkPMColor xor_modeproc(SkPMColor src, SkPMColor dst) {
    unsigned sa = SkGetPackedA32(src);
    unsigned da = SkGetPackedA32(dst);
    unsigned isa = 255 - sa;
    unsigned ida = 255 - da;

    return SkPackARGB32(sa + da - (SkAlphaMulAlpha(sa, da) << 1),
                        SkAlphaMulAlpha(ida, SkGetPackedR32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedR32(dst)),
                        SkAlphaMulAlpha(ida, SkGetPackedG32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedG32(dst)),
                        SkAlphaMulAlpha(ida, SkGetPackedB32(src)) +
                            SkAlphaMulAlpha(isa, SkGetPackedB32(dst)));
}




static SkPMColor plus_modeproc(SkPMColor src, SkPMColor dst) {
    unsigned b = saturated_add(SkGetPackedB32(src), SkGetPackedB32(dst));
    unsigned g = saturated_add(SkGetPackedG32(src), SkGetPackedG32(dst));
    unsigned r = saturated_add(SkGetPackedR32(src), SkGetPackedR32(dst));
    unsigned a = saturated_add(SkGetPackedA32(src), SkGetPackedA32(dst));
    return SkPackARGB32(a, r, g, b);
}


static SkPMColor modulate_modeproc(SkPMColor src, SkPMColor dst) {
    int a = SkAlphaMulAlpha(SkGetPackedA32(src), SkGetPackedA32(dst));
    int r = SkAlphaMulAlpha(SkGetPackedR32(src), SkGetPackedR32(dst));
    int g = SkAlphaMulAlpha(SkGetPackedG32(src), SkGetPackedG32(dst));
    int b = SkAlphaMulAlpha(SkGetPackedB32(src), SkGetPackedB32(dst));
    return SkPackARGB32(a, r, g, b);
}

static inline int srcover_byte(int a, int b) {
    return a + b - SkAlphaMulAlpha(a, b);
}




static int blendfunc_multiply_byte(int sc, int dc, int sa, int da) {
    return clamp_div255round(sc * (255 - da)  + dc * (255 - sa)  + sc * dc);
}

static SkPMColor multiply_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = blendfunc_multiply_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = blendfunc_multiply_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = blendfunc_multiply_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static SkPMColor screen_modeproc(SkPMColor src, SkPMColor dst) {
    int a = srcover_byte(SkGetPackedA32(src), SkGetPackedA32(dst));
    int r = srcover_byte(SkGetPackedR32(src), SkGetPackedR32(dst));
    int g = srcover_byte(SkGetPackedG32(src), SkGetPackedG32(dst));
    int b = srcover_byte(SkGetPackedB32(src), SkGetPackedB32(dst));
    return SkPackARGB32(a, r, g, b);
}


static inline int overlay_byte(int sc, int dc, int sa, int da) {
    int tmp = sc * (255 - da) + dc * (255 - sa);
    int rc;
    if (2 * dc <= da) {
        rc = 2 * sc * dc;
    } else {
        rc = sa * da - 2 * (da - dc) * (sa - sc);
    }
    return clamp_div255round(rc + tmp);
}
static SkPMColor overlay_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = overlay_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = overlay_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = overlay_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int darken_byte(int sc, int dc, int sa, int da) {
    int sd = sc * da;
    int ds = dc * sa;
    if (sd < ds) {
        
        return sc + dc - SkDiv255Round(ds);
    } else {
        
        return dc + sc - SkDiv255Round(sd);
    }
}
static SkPMColor darken_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = darken_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = darken_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = darken_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int lighten_byte(int sc, int dc, int sa, int da) {
    int sd = sc * da;
    int ds = dc * sa;
    if (sd > ds) {
        
        return sc + dc - SkDiv255Round(ds);
    } else {
        
        return dc + sc - SkDiv255Round(sd);
    }
}
static SkPMColor lighten_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = lighten_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = lighten_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = lighten_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int colordodge_byte(int sc, int dc, int sa, int da) {
    int diff = sa - sc;
    int rc;
    if (0 == dc) {
        return SkAlphaMulAlpha(sc, 255 - da);
    } else if (0 == diff) {
        rc = sa * da + sc * (255 - da) + dc * (255 - sa);
    } else {
        diff = dc * sa / diff;
        rc = sa * ((da < diff) ? da : diff) + sc * (255 - da) + dc * (255 - sa);
    }
    return clamp_div255round(rc);
}
static SkPMColor colordodge_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = colordodge_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = colordodge_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = colordodge_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int colorburn_byte(int sc, int dc, int sa, int da) {
    int rc;
    if (dc == da) {
        rc = sa * da + sc * (255 - da) + dc * (255 - sa);
    } else if (0 == sc) {
        return SkAlphaMulAlpha(dc, 255 - sa);
    } else {
        int tmp = (da - dc) * sa / sc;
        rc = sa * (da - ((da < tmp) ? da : tmp))
            + sc * (255 - da) + dc * (255 - sa);
    }
    return clamp_div255round(rc);
}
static SkPMColor colorburn_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = colorburn_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = colorburn_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = colorburn_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int hardlight_byte(int sc, int dc, int sa, int da) {
    int rc;
    if (2 * sc <= sa) {
        rc = 2 * sc * dc;
    } else {
        rc = sa * da - 2 * (da - dc) * (sa - sc);
    }
    return clamp_div255round(rc + sc * (255 - da) + dc * (255 - sa));
}
static SkPMColor hardlight_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = hardlight_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = hardlight_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = hardlight_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static U8CPU sqrt_unit_byte(U8CPU n) {
    return SkSqrtBits(n, 15+4);
}


static inline int softlight_byte(int sc, int dc, int sa, int da) {
    int m = da ? dc * 256 / da : 0;
    int rc;
    if (2 * sc <= sa) {
        rc = dc * (sa + ((2 * sc - sa) * (256 - m) >> 8));
    } else if (4 * dc <= da) {
        int tmp = (4 * m * (4 * m + 256) * (m - 256) >> 16) + 7 * m;
        rc = dc * sa + (da * (2 * sc - sa) * tmp >> 8);
    } else {
        int tmp = sqrt_unit_byte(m) - m;
        rc = dc * sa + (da * (2 * sc - sa) * tmp >> 8);
    }
    return clamp_div255round(rc + sc * (255 - da) + dc * (255 - sa));
}
static SkPMColor softlight_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = softlight_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = softlight_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = softlight_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int difference_byte(int sc, int dc, int sa, int da) {
    int tmp = SkMin32(sc * da, dc * sa);
    return clamp_signed_byte(sc + dc - 2 * SkDiv255Round(tmp));
}
static SkPMColor difference_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = difference_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = difference_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = difference_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}


static inline int exclusion_byte(int sc, int dc, int, int) {
    
    

    
    int r = 255*(sc + dc) - 2 * sc * dc;
    return clamp_div255round(r);
}
static SkPMColor exclusion_modeproc(SkPMColor src, SkPMColor dst) {
    int sa = SkGetPackedA32(src);
    int da = SkGetPackedA32(dst);
    int a = srcover_byte(sa, da);
    int r = exclusion_byte(SkGetPackedR32(src), SkGetPackedR32(dst), sa, da);
    int g = exclusion_byte(SkGetPackedG32(src), SkGetPackedG32(dst), sa, da);
    int b = exclusion_byte(SkGetPackedB32(src), SkGetPackedB32(dst), sa, da);
    return SkPackARGB32(a, r, g, b);
}






static inline int Lum(int r, int g, int b)
{
    return SkDiv255Round(r * 77 + g * 150 + b * 28);
}

static inline int min2(int a, int b) { return a < b ? a : b; }
static inline int max2(int a, int b) { return a > b ? a : b; }
#define minimum(a, b, c) min2(min2(a, b), c)
#define maximum(a, b, c) max2(max2(a, b), c)

static inline int Sat(int r, int g, int b) {
    return maximum(r, g, b) - minimum(r, g, b);
}

static inline void setSaturationComponents(int* Cmin, int* Cmid, int* Cmax, int s) {
    if(*Cmax > *Cmin) {
        *Cmid =  SkMulDiv(*Cmid - *Cmin, s, *Cmax - *Cmin);
        *Cmax = s;
    } else {
        *Cmax = 0;
        *Cmid = 0;
    }

    *Cmin = 0;
}

static inline void SetSat(int* r, int* g, int* b, int s) {
    if(*r <= *g) {
        if(*g <= *b) {
            setSaturationComponents(r, g, b, s);
        } else if(*r <= *b) {
            setSaturationComponents(r, b, g, s);
        } else {
            setSaturationComponents(b, r, g, s);
        }
    } else if(*r <= *b) {
        setSaturationComponents(g, r, b, s);
    } else if(*g <= *b) {
        setSaturationComponents(g, b, r, s);
    } else {
        setSaturationComponents(b, g, r, s);
    }
}

static inline void clipColor(int* r, int* g, int* b, int a) {
    int L = Lum(*r, *g, *b);
    int n = minimum(*r, *g, *b);
    int x = maximum(*r, *g, *b);
    int denom;
    if ((n < 0) && (denom = L - n)) { 
       *r = L + SkMulDiv(*r - L, L, denom);
       *g = L + SkMulDiv(*g - L, L, denom);
       *b = L + SkMulDiv(*b - L, L, denom);
    }

    if ((x > a) && (denom = x - L)) { 
       int numer = a - L;
       *r = L + SkMulDiv(*r - L, numer, denom);
       *g = L + SkMulDiv(*g - L, numer, denom);
       *b = L + SkMulDiv(*b - L, numer, denom);
    }
}

static inline void SetLum(int* r, int* g, int* b, int a, int l) {
  int d = l - Lum(*r, *g, *b);
  *r +=  d;
  *g +=  d;
  *b +=  d;

  clipColor(r, g, b, a);
}


#define  blendfunc_nonsep_byte(sc, dc, sa, da, blendval) \
  clamp_div255round(sc * (255 - da) +  dc * (255 - sa) + blendval)




static SkPMColor hue_modeproc(SkPMColor src, SkPMColor dst) {
    int sr = SkGetPackedR32(src);
    int sg = SkGetPackedG32(src);
    int sb = SkGetPackedB32(src);
    int sa = SkGetPackedA32(src);

    int dr = SkGetPackedR32(dst);
    int dg = SkGetPackedG32(dst);
    int db = SkGetPackedB32(dst);
    int da = SkGetPackedA32(dst);
    int Sr, Sg, Sb;

    if(sa && da) {
        Sr = sr * sa;
        Sg = sg * sa;
        Sb = sb * sa;
        SetSat(&Sr, &Sg, &Sb, Sat(dr, dg, db) * sa);
        SetLum(&Sr, &Sg, &Sb, sa * da, Lum(dr, dg, db) * sa);
    } else {
        Sr = 0;
        Sg = 0;
        Sb = 0;
    }

    int a = srcover_byte(sa, da);
    int r = blendfunc_nonsep_byte(sr, dr, sa, da, Sr);
    int g = blendfunc_nonsep_byte(sg, dg, sa, da, Sg);
    int b = blendfunc_nonsep_byte(sb, db, sa, da, Sb);
    return SkPackARGB32(a, r, g, b);
}




static SkPMColor saturation_modeproc(SkPMColor src, SkPMColor dst) {
    int sr = SkGetPackedR32(src);
    int sg = SkGetPackedG32(src);
    int sb = SkGetPackedB32(src);
    int sa = SkGetPackedA32(src);

    int dr = SkGetPackedR32(dst);
    int dg = SkGetPackedG32(dst);
    int db = SkGetPackedB32(dst);
    int da = SkGetPackedA32(dst);
    int Dr, Dg, Db;

    if(sa && da) {
        Dr = dr * sa;
        Dg = dg * sa;
        Db = db * sa;
        SetSat(&Dr, &Dg, &Db, Sat(sr, sg, sb) * da);
        SetLum(&Dr, &Dg, &Db, sa * da, Lum(dr, dg, db) * sa);
    } else {
        Dr = 0;
        Dg = 0;
        Db = 0;
    }

    int a = srcover_byte(sa, da);
    int r = blendfunc_nonsep_byte(sr, dr, sa, da, Dr);
    int g = blendfunc_nonsep_byte(sg, dg, sa, da, Dg);
    int b = blendfunc_nonsep_byte(sb, db, sa, da, Db);
    return SkPackARGB32(a, r, g, b);
}




static SkPMColor color_modeproc(SkPMColor src, SkPMColor dst) {
    int sr = SkGetPackedR32(src);
    int sg = SkGetPackedG32(src);
    int sb = SkGetPackedB32(src);
    int sa = SkGetPackedA32(src);

    int dr = SkGetPackedR32(dst);
    int dg = SkGetPackedG32(dst);
    int db = SkGetPackedB32(dst);
    int da = SkGetPackedA32(dst);
    int Sr, Sg, Sb;

    if(sa && da) {
        Sr = sr * da;
        Sg = sg * da;
        Sb = sb * da;
        SetLum(&Sr, &Sg, &Sb, sa * da, Lum(dr, dg, db) * sa);
    } else {
        Sr = 0;
        Sg = 0;
        Sb = 0;
    }

    int a = srcover_byte(sa, da);
    int r = blendfunc_nonsep_byte(sr, dr, sa, da, Sr);
    int g = blendfunc_nonsep_byte(sg, dg, sa, da, Sg);
    int b = blendfunc_nonsep_byte(sb, db, sa, da, Sb);
    return SkPackARGB32(a, r, g, b);
}




static SkPMColor luminosity_modeproc(SkPMColor src, SkPMColor dst) {
    int sr = SkGetPackedR32(src);
    int sg = SkGetPackedG32(src);
    int sb = SkGetPackedB32(src);
    int sa = SkGetPackedA32(src);

    int dr = SkGetPackedR32(dst);
    int dg = SkGetPackedG32(dst);
    int db = SkGetPackedB32(dst);
    int da = SkGetPackedA32(dst);
    int Dr, Dg, Db;

    if(sa && da) {
        Dr = dr * sa;
        Dg = dg * sa;
        Db = db * sa;
        SetLum(&Dr, &Dg, &Db, sa * da, Lum(sr, sg, sb) * da);
    } else {
        Dr = 0;
        Dg = 0;
        Db = 0;
    }

    int a = srcover_byte(sa, da);
    int r = blendfunc_nonsep_byte(sr, dr, sa, da, Dr);
    int g = blendfunc_nonsep_byte(sg, dg, sa, da, Dg);
    int b = blendfunc_nonsep_byte(sb, db, sa, da, Db);
    return SkPackARGB32(a, r, g, b);
}

const ProcCoeff gProcCoeffs[] = {
    { clear_modeproc,   SkXfermode::kZero_Coeff,    SkXfermode::kZero_Coeff },
    { src_modeproc,     SkXfermode::kOne_Coeff,     SkXfermode::kZero_Coeff },
    { dst_modeproc,     SkXfermode::kZero_Coeff,    SkXfermode::kOne_Coeff },
    { srcover_modeproc, SkXfermode::kOne_Coeff,     SkXfermode::kISA_Coeff },
    { dstover_modeproc, SkXfermode::kIDA_Coeff,     SkXfermode::kOne_Coeff },
    { srcin_modeproc,   SkXfermode::kDA_Coeff,      SkXfermode::kZero_Coeff },
    { dstin_modeproc,   SkXfermode::kZero_Coeff,    SkXfermode::kSA_Coeff },
    { srcout_modeproc,  SkXfermode::kIDA_Coeff,     SkXfermode::kZero_Coeff },
    { dstout_modeproc,  SkXfermode::kZero_Coeff,    SkXfermode::kISA_Coeff },
    { srcatop_modeproc, SkXfermode::kDA_Coeff,      SkXfermode::kISA_Coeff },
    { dstatop_modeproc, SkXfermode::kIDA_Coeff,     SkXfermode::kSA_Coeff },
    { xor_modeproc,     SkXfermode::kIDA_Coeff,     SkXfermode::kISA_Coeff },

    { plus_modeproc,    SkXfermode::kOne_Coeff,     SkXfermode::kOne_Coeff },
    { modulate_modeproc,SkXfermode::kZero_Coeff,    SkXfermode::kSC_Coeff },
    { screen_modeproc,  SkXfermode::kOne_Coeff,     SkXfermode::kISC_Coeff },
    { overlay_modeproc,     CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { darken_modeproc,      CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { lighten_modeproc,     CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { colordodge_modeproc,  CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { colorburn_modeproc,   CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { hardlight_modeproc,   CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { softlight_modeproc,   CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { difference_modeproc,  CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { exclusion_modeproc,   CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { multiply_modeproc,    CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { hue_modeproc,         CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { saturation_modeproc,  CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { color_modeproc,       CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
    { luminosity_modeproc,  CANNOT_USE_COEFF,       CANNOT_USE_COEFF },
};



bool SkXfermode::asCoeff(Coeff* src, Coeff* dst) const {
    return false;
}

bool SkXfermode::asMode(Mode* mode) const {
    return false;
}

bool SkXfermode::asNewEffect(GrEffect** effect, GrTexture* background) const {
    return false;
}

bool SkXfermode::AsNewEffectOrCoeff(SkXfermode* xfermode,
                                    GrEffect** effect,
                                    Coeff* src,
                                    Coeff* dst,
                                    GrTexture* background) {
    if (NULL == xfermode) {
        return ModeAsCoeff(kSrcOver_Mode, src, dst);
    } else if (xfermode->asCoeff(src, dst)) {
        return true;
    } else {
        return xfermode->asNewEffect(effect, background);
    }
}

SkPMColor SkXfermode::xferColor(SkPMColor src, SkPMColor dst) const{
    
    return dst;
}

void SkXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                        const SkPMColor* SK_RESTRICT src, int count,
                        const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    if (NULL == aa) {
        for (int i = count - 1; i >= 0; --i) {
            dst[i] = this->xferColor(src[i], dst[i]);
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0 != a) {
                SkPMColor dstC = dst[i];
                SkPMColor C = this->xferColor(src[i], dstC);
                if (0xFF != a) {
                    C = SkFourByteInterp(C, dstC, a);
                }
                dst[i] = C;
            }
        }
    }
}

void SkXfermode::xfer16(uint16_t* dst,
                        const SkPMColor* SK_RESTRICT src, int count,
                        const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    if (NULL == aa) {
        for (int i = count - 1; i >= 0; --i) {
            SkPMColor dstC = SkPixel16ToPixel32(dst[i]);
            dst[i] = SkPixel32ToPixel16_ToU16(this->xferColor(src[i], dstC));
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0 != a) {
                SkPMColor dstC = SkPixel16ToPixel32(dst[i]);
                SkPMColor C = this->xferColor(src[i], dstC);
                if (0xFF != a) {
                    C = SkFourByteInterp(C, dstC, a);
                }
                dst[i] = SkPixel32ToPixel16_ToU16(C);
            }
        }
    }
}

void SkXfermode::xferA8(SkAlpha* SK_RESTRICT dst,
                        const SkPMColor src[], int count,
                        const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    if (NULL == aa) {
        for (int i = count - 1; i >= 0; --i) {
            SkPMColor res = this->xferColor(src[i], (dst[i] << SK_A32_SHIFT));
            dst[i] = SkToU8(SkGetPackedA32(res));
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0 != a) {
                SkAlpha dstA = dst[i];
                unsigned A = SkGetPackedA32(this->xferColor(src[i],
                                            (SkPMColor)(dstA << SK_A32_SHIFT)));
                if (0xFF != a) {
                    A = SkAlphaBlend(A, dstA, SkAlpha255To256(a));
                }
                dst[i] = SkToU8(A);
            }
        }
    }
}



#if SK_SUPPORT_GPU

#include "GrEffect.h"
#include "GrCoordTransform.h"
#include "GrEffectUnitTest.h"
#include "GrTBackendEffectFactory.h"
#include "gl/GrGLEffect.h"
#include "gl/GrGLShaderBuilder.h"




class XferEffect : public GrEffect {
public:
    static bool IsSupportedMode(SkXfermode::Mode mode) {
        return mode > SkXfermode::kLastCoeffMode && mode <= SkXfermode::kLastMode;
    }

    static GrEffect* Create(SkXfermode::Mode mode, GrTexture* background) {
        if (!IsSupportedMode(mode)) {
            return NULL;
        } else {
            return SkNEW_ARGS(XferEffect, (mode, background));
        }
    }

    virtual void getConstantColorComponents(GrColor* color,
                                            uint32_t* validFlags) const SK_OVERRIDE {
        *validFlags = 0;
    }

    virtual const GrBackendEffectFactory& getFactory() const SK_OVERRIDE {
        return GrTBackendEffectFactory<XferEffect>::getInstance();
    }

    static const char* Name() { return "XferEffect"; }

    SkXfermode::Mode mode() const { return fMode; }
    const GrTextureAccess&  backgroundAccess() const { return fBackgroundAccess; }

    class GLEffect : public GrGLEffect {
    public:
        GLEffect(const GrBackendEffectFactory& factory, const GrDrawEffect&)
            : GrGLEffect(factory) {
        }
        virtual void emitCode(GrGLShaderBuilder* builder,
                              const GrDrawEffect& drawEffect,
                              const GrEffectKey& key,
                              const char* outputColor,
                              const char* inputColor,
                              const TransformedCoordsArray& coords,
                              const TextureSamplerArray& samplers) SK_OVERRIDE {
            SkXfermode::Mode mode = drawEffect.castEffect<XferEffect>().mode();
            const GrTexture* backgroundTex = drawEffect.castEffect<XferEffect>().backgroundAccess().getTexture();
            const char* dstColor;
            if (backgroundTex) {
                dstColor = "bgColor";
                builder->fsCodeAppendf("\t\tvec4 %s = ", dstColor);
                builder->fsAppendTextureLookup(samplers[0], coords[0].c_str(), coords[0].type());
                builder->fsCodeAppendf(";\n");
            } else {
                dstColor = builder->dstColor();
            }
            SkASSERT(NULL != dstColor);

            
            if (NULL == inputColor) {
                builder->fsCodeAppendf("\t\tconst vec4 ones = vec4(1);\n");
                inputColor = "ones";
            }
            builder->fsCodeAppendf("\t\t// SkXfermode::Mode: %s\n", SkXfermode::ModeName(mode));

            
            builder->fsCodeAppendf("\t\t%s.a = %s.a + (1.0 - %s.a) * %s.a;\n",
                                    outputColor, inputColor, inputColor, dstColor);

            switch (mode) {
                case SkXfermode::kOverlay_Mode:
                    
                    HardLight(builder, outputColor, dstColor, inputColor);
                    break;
                case SkXfermode::kDarken_Mode:
                    builder->fsCodeAppendf("\t\t%s.rgb = min((1.0 - %s.a) * %s.rgb + %s.rgb, "
                                                            "(1.0 - %s.a) * %s.rgb + %s.rgb);\n",
                                            outputColor,
                                            inputColor, dstColor, inputColor,
                                            dstColor, inputColor, dstColor);
                    break;
                case SkXfermode::kLighten_Mode:
                    builder->fsCodeAppendf("\t\t%s.rgb = max((1.0 - %s.a) * %s.rgb + %s.rgb, "
                                                            "(1.0 - %s.a) * %s.rgb + %s.rgb);\n",
                                            outputColor,
                                            inputColor, dstColor, inputColor,
                                            dstColor, inputColor, dstColor);
                    break;
                case SkXfermode::kColorDodge_Mode:
                    ColorDodgeComponent(builder, outputColor, inputColor, dstColor, 'r');
                    ColorDodgeComponent(builder, outputColor, inputColor, dstColor, 'g');
                    ColorDodgeComponent(builder, outputColor, inputColor, dstColor, 'b');
                    break;
                case SkXfermode::kColorBurn_Mode:
                    ColorBurnComponent(builder, outputColor, inputColor, dstColor, 'r');
                    ColorBurnComponent(builder, outputColor, inputColor, dstColor, 'g');
                    ColorBurnComponent(builder, outputColor, inputColor, dstColor, 'b');
                    break;
                case SkXfermode::kHardLight_Mode:
                    HardLight(builder, outputColor, inputColor, dstColor);
                    break;
                case SkXfermode::kSoftLight_Mode:
                    builder->fsCodeAppendf("\t\tif (0.0 == %s.a) {\n", dstColor);
                    builder->fsCodeAppendf("\t\t\t%s.rgba = %s;\n", outputColor, inputColor);
                    builder->fsCodeAppendf("\t\t} else {\n");
                    SoftLightComponentPosDstAlpha(builder, outputColor, inputColor, dstColor, 'r');
                    SoftLightComponentPosDstAlpha(builder, outputColor, inputColor, dstColor, 'g');
                    SoftLightComponentPosDstAlpha(builder, outputColor, inputColor, dstColor, 'b');
                    builder->fsCodeAppendf("\t\t}\n");
                    break;
                case SkXfermode::kDifference_Mode:
                    builder->fsCodeAppendf("\t\t%s.rgb = %s.rgb + %s.rgb -"
                                                       "2.0 * min(%s.rgb * %s.a, %s.rgb * %s.a);\n",
                                           outputColor, inputColor, dstColor, inputColor, dstColor,
                                           dstColor, inputColor);
                    break;
                case SkXfermode::kExclusion_Mode:
                    builder->fsCodeAppendf("\t\t%s.rgb = %s.rgb + %s.rgb - "
                                                        "2.0 * %s.rgb * %s.rgb;\n",
                                           outputColor, dstColor, inputColor, dstColor, inputColor);
                    break;
                case SkXfermode::kMultiply_Mode:
                    builder->fsCodeAppendf("\t\t%s.rgb = (1.0 - %s.a) * %s.rgb + "
                                                        "(1.0 - %s.a) * %s.rgb + "
                                                         "%s.rgb * %s.rgb;\n",
                                           outputColor, inputColor, dstColor, dstColor, inputColor,
                                           inputColor, dstColor);
                    break;
                case SkXfermode::kHue_Mode: {
                    
                    SkString setSat, setLum;
                    AddSatFunction(builder, &setSat);
                    AddLumFunction(builder, &setLum);
                    builder->fsCodeAppendf("\t\tvec4 dstSrcAlpha = %s * %s.a;\n",
                                           dstColor, inputColor);
                    builder->fsCodeAppendf("\t\t%s.rgb = %s(%s(%s.rgb * %s.a, dstSrcAlpha.rgb), dstSrcAlpha.a, dstSrcAlpha.rgb);\n",
                                           outputColor, setLum.c_str(), setSat.c_str(), inputColor,
                                           dstColor);
                    builder->fsCodeAppendf("\t\t%s.rgb += (1.0 - %s.a) * %s.rgb + (1.0 - %s.a) * %s.rgb;\n",
                                           outputColor, inputColor, dstColor, dstColor, inputColor);
                    break;
                }
                case SkXfermode::kSaturation_Mode: {
                    
                    SkString setSat, setLum;
                    AddSatFunction(builder, &setSat);
                    AddLumFunction(builder, &setLum);
                    builder->fsCodeAppendf("\t\tvec4 dstSrcAlpha = %s * %s.a;\n",
                                           dstColor, inputColor);
                    builder->fsCodeAppendf("\t\t%s.rgb = %s(%s(dstSrcAlpha.rgb, %s.rgb * %s.a), dstSrcAlpha.a, dstSrcAlpha.rgb);\n",
                                           outputColor, setLum.c_str(), setSat.c_str(), inputColor,
                                           dstColor);
                    builder->fsCodeAppendf("\t\t%s.rgb += (1.0 - %s.a) * %s.rgb + (1.0 - %s.a) * %s.rgb;\n",
                                           outputColor, inputColor, dstColor, dstColor, inputColor);
                    break;
                }
                case SkXfermode::kColor_Mode: {
                    
                    SkString setLum;
                    AddLumFunction(builder, &setLum);
                    builder->fsCodeAppendf("\t\tvec4 srcDstAlpha = %s * %s.a;\n",
                                           inputColor, dstColor);
                    builder->fsCodeAppendf("\t\t%s.rgb = %s(srcDstAlpha.rgb, srcDstAlpha.a, %s.rgb * %s.a);\n",
                                           outputColor, setLum.c_str(), dstColor, inputColor);
                    builder->fsCodeAppendf("\t\t%s.rgb += (1.0 - %s.a) * %s.rgb + (1.0 - %s.a) * %s.rgb;\n",
                                           outputColor, inputColor, dstColor, dstColor, inputColor);
                    break;
                }
                case SkXfermode::kLuminosity_Mode: {
                    
                    SkString setLum;
                    AddLumFunction(builder, &setLum);
                    builder->fsCodeAppendf("\t\tvec4 srcDstAlpha = %s * %s.a;\n",
                                           inputColor, dstColor);
                    builder->fsCodeAppendf("\t\t%s.rgb = %s(%s.rgb * %s.a, srcDstAlpha.a, srcDstAlpha.rgb);\n",
                                           outputColor, setLum.c_str(), dstColor, inputColor);
                    builder->fsCodeAppendf("\t\t%s.rgb += (1.0 - %s.a) * %s.rgb + (1.0 - %s.a) * %s.rgb;\n",
                                           outputColor, inputColor, dstColor, dstColor, inputColor);
                    break;
                }
                default:
                    SkFAIL("Unknown XferEffect mode.");
                    break;
            }
        }

        static inline void GenKey(const GrDrawEffect& drawEffect, const GrGLCaps&,
                                  GrEffectKeyBuilder* b) {
            
            uint32_t key = drawEffect.effect()->numTextures();
            SkASSERT(key <= 1);
            key |= drawEffect.castEffect<XferEffect>().mode() << 1;
            b->add32(key);
        }

    private:
        static void HardLight(GrGLShaderBuilder* builder,
                              const char* final,
                              const char* src,
                              const char* dst) {
            static const char kComponents[] = {'r', 'g', 'b'};
            for (size_t i = 0; i < SK_ARRAY_COUNT(kComponents); ++i) {
                char component = kComponents[i];
                builder->fsCodeAppendf("\t\tif (2.0 * %s.%c <= %s.a) {\n", src, component, src);
                builder->fsCodeAppendf("\t\t\t%s.%c = 2.0 * %s.%c * %s.%c;\n", final, component, src, component, dst, component);
                builder->fsCodeAppend("\t\t} else {\n");
                builder->fsCodeAppendf("\t\t\t%s.%c = %s.a * %s.a - 2.0 * (%s.a - %s.%c) * (%s.a - %s.%c);\n",
                                       final, component, src, dst, dst, dst, component, src, src, component);
                builder->fsCodeAppend("\t\t}\n");
            }
            builder->fsCodeAppendf("\t\t%s.rgb += %s.rgb * (1.0 - %s.a) + %s.rgb * (1.0 - %s.a);\n",
                                   final, src, dst, dst, src);
        }

        
        static void ColorDodgeComponent(GrGLShaderBuilder* builder,
                                        const char* final,
                                        const char* src,
                                        const char* dst,
                                        const char component) {
            builder->fsCodeAppendf("\t\tif (0.0 == %s.%c) {\n", dst, component);
            builder->fsCodeAppendf("\t\t\t%s.%c = %s.%c * (1.0 - %s.a);\n",
                                   final, component, src, component, dst);
            builder->fsCodeAppend("\t\t} else {\n");
            builder->fsCodeAppendf("\t\t\tfloat d = %s.a - %s.%c;\n", src, src, component);
            builder->fsCodeAppend("\t\t\tif (0.0 == d) {\n");
            builder->fsCodeAppendf("\t\t\t\t%s.%c = %s.a * %s.a + %s.%c * (1.0 - %s.a) + %s.%c * (1.0 - %s.a);\n",
                                   final, component, src, dst, src, component, dst, dst, component,
                                   src);
            builder->fsCodeAppend("\t\t\t} else {\n");
            builder->fsCodeAppendf("\t\t\t\td = min(%s.a, %s.%c * %s.a / d);\n",
                                   dst, dst, component, src);
            builder->fsCodeAppendf("\t\t\t\t%s.%c = d * %s.a + %s.%c * (1.0 - %s.a) + %s.%c * (1.0 - %s.a);\n",
                                   final, component, src, src, component, dst, dst, component, src);
            builder->fsCodeAppend("\t\t\t}\n");
            builder->fsCodeAppend("\t\t}\n");
        }

        
        static void ColorBurnComponent(GrGLShaderBuilder* builder,
                                       const char* final,
                                       const char* src,
                                       const char* dst,
                                       const char component) {
            builder->fsCodeAppendf("\t\tif (%s.a == %s.%c) {\n", dst, dst, component);
            builder->fsCodeAppendf("\t\t\t%s.%c = %s.a * %s.a + %s.%c * (1.0 - %s.a) + %s.%c * (1.0 - %s.a);\n",
                                   final, component, src, dst, src, component, dst, dst, component,
                                   src);
            builder->fsCodeAppendf("\t\t} else if (0.0 == %s.%c) {\n", src, component);
            builder->fsCodeAppendf("\t\t\t%s.%c = %s.%c * (1.0 - %s.a);\n",
                                   final, component, dst, component, src);
            builder->fsCodeAppend("\t\t} else {\n");
            builder->fsCodeAppendf("\t\t\tfloat d = max(0.0, %s.a - (%s.a - %s.%c) * %s.a / %s.%c);\n",
                                   dst, dst, dst, component, src, src, component);
            builder->fsCodeAppendf("\t\t\t%s.%c = %s.a * d + %s.%c * (1.0 - %s.a) + %s.%c * (1.0 - %s.a);\n",
                                   final, component, src, src, component, dst, dst, component, src);
            builder->fsCodeAppend("\t\t}\n");
        }

        
        static void SoftLightComponentPosDstAlpha(GrGLShaderBuilder* builder,
                                                  const char* final,
                                                  const char* src,
                                                  const char* dst,
                                                  const char component) {
            
            builder->fsCodeAppendf("\t\t\tif (2.0 * %s.%c <= %s.a) {\n", src, component, src);
            
            builder->fsCodeAppendf("\t\t\t\t%s.%c = (%s.%c*%s.%c*(%s.a - 2.0*%s.%c)) / %s.a + (1.0 - %s.a) * %s.%c + %s.%c*(-%s.a + 2.0*%s.%c + 1.0);\n",
                                   final, component, dst, component, dst, component, src, src,
                                   component, dst, dst, src, component, dst, component, src, src,
                                   component);
            
            builder->fsCodeAppendf("\t\t\t} else if (4.0 * %s.%c <= %s.a) {\n",
                                   dst, component, dst);
            builder->fsCodeAppendf("\t\t\t\tfloat DSqd = %s.%c * %s.%c;\n",
                                   dst, component, dst, component);
            builder->fsCodeAppendf("\t\t\t\tfloat DCub = DSqd * %s.%c;\n", dst, component);
            builder->fsCodeAppendf("\t\t\t\tfloat DaSqd = %s.a * %s.a;\n", dst, dst);
            builder->fsCodeAppendf("\t\t\t\tfloat DaCub = DaSqd * %s.a;\n", dst);
            
            builder->fsCodeAppendf("\t\t\t\t%s.%c = (-DaCub*%s.%c + DaSqd*(%s.%c - %s.%c * (3.0*%s.a - 6.0*%s.%c - 1.0)) + 12.0*%s.a*DSqd*(%s.a - 2.0*%s.%c) - 16.0*DCub * (%s.a - 2.0*%s.%c)) / DaSqd;\n",
                                   final, component, src, component, src, component, dst, component,
                                   src, src, component, dst, src, src, component, src, src,
                                   component);
            builder->fsCodeAppendf("\t\t\t} else {\n");
            
            builder->fsCodeAppendf("\t\t\t\t%s.%c = -sqrt(%s.a*%s.%c)*(%s.a - 2.0*%s.%c) - %s.a*%s.%c + %s.%c*(%s.a - 2.0*%s.%c + 1.0) + %s.%c;\n",
                                    final, component, dst, dst, component, src, src, component, dst,
                                    src, component, dst, component, src, src, component, src,
                                    component);
            builder->fsCodeAppendf("\t\t\t}\n");
        }

        
        
        
        
        static void AddLumFunction(GrGLShaderBuilder* builder, SkString* setLumFunction) {
            
            SkString getFunction;
            GrGLShaderVar getLumArgs[] = {
                GrGLShaderVar("color", kVec3f_GrSLType),
            };
            SkString getLumBody("\treturn dot(vec3(0.3, 0.59, 0.11), color);\n");
            builder->fsEmitFunction(kFloat_GrSLType,
                                    "luminance",
                                    SK_ARRAY_COUNT(getLumArgs), getLumArgs,
                                    getLumBody.c_str(),
                                    &getFunction);

            
            GrGLShaderVar setLumArgs[] = {
                GrGLShaderVar("hueSat", kVec3f_GrSLType),
                GrGLShaderVar("alpha", kFloat_GrSLType),
                GrGLShaderVar("lumColor", kVec3f_GrSLType),
            };
            SkString setLumBody;
            setLumBody.printf("\tfloat diff = %s(lumColor - hueSat);\n", getFunction.c_str());
            setLumBody.append("\tvec3 outColor = hueSat + diff;\n");
            setLumBody.appendf("\tfloat outLum = %s(outColor);\n", getFunction.c_str());
            setLumBody.append("\tfloat minComp = min(min(outColor.r, outColor.g), outColor.b);\n"
                              "\tfloat maxComp = max(max(outColor.r, outColor.g), outColor.b);\n"
                              "\tif (minComp < 0.0) {\n"
                              "\t\toutColor = outLum + ((outColor - vec3(outLum, outLum, outLum)) * outLum) / (outLum - minComp);\n"
                              "\t}\n"
                              "\tif (maxComp > alpha) {\n"
                              "\t\toutColor = outLum + ((outColor - vec3(outLum, outLum, outLum)) * (alpha - outLum)) / (maxComp - outLum);\n"
                              "\t}\n"
                              "\treturn outColor;\n");
            builder->fsEmitFunction(kVec3f_GrSLType,
                                    "set_luminance",
                                    SK_ARRAY_COUNT(setLumArgs), setLumArgs,
                                    setLumBody.c_str(),
                                    setLumFunction);
        }

        
        
        
        static void AddSatFunction(GrGLShaderBuilder* builder, SkString* setSatFunction) {
            
            SkString getFunction;
            GrGLShaderVar getSatArgs[] = { GrGLShaderVar("color", kVec3f_GrSLType) };
            SkString getSatBody;
            getSatBody.printf("\treturn max(max(color.r, color.g), color.b) - "
                              "min(min(color.r, color.g), color.b);\n");
            builder->fsEmitFunction(kFloat_GrSLType,
                                    "saturation",
                                    SK_ARRAY_COUNT(getSatArgs), getSatArgs,
                                    getSatBody.c_str(),
                                    &getFunction);

            
            
            
            
            SkString helperFunction;
            GrGLShaderVar helperArgs[] = {
                GrGLShaderVar("minComp", kFloat_GrSLType),
                GrGLShaderVar("midComp", kFloat_GrSLType),
                GrGLShaderVar("maxComp", kFloat_GrSLType),
                GrGLShaderVar("sat", kFloat_GrSLType),
            };
            static const char kHelperBody[] = "\tif (minComp < maxComp) {\n"
                                              "\t\tvec3 result;\n"
                                              "\t\tresult.r = 0.0;\n"
                                              "\t\tresult.g = sat * (midComp - minComp) / (maxComp - minComp);\n"
                                              "\t\tresult.b = sat;\n"
                                              "\t\treturn result;\n"
                                              "\t} else {\n"
                                              "\t\treturn vec3(0, 0, 0);\n"
                                              "\t}\n";
            builder->fsEmitFunction(kVec3f_GrSLType,
                                    "set_saturation_helper",
                                    SK_ARRAY_COUNT(helperArgs), helperArgs,
                                    kHelperBody,
                                    &helperFunction);

            GrGLShaderVar setSatArgs[] = {
                GrGLShaderVar("hueLumColor", kVec3f_GrSLType),
                GrGLShaderVar("satColor", kVec3f_GrSLType),
            };
            const char* helpFunc = helperFunction.c_str();
            SkString setSatBody;
            setSatBody.appendf("\tfloat sat = %s(satColor);\n"
                               "\tif (hueLumColor.r <= hueLumColor.g) {\n"
                               "\t\tif (hueLumColor.g <= hueLumColor.b) {\n"
                               "\t\t\thueLumColor.rgb = %s(hueLumColor.r, hueLumColor.g, hueLumColor.b, sat);\n"
                               "\t\t} else if (hueLumColor.r <= hueLumColor.b) {\n"
                               "\t\t\thueLumColor.rbg = %s(hueLumColor.r, hueLumColor.b, hueLumColor.g, sat);\n"
                               "\t\t} else {\n"
                               "\t\t\thueLumColor.brg = %s(hueLumColor.b, hueLumColor.r, hueLumColor.g, sat);\n"
                               "\t\t}\n"
                               "\t} else if (hueLumColor.r <= hueLumColor.b) {\n"
                               "\t\thueLumColor.grb = %s(hueLumColor.g, hueLumColor.r, hueLumColor.b, sat);\n"
                               "\t} else if (hueLumColor.g <= hueLumColor.b) {\n"
                               "\t\thueLumColor.gbr = %s(hueLumColor.g, hueLumColor.b, hueLumColor.r, sat);\n"
                               "\t} else {\n"
                               "\t\thueLumColor.bgr = %s(hueLumColor.b, hueLumColor.g, hueLumColor.r, sat);\n"
                               "\t}\n"
                               "\treturn hueLumColor;\n",
                               getFunction.c_str(), helpFunc, helpFunc, helpFunc, helpFunc,
                               helpFunc, helpFunc);
            builder->fsEmitFunction(kVec3f_GrSLType,
                                    "set_saturation",
                                    SK_ARRAY_COUNT(setSatArgs), setSatArgs,
                                    setSatBody.c_str(),
                                    setSatFunction);

        }

        typedef GrGLEffect INHERITED;
    };

    GR_DECLARE_EFFECT_TEST;

private:
    XferEffect(SkXfermode::Mode mode, GrTexture* background)
        : fMode(mode) {
        if (background) {
            fBackgroundTransform.reset(kLocal_GrCoordSet, background);
            this->addCoordTransform(&fBackgroundTransform);
            fBackgroundAccess.reset(background);
            this->addTextureAccess(&fBackgroundAccess);
        } else {
            this->setWillReadDstColor();
        }
    }
    virtual bool onIsEqual(const GrEffect& other) const SK_OVERRIDE {
        const XferEffect& s = CastEffect<XferEffect>(other);
        return fMode == s.fMode &&
               fBackgroundAccess.getTexture() == s.fBackgroundAccess.getTexture();
    }

    SkXfermode::Mode fMode;
    GrCoordTransform fBackgroundTransform;
    GrTextureAccess  fBackgroundAccess;

    typedef GrEffect INHERITED;
};

GR_DEFINE_EFFECT_TEST(XferEffect);
GrEffect* XferEffect::TestCreate(SkRandom* rand,
                                 GrContext*,
                                 const GrDrawTargetCaps&,
                                 GrTexture*[]) {
    int mode = rand->nextRangeU(SkXfermode::kLastCoeffMode + 1, SkXfermode::kLastSeparableMode);

    return SkNEW_ARGS(XferEffect, (static_cast<SkXfermode::Mode>(mode), NULL));
}

#endif




SkProcCoeffXfermode::SkProcCoeffXfermode(SkReadBuffer& buffer) : INHERITED(buffer) {
    uint32_t mode32 = buffer.read32() % SK_ARRAY_COUNT(gProcCoeffs);
    if (mode32 >= SK_ARRAY_COUNT(gProcCoeffs)) {
        
        mode32 = SkXfermode::kSrcOut_Mode;
    }
    fMode = (SkXfermode::Mode)mode32;

    const ProcCoeff& rec = gProcCoeffs[fMode];
    fProc = rec.fProc;
    
    fSrcCoeff = rec.fSC;
    fDstCoeff = rec.fDC;
}

bool SkProcCoeffXfermode::asMode(Mode* mode) const {
    if (mode) {
        *mode = fMode;
    }
    return true;
}

bool SkProcCoeffXfermode::asCoeff(Coeff* sc, Coeff* dc) const {
    if (CANNOT_USE_COEFF == fSrcCoeff) {
        return false;
    }

    if (sc) {
        *sc = fSrcCoeff;
    }
    if (dc) {
        *dc = fDstCoeff;
    }
    return true;
}

void SkProcCoeffXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src, int count,
                                 const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    SkXfermodeProc proc = fProc;

    if (NULL != proc) {
        if (NULL == aa) {
            for (int i = count - 1; i >= 0; --i) {
                dst[i] = proc(src[i], dst[i]);
            }
        } else {
            for (int i = count - 1; i >= 0; --i) {
                unsigned a = aa[i];
                if (0 != a) {
                    SkPMColor dstC = dst[i];
                    SkPMColor C = proc(src[i], dstC);
                    if (a != 0xFF) {
                        C = SkFourByteInterp(C, dstC, a);
                    }
                    dst[i] = C;
                }
            }
        }
    }
}

void SkProcCoeffXfermode::xfer16(uint16_t* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src, int count,
                                 const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    SkXfermodeProc proc = fProc;

    if (NULL != proc) {
        if (NULL == aa) {
            for (int i = count - 1; i >= 0; --i) {
                SkPMColor dstC = SkPixel16ToPixel32(dst[i]);
                dst[i] = SkPixel32ToPixel16_ToU16(proc(src[i], dstC));
            }
        } else {
            for (int i = count - 1; i >= 0; --i) {
                unsigned a = aa[i];
                if (0 != a) {
                    SkPMColor dstC = SkPixel16ToPixel32(dst[i]);
                    SkPMColor C = proc(src[i], dstC);
                    if (0xFF != a) {
                        C = SkFourByteInterp(C, dstC, a);
                    }
                    dst[i] = SkPixel32ToPixel16_ToU16(C);
                }
            }
        }
    }
}

void SkProcCoeffXfermode::xferA8(SkAlpha* SK_RESTRICT dst,
                                 const SkPMColor* SK_RESTRICT src, int count,
                                 const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    SkXfermodeProc proc = fProc;

    if (NULL != proc) {
        if (NULL == aa) {
            for (int i = count - 1; i >= 0; --i) {
                SkPMColor res = proc(src[i], dst[i] << SK_A32_SHIFT);
                dst[i] = SkToU8(SkGetPackedA32(res));
            }
        } else {
            for (int i = count - 1; i >= 0; --i) {
                unsigned a = aa[i];
                if (0 != a) {
                    SkAlpha dstA = dst[i];
                    SkPMColor res = proc(src[i], dstA << SK_A32_SHIFT);
                    unsigned A = SkGetPackedA32(res);
                    if (0xFF != a) {
                        A = SkAlphaBlend(A, dstA, SkAlpha255To256(a));
                    }
                    dst[i] = SkToU8(A);
                }
            }
        }
    }
}

#if SK_SUPPORT_GPU
bool SkProcCoeffXfermode::asNewEffect(GrEffect** effect, GrTexture* background) const {
    if (XferEffect::IsSupportedMode(fMode)) {
        if (NULL != effect) {
            *effect = XferEffect::Create(fMode, background);
            SkASSERT(NULL != *effect);
        }
        return true;
    }
    return false;
}
#endif

void SkProcCoeffXfermode::flatten(SkWriteBuffer& buffer) const {
    this->INHERITED::flatten(buffer);
    buffer.write32(fMode);
}

const char* SkXfermode::ModeName(Mode mode) {
    SkASSERT((unsigned) mode <= (unsigned)kLastMode);
    const char* gModeStrings[] = {
        "Clear", "Src", "Dst", "SrcOver", "DstOver", "SrcIn", "DstIn",
        "SrcOut", "DstOut", "SrcATop", "DstATop", "Xor", "Plus",
        "Modulate", "Screen", "Overlay", "Darken", "Lighten", "ColorDodge",
        "ColorBurn", "HardLight", "SoftLight", "Difference", "Exclusion",
        "Multiply", "Hue", "Saturation", "Color",  "Luminosity"
    };
    return gModeStrings[mode];
    SK_COMPILE_ASSERT(SK_ARRAY_COUNT(gModeStrings) == kLastMode + 1, mode_count);
}

#ifndef SK_IGNORE_TO_STRING
void SkProcCoeffXfermode::toString(SkString* str) const {
    str->append("SkProcCoeffXfermode: ");

    str->append("mode: ");
    str->append(ModeName(fMode));

    static const char* gCoeffStrings[kCoeffCount] = {
        "Zero", "One", "SC", "ISC", "DC", "IDC", "SA", "ISA", "DA", "IDA"
    };

    str->append(" src: ");
    if (CANNOT_USE_COEFF == fSrcCoeff) {
        str->append("can't use");
    } else {
        str->append(gCoeffStrings[fSrcCoeff]);
    }

    str->append(" dst: ");
    if (CANNOT_USE_COEFF == fDstCoeff) {
        str->append("can't use");
    } else {
        str->append(gCoeffStrings[fDstCoeff]);
    }
}
#endif



class SkClearXfermode : public SkProcCoeffXfermode {
public:
    static SkClearXfermode* Create(const ProcCoeff& rec) {
        return SkNEW_ARGS(SkClearXfermode, (rec));
    }

    virtual void xfer32(SkPMColor*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;
    virtual void xferA8(SkAlpha*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkClearXfermode)

private:
    SkClearXfermode(const ProcCoeff& rec) : SkProcCoeffXfermode(rec, kClear_Mode) {}
    SkClearXfermode(SkReadBuffer& buffer)
        : SkProcCoeffXfermode(buffer) {}

    typedef SkProcCoeffXfermode INHERITED;
};

void SkClearXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                             const SkPMColor* SK_RESTRICT, int count,
                             const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && count >= 0);

    if (NULL == aa) {
        memset(dst, 0, count << 2);
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0xFF == a) {
                dst[i] = 0;
            } else if (a != 0) {
                dst[i] = SkAlphaMulQ(dst[i], SkAlpha255To256(255 - a));
            }
        }
    }
}
void SkClearXfermode::xferA8(SkAlpha* SK_RESTRICT dst,
                             const SkPMColor* SK_RESTRICT, int count,
                             const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && count >= 0);

    if (NULL == aa) {
        memset(dst, 0, count);
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0xFF == a) {
                dst[i] = 0;
            } else if (0 != a) {
                dst[i] = SkAlphaMulAlpha(dst[i], 255 - a);
            }
        }
    }
}

#ifndef SK_IGNORE_TO_STRING
void SkClearXfermode::toString(SkString* str) const {
    this->INHERITED::toString(str);
}
#endif



class SkSrcXfermode : public SkProcCoeffXfermode {
public:
    static SkSrcXfermode* Create(const ProcCoeff& rec) {
        return SkNEW_ARGS(SkSrcXfermode, (rec));
    }

    virtual void xfer32(SkPMColor*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;
    virtual void xferA8(SkAlpha*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkSrcXfermode)

private:
    SkSrcXfermode(const ProcCoeff& rec) : SkProcCoeffXfermode(rec, kSrc_Mode) {}
    SkSrcXfermode(SkReadBuffer& buffer)
        : SkProcCoeffXfermode(buffer) {}

    typedef SkProcCoeffXfermode INHERITED;
};

void SkSrcXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src, int count,
                           const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    if (NULL == aa) {
        memcpy(dst, src, count << 2);
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (a == 0xFF) {
                dst[i] = src[i];
            } else if (a != 0) {
                dst[i] = SkFourByteInterp(src[i], dst[i], a);
            }
        }
    }
}

void SkSrcXfermode::xferA8(SkAlpha* SK_RESTRICT dst,
                           const SkPMColor* SK_RESTRICT src, int count,
                           const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src && count >= 0);

    if (NULL == aa) {
        for (int i = count - 1; i >= 0; --i) {
            dst[i] = SkToU8(SkGetPackedA32(src[i]));
        }
    } else {
        for (int i = count - 1; i >= 0; --i) {
            unsigned a = aa[i];
            if (0 != a) {
                unsigned srcA = SkGetPackedA32(src[i]);
                if (a == 0xFF) {
                    dst[i] = SkToU8(srcA);
                } else {
                    dst[i] = SkToU8(SkAlphaBlend(srcA, dst[i], a));
                }
            }
        }
    }
}
#ifndef SK_IGNORE_TO_STRING
void SkSrcXfermode::toString(SkString* str) const {
    this->INHERITED::toString(str);
}
#endif



class SkDstInXfermode : public SkProcCoeffXfermode {
public:
    static SkDstInXfermode* Create(const ProcCoeff& rec) {
        return SkNEW_ARGS(SkDstInXfermode, (rec));
    }

    virtual void xfer32(SkPMColor*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDstInXfermode)

private:
    SkDstInXfermode(const ProcCoeff& rec) : SkProcCoeffXfermode(rec, kDstIn_Mode) {}
    SkDstInXfermode(SkReadBuffer& buffer) : INHERITED(buffer) {}

    typedef SkProcCoeffXfermode INHERITED;
};

void SkDstInXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                             const SkPMColor* SK_RESTRICT src, int count,
                             const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src);

    if (count <= 0) {
        return;
    }
    if (NULL != aa) {
        return this->INHERITED::xfer32(dst, src, count, aa);
    }

    do {
        unsigned a = SkGetPackedA32(*src);
        *dst = SkAlphaMulQ(*dst, SkAlpha255To256(a));
        dst++;
        src++;
    } while (--count != 0);
}

#ifndef SK_IGNORE_TO_STRING
void SkDstInXfermode::toString(SkString* str) const {
    this->INHERITED::toString(str);
}
#endif



class SkDstOutXfermode : public SkProcCoeffXfermode {
public:
    static SkDstOutXfermode* Create(const ProcCoeff& rec) {
        return SkNEW_ARGS(SkDstOutXfermode, (rec));
    }

    virtual void xfer32(SkPMColor*, const SkPMColor*, int, const SkAlpha*) const SK_OVERRIDE;

    SK_TO_STRING_OVERRIDE()
    SK_DECLARE_PUBLIC_FLATTENABLE_DESERIALIZATION_PROCS(SkDstOutXfermode)

private:
    SkDstOutXfermode(const ProcCoeff& rec) : SkProcCoeffXfermode(rec, kDstOut_Mode) {}
    SkDstOutXfermode(SkReadBuffer& buffer)
        : INHERITED(buffer) {}

    typedef SkProcCoeffXfermode INHERITED;
};

void SkDstOutXfermode::xfer32(SkPMColor* SK_RESTRICT dst,
                              const SkPMColor* SK_RESTRICT src, int count,
                              const SkAlpha* SK_RESTRICT aa) const {
    SkASSERT(dst && src);

    if (count <= 0) {
        return;
    }
    if (NULL != aa) {
        return this->INHERITED::xfer32(dst, src, count, aa);
    }

    do {
        unsigned a = SkGetPackedA32(*src);
        *dst = SkAlphaMulQ(*dst, SkAlpha255To256(255 - a));
        dst++;
        src++;
    } while (--count != 0);
}

#ifndef SK_IGNORE_TO_STRING
void SkDstOutXfermode::toString(SkString* str) const {
    this->INHERITED::toString(str);
}
#endif



extern SkProcCoeffXfermode* SkPlatformXfermodeFactory(const ProcCoeff& rec, SkXfermode::Mode mode);
extern SkXfermodeProc SkPlatformXfermodeProcFactory(SkXfermode::Mode mode);


namespace {
SkXfermode* create_mode(int iMode) {
    SkXfermode::Mode mode = (SkXfermode::Mode)iMode;

    ProcCoeff rec = gProcCoeffs[mode];
    SkXfermodeProc pp = SkPlatformXfermodeProcFactory(mode);
    if (pp != NULL) {
        rec.fProc = pp;
    }

    SkXfermode* xfer = NULL;
    
    SkProcCoeffXfermode* xfm = SkPlatformXfermodeFactory(rec, mode);
    if (xfm != NULL) {
        xfer = xfm;
    } else {
        
        
        
        switch (mode) {
            case SkXfermode::kClear_Mode:
                xfer = SkClearXfermode::Create(rec);
                break;
            case SkXfermode::kSrc_Mode:
                xfer = SkSrcXfermode::Create(rec);
                break;
            case SkXfermode::kSrcOver_Mode:
                SkASSERT(false);    
                break;
            case SkXfermode::kDstIn_Mode:
                xfer = SkDstInXfermode::Create(rec);
                break;
            case SkXfermode::kDstOut_Mode:
                xfer = SkDstOutXfermode::Create(rec);
                break;
            default:
                
                xfer = SkProcCoeffXfermode::Create(rec, mode);
                break;
        }
    }
    return xfer;
}
}  


SkXfermode* SkXfermode::Create(Mode mode) {
    SkASSERT(SK_ARRAY_COUNT(gProcCoeffs) == kModeCount);

    if ((unsigned)mode >= kModeCount) {
        
        return NULL;
    }

    
    
    if (kSrcOver_Mode == mode) {
        return NULL;
    }

    SK_DECLARE_STATIC_LAZY_PTR_ARRAY(SkXfermode, cached, kModeCount, create_mode);
    return SkSafeRef(cached[mode]);
}

SkXfermodeProc SkXfermode::GetProc(Mode mode) {
    SkXfermodeProc  proc = NULL;
    if ((unsigned)mode < kModeCount) {
        proc = gProcCoeffs[mode].fProc;
    }
    return proc;
}

bool SkXfermode::ModeAsCoeff(Mode mode, Coeff* src, Coeff* dst) {
    SkASSERT(SK_ARRAY_COUNT(gProcCoeffs) == kModeCount);

    if ((unsigned)mode >= (unsigned)kModeCount) {
        
        return false;
    }

    const ProcCoeff& rec = gProcCoeffs[mode];

    if (CANNOT_USE_COEFF == rec.fSC) {
        return false;
    }

    SkASSERT(CANNOT_USE_COEFF != rec.fDC);
    if (src) {
        *src = rec.fSC;
    }
    if (dst) {
        *dst = rec.fDC;
    }
    return true;
}

bool SkXfermode::AsMode(const SkXfermode* xfer, Mode* mode) {
    if (NULL == xfer) {
        if (mode) {
            *mode = kSrcOver_Mode;
        }
        return true;
    }
    return xfer->asMode(mode);
}

bool SkXfermode::AsCoeff(const SkXfermode* xfer, Coeff* src, Coeff* dst) {
    if (NULL == xfer) {
        return ModeAsCoeff(kSrcOver_Mode, src, dst);
    }
    return xfer->asCoeff(src, dst);
}

bool SkXfermode::IsMode(const SkXfermode* xfer, Mode mode) {
    
    Mode m = kSrcOver_Mode;
    if (xfer && !xfer->asMode(&m)) {
        return false;
    }
    return mode == m;
}




#ifdef SK_DEBUG
static bool require_255(SkPMColor src) { return SkGetPackedA32(src) == 0xFF; }
static bool require_0(SkPMColor src) { return SkGetPackedA32(src) == 0; }
#endif

static uint16_t src_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return SkPixel32ToPixel16(src);
}

static uint16_t dst_modeproc16(SkPMColor src, uint16_t dst) {
    return dst;
}

static uint16_t srcover_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return dst;
}

static uint16_t srcover_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return SkPixel32ToPixel16(src);
}

static uint16_t dstover_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return dst;
}

static uint16_t dstover_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return dst;
}

static uint16_t srcin_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return SkPixel32ToPixel16(src);
}

static uint16_t dstin_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return dst;
}

static uint16_t dstout_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return dst;
}

static uint16_t srcatop_modeproc16(SkPMColor src, uint16_t dst) {
    unsigned isa = 255 - SkGetPackedA32(src);

    return SkPackRGB16(
           SkPacked32ToR16(src) + SkAlphaMulAlpha(SkGetPackedR16(dst), isa),
           SkPacked32ToG16(src) + SkAlphaMulAlpha(SkGetPackedG16(dst), isa),
           SkPacked32ToB16(src) + SkAlphaMulAlpha(SkGetPackedB16(dst), isa));
}

static uint16_t srcatop_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return dst;
}

static uint16_t srcatop_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return SkPixel32ToPixel16(src);
}

static uint16_t dstatop_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    return dst;
}
















static uint16_t darken_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return 0;
}

static uint16_t darken_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    unsigned r = SkFastMin32(SkPacked32ToR16(src), SkGetPackedR16(dst));
    unsigned g = SkFastMin32(SkPacked32ToG16(src), SkGetPackedG16(dst));
    unsigned b = SkFastMin32(SkPacked32ToB16(src), SkGetPackedB16(dst));
    return SkPackRGB16(r, g, b);
}

static uint16_t lighten_modeproc16_0(SkPMColor src, uint16_t dst) {
    SkASSERT(require_0(src));
    return dst;
}

static uint16_t lighten_modeproc16_255(SkPMColor src, uint16_t dst) {
    SkASSERT(require_255(src));
    unsigned r = SkMax32(SkPacked32ToR16(src), SkGetPackedR16(dst));
    unsigned g = SkMax32(SkPacked32ToG16(src), SkGetPackedG16(dst));
    unsigned b = SkMax32(SkPacked32ToB16(src), SkGetPackedB16(dst));
    return SkPackRGB16(r, g, b);
}

struct Proc16Rec {
    SkXfermodeProc16    fProc16_0;
    SkXfermodeProc16    fProc16_255;
    SkXfermodeProc16    fProc16_General;
};

static const Proc16Rec gModeProcs16[] = {
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 src_modeproc16_255,     NULL            },
    { dst_modeproc16,       dst_modeproc16,         dst_modeproc16  },
    { srcover_modeproc16_0, srcover_modeproc16_255, NULL            },
    { dstover_modeproc16_0, dstover_modeproc16_255, NULL            },
    { NULL,                 srcin_modeproc16_255,   NULL            },
    { NULL,                 dstin_modeproc16_255,   NULL            },
    { NULL,                 NULL,                   NULL            },
    { dstout_modeproc16_0,  NULL,                   NULL            },
    { srcatop_modeproc16_0, srcatop_modeproc16_255, srcatop_modeproc16  },
    { NULL,                 dstatop_modeproc16_255, NULL            },
    { NULL,                 NULL,                   NULL            }, 

    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { darken_modeproc16_0,  darken_modeproc16_255,  NULL            }, 
    { lighten_modeproc16_0, lighten_modeproc16_255, NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
    { NULL,                 NULL,                   NULL            }, 
};

SkXfermodeProc16 SkXfermode::GetProc16(Mode mode, SkColor srcColor) {
    SkXfermodeProc16  proc16 = NULL;
    if ((unsigned)mode < kModeCount) {
        const Proc16Rec& rec = gModeProcs16[mode];
        unsigned a = SkColorGetA(srcColor);

        if (0 == a) {
            proc16 = rec.fProc16_0;
        } else if (255 == a) {
            proc16 = rec.fProc16_255;
        } else {
            proc16 = rec.fProc16_General;
        }
    }
    return proc16;
}

SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_START(SkXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkProcCoeffXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkClearXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkSrcXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDstInXfermode)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkDstOutXfermode)
#if !SK_ARM_NEON_IS_NONE
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkNEONProcCoeffXfermode)
#endif
#if defined(SK_CPU_X86) && !defined(SK_BUILD_FOR_IOS)
    SK_DEFINE_FLATTENABLE_REGISTRAR_ENTRY(SkSSE2ProcCoeffXfermode)
#endif
SK_DEFINE_FLATTENABLE_REGISTRAR_GROUP_END
