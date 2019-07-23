









































#ifndef GFX_FT2UTILS_H
#define GFX_FT2UTILS_H

#include "cairo-ft.h"
#include "gfxFT2FontBase.h"

typedef struct FT_FaceRec_* FT_Face;

class gfxFT2LockedFace {
public:
    gfxFT2LockedFace(gfxFT2FontBase *aFont) :
        mGfxFont(aFont),
        mFace(cairo_ft_scaled_font_lock_face(aFont->CairoScaledFont()))
    { }
    ~gfxFT2LockedFace()
    {
        if (mFace) {
            cairo_ft_scaled_font_unlock_face(mGfxFont->CairoScaledFont());
        }
    }

    FT_Face get() { return mFace; };

    




    virtual PRUint32 GetGlyph(PRUint32 aCharCode);

    void GetMetrics(gfxFont::Metrics* aMetrics, PRUint32* aSpaceGlyph);

protected:
    




    PRUint32 GetCharExtents(char aChar, cairo_text_extents_t* aExtents);

    nsRefPtr<gfxFT2FontBase> mGfxFont;
    FT_Face mFace;
};




#define FLOAT_FROM_26_6(x) ((x) / 64.0)
#define FLOAT_FROM_16_16(x) ((x) / 65536.0)
#define ROUND_26_6_TO_INT(x) ((x) >= 0 ?  ((32 + (x)) >> 6) \
                                       : -((32 - (x)) >> 6))

#endif 
