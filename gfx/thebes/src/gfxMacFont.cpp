







































#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxPlatformMac.h"
#include "gfxContext.h"

#include "cairo-quartz.h"

gfxMacFont::gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                       PRBool aNeedsBold)
    : gfxFont(aFontEntry, aFontStyle),
      mATSFont(aFontEntry->GetFontRef()),
      mFontFace(nsnull),
      mScaledFont(nsnull),
      mAdjustedSize(0.0)
{
    
    PRInt8 baseWeight, weightDistance;
    mStyle.ComputeWeightAndOffset(&baseWeight, &weightDistance);
    PRUint16 targetWeight = (baseWeight * 100) + (weightDistance * 100);

    
    
    
    
    if (!aFontEntry->IsBold()
        && ((weightDistance == 0 && targetWeight >= 600) || (weightDistance > 0 && aNeedsBold)))
    {
        mSyntheticBoldOffset = 1;  
    }

    
    InitMetrics();
    if (!mIsValid)
        return;

    CGFontRef cgFont = ::CGFontCreateWithPlatformFont(&mATSFont);
    mFontFace = cairo_quartz_font_face_create_for_cgfont(cgFont);
    ::CGFontRelease(cgFont);

    cairo_status_t cairoerr = cairo_font_face_status(mFontFace);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = PR_FALSE;
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create Cairo font face: %s status: %d",
                NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
        return;
    }

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    
    PRBool needsOblique =
        (mFontEntry != NULL) &&
        (!mFontEntry->IsItalic() && (mStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

    if (needsOblique) {
        double skewfactor = (needsOblique ? Fix2X(kATSItalicQDSkew) : 0);

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

    cairo_font_options_t *fontOptions = cairo_font_options_create();

    
    if (mAdjustedSize <= (float) gfxPlatformMac::GetPlatform()->GetAntiAliasingThreshold()) {
        cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
    }

    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix, &ctm, fontOptions);
    cairo_font_options_destroy(fontOptions);

    cairoerr = cairo_scaled_font_status(mScaledFont);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = PR_FALSE;
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create scaled font: %s status: %d",
                NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
    }

    mShaper = new gfxCoreTextShaper(this);
}

gfxMacFont::~gfxMacFont()
{
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
    }
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
    }
}

PRBool
gfxMacFont::SetupCairoFont(gfxContext *aContext)
{
    if (cairo_scaled_font_status(mScaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), mScaledFont);
    return PR_TRUE;
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
    return floor(aValue/aFraction + 0.5) * aFraction;
}

void
gfxMacFont::InitMetrics()
{
    gfxFloat size =
        PR_MAX(((mAdjustedSize != 0.0f) ? mAdjustedSize : mStyle.size), 1.0f);

    ATSFontMetrics atsMetrics;
    OSStatus err;

    err = ::ATSFontGetHorizontalMetrics(mATSFont, kATSOptionFlagsDefault,
                                        &atsMetrics);
    if (err != noErr) {
        mIsValid = PR_FALSE;

#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s err: %8.8x",
                NS_ConvertUTF16toUTF8(mFontEntry->Name()).get(), PRUint32(err));
        NS_WARNING(warnBuf);
#endif
        return;
    }

    
    CTFontRef aCTFont =
        ::CTFontCreateWithPlatformFont(mATSFont, size, NULL, NULL);

    
    
    if (atsMetrics.xHeight > 0)
        mMetrics.xHeight = atsMetrics.xHeight * size;
    else
        mMetrics.xHeight = GetCharHeight(aCTFont, 'x');

    if (mAdjustedSize == 0.0f) {
        if (mMetrics.xHeight != 0.0f && mStyle.sizeAdjust != 0.0f) {
            gfxFloat aspect = mMetrics.xHeight / size;
            mAdjustedSize = mStyle.GetAdjustedSize(aspect);

            
            
            InitMetrics();

            
            ::CFRelease(aCTFont);
            return;
        }
        mAdjustedSize = size;
    }

    mMetrics.superscriptOffset = mMetrics.xHeight;
    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.underlineOffset = atsMetrics.underlinePosition * size;
    mMetrics.underlineSize = atsMetrics.underlineThickness * size;
    mMetrics.strikeoutSize = mMetrics.underlineSize;
    mMetrics.strikeoutOffset = mMetrics.xHeight / 2;

    mMetrics.externalLeading = atsMetrics.leading * size;
    mMetrics.emHeight = size;
    mMetrics.maxAscent =
      NS_ceil(RoundToNearestMultiple(atsMetrics.ascent * size, 1/1024.0));
    mMetrics.maxDescent =
      NS_ceil(-RoundToNearestMultiple(atsMetrics.descent * size, 1/1024.0));

    mMetrics.maxHeight = mMetrics.maxAscent + mMetrics.maxDescent;
    if (mMetrics.maxHeight - mMetrics.emHeight > 0.0)
        mMetrics.internalLeading = mMetrics.maxHeight - mMetrics.emHeight;
    else
        mMetrics.internalLeading = 0.0;

    mMetrics.maxAdvance = atsMetrics.maxAdvanceWidth * size + mSyntheticBoldOffset;

    mMetrics.emAscent = mMetrics.maxAscent * mMetrics.emHeight / mMetrics.maxHeight;
    mMetrics.emDescent = mMetrics.emHeight - mMetrics.emAscent;

    PRUint32 glyphID;
    float xWidth = GetCharWidth(aCTFont, 'x', &glyphID);
    if (atsMetrics.avgAdvanceWidth != 0.0)
        mMetrics.aveCharWidth = PR_MIN(atsMetrics.avgAdvanceWidth * size, xWidth);
    else if (glyphID != 0)
        mMetrics.aveCharWidth = xWidth;
    else
        mMetrics.aveCharWidth = mMetrics.maxAdvance;
    mMetrics.aveCharWidth += mSyntheticBoldOffset;

    if (mFontEntry->IsFixedPitch()) {
        
        
        
        mMetrics.maxAdvance = mMetrics.aveCharWidth;
    }

    mMetrics.spaceWidth = GetCharWidth(aCTFont, ' ', &glyphID);
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth(aCTFont, '0', &glyphID);
    if (glyphID == 0)
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;

    ::CFRelease(aCTFont);

    SanitizeMetrics(&mMetrics, mFontEntry->mIsBadUnderlineFont);

    mIsValid = PR_TRUE;

#if 0
    fprintf (stderr, "Font: %p (%s) size: %f\n", this,
             NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);

    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

float
gfxMacFont::GetCharWidth(CTFontRef aCTFont, PRUnichar aUniChar,
                         PRUint32 *aGlyphID)
{
    UniChar c = aUniChar;
    CGGlyph glyph;
    if (::CTFontGetGlyphsForCharacters(aCTFont, &c, &glyph, 1)) {
        CGSize advance;
        ::CTFontGetAdvancesForGlyphs(aCTFont,
                                     kCTFontHorizontalOrientation,
                                     &glyph,
                                     &advance,
                                     1);
        if (aGlyphID != nsnull)
            *aGlyphID = glyph;
        return advance.width;
    }

    
    if (aGlyphID != nsnull)
        *aGlyphID = 0;
    return 0;
}

float
gfxMacFont::GetCharHeight(CTFontRef aCTFont, PRUnichar aUniChar)
{
    UniChar c = aUniChar;
    CGGlyph glyph;
    if (::CTFontGetGlyphsForCharacters(aCTFont, &c, &glyph, 1)) {
        CGRect boundingRect;
        ::CTFontGetBoundingRectsForGlyphs(aCTFont,
                                          kCTFontHorizontalOrientation,
                                          &glyph,
                                          &boundingRect,
                                          1);
        return boundingRect.size.height;
    }

    return 0;
}
