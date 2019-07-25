








#ifndef SkAutoKern_DEFINED
#define SkAutoKern_DEFINED

#include "SkScalerContext.h"

#define SkAutoKern_AdjustF(prev, next)    (((next) - (prev) + 32) >> 6 << 16)
#define SkAutoKern_AdjustS(prev, next)    SkIntToScalar(((next) - (prev) + 32) >> 6)






class SkAutoKern {
public:
    SkAutoKern() : fPrevRsbDelta(0) {}

    SkFixed  adjust(const SkGlyph&  glyph) 
    {



#if 0
        int  distort = fPrevRsbDelta - glyph.fLsbDelta;

        fPrevRsbDelta = glyph.fRsbDelta;

        if (distort >= 32)
            return -SK_Fixed1;
        else if (distort < -32)
            return +SK_Fixed1;
        else
            return 0;
#else
        SkFixed adjust = SkAutoKern_AdjustF(fPrevRsbDelta, glyph.fLsbDelta);
        fPrevRsbDelta = glyph.fRsbDelta;
        return adjust;
#endif
    }
private:
    int   fPrevRsbDelta;
};

#endif

