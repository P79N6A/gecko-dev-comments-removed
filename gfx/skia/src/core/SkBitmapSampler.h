








#ifndef SkBitmapSampler_DEFINED
#define SkBitmapSampler_DEFINED

#include "SkBitmap.h"
#include "SkPaint.h"
#include "SkShader.h"

typedef int (*SkTileModeProc)(int value, unsigned max);

class SkBitmapSampler {
public:
    SkBitmapSampler(const SkBitmap&, bool filter, SkShader::TileMode tmx, SkShader::TileMode tmy);
    virtual ~SkBitmapSampler() {}

    const SkBitmap&     getBitmap() const { return fBitmap; }
    bool                getFilterBitmap() const { return fFilterBitmap; }
    SkShader::TileMode  getTileModeX() const { return fTileModeX; }
    SkShader::TileMode  getTileModeY() const { return fTileModeY; }

    

    virtual SkPMColor sample(SkFixed x, SkFixed y) const = 0;

    virtual void setPaint(const SkPaint& paint);

    
    static SkBitmapSampler* Create(const SkBitmap&, bool filter,
                                   SkShader::TileMode tmx, SkShader::TileMode tmy);

protected:
    const SkBitmap&     fBitmap;
    uint16_t            fMaxX, fMaxY;
    bool                fFilterBitmap;
    SkShader::TileMode  fTileModeX;
    SkShader::TileMode  fTileModeY;
    SkTileModeProc      fTileProcX;
    SkTileModeProc      fTileProcY;

    
    SkBitmapSampler& operator=(const SkBitmapSampler&);
};

static inline int fixed_clamp(SkFixed x)
{
#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (x >> 16)
        x = 0xFFFF;
    if (x < 0)
        x = 0;
#else
    if (x >> 16)
    {
        if (x < 0)
            x = 0;
        else
            x = 0xFFFF;
    }
#endif
    return x;
}



static inline int fixed_repeat(SkFixed x)
{
    return x & 0xFFFF;
}

static inline int fixed_mirror(SkFixed x)
{
    SkFixed s = x << 15 >> 31;
    
    return (x ^ s) & 0xFFFF;
}

static inline bool is_pow2(int count)
{
    SkASSERT(count > 0);
    return (count & (count - 1)) == 0;
}

static inline int do_clamp(int index, unsigned max)
{
    SkASSERT((int)max >= 0);

#ifdef SK_CPU_HAS_CONDITIONAL_INSTR
    if (index > (int)max)
        index = max;
    if (index < 0)
        index = 0;
#else
    if ((unsigned)index > max)
    {
        if (index < 0)
            index = 0;
        else
            index = max;
    }
#endif
    return index;
}

static inline int do_repeat_mod(int index, unsigned max)
{
    SkASSERT((int)max >= 0);

    if ((unsigned)index > max)
    {
        if (index < 0)
            index = max - (~index % (max + 1));
        else
            index = index % (max + 1);
    }
    return index;
}

static inline int do_repeat_pow2(int index, unsigned max)
{
    SkASSERT((int)max >= 0 && is_pow2(max + 1));

    return index & max;
}

static inline int do_mirror_mod(int index, unsigned max)
{
    SkASSERT((int)max >= 0);

    
    
    
    index ^= index >> 31;

    if ((unsigned)index > max)
    {
        int mod = (max + 1) << 1;
        index = index % mod;
        if ((unsigned)index > max)
            index = mod - index - 1;
    }
    return index;
}

static inline int do_mirror_pow2(int index, unsigned max)
{
    SkASSERT((int)max >= 0 && is_pow2(max + 1));

    int s = (index & (max + 1)) - 1;
    s = ~(s >> 31);
    
    return (index ^ s) & max;
}

#endif
