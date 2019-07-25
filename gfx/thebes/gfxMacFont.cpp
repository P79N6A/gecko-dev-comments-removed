







































#include "gfxMacFont.h"
#include "gfxCoreTextShaper.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxPlatformMac.h"
#include "gfxContext.h"
#include "gfxUnicodeProperties.h"
#include "gfxFontUtils.h"

#include "cairo-quartz.h"

using namespace mozilla;

gfxMacFont::gfxMacFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                       PRBool aNeedsBold)
    : gfxFont(aFontEntry, aFontStyle),
      mATSFont(aFontEntry->GetFontRef()),
      mCGFont(nsnull),
      mFontFace(nsnull),
      mScaledFont(nsnull)
{
    if (aNeedsBold) {
        mSyntheticBoldOffset = 1;  
    }

    mCGFont = ::CGFontCreateWithPlatformFont(&mATSFont);
    if (!mCGFont) {
        mIsValid = PR_FALSE;
        return;
    }

    
    InitMetrics();
    if (!mIsValid) {
        return;
    }

    mFontFace = cairo_quartz_font_face_create_for_cgfont(mCGFont);

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
        (!mFontEntry->IsItalic() &&
         (mStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

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

    
    if (mAdjustedSize <=
        (gfxFloat)gfxPlatformMac::GetPlatform()->GetAntiAliasingThreshold()) {
        cairo_font_options_set_antialias(fontOptions, CAIRO_ANTIALIAS_NONE);
    }

    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix, &ctm,
                                           fontOptions);
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

    if (FontCanSupportHarfBuzz()) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
}

gfxMacFont::~gfxMacFont()
{
    if (mScaledFont) {
        cairo_scaled_font_destroy(mScaledFont);
    }
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
    }

    
    ::CGFontRelease(mCGFont);
}

PRBool
gfxMacFont::InitTextRun(gfxContext *aContext,
                        gfxTextRun *aTextRun,
                        const PRUnichar *aString,
                        PRUint32 aRunStart,
                        PRUint32 aRunLength,
                        PRInt32 aRunScript,
                        PRBool aPreferPlatformShaping)
{
    if (!mIsValid) {
        NS_WARNING("invalid font! expect incorrect text rendering");
        return PR_FALSE;
    }

    PRBool ok = gfxFont::InitTextRun(aContext, aTextRun, aString,
                                     aRunStart, aRunLength, aRunScript,
        static_cast<MacOSFontEntry*>(GetFontEntry())->RequiresAATLayout());

    aTextRun->AdjustAdvancesForSyntheticBold(aRunStart, aRunLength);

    return ok;
}

void
gfxMacFont::CreatePlatformShaper()
{
    mPlatformShaper = new gfxCoreTextShaper(this);
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

gfxFont::RunMetrics
gfxMacFont::Measure(gfxTextRun *aTextRun,
                    PRUint32 aStart, PRUint32 aEnd,
                    BoundingBoxType aBoundingBoxType,
                    gfxContext *aRefContext,
                    Spacing *aSpacing)
{
    gfxFont::RunMetrics metrics =
        gfxFont::Measure(aTextRun, aStart, aEnd,
                         aBoundingBoxType, aRefContext, aSpacing);

    
    
    if (aBoundingBoxType != TIGHT_HINTED_OUTLINE_EXTENTS &&
        metrics.mBoundingBox.width > 0) {
        metrics.mBoundingBox.x -= aTextRun->GetAppUnitsPerDevUnit();
        metrics.mBoundingBox.width += aTextRun->GetAppUnitsPerDevUnit() * 2;
    }

    return metrics;
}

void
gfxMacFont::InitMetrics()
{
    mIsValid = PR_FALSE;
    ::memset(&mMetrics, 0, sizeof(mMetrics));

    PRUint32 upem = 0;

    
    
    
    
    
    const PRUint32 kHeadTableTag = TRUETYPE_TAG('h','e','a','d');
    AutoFallibleTArray<PRUint8,sizeof(HeadTable)> headData;
    if (NS_SUCCEEDED(mFontEntry->GetFontTable(kHeadTableTag, headData)) &&
        headData.Length() >= sizeof(HeadTable)) {
        HeadTable *head = reinterpret_cast<HeadTable*>(headData.Elements());
        upem = head->unitsPerEm;
    } else {
        upem = ::CGFontGetUnitsPerEm(mCGFont);
    }

    if (upem < 16 || upem > 16384) {
        
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s (invalid unitsPerEm value)",
                NS_ConvertUTF16toUTF8(mFontEntry->Name()).get());
        NS_WARNING(warnBuf);
#endif
        return;
    }

    mAdjustedSize = NS_MAX(mStyle.size, 1.0);
    mFUnitsConvFactor = mAdjustedSize / upem;

    
    
    
    gfxFloat cgConvFactor;
    if (static_cast<MacOSFontEntry*>(mFontEntry.get())->IsCFF()) {
        cgConvFactor = mAdjustedSize / ::CGFontGetUnitsPerEm(mCGFont);
    } else {
        cgConvFactor = mFUnitsConvFactor;
    }

    
    
    if (!InitMetricsFromSfntTables(mMetrics) &&
        (!mFontEntry->IsUserFont() || mFontEntry->IsLocalUserFont())) {
        InitMetricsFromATSMetrics();
    }
    if (!mIsValid) {
        return;
    }

    if (mMetrics.xHeight == 0.0) {
        mMetrics.xHeight = ::CGFontGetXHeight(mCGFont) * cgConvFactor;
    }

    if (mStyle.sizeAdjust != 0.0 && mStyle.size > 0.0 &&
        mMetrics.xHeight > 0.0) {
        
        gfxFloat aspect = mMetrics.xHeight / mStyle.size;
        mAdjustedSize = mStyle.GetAdjustedSize(aspect);
        mFUnitsConvFactor = mAdjustedSize / upem;
        if (static_cast<MacOSFontEntry*>(mFontEntry.get())->IsCFF()) {
            cgConvFactor = mAdjustedSize / ::CGFontGetUnitsPerEm(mCGFont);
        } else {
            cgConvFactor = mFUnitsConvFactor;
        }
        mMetrics.xHeight = 0.0;
        if (!InitMetricsFromSfntTables(mMetrics) &&
            (!mFontEntry->IsUserFont() || mFontEntry->IsLocalUserFont())) {
            InitMetricsFromATSMetrics();
        }
        if (!mIsValid) {
            
            
            return;
        }
        if (mMetrics.xHeight == 0.0) {
            mMetrics.xHeight = ::CGFontGetXHeight(mCGFont) * cgConvFactor;
        }
    }

    
    
    

    mMetrics.emHeight = mAdjustedSize;

    
    

    CFDataRef cmap =
        ::CGFontCopyTableForTag(mCGFont, TRUETYPE_TAG('c','m','a','p'));

    PRUint32 glyphID;
    if (mMetrics.aveCharWidth <= 0) {
        mMetrics.aveCharWidth = GetCharWidth(cmap, 'x', &glyphID,
                                             cgConvFactor);
        if (glyphID == 0) {
            
            mMetrics.aveCharWidth = mMetrics.maxAdvance;
        }
    }
    mMetrics.aveCharWidth += mSyntheticBoldOffset;
    mMetrics.maxAdvance += mSyntheticBoldOffset;

    mMetrics.spaceWidth = GetCharWidth(cmap, ' ', &glyphID, cgConvFactor);
    if (glyphID == 0) {
        
        mMetrics.spaceWidth = mMetrics.aveCharWidth;
    }
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth(cmap, '0', &glyphID,
                                               cgConvFactor);
    if (glyphID == 0) {
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;
    }

    if (cmap) {
        ::CFRelease(cmap);
    }

    CalculateDerivedMetrics(mMetrics);

    SanitizeMetrics(&mMetrics, mFontEntry->mIsBadUnderlineFont);

#if 0
    fprintf (stderr, "Font: %p (%s) size: %f\n", this,
             NS_ConvertUTF16toUTF8(GetName()).get(), mStyle.size);

    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.internalLeading, mMetrics.externalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f supOff: %f subOff: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

gfxFloat
gfxMacFont::GetCharWidth(CFDataRef aCmap, PRUnichar aUniChar,
                         PRUint32 *aGlyphID, gfxFloat aConvFactor)
{
    CGGlyph glyph = 0;
    
    if (aCmap) {
        glyph = gfxFontUtils::MapCharToGlyph(::CFDataGetBytePtr(aCmap),
                                             ::CFDataGetLength(aCmap),
                                             aUniChar);
    }

    if (aGlyphID) {
        *aGlyphID = glyph;
    }

    if (glyph) {
        int advance;
        if (::CGFontGetGlyphAdvances(mCGFont, &glyph, 1, &advance)) {
            return advance * aConvFactor;
        }
    }

    return 0;
}

 void
gfxMacFont::DestroyBlobFunc(void* aUserData)
{
    ::CFRelease((CFDataRef)aUserData);
}

hb_blob_t *
gfxMacFont::GetFontTable(PRUint32 aTag)
{
    CFDataRef dataRef = ::CGFontCopyTableForTag(mCGFont, aTag);
    if (dataRef) {
        return hb_blob_create((const char*)::CFDataGetBytePtr(dataRef),
                              ::CFDataGetLength(dataRef),
                              HB_MEMORY_MODE_READONLY,
                              DestroyBlobFunc, (void*)dataRef);
    }

    if (mFontEntry->IsUserFont() && !mFontEntry->IsLocalUserFont()) {
        
        
        hb_blob_t *blob;
        if (mFontEntry->GetExistingFontTable(aTag, &blob)) {
            return blob;
        }
    }

    return nsnull;
}







void
gfxMacFont::InitMetricsFromATSMetrics()
{
    ATSFontMetrics atsMetrics;
    OSStatus err;

    err = ::ATSFontGetHorizontalMetrics(mATSFont, kATSOptionFlagsDefault,
                                        &atsMetrics);
    if (err != noErr) {
#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s err: %8.8x",
                NS_ConvertUTF16toUTF8(mFontEntry->Name()).get(), PRUint32(err));
        NS_WARNING(warnBuf);
#endif
        return;
    }

    mMetrics.underlineOffset = atsMetrics.underlinePosition * mAdjustedSize;
    mMetrics.underlineSize = atsMetrics.underlineThickness * mAdjustedSize;

    mMetrics.externalLeading = atsMetrics.leading * mAdjustedSize;

    mMetrics.maxAscent = atsMetrics.ascent * mAdjustedSize;
    mMetrics.maxDescent = -atsMetrics.descent * mAdjustedSize;

    mMetrics.maxAdvance = atsMetrics.maxAdvanceWidth * mAdjustedSize;
    mMetrics.aveCharWidth = atsMetrics.avgAdvanceWidth * mAdjustedSize;
    mMetrics.xHeight = atsMetrics.xHeight * mAdjustedSize;

    mIsValid = PR_TRUE;
}
