




#ifndef GFX_FT2UTILS_H
#define GFX_FT2UTILS_H

#include "cairo-ft.h"
#include "gfxFT2FontBase.h"
#include "mozilla/Likely.h"




#define FLOAT_FROM_26_6(x) ((x) / 64.0)
#define FLOAT_FROM_16_16(x) ((x) / 65536.0)
#define ROUND_26_6_TO_INT(x) ((x) >= 0 ?  ((32 + (x)) >> 6) \
                                       : -((32 - (x)) >> 6))

typedef struct FT_FaceRec_* FT_Face;

class gfxFT2LockedFace {
public:
    explicit gfxFT2LockedFace(gfxFT2FontBase *aFont) :
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

    




    uint32_t GetGlyph(uint32_t aCharCode);
    


    uint32_t GetUVSGlyph(uint32_t aCharCode, uint32_t aVariantSelector);

    void GetMetrics(gfxFont::Metrics* aMetrics, uint32_t* aSpaceGlyph);

    
    
    gfxFloat XScale()
    {
        if (MOZ_UNLIKELY(!mFace))
            return 0.0;

        const FT_Size_Metrics& ftMetrics = mFace->size->metrics;

        if (FT_IS_SCALABLE(mFace)) {
            
            
            
            
            
            
            
            return FLOAT_FROM_26_6(FLOAT_FROM_16_16(ftMetrics.x_scale));
        }

        
        
        
        return gfxFloat(ftMetrics.x_ppem) / gfxFloat(mFace->units_per_EM);
    }

protected:
    




    uint32_t GetCharExtents(char aChar, cairo_text_extents_t* aExtents);

    typedef FT_UInt (*CharVariantFunction)(FT_Face  face,
                                           FT_ULong charcode,
                                           FT_ULong variantSelector);
    CharVariantFunction FindCharVariantFunction();

    nsRefPtr<gfxFT2FontBase> mGfxFont;
    FT_Face mFace;
};

#endif 
