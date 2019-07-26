





#include "SkBitmapProcState.h"
#include "SkPerspIter.h"
#include "SkShader.h"
#include "SkUtils.h"
#include "SkUtilsArm.h"
#include "SkBitmapProcState_utils.h"








static inline int sk_int_mod(int x, int n) {
    SkASSERT(n > 0);
    if ((unsigned)x >= (unsigned)n) {
        if (x < 0) {
            x = n + ~(~x % n);
        } else {
            x = x % n;
        }
    }
    return x;
}

void decal_nofilter_scale(uint32_t dst[], SkFixed fx, SkFixed dx, int count);
void decal_filter_scale(uint32_t dst[], SkFixed fx, SkFixed dx, int count);


#if !SK_ARM_NEON_IS_NONE


extern const SkBitmapProcState::MatrixProc ClampX_ClampY_Procs_neon[];
extern const SkBitmapProcState::MatrixProc RepeatX_RepeatY_Procs_neon[];

#endif 


#if !SK_ARM_NEON_IS_ALWAYS
#define MAKENAME(suffix)        ClampX_ClampY ## suffix
#define TILEX_PROCF(fx, max)    SkClampMax((fx) >> 16, max)
#define TILEY_PROCF(fy, max)    SkClampMax((fy) >> 16, max)
#define TILEX_LOW_BITS(fx, max) (((fx) >> 12) & 0xF)
#define TILEY_LOW_BITS(fy, max) (((fy) >> 12) & 0xF)
#define CHECK_FOR_DECAL
#include "SkBitmapProcState_matrix.h"

#define MAKENAME(suffix)        RepeatX_RepeatY ## suffix
#define TILEX_PROCF(fx, max)    SK_USHIFT16(((fx) & 0xFFFF) * ((max) + 1))
#define TILEY_PROCF(fy, max)    SK_USHIFT16(((fy) & 0xFFFF) * ((max) + 1))
#define TILEX_LOW_BITS(fx, max) ((((fx) & 0xFFFF) * ((max) + 1) >> 12) & 0xF)
#define TILEY_LOW_BITS(fy, max) ((((fy) & 0xFFFF) * ((max) + 1) >> 12) & 0xF)
#include "SkBitmapProcState_matrix.h"
#endif

#define MAKENAME(suffix)        GeneralXY ## suffix
#define PREAMBLE(state)         SkBitmapProcState::FixedTileProc tileProcX = (state).fTileProcX; (void) tileProcX; \
                                SkBitmapProcState::FixedTileProc tileProcY = (state).fTileProcY; (void) tileProcY; \
                                SkBitmapProcState::FixedTileLowBitsProc tileLowBitsProcX = (state).fTileLowBitsProcX; (void) tileLowBitsProcX; \
                                SkBitmapProcState::FixedTileLowBitsProc tileLowBitsProcY = (state).fTileLowBitsProcY; (void) tileLowBitsProcY
#define PREAMBLE_PARAM_X        , SkBitmapProcState::FixedTileProc tileProcX, SkBitmapProcState::FixedTileLowBitsProc tileLowBitsProcX
#define PREAMBLE_PARAM_Y        , SkBitmapProcState::FixedTileProc tileProcY, SkBitmapProcState::FixedTileLowBitsProc tileLowBitsProcY
#define PREAMBLE_ARG_X          , tileProcX, tileLowBitsProcX
#define PREAMBLE_ARG_Y          , tileProcY, tileLowBitsProcY
#define TILEX_PROCF(fx, max)    SK_USHIFT16(tileProcX(fx) * ((max) + 1))
#define TILEY_PROCF(fy, max)    SK_USHIFT16(tileProcY(fy) * ((max) + 1))
#define TILEX_LOW_BITS(fx, max) tileLowBitsProcX(fx, (max) + 1)
#define TILEY_LOW_BITS(fy, max) tileLowBitsProcY(fy, (max) + 1)
#include "SkBitmapProcState_matrix.h"

static inline U16CPU fixed_clamp(SkFixed x)
{
    if (x < 0) {
        x = 0;
    }
    if (x >> 16) {
        x = 0xFFFF;
    }
    return x;
}

static inline U16CPU fixed_repeat(SkFixed x)
{
    return x & 0xFFFF;
}



#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", off)
#endif

static inline U16CPU fixed_mirror(SkFixed x)
{
    SkFixed s = x << 15 >> 31;
    
    return (x ^ s) & 0xFFFF;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
#pragma optimize("", on)
#endif

static SkBitmapProcState::FixedTileProc choose_tile_proc(unsigned m)
{
    if (SkShader::kClamp_TileMode == m)
        return fixed_clamp;
    if (SkShader::kRepeat_TileMode == m)
        return fixed_repeat;
    SkASSERT(SkShader::kMirror_TileMode == m);
    return fixed_mirror;
}

static inline U16CPU fixed_clamp_lowbits(SkFixed x, int) {
    return (x >> 12) & 0xF;
}

static inline U16CPU fixed_repeat_or_mirrow_lowbits(SkFixed x, int scale) {
    return ((x * scale) >> 12) & 0xF;
}

static SkBitmapProcState::FixedTileLowBitsProc choose_tile_lowbits_proc(unsigned m) {
    if (SkShader::kClamp_TileMode == m) {
        return fixed_clamp_lowbits;
    } else {
        SkASSERT(SkShader::kMirror_TileMode == m ||
                 SkShader::kRepeat_TileMode == m);
        
        return fixed_repeat_or_mirrow_lowbits;
    }
}

static inline U16CPU int_clamp(int x, int n) {
    if (x >= n) {
        x = n - 1;
    }
    if (x < 0) {
        x = 0;
    }
    return x;
}

static inline U16CPU int_repeat(int x, int n) {
    return sk_int_mod(x, n);
}

static inline U16CPU int_mirror(int x, int n) {
    x = sk_int_mod(x, 2 * n);
    if (x >= n) {
        x = n + ~(x - n);
    }
    return x;
}

#if 0
static void test_int_tileprocs() {
    for (int i = -8; i <= 8; i++) {
        SkDebugf(" int_mirror(%2d, 3) = %d\n", i, int_mirror(i, 3));
    }
}
#endif

static SkBitmapProcState::IntTileProc choose_int_tile_proc(unsigned tm) {
    if (SkShader::kClamp_TileMode == tm)
        return int_clamp;
    if (SkShader::kRepeat_TileMode == tm)
        return int_repeat;
    SkASSERT(SkShader::kMirror_TileMode == tm);
    return int_mirror;
}



void decal_nofilter_scale(uint32_t dst[], SkFixed fx, SkFixed dx, int count)
{
    int i;

    for (i = (count >> 2); i > 0; --i)
    {
        *dst++ = pack_two_shorts(fx >> 16, (fx + dx) >> 16);
        fx += dx+dx;
        *dst++ = pack_two_shorts(fx >> 16, (fx + dx) >> 16);
        fx += dx+dx;
    }
    count &= 3;

    uint16_t* xx = (uint16_t*)dst;
    for (i = count; i > 0; --i) {
        *xx++ = SkToU16(fx >> 16); fx += dx;
    }
}

void decal_filter_scale(uint32_t dst[], SkFixed fx, SkFixed dx, int count)
{


    if (count & 1)
    {
        SkASSERT((fx >> (16 + 14)) == 0);
        *dst++ = (fx >> 12 << 14) | ((fx >> 16) + 1);
        fx += dx;
    }
    while ((count -= 2) >= 0)
    {
        SkASSERT((fx >> (16 + 14)) == 0);
        *dst++ = (fx >> 12 << 14) | ((fx >> 16) + 1);
        fx += dx;

        *dst++ = (fx >> 12 << 14) | ((fx >> 16) + 1);
        fx += dx;
    }
}





static void fill_sequential(uint16_t xptr[], int start, int count) {
#if 1
    if (reinterpret_cast<intptr_t>(xptr) & 0x2) {
        *xptr++ = start++;
        count -= 1;
    }
    if (count > 3) {
        uint32_t* xxptr = reinterpret_cast<uint32_t*>(xptr);
        uint32_t pattern0 = PACK_TWO_SHORTS(start + 0, start + 1);
        uint32_t pattern1 = PACK_TWO_SHORTS(start + 2, start + 3);
        start += count & ~3;
        int qcount = count >> 2;
        do {
            *xxptr++ = pattern0;
            pattern0 += 0x40004;
            *xxptr++ = pattern1;
            pattern1 += 0x40004;
        } while (--qcount != 0);
        xptr = reinterpret_cast<uint16_t*>(xxptr);
        count &= 3;
    }
    while (--count >= 0) {
        *xptr++ = start++;
    }
#else
    for (int i = 0; i < count; i++) {
        *xptr++ = start++;
    }
#endif
}

static int nofilter_trans_preamble(const SkBitmapProcState& s, uint32_t** xy,
                                   int x, int y) {
    SkPoint pt;
    s.fInvProc(s.fInvMatrix, SkIntToScalar(x) + SK_ScalarHalf,
               SkIntToScalar(y) + SK_ScalarHalf, &pt);
    **xy = s.fIntTileProcY(SkScalarToFixed(pt.fY) >> 16,
                           s.fBitmap->height());
    *xy += 1;   
    
    return SkScalarToFixed(pt.fX) >> 16;
}

static void clampx_nofilter_trans(const SkBitmapProcState& s,
                                  uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~SkMatrix::kTranslate_Mask) == 0);

    int xpos = nofilter_trans_preamble(s, &xy, x, y);
    const int width = s.fBitmap->width();
    if (1 == width) {
        
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    uint16_t* xptr = reinterpret_cast<uint16_t*>(xy);
    int n;

    
    if (xpos < 0) {
        n = -xpos;
        if (n > count) {
            n = count;
        }
        memset(xptr, 0, n * sizeof(uint16_t));
        count -= n;
        if (0 == count) {
            return;
        }
        xptr += n;
        xpos = 0;
    }

    
    if (xpos < width) {
        n = width - xpos;
        if (n > count) {
            n = count;
        }
        fill_sequential(xptr, xpos, n);
        count -= n;
        if (0 == count) {
            return;
        }
        xptr += n;
    }

    
    sk_memset16(xptr, width - 1, count);
}

static void repeatx_nofilter_trans(const SkBitmapProcState& s,
                                   uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~SkMatrix::kTranslate_Mask) == 0);

    int xpos = nofilter_trans_preamble(s, &xy, x, y);
    const int width = s.fBitmap->width();
    if (1 == width) {
        
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    uint16_t* xptr = reinterpret_cast<uint16_t*>(xy);
    int start = sk_int_mod(xpos, width);
    int n = width - start;
    if (n > count) {
        n = count;
    }
    fill_sequential(xptr, start, n);
    xptr += n;
    count -= n;

    while (count >= width) {
        fill_sequential(xptr, 0, width);
        xptr += width;
        count -= width;
    }

    if (count > 0) {
        fill_sequential(xptr, 0, count);
    }
}

static void fill_backwards(uint16_t xptr[], int pos, int count) {
    for (int i = 0; i < count; i++) {
        SkASSERT(pos >= 0);
        xptr[i] = pos--;
    }
}

static void mirrorx_nofilter_trans(const SkBitmapProcState& s,
                                   uint32_t xy[], int count, int x, int y) {
    SkASSERT((s.fInvType & ~SkMatrix::kTranslate_Mask) == 0);

    int xpos = nofilter_trans_preamble(s, &xy, x, y);
    const int width = s.fBitmap->width();
    if (1 == width) {
        
        memset(xy, 0, count * sizeof(uint16_t));
        return;
    }

    uint16_t* xptr = reinterpret_cast<uint16_t*>(xy);
    
    bool forward;
    int n;
    int start = sk_int_mod(xpos, 2 * width);
    if (start >= width) {
        start = width + ~(start - width);
        forward = false;
        n = start + 1;  
    } else {
        forward = true;
        n = width - start;  
    }
    if (n > count) {
        n = count;
    }
    if (forward) {
        fill_sequential(xptr, start, n);
    } else {
        fill_backwards(xptr, start, n);
    }
    forward = !forward;
    xptr += n;
    count -= n;

    while (count >= width) {
        if (forward) {
            fill_sequential(xptr, 0, width);
        } else {
            fill_backwards(xptr, width - 1, width);
        }
        forward = !forward;
        xptr += width;
        count -= width;
    }

    if (count > 0) {
        if (forward) {
            fill_sequential(xptr, 0, count);
        } else {
            fill_backwards(xptr, width - 1, count);
        }
    }
}



SkBitmapProcState::MatrixProc
SkBitmapProcState::chooseMatrixProc(bool trivial_matrix) {

    
    if (trivial_matrix) {
        SkASSERT(SkPaint::kNone_FilterLevel == fFilterLevel);
        fIntTileProcY = choose_int_tile_proc(fTileModeY);
        switch (fTileModeX) {
            case SkShader::kClamp_TileMode:
                return clampx_nofilter_trans;
            case SkShader::kRepeat_TileMode:
                return repeatx_nofilter_trans;
            case SkShader::kMirror_TileMode:
                return mirrorx_nofilter_trans;
        }
    }

    int index = 0;
    if (fFilterLevel != SkPaint::kNone_FilterLevel) {
        index = 1;
    }
    if (fInvType & SkMatrix::kPerspective_Mask) {
        index += 4;
    } else if (fInvType & SkMatrix::kAffine_Mask) {
        index += 2;
    }

    if (SkShader::kClamp_TileMode == fTileModeX &&
        SkShader::kClamp_TileMode == fTileModeY)
    {
        
        fFilterOneX = SK_Fixed1;
        fFilterOneY = SK_Fixed1;
        return SK_ARM_NEON_WRAP(ClampX_ClampY_Procs)[index];
    }

    
    fFilterOneX = SK_Fixed1 / fBitmap->width();
    fFilterOneY = SK_Fixed1 / fBitmap->height();

    if (SkShader::kRepeat_TileMode == fTileModeX &&
        SkShader::kRepeat_TileMode == fTileModeY)
    {
        return SK_ARM_NEON_WRAP(RepeatX_RepeatY_Procs)[index];
    }

    fTileProcX = choose_tile_proc(fTileModeX);
    fTileProcY = choose_tile_proc(fTileModeY);
    fTileLowBitsProcX = choose_tile_lowbits_proc(fTileModeX);
    fTileLowBitsProcY = choose_tile_lowbits_proc(fTileModeY);
    return GeneralXY_Procs[index];
}
