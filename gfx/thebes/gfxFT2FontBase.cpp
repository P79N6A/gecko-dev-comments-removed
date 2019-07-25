









































#include "gfxFT2FontBase.h"
#include "gfxFT2Utils.h"

gfxFT2FontBase::gfxFT2FontBase(cairo_scaled_font_t *aScaledFont,
                               gfxFontEntry *aFontEntry,
                               const gfxFontStyle *aFontStyle)
    : gfxFont(aFontEntry, aFontStyle),
      mScaledFont(aScaledFont),
      mSpaceGlyph(0),
      mHasMetrics(PR_FALSE)
{
    cairo_scaled_font_reference(mScaledFont);
}

gfxFT2FontBase::~gfxFT2FontBase()
{
    cairo_scaled_font_destroy(mScaledFont);
}

PRUint32
gfxFT2FontBase::GetGlyph(PRUint32 aCharCode)
{
    
    
    

    cairo_font_face_t *face =
        cairo_scaled_font_get_font_face(CairoScaledFont());

    if (cairo_font_face_status(face) != CAIRO_STATUS_SUCCESS)
        return 0;

    
    
    
    
    

    struct CmapCacheSlot {
        PRUint32 mCharCode;
        PRUint32 mGlyphIndex;
    };
    const PRUint32 kNumSlots = 256;
    static cairo_user_data_key_t sCmapCacheKey;

    CmapCacheSlot *slots = static_cast<CmapCacheSlot*>
        (cairo_font_face_get_user_data(face, &sCmapCacheKey));

    if (!slots) {
        
        
        
        
        slots = static_cast<CmapCacheSlot*>
            (calloc(kNumSlots, sizeof(CmapCacheSlot)));
        if (!slots)
            return 0;

        cairo_status_t status =
            cairo_font_face_set_user_data(face, &sCmapCacheKey, slots, free);
        if (status != CAIRO_STATUS_SUCCESS) { 
            free(slots);
            return 0;
        }

        
        
        
        
        slots[0].mCharCode = 1;
    }

    CmapCacheSlot *slot = &slots[aCharCode % kNumSlots];
    if (slot->mCharCode != aCharCode) {
        slot->mCharCode = aCharCode;
        slot->mGlyphIndex = gfxFT2LockedFace(this).GetGlyph(aCharCode);
    }

    return slot->mGlyphIndex;
}

void
gfxFT2FontBase::GetGlyphExtents(PRUint32 aGlyph, cairo_text_extents_t* aExtents)
{
    NS_PRECONDITION(aExtents != NULL, "aExtents must not be NULL");

    cairo_glyph_t glyphs[1];
    glyphs[0].index = aGlyph;
    glyphs[0].x = 0.0;
    glyphs[0].y = 0.0;
    
    
    
    
    
    cairo_scaled_font_glyph_extents(CairoScaledFont(), glyphs, 1, aExtents);
}

const gfxFont::Metrics&
gfxFT2FontBase::GetMetrics()
{
    if (mHasMetrics)
        return mMetrics;

    if (NS_UNLIKELY(GetStyle()->size <= 0.0)) {
        new(&mMetrics) gfxFont::Metrics(); 
        mSpaceGlyph = 0;
    } else {
        gfxFT2LockedFace(this).GetMetrics(&mMetrics, &mSpaceGlyph);
    }

    SanitizeMetrics(&mMetrics, PR_FALSE);

#if 0
    
    

    fprintf (stderr, "Font: %s\n", NS_ConvertUTF16toUTF8(GetName()).get());
    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f\n", mMetrics.maxAscent, mMetrics.maxDescent);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.externalLeading, mMetrics.internalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif

    mHasMetrics = PR_TRUE;
    return mMetrics;
}

nsString
gfxFT2FontBase::GetUniqueName()
{
    return GetName();
}


PRUint32
gfxFT2FontBase::GetSpaceGlyph()
{
    NS_ASSERTION(GetStyle()->size != 0,
                 "forgot to short-circuit a text run with zero-sized font?");
    GetMetrics();
    return mSpaceGlyph;
}

PRBool
gfxFT2FontBase::SetupCairoFont(gfxContext *aContext)
{
    cairo_t *cr = aContext->GetCairo();

    
    
    
    
    
    
    cairo_scaled_font_t *cairoFont = CairoScaledFont();

    if (cairo_scaled_font_status(cairoFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    cairo_set_scaled_font(cr, cairoFont);
    return PR_TRUE;
}
