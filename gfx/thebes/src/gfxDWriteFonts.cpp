




































#include "gfxDWriteFonts.h"
#include "gfxDWriteShaper.h"
#include "gfxDWriteFontList.h"
#include "gfxContext.h"
#include <dwrite.h>

#include "gfxDWriteTextAnalysis.h"


#define OBLIQUE_SKEW_FACTOR 0.3



gfxDWriteFont::gfxDWriteFont(gfxFontEntry *aFontEntry,
                             const gfxFontStyle *aFontStyle,
                             PRBool aNeedsBold)
    : gfxFont(aFontEntry, aFontStyle)
    , mAdjustedSize(0.0f)
    , mCairoFontFace(nsnull)
    , mCairoScaledFont(nsnull)
    , mNeedsOblique(PR_FALSE)
{
    gfxDWriteFontEntry *fe =
        static_cast<gfxDWriteFontEntry*>(aFontEntry);
    nsresult rv;
    DWRITE_FONT_SIMULATIONS sims = DWRITE_FONT_SIMULATIONS_NONE;
    if ((GetStyle()->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) &&
        !fe->IsItalic()) {
            
            
            mNeedsOblique = PR_TRUE;
    }
    PRInt8 baseWeight, weightDistance;
    GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    if (aNeedsBold) {
        sims |= DWRITE_FONT_SIMULATIONS_BOLD;
    }

    rv = fe->CreateFontFace(getter_AddRefs(mFontFace), sims);

    if (NS_FAILED(rv)) {
        mIsValid = PR_FALSE;
        return;
    }

    ComputeMetrics();

    mShaper = new gfxDWriteShaper(this);
}

gfxDWriteFont::~gfxDWriteFont()
{
    if (mCairoFontFace) {
        cairo_font_face_destroy(mCairoFontFace);
    }
    if (mCairoScaledFont) {
        cairo_scaled_font_destroy(mCairoScaledFont);
    }
}

nsString
gfxDWriteFont::GetUniqueName()
{
    return mFontEntry->Name();
}

const gfxFont::Metrics&
gfxDWriteFont::GetMetrics()
{
    return mMetrics;
}

void
gfxDWriteFont::ComputeMetrics()
{
    DWRITE_FONT_METRICS fontMetrics;
    mFontFace->GetMetrics(&fontMetrics);

    if (mStyle.sizeAdjust != 0.0) {
        gfxFloat aspect = (gfxFloat)fontMetrics.xHeight /
                   fontMetrics.designUnitsPerEm;
        mAdjustedSize = mStyle.GetAdjustedSize(aspect);
    } else {
        mAdjustedSize = mStyle.size;
    }

    mMetrics.xHeight =
        ((gfxFloat)fontMetrics.xHeight /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;

    mMetrics.maxAscent = 
        ceil(((gfxFloat)fontMetrics.ascent /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize);
    mMetrics.maxDescent = 
        ceil(((gfxFloat)fontMetrics.descent /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize);
    mMetrics.maxHeight = mMetrics.maxAscent + mMetrics.maxDescent;

    mMetrics.emHeight = mAdjustedSize;
    mMetrics.emAscent = mMetrics.emHeight *
        mMetrics.maxAscent / mMetrics.maxHeight;
    mMetrics.emDescent = mMetrics.emHeight - mMetrics.emAscent;

    mMetrics.maxAdvance = mAdjustedSize;

    mMetrics.internalLeading = 
        ceil(((gfxFloat)(fontMetrics.ascent + 
                    fontMetrics.descent - 
                    fontMetrics.designUnitsPerEm) / 
                    fontMetrics.designUnitsPerEm) * mAdjustedSize);
    mMetrics.externalLeading = 
        ceil(((gfxFloat)fontMetrics.lineGap /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize);

    UINT16 glyph = (PRUint16)GetSpaceGlyph();
    DWRITE_GLYPH_METRICS metrics;
    mFontFace->GetDesignGlyphMetrics(&glyph, 1, &metrics);
    mMetrics.spaceWidth = 
        ((gfxFloat)metrics.advanceWidth /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;
    UINT32 ucs = L'x';
    if (SUCCEEDED(mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph)) &&
        SUCCEEDED(mFontFace->GetDesignGlyphMetrics(&glyph, 1, &metrics))) {    
        mMetrics.aveCharWidth = 
            ((gfxFloat)metrics.advanceWidth /
                       fontMetrics.designUnitsPerEm) * mAdjustedSize;
    } else {
        
        mMetrics.aveCharWidth = 
            ((gfxFloat)fontMetrics.xHeight /
                       fontMetrics.designUnitsPerEm) * mAdjustedSize;
    }
    ucs = L'0';
    if (FAILED(mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph)) &&
        SUCCEEDED(mFontFace->GetDesignGlyphMetrics(&glyph, 1, &metrics))) {
        mMetrics.zeroOrAveCharWidth = 
            ((gfxFloat)metrics.advanceWidth /
                       fontMetrics.designUnitsPerEm) * mAdjustedSize;
    } else {
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;
    }
    mMetrics.underlineOffset = 
        ((gfxFloat)fontMetrics.underlinePosition /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;
    mMetrics.underlineSize = 
        ((gfxFloat)fontMetrics.underlineThickness /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;
    mMetrics.strikeoutOffset = 
        ((gfxFloat)fontMetrics.strikethroughPosition /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;
    mMetrics.strikeoutSize = 
        ((gfxFloat)fontMetrics.strikethroughThickness /
                   fontMetrics.designUnitsPerEm) * mAdjustedSize;
    mMetrics.superscriptOffset = 0;
    mMetrics.subscriptOffset = 0;

    SanitizeMetrics(&mMetrics, PR_FALSE);

#if 0
    printf("Font: %p (%s) size: %f\n", this,
           NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);
    printf("    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    printf("    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    printf("    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    printf("    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    printf("    uOff: %f uSize: %f stOff: %f stSize: %f supOff: %f subOff: %f\n",
           mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize,
           mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

PRUint32
gfxDWriteFont::GetSpaceGlyph()
{
    UINT32 ucs = L' ';
    UINT16 glyph;
    HRESULT hr;
    hr = mFontFace->GetGlyphIndicesA(&ucs, 1, &glyph);
    if (FAILED(hr)) {
        return 0;
    }
    return glyph;
}

PRBool
gfxDWriteFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}

cairo_font_face_t *
gfxDWriteFont::CairoFontFace()
{
    if (!mCairoFontFace) {
#ifdef CAIRO_HAS_DWRITE_FONT
        mCairoFontFace = 
            cairo_dwrite_font_face_create_for_dwrite_fontface(
            ((gfxDWriteFontEntry*)mFontEntry.get())->mFont, mFontFace);
#endif
    }
    return mCairoFontFace;
}


cairo_scaled_font_t *
gfxDWriteFont::CairoScaledFont()
{
    if (!mCairoScaledFont) {
        cairo_matrix_t sizeMatrix;
        cairo_matrix_t identityMatrix;

        cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);
        cairo_matrix_init_identity(&identityMatrix);

        cairo_font_options_t *fontOptions = cairo_font_options_create();
        if (mNeedsOblique) {
            double skewfactor = OBLIQUE_SKEW_FACTOR;

            cairo_matrix_t style;
            cairo_matrix_init(&style,
                              1,                
                              0,                
                              -1 * skewfactor,  
                              1,                
                              0,                
                              0);               
            cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
        }

        mCairoScaledFont = cairo_scaled_font_create(CairoFontFace(),
                                                    &sizeMatrix,
                                                    &identityMatrix,
                                                    fontOptions);
        cairo_font_options_destroy(fontOptions);
    }

    NS_ASSERTION(mAdjustedSize == 0.0 ||
                 cairo_scaled_font_status(mCairoScaledFont) 
                   == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    return mCairoScaledFont;
}
