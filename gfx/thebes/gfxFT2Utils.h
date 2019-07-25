









































#ifndef GFX_FT2UTILS_H
#define GFX_FT2UTILS_H

#include "cairo-ft.h"
#include "gfxFT2FontBase.h"




#define FLOAT_FROM_26_6(x) ((x) / 64.0)
#define FLOAT_FROM_16_16(x) ((x) / 65536.0)
#define ROUND_26_6_TO_INT(x) ((x) >= 0 ?  ((32 + (x)) >> 6) \
                                       : -((32 - (x)) >> 6))

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

    




    PRUint32 GetGlyph(PRUint32 aCharCode);
    


    PRUint32 GetUVSGlyph(PRUint32 aCharCode, PRUint32 aVariantSelector);

    void GetMetrics(gfxFont::Metrics* aMetrics, PRUint32* aSpaceGlyph);

    bool GetFontTable(PRUint32 aTag, FallibleTArray<PRUint8>& aBuffer);

    
    
    gfxFloat XScale()
    {
        if (NS_UNLIKELY(!mFace))
            return 0.0;

        const FT_Size_Metrics& ftMetrics = mFace->size->metrics;

        if (FT_IS_SCALABLE(mFace)) {
            
            
            
            
            
            
            
            return FLOAT_FROM_26_6(FLOAT_FROM_16_16(ftMetrics.x_scale));
        }

        
        
        
        return gfxFloat(ftMetrics.x_ppem) / gfxFloat(mFace->units_per_EM);
    }

protected:
    




    PRUint32 GetCharExtents(char aChar, cairo_text_extents_t* aExtents);

    typedef FT_UInt (*CharVariantFunction)(FT_Face  face,
                                           FT_ULong charcode,
                                           FT_ULong variantSelector);
    CharVariantFunction FindCharVariantFunction();

    nsRefPtr<gfxFT2FontBase> mGfxFont;
    FT_Face mFace;
};

#endif 
