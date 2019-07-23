






































#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"

#include "gfxTypes.h"

#include "nsPromiseFlatString.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxAtsuiFonts.h"

#include "gfxFontTest.h"
#include "gfxFontUtils.h"

#include "cairo-atsui.h"

#include "gfxQuartzSurface.h"
#include "gfxQuartzFontCache.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsIPrefLocalizedString.h"
#include "nsServiceManagerUtils.h"
#include "nsUnicodeRange.h"
#include "nsCRT.h"




#define ROUND(x) (floor((x) + 0.5))


#ifdef FloatToFixed
#undef FloatToFixed
#define FloatToFixed(a)     ((Fixed)((float)(a) * fixed1))
#endif


#if 0
OSStatus ATSUGetStyleGroup(ATSUStyle style, void **styleGroup);
OSStatus ATSUDisposeStyleGroup(void *styleGroup);
OSStatus ATSUConvertCharToGlyphs(void *styleGroup,
                                 PRunichar *buffer
                                 unsigned int bufferLength,
                                 void *glyphVector);
OSStatus ATSInitializeGlyphVector(int size, void *glyphVectorPtr);
OSStatus ATSClearGlyphVector(void *glyphVectorPtr);
#endif



static const char *gPrefLangNames[] = {
    "x-western",
    "x-central-euro",
    "ja",
    "zh-TW",
    "zh-CN",
    "zh-HK",
    "ko",
    "x-cyrillic",
    "x-baltic",
    "el",
    "tr",
    "th",
    "he",
    "ar",
    "x-devanagari",
    "x-tamil",
    "x-armn",
    "x-beng",
    "x-cans",
    "x-ethi",
    "x-geor",
    "x-gujr",
    "x-guru",
    "x-khmr",
    "x-mlym",
    "x-unicode",
    "x-user-def"
};



enum eFontPrefLang {
    eFontPrefLang_Western     =  0,
    eFontPrefLang_CentEuro    =  1,
    eFontPrefLang_Japanese    =  2,
    eFontPrefLang_ChineseTW   =  3,
    eFontPrefLang_ChineseCN   =  4,
    eFontPrefLang_ChineseHK   =  5,
    eFontPrefLang_Korean      =  6,
    eFontPrefLang_Cyrillic    =  7,
    eFontPrefLang_Baltic      =  8,
    eFontPrefLang_Greek       =  9,
    eFontPrefLang_Turkish     = 10,
    eFontPrefLang_Thai        = 11,
    eFontPrefLang_Hebrew      = 12,
    eFontPrefLang_Arabic      = 13,
    eFontPrefLang_Devanagari  = 14,
    eFontPrefLang_Tamil       = 15,
    eFontPrefLang_Armenian    = 16,
    eFontPrefLang_Bengali     = 17,
    eFontPrefLang_Canadian    = 18,
    eFontPrefLang_Ethiopic    = 19,
    eFontPrefLang_Georgian    = 20,
    eFontPrefLang_Gujarati    = 21,
    eFontPrefLang_Gurmukhi    = 22,
    eFontPrefLang_Khmer       = 23,
    eFontPrefLang_Malayalam   = 24,

    eFontPrefLang_LangCount   = 25, 

    eFontPrefLang_Others      = 25, 
    eFontPrefLang_UserDefined = 26,

    eFontPrefLang_CJKSet      = 27, 
    eFontPrefLang_AllCount    = 28
};

static nsresult AppendAllPrefFonts(nsTArray<nsRefPtr<gfxFont> > *aFonts,
                eFontPrefLang aLang, PRUint32& didAppendBits, const gfxFontStyle *aStyle);

static eFontPrefLang GetFontPrefLangFor(const char* aLang);
eFontPrefLang GetFontPrefLangFor(PRUint8 aUnicodeRange);



gfxAtsuiFont::gfxAtsuiFont(ATSUFontID fontID,
                           const nsAString& name,
                           const gfxFontStyle *fontStyle)
    : gfxFont(name, fontStyle),
      mFontStyle(fontStyle), mATSUFontID(fontID), mATSUStyle(nsnull),
      mHasMirroring(PR_FALSE), mHasMirroringLookedUp(PR_FALSE), mAdjustedSize(0.0f)
{
    ATSFontRef fontRef = FMGetATSFontRefFromFont(fontID);

    InitMetrics(fontID, fontRef);

    mFontFace = cairo_atsui_font_face_create_for_atsu_font_id(mATSUFontID);
    
    mFontEntry = gfxQuartzFontCache::SharedFontCache()->FindFontEntry(mATSUFontID);                   
    
    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    cairo_font_options_t *fontOptions = cairo_font_options_create();
    mScaledFont = cairo_scaled_font_create(mFontFace, &sizeMatrix, &ctm, fontOptions);
    cairo_font_options_destroy(fontOptions);
    NS_ASSERTION(cairo_scaled_font_status(mScaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to create scaled font");
}

void
gfxAtsuiFont::InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef)
{
    

    ATSUAttributeTag styleTags[] = {
        kATSUFontTag,
        kATSUSizeTag,
        kATSUFontMatrixTag
    };

    ByteCount styleArgSizes[] = {
        sizeof(ATSUFontID),
        sizeof(Fixed),
        sizeof(CGAffineTransform),
        sizeof(Fract)
    };

    gfxFloat size =
        PR_MAX(((mAdjustedSize != 0.0f) ? mAdjustedSize : GetStyle()->size), 1.0f);

    

    
    Fixed fSize = FloatToFixed(size);
    ATSUFontID fid = aFontID;
    
    CGAffineTransform transform = CGAffineTransformMakeScale(1, -1);

    ATSUAttributeValuePtr styleArgs[] = {
        &fid,
        &fSize,
        &transform,
    };

    if (mATSUStyle)
        ATSUDisposeStyle(mATSUStyle);

    ATSUCreateStyle(&mATSUStyle);
    ATSUSetAttributes(mATSUStyle,
                      sizeof(styleTags)/sizeof(ATSUAttributeTag),
                      styleTags,
                      styleArgSizes,
                      styleArgs);

    

    ATSFontMetrics atsMetrics;
    ATSFontGetHorizontalMetrics(aFontRef, kATSOptionFlagsDefault,
                                &atsMetrics);

    if (atsMetrics.xHeight)
        mMetrics.xHeight = atsMetrics.xHeight * size;
    else
        mMetrics.xHeight = GetCharHeight('x');

    if (mAdjustedSize == 0.0f) {
        if (mMetrics.xHeight != 0.0f && GetStyle()->sizeAdjust != 0.0f) {
            gfxFloat aspect = mMetrics.xHeight / size;
            mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);
            InitMetrics(aFontID, aFontRef);
            return;
        }
        mAdjustedSize = size;
    }

    mMetrics.emHeight = size;

    mMetrics.maxAscent = NS_ceil(atsMetrics.ascent * size);
    mMetrics.maxDescent = NS_ceil(- (atsMetrics.descent * size));

    mMetrics.maxHeight = mMetrics.maxAscent + mMetrics.maxDescent;

    if (mMetrics.maxHeight - mMetrics.emHeight > 0)
        mMetrics.internalLeading = mMetrics.maxHeight - mMetrics.emHeight; 
    else
        mMetrics.internalLeading = 0.0;
    mMetrics.externalLeading = atsMetrics.leading * size;

    mMetrics.emAscent = mMetrics.maxAscent * mMetrics.emHeight / mMetrics.maxHeight;
    mMetrics.emDescent = mMetrics.emHeight - mMetrics.emAscent;

    mMetrics.maxAdvance = atsMetrics.maxAdvanceWidth * size;

    float xWidth = GetCharWidth('x');
    if (atsMetrics.avgAdvanceWidth != 0.0)
        mMetrics.aveCharWidth =
            PR_MIN(atsMetrics.avgAdvanceWidth * size, xWidth);
    else
        mMetrics.aveCharWidth = xWidth;

    if (gfxQuartzFontCache::SharedFontCache()->IsFixedPitch(aFontID)) {
        
        
        
        mMetrics.maxAdvance = mMetrics.aveCharWidth;
    }

    mMetrics.underlineOffset = -mMetrics.maxDescent - atsMetrics.underlinePosition * size;
    
    mMetrics.underlineSize = PR_MAX(1.0, atsMetrics.underlineThickness * size);

    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.superscriptOffset = mMetrics.xHeight;

    mMetrics.strikeoutOffset = mMetrics.xHeight / 2.0;
    mMetrics.strikeoutSize = mMetrics.underlineSize;

    PRUint32 glyphID;
    mMetrics.spaceWidth = GetCharWidth(' ', &glyphID);
    mSpaceGlyph = glyphID;

#if 0
    fprintf (stderr, "Font: %p size: %f (fixed: %d)", this, size, gfxQuartzFontCache::SharedFontCache()->IsFixedPitch(aFontID));
    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f maxAdvance: %f\n", mMetrics.maxAscent, mMetrics.maxDescent, mMetrics.maxAdvance);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.externalLeading, mMetrics.internalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

PRBool
gfxAtsuiFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}

nsString
gfxAtsuiFont::GetUniqueName()
{
    return mName;
}

float
gfxAtsuiFont::GetCharWidth(PRUnichar c, PRUint32 *aGlyphID)
{
    
    
    
    ATSUTextLayout layout;

    UniCharCount one = 1;
    ATSUCreateTextLayoutWithTextPtr(&c, 0, 1, 1, 1, &one, &mATSUStyle, &layout);

    ATSTrapezoid trap;
    ItemCount numBounds;
    ATSUGetGlyphBounds(layout, FloatToFixed(0.0), FloatToFixed(0.0),
                       0, 1, kATSUseFractionalOrigins, 1, &trap, &numBounds);

    float f =
        FixedToFloat(PR_MAX(trap.upperRight.x, trap.lowerRight.x)) -
        FixedToFloat(PR_MIN(trap.upperLeft.x, trap.lowerLeft.x));

    if (aGlyphID) {
        ATSUGlyphInfoArray glyphInfo;
        ByteCount bytes = sizeof(glyphInfo);
        ATSUGetGlyphInfo(layout, 0, 1, &bytes, &glyphInfo);
        *aGlyphID = glyphInfo.glyphs[0].glyphID;
    }

    ATSUDisposeTextLayout(layout);

    return f;
}

float
gfxAtsuiFont::GetCharHeight(PRUnichar c)
{
    
    
    
    ATSUTextLayout layout;

    UniCharCount one = 1;
    ATSUCreateTextLayoutWithTextPtr(&c, 0, 1, 1, 1, &one, &mATSUStyle, &layout);

    Rect rect;
    ATSUMeasureTextImage(layout, 0, 1, 0, 0, &rect);

    ATSUDisposeTextLayout(layout);

    return rect.bottom - rect.top;
}

gfxAtsuiFont::~gfxAtsuiFont()
{
    cairo_scaled_font_destroy(mScaledFont);
    cairo_font_face_destroy(mFontFace);

    ATSUDisposeStyle(mATSUStyle);
}

const gfxFont::Metrics&
gfxAtsuiFont::GetMetrics()
{
    return mMetrics;
}

void
gfxAtsuiFont::SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
        PRBool aNeedTight, gfxGlyphExtents *aExtents)
{
    ATSGlyphScreenMetrics metrics;
    GlyphID glyph = aGlyphID;
    OSStatus err = ATSUGlyphGetScreenMetrics(mATSUStyle, 1, &glyph, 0, false, false,
                                             &metrics);
    if (err != noErr)
        return;
    PRUint32 appUnitsPerDevUnit = aExtents->GetAppUnitsPerDevUnit();
    
    if (!aNeedTight && metrics.topLeft.x >= 0 &&
        -metrics.topLeft.y + metrics.height <= mMetrics.maxAscent &&
        metrics.topLeft.y <= mMetrics.maxDescent) {
        PRUint32 appUnitsWidth =
            PRUint32(NS_ceil((metrics.topLeft.x + metrics.width)*appUnitsPerDevUnit));
        if (appUnitsWidth < gfxGlyphExtents::INVALID_WIDTH) {
            aExtents->SetContainedGlyphWidthAppUnits(aGlyphID, PRUint16(appUnitsWidth));
            return;
        }
    }

    double d2a = appUnitsPerDevUnit;
    gfxRect bounds(metrics.topLeft.x*d2a, (metrics.topLeft.y - metrics.height)*d2a,
                   metrics.width*d2a, metrics.height*d2a);
    aExtents->SetTightGlyphExtents(aGlyphID, bounds);
}

PRBool 
gfxAtsuiFont::HasMirroringInfo()
{
    if (!mHasMirroringLookedUp) {
        OSStatus status;
        ByteCount size;
        
        
        status = ATSFontGetTable(GetATSUFontID(), 'prop', 0, 0, 0, &size);
        mHasMirroring = (status == noErr);
        mHasMirroringLookedUp = PR_TRUE;
    }
    
    return mHasMirroring;
}

PRBool gfxAtsuiFont::TestCharacterMap(PRUint32 aCh) {
    return mFontEntry->TestCharacterMap(aCh);
}






static already_AddRefed<gfxAtsuiFont>
GetOrMakeFont(ATSUFontID aFontID, const gfxFontStyle *aStyle)
{
    const nsAString& name =
        gfxQuartzFontCache::SharedFontCache()->GetPostscriptNameForFontID(aFontID);
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(name, aStyle);
    if (!font) {
        font = new gfxAtsuiFont(aFontID, name, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxAtsuiFont *>(f);
}

gfxAtsuiFontGroup::gfxAtsuiFontGroup(const nsAString& families,
                                     const gfxFontStyle *aStyle)
    : gfxFontGroup(families, aStyle)
{
    ForEachFont(FindATSUFont, this);

    if (mFonts.Length() == 0) {
        
        
        
        

        

        
        
        
        ATSUFontID fontID = gfxQuartzFontCache::SharedFontCache()->GetDefaultATSUFontID (aStyle);
        NS_ASSERTION(fontID != kATSUInvalidFontID, "invalid default font returned by GetDefaultATSUFontID");

        nsRefPtr<gfxAtsuiFont> font = GetOrMakeFont(fontID, aStyle);
        if (font) {
            mFonts.AppendElement(font);
        }
    }
}

PRBool
gfxAtsuiFontGroup::FindATSUFont(const nsAString& aName,
                                const nsACString& aGenericName,
                                void *closure)
{
    gfxAtsuiFontGroup *fontGroup = (gfxAtsuiFontGroup*) closure;
    const gfxFontStyle *fontStyle = fontGroup->GetStyle();

    gfxQuartzFontCache *fc = gfxQuartzFontCache::SharedFontCache();
    ATSUFontID fontID = fc->FindATSUFontIDForFamilyAndStyle (aName, fontStyle);

    if (fontID != kATSUInvalidFontID && !fontGroup->HasFont(fontID)) {
        
        nsRefPtr<gfxAtsuiFont> font = GetOrMakeFont(fontID, fontStyle);
        if (font) {
            fontGroup->mFonts.AppendElement(font);
        }
    }

    return PR_TRUE;
}

gfxFontGroup *
gfxAtsuiFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxAtsuiFontGroup(mFamilies, aStyle);
}

static void
SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString)
{
    TextBreakLocatorRef locator;
    OSStatus status = UCCreateTextBreakLocator(NULL, 0, kUCTextBreakClusterMask,
                                               &locator);
    if (status != noErr)
        return;
    UniCharArrayOffset breakOffset = 0;
    UCTextBreakOptions options = kUCTextBreakLeadingEdgeMask;
    PRUint32 length = aTextRun->GetLength();
    while (breakOffset < length) {
        UniCharArrayOffset next;
        status = UCFindTextBreak(locator, kUCTextBreakClusterMask, options,
                                 aString, length, breakOffset, &next);
        if (status != noErr)
            break;
        options |= kUCTextBreakIterateMask;
        PRUint32 i;
        for (i = breakOffset + 1; i < next; ++i) {
            gfxTextRun::CompressedGlyph g;
            
            
            
            aTextRun->SetGlyphs(i, g.SetComplex(PR_FALSE, PR_TRUE, 0), nsnull);
        }
        breakOffset = next;
    }
    UCDisposeTextBreakLocator(&locator);
}

#define UNICODE_LRO 0x202d
#define UNICODE_RLO 0x202e
#define UNICODE_PDF 0x202c

static void
AppendDirectionalIndicator(PRUint32 aFlags, nsAString& aString)
{
    static const PRUnichar overrides[2] = { UNICODE_LRO, UNICODE_RLO };
    aString.Append(overrides[(aFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0]);
}








static PRUint32
FindTextRunSegmentLength(gfxTextRun *aTextRun, PRUint32 aOffset, PRUint32 aMaxLength)
{
    if (aOffset + aMaxLength >= aTextRun->GetLength()) {
        
        
        return aTextRun->GetLength() - aOffset;
    }

    
    
    PRUint32 end;
    for (end = aOffset + aMaxLength; end > aOffset; --end) {
        if (aTextRun->IsClusterStart(end) &&
            (aTextRun->GetChar(end) == ' ' || aTextRun->GetChar(end - 1) == ' '))
            return end - aOffset;
    }

    
    for (end = aOffset + aMaxLength; end > aOffset; --end) {
        if (aTextRun->IsClusterStart(end))
            return end - aOffset;
    }

    
    
    for (end = aOffset + 1; end < aTextRun->GetLength(); ++end) {
        if (aTextRun->IsClusterStart(end))
            return end - aOffset;
    }
    return aTextRun->GetLength() - aOffset;
}

PRUint32
gfxAtsuiFontGroup::GuessMaximumStringLength()
{
    
    
    
    
    
    
    
    
    
    PRUint32 maxAdvance = PRUint32(GetFontAt(0)->GetMetrics().maxAdvance);
    PRUint32 chars = 0x7FFF/PR_MAX(1, maxAdvance);
    return PR_MAX(1, chars);
}
















gfxTextRun *
gfxAtsuiFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    textRun->RecordSurrogates(aString);
    SetupClusterBoundaries(textRun, aString);

    PRUint32 maxLen;
    nsAutoString utf16;
    for (maxLen = GuessMaximumStringLength(); maxLen > 0; maxLen /= 2) {
        PRUint32 start = 0;
        while (start < aLength) {
            PRUint32 len = FindTextRunSegmentLength(textRun, start, maxLen);

            utf16.Truncate();
            AppendDirectionalIndicator(aFlags, utf16);
            utf16.Append(aString + start, len);
            
            
            utf16.Append('.');
            utf16.Append(UNICODE_PDF);
            if (!InitTextRun(textRun, utf16.get(), utf16.Length(), PR_TRUE,
                             start, len) && maxLen > 1)
                break;
            start += len;
        }
        if (start == aLength)
            break;
        textRun->ResetGlyphRuns();
    }
    
    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxAtsuiFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "should be marked 8bit");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    PRUint32 maxLen;
    nsAutoString utf16;
    for (maxLen = GuessMaximumStringLength(); maxLen > 0; maxLen /= 2) {
        PRUint32 start = 0;
        while (start < aLength) {
            PRUint32 len = FindTextRunSegmentLength(textRun, start, maxLen);

            nsDependentCSubstring cString(reinterpret_cast<const char*>(aString + start),
                                          reinterpret_cast<const char*>(aString + start + len));
            utf16.Truncate();
            PRBool wrapBidi = (aFlags & TEXT_IS_RTL) != 0;
            if (wrapBidi) {
              AppendDirectionalIndicator(aFlags, utf16);
            }
            AppendASCIItoUTF16(cString, utf16);
            if (wrapBidi) {
              utf16.Append('.');
              utf16.Append(UNICODE_PDF);
            }
            if (!InitTextRun(textRun, utf16.get(), utf16.Length(), wrapBidi,
                             start, len) && maxLen > 1)
                break;
            start += len;
        }
        if (start == aLength)
            break;
        textRun->ResetGlyphRuns();
    }

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

already_AddRefed<gfxAtsuiFont>
gfxAtsuiFontGroup::FindFontFor(ATSUFontID fid)
{
    gfxAtsuiFont *font;

    
    
    
    for (PRUint32 i = 0; i < FontListLength(); i++) {
        font = GetFontAt(i);
        if (font->GetATSUFontID() == fid)
            return font;
    }

    
    nsRefPtr<gfxAtsuiFont> f = GetOrMakeFont(fid, GetStyle());
    return f.forget();
}

PRBool
gfxAtsuiFontGroup::HasFont(ATSUFontID fid)
{
    for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
        if (fid == static_cast<gfxAtsuiFont *>(mFonts.ElementAt(i).get())->GetATSUFontID())
            return PR_TRUE;
    }
    return PR_FALSE;
}

already_AddRefed<gfxAtsuiFont>
gfxAtsuiFontGroup::FindFontForChar(PRUint32 aCh, PRUint32 aPrevCh, PRUint32 aNextCh, gfxAtsuiFont* aPrevMatchedFont)
{
    nsRefPtr<gfxAtsuiFont>    selectedFont;
    
    
    
    if (gfxFontUtils::IsJoiner(aCh) || gfxFontUtils::IsJoiner(aPrevCh) || gfxFontUtils::IsJoiner(aNextCh)) {
        if (aPrevMatchedFont && aPrevMatchedFont->TestCharacterMap(aCh)) {
            selectedFont = aPrevMatchedFont;
            return selectedFont.forget();
        }
    }
    
    
    selectedFont = WhichFontSupportsChar(mFonts, aCh);
    
    
    if ((aCh >= 0xE000  && aCh <= 0xF8FF) || 
        (aCh >= 0xF0000 && aCh <= 0x10FFFD))
        return selectedFont.forget();
    if ( selectedFont ) 
        return selectedFont.forget();
    
    
    if (aCh <= 0xFFFF) {  
        nsresult rv;
        PRUint32 unicodeRange = FindCharUnicodeRange(aCh);
        PRUint32 didAppendFonts = 0;
                
        nsAutoTArray<nsRefPtr<gfxFont>, 15> prefFonts;

        
        eFontPrefLang prefLang = GetFontPrefLangFor(unicodeRange);
        
        rv = AppendAllPrefFonts(&prefFonts, prefLang, didAppendFonts, GetStyle());
        if (!NS_FAILED(rv)) {
            selectedFont = WhichFontSupportsChar(prefFonts, aCh);
            if (selectedFont) { 
                return selectedFont.forget();
            }
        }
    }

    
    
    if (!selectedFont && aPrevMatchedFont && aPrevMatchedFont->TestCharacterMap(aCh)) {
        selectedFont = aPrevMatchedFont;
        return selectedFont.forget();
    }
    
    
    if (!selectedFont) {
        FontEntry *fe;
        
        fe = gfxQuartzFontCache::SharedFontCache()->FindFontForChar(aCh, GetFontAt(0));
        if (fe) {
            selectedFont = FindFontFor(fe->GetFontID());
            return selectedFont.forget();
        }
    }

    return nsnull;
}




class AutoLayoutDataArrayPtr {
public:
    AutoLayoutDataArrayPtr(ATSULineRef aLineRef,
                           ATSUDirectDataSelector aSelector)
        : mLineRef(aLineRef), mSelector(aSelector)
    {
        OSStatus status =
            ATSUDirectGetLayoutDataArrayPtrFromLineRef(aLineRef,
                aSelector, PR_FALSE, &mArray, &mItemCount);
        if (status != noErr) {
            mArray = NULL;
            mItemCount = 0;
        }
    }
    ~AutoLayoutDataArrayPtr() {
        if (mArray) {
            ATSUDirectReleaseLayoutDataArrayPtr(mLineRef, mSelector, &mArray);
        }
    }

    void     *mArray;
    ItemCount mItemCount;

private:
    ATSULineRef            mLineRef;
    ATSUDirectDataSelector mSelector;
};

#define ATSUI_SPECIAL_GLYPH_ID       0xFFFF




#define ATSUI_OVERRUNNING_GLYPH_FLAG 0x100000




static PRInt32
GetAdvanceAppUnits(ATSLayoutRecord *aGlyphs, PRUint32 aGlyphCount,
                   PRUint32 aAppUnitsPerDevUnit)
{
    Fixed fixedAdvance = aGlyphs[aGlyphCount].realPos - aGlyphs->realPos;
    return PRInt32((PRInt64(fixedAdvance)*aAppUnitsPerDevUnit + (1 << 15)) >> 16);
}






static void
SetGlyphsForCharacterGroup(ATSLayoutRecord *aGlyphs, PRUint32 aGlyphCount,
                           Fixed *aBaselineDeltas, PRUint32 aAppUnitsPerDevUnit,
                           gfxTextRun *aRun, PRUint32 aSegmentStart,
                           const PRPackedBool *aUnmatched,
                           const PRUnichar *aString,
                           const PRUint32 aLength)
{
    NS_ASSERTION(aGlyphCount > 0, "Must set at least one glyph");
    PRUint32 firstOffset = aGlyphs[0].originalOffset;
    PRUint32 lastOffset = firstOffset;
    PRUint32 i;
    PRUint32 regularGlyphCount = 0;
    ATSLayoutRecord *displayGlyph = nsnull;
    PRBool inOrder = PR_TRUE;
    PRBool allMatched = PR_TRUE;

    for (i = 0; i < aGlyphCount; ++i) {
        ATSLayoutRecord *glyph = &aGlyphs[i];
        PRUint32 offset = glyph->originalOffset;
        firstOffset = PR_MIN(firstOffset, offset);
        lastOffset = PR_MAX(lastOffset, offset);
        if (aUnmatched && aUnmatched[offset/2]) {
            allMatched = PR_FALSE;
        }
        if (glyph->glyphID != ATSUI_SPECIAL_GLYPH_ID) {
            ++regularGlyphCount;
            displayGlyph = glyph;
        }
        if (i > 0 && aRun->IsRightToLeft() != (offset < aGlyphs[i - 1].originalOffset)) { 
            inOrder = PR_FALSE;
        }
    }

    NS_ASSERTION(!gfxFontGroup::IsInvalidChar(aString[firstOffset/2]),
                 "Invalid char passed in");

    if (!allMatched) {
        for (i = firstOffset; i <= lastOffset; ++i) {
            PRUint32 index = i/2;
            if (NS_IS_HIGH_SURROGATE(aString[index]) &&
                index + 1 < aLength &&
                NS_IS_LOW_SURROGATE(aString[index + 1])) {
                aRun->SetMissingGlyph(aSegmentStart + index,
                                      SURROGATE_TO_UCS4(aString[index],
                                                        aString[index + 1]));
            } else {
                aRun->SetMissingGlyph(aSegmentStart + index, aString[index]);
            }
        }
        return;
    }

    gfxTextRun::CompressedGlyph g;
    PRUint32 offset;
    
    
    
    
    for (offset = firstOffset + 2; offset <= lastOffset; offset += 2) {
        PRUint32 index = offset/2;        
        PRBool makeClusterStart = inOrder && aRun->IsClusterStart(index);
        g.SetComplex(makeClusterStart, PR_FALSE, 0);
        aRun->SetGlyphs(aSegmentStart + index, g, nsnull);
    }

    
    PRInt32 advance = GetAdvanceAppUnits(aGlyphs, aGlyphCount, aAppUnitsPerDevUnit);
    PRUint32 charIndex = aSegmentStart + firstOffset/2;
    if (regularGlyphCount == 1) {
        if (advance >= 0 &&
            (!aBaselineDeltas || aBaselineDeltas[displayGlyph - aGlyphs] == 0) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(displayGlyph->glyphID)) {
            aRun->SetSimpleGlyph(charIndex, g.SetSimpleGlyph(advance, displayGlyph->glyphID));
            return;
        }
    }

    nsAutoTArray<gfxTextRun::DetailedGlyph,10> detailedGlyphs;
    ATSLayoutRecord *advanceStart = aGlyphs;
    for (i = 0; i < aGlyphCount; ++i) {
        ATSLayoutRecord *glyph = &aGlyphs[i];
        if (glyph->glyphID != ATSUI_SPECIAL_GLYPH_ID) {
            if (glyph->originalOffset > firstOffset) {
                PRUint32 glyphCharIndex = aSegmentStart + glyph->originalOffset/2;
                PRUint32 glyphRunIndex = aRun->FindFirstGlyphRunContaining(glyphCharIndex);
                PRUint32 numGlyphRuns;
                const gfxTextRun::GlyphRun *glyphRun = aRun->GetGlyphRuns(&numGlyphRuns) + glyphRunIndex;

                if (glyphRun->mCharacterOffset > charIndex) {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    NS_ERROR("Font change inside character group!");
                    
                    continue;
                }
            }

            gfxTextRun::DetailedGlyph *details = detailedGlyphs.AppendElement();
            if (!details)
                return;
            details->mAdvance = 0;
            details->mGlyphID = glyph->glyphID;
            details->mXOffset = 0;
            if (detailedGlyphs.Length() > 1) {
                details->mXOffset +=
                    GetAdvanceAppUnits(advanceStart, glyph - advanceStart,
                                       aAppUnitsPerDevUnit);
            }
            details->mYOffset = !aBaselineDeltas ? 0.0f
                : FixedToFloat(aBaselineDeltas[i])*aAppUnitsPerDevUnit;
        }
    }
    if (detailedGlyphs.Length() == 0) {
        NS_WARNING("No glyphs visible at all!");
        aRun->SetGlyphs(aSegmentStart + charIndex, g.SetMissing(0), nsnull);
        return;
    }

    
    PRInt32 clusterAdvance = GetAdvanceAppUnits(aGlyphs, aGlyphCount, aAppUnitsPerDevUnit);
    if (aRun->IsRightToLeft())
        detailedGlyphs[0].mAdvance = clusterAdvance;
    else
        detailedGlyphs[detailedGlyphs.Length() - 1].mAdvance = clusterAdvance;
    g.SetComplex(aRun->IsClusterStart(charIndex), PR_TRUE, detailedGlyphs.Length());
    aRun->SetGlyphs(charIndex, g, detailedGlyphs.Elements());
}




static PRBool
PostLayoutCallback(ATSULineRef aLine, gfxTextRun *aRun,
                   const PRUnichar *aString, PRBool aWrapped,
                   const PRPackedBool *aUnmatched,
                   PRUint32 aSegmentStart, PRUint32 aSegmentLength)
{
    
    
    
    AutoLayoutDataArrayPtr baselineDeltasArray(aLine, kATSUDirectDataBaselineDeltaFixedArray);
    Fixed *baselineDeltas = static_cast<Fixed *>(baselineDeltasArray.mArray);
    AutoLayoutDataArrayPtr glyphRecordsArray(aLine, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent);

    PRUint32 numGlyphs = glyphRecordsArray.mItemCount;
    if (numGlyphs == 0 || !glyphRecordsArray.mArray) {
        NS_WARNING("Failed to retrieve key glyph data");
        return PR_FALSE;
    }
    ATSLayoutRecord *glyphRecords = static_cast<ATSLayoutRecord *>(glyphRecordsArray.mArray);
    NS_ASSERTION(!baselineDeltas || baselineDeltasArray.mItemCount == numGlyphs,
                 "Mismatched glyph counts");
    NS_ASSERTION(glyphRecords[numGlyphs - 1].flags & kATSGlyphInfoTerminatorGlyph,
                 "Last glyph should be a terminator glyph");
    --numGlyphs;
    if (numGlyphs == 0)
        return PR_FALSE;

    PRUint32 appUnitsPerDevUnit = aRun->GetAppUnitsPerDevUnit();
    PRBool isRTL = aRun->IsRightToLeft();

    if (aWrapped) {
        
        
        if (isRTL) {
            NS_ASSERTION(glyphRecords[0].originalOffset == aSegmentLength*2,
                         "Couldn't find glyph for trailing marker");
            glyphRecords++;
        } else {
            NS_ASSERTION(glyphRecords[numGlyphs - 1].originalOffset == aSegmentLength*2,
                         "Couldn't find glyph for trailing marker");
        }
        --numGlyphs;
        if (numGlyphs == 0)
            return PR_FALSE;
    }

    PRUint32 allFlags = 0;
    
    
    PRInt32 direction = PRInt32(aRun->GetDirection());
    while (numGlyphs > 0) {
        PRUint32 glyphIndex = isRTL ? numGlyphs - 1 : 0;
        PRUint32 lastOffset = glyphRecords[glyphIndex].originalOffset;
        PRUint32 glyphCount = 1;
        
        while (glyphCount < numGlyphs) {
            ATSLayoutRecord *glyph = &glyphRecords[glyphIndex + direction*glyphCount];
            PRUint32 glyphOffset = glyph->originalOffset;
            allFlags |= glyph->flags;
            if (glyphOffset <= lastOffset) {
                
                
                
                
                
                
                
                
            } else {
                
                if (glyph->glyphID != ATSUI_SPECIAL_GLYPH_ID) {
                    
                    break;
                }
                if (aUnmatched && aUnmatched[glyphOffset/2]) {
                    
                    break;
                }
                
                
                lastOffset = glyphOffset;
            }
            ++glyphCount;
        }
        if (isRTL) {
            SetGlyphsForCharacterGroup(glyphRecords + numGlyphs - glyphCount,
                                       glyphCount,
                                       baselineDeltas ? baselineDeltas + numGlyphs - glyphCount : nsnull,
                                       appUnitsPerDevUnit, aRun, aSegmentStart,
                                       aUnmatched, aString, aSegmentLength);
        } else {
            SetGlyphsForCharacterGroup(glyphRecords,
                                       glyphCount, baselineDeltas,
                                       appUnitsPerDevUnit, aRun, aSegmentStart,
                                       aUnmatched, aString, aSegmentLength);
            glyphRecords += glyphCount;
            if (baselineDeltas) {
                baselineDeltas += glyphCount;
            }
        }
        numGlyphs -= glyphCount;
    }
    
    return (allFlags & ATSUI_OVERRUNNING_GLYPH_FLAG) != 0;
}

struct PostLayoutCallbackClosure {
    gfxTextRun                  *mTextRun;
    const PRUnichar             *mString;
    
    
    nsAutoArrayPtr<PRPackedBool> mUnmatchedChars;
    PRUint32                     mSegmentStart;
    PRUint32                     mSegmentLength;
    
    
    PRPackedBool                 mWrapped;
    
    PRPackedBool                 mOverrunningGlyphs;
};


static PostLayoutCallbackClosure *gCallbackClosure = nsnull;

static OSStatus
PostLayoutOperationCallback(ATSULayoutOperationSelector iCurrentOperation, 
                            ATSULineRef iLineRef, 
                            UInt32 iRefCon, 
                            void *iOperationCallbackParameterPtr, 
                            ATSULayoutOperationCallbackStatus *oCallbackStatus)
{
    gCallbackClosure->mOverrunningGlyphs =
        PostLayoutCallback(iLineRef, gCallbackClosure->mTextRun,
                           gCallbackClosure->mString, gCallbackClosure->mWrapped,
                           gCallbackClosure->mUnmatchedChars,
                           gCallbackClosure->mSegmentStart,
                           gCallbackClosure->mSegmentLength);
    *oCallbackStatus = kATSULayoutOperationCallbackStatusContinue;
    return noErr;
}

static eFontPrefLang
GetFontPrefLangFor(const char* aLang)
{
    if (!aLang || aLang[0])
        return eFontPrefLang_Others;
    for (PRUint32 i = 0; i < PRUint32(eFontPrefLang_LangCount); ++i) {
        if (!PL_strcasecmp(gPrefLangNames[i], aLang))
            return eFontPrefLang(i);
    }
    return eFontPrefLang_Others;
}

eFontPrefLang
GetFontPrefLangFor(PRUint8 aUnicodeRange)
{
    switch (aUnicodeRange) {
        case kRangeCyrillic:   return eFontPrefLang_Cyrillic;
        case kRangeGreek:      return eFontPrefLang_Greek;
        case kRangeTurkish:    return eFontPrefLang_Turkish;
        case kRangeHebrew:     return eFontPrefLang_Hebrew;
        case kRangeArabic:     return eFontPrefLang_Arabic;
        case kRangeBaltic:     return eFontPrefLang_Baltic;
        case kRangeThai:       return eFontPrefLang_Thai;
        case kRangeKorean:     return eFontPrefLang_Korean;
        case kRangeJapanese:   return eFontPrefLang_Japanese;
        case kRangeSChinese:   return eFontPrefLang_ChineseCN;
        case kRangeTChinese:   return eFontPrefLang_ChineseTW;
        case kRangeDevanagari: return eFontPrefLang_Devanagari;
        case kRangeTamil:      return eFontPrefLang_Tamil;
        case kRangeArmenian:   return eFontPrefLang_Armenian;
        case kRangeBengali:    return eFontPrefLang_Bengali;
        case kRangeCanadian:   return eFontPrefLang_Canadian;
        case kRangeEthiopic:   return eFontPrefLang_Ethiopic;
        case kRangeGeorgian:   return eFontPrefLang_Georgian;
        case kRangeGujarati:   return eFontPrefLang_Gujarati;
        case kRangeGurmukhi:   return eFontPrefLang_Gurmukhi;
        case kRangeKhmer:      return eFontPrefLang_Khmer;
        case kRangeMalayalam:  return eFontPrefLang_Malayalam;
        case kRangeSetCJK:     return eFontPrefLang_CJKSet;
        default:               return eFontPrefLang_Others;
    }
}

static const char*
GetPrefLangName(eFontPrefLang aLang)
{
    if (PRUint32(aLang) < PRUint32(eFontPrefLang_AllCount))
        return gPrefLangNames[PRUint32(aLang)];
    return nsnull;
}

struct AFLClosure {
    const gfxFontStyle *style;
    nsTArray<nsRefPtr<gfxFont> > *fontArray;
};



PRBool
AppendFontToList(const nsAString& aName,
                 const nsACString& aGenericName,
                 void *closure)
{
    struct AFLClosure *afl = (struct AFLClosure *) closure;

    gfxQuartzFontCache *fc = gfxQuartzFontCache::SharedFontCache();
    ATSUFontID fontID = fc->FindATSUFontIDForFamilyAndStyle (aName, afl->style);

    if (fontID != kATSUInvalidFontID) {
        
        nsRefPtr<gfxAtsuiFont> font = GetOrMakeFont(fontID, afl->style);
        if (font) {
            afl->fontArray->AppendElement(font);
        }
    }

    return PR_TRUE;
}

static nsresult
AppendPrefFonts(nsTArray<nsRefPtr<gfxFont> > *aFonts,
                eFontPrefLang aLang,
                PRUint32& didAppendBits,
                const gfxFontStyle *aStyle)
{
    if (didAppendBits & (1 << aLang))
        return NS_OK;

    didAppendBits |= (1 << aLang);

    const char* langGroup = GetPrefLangName(aLang);
    if (!langGroup || !langGroup[0]) {
        NS_ERROR("The langGroup is null");
        return NS_ERROR_FAILURE;
    }
    gfxPlatform *platform = gfxPlatform::GetPlatform();
    NS_ENSURE_TRUE(platform, NS_ERROR_OUT_OF_MEMORY);
    nsString fonts;
    platform->GetPrefFonts(langGroup, fonts, PR_FALSE);
    if (fonts.IsEmpty())
        return NS_OK;

    struct AFLClosure afl = { aStyle, aFonts };
    gfxFontGroup::ForEachFont(fonts, nsDependentCString(langGroup),
                              AppendFontToList, &afl);
    return NS_OK;
}

static nsresult
AppendCJKPrefFonts(nsTArray<nsRefPtr<gfxFont> > *aFonts,
                   PRUint32& didAppendBits,
                   const gfxFontStyle *aStyle)
{
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));

    
    nsCAutoString list;
    nsresult rv;
    if (prefs) {
        nsCOMPtr<nsIPrefLocalizedString> prefString;
        rv = prefs->GetComplexValue("intl.accept_languages", NS_GET_IID(nsIPrefLocalizedString), getter_AddRefs(prefString));
        if (prefString) {
            nsAutoString temp;
            prefString->ToString(getter_Copies(temp));
            LossyCopyUTF16toASCII(temp, list);
        }
    }
    
    if (NS_SUCCEEDED(rv) && !list.IsEmpty()) {
        const char kComma = ',';
        const char *p, *p_end;
        list.BeginReading(p);
        list.EndReading(p_end);
        while (p < p_end) {
            while (nsCRT::IsAsciiSpace(*p)) {
                if (++p == p_end)
                    break;
            }
            if (p == p_end)
                break;
            const char *start = p;
            while (++p != p_end && *p != kComma)
                 ;
            nsCAutoString lang(Substring(start, p));
            lang.CompressWhitespace(PR_FALSE, PR_TRUE);
            eFontPrefLang fpl = GetFontPrefLangFor(lang.get());
            switch (fpl) {
                case eFontPrefLang_Japanese:
                case eFontPrefLang_Korean:
                case eFontPrefLang_ChineseCN:
                case eFontPrefLang_ChineseHK:
                case eFontPrefLang_ChineseTW:
                    rv = AppendPrefFonts(aFonts, fpl, didAppendBits, aStyle);
                    NS_ENSURE_SUCCESS(rv, rv);
                    break;
                default:
                    break;
            }
            p++;
        }
    }

    
    ScriptCode sysScript = ::GetScriptManagerVariable(smSysScript);
    
    switch (sysScript) {
        case smJapanese:    rv = AppendPrefFonts(aFonts, eFontPrefLang_Japanese, didAppendBits, aStyle);  break;
        case smTradChinese: rv = AppendPrefFonts(aFonts, eFontPrefLang_ChineseTW, didAppendBits, aStyle); break;
        case smKorean:      rv = AppendPrefFonts(aFonts, eFontPrefLang_Korean, didAppendBits, aStyle);    break;
        case smSimpChinese: rv = AppendPrefFonts(aFonts, eFontPrefLang_ChineseCN, didAppendBits, aStyle); break;
        default:            rv = NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = AppendPrefFonts(aFonts, eFontPrefLang_Japanese, didAppendBits, aStyle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AppendPrefFonts(aFonts, eFontPrefLang_Korean, didAppendBits, aStyle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AppendPrefFonts(aFonts, eFontPrefLang_ChineseCN, didAppendBits, aStyle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AppendPrefFonts(aFonts, eFontPrefLang_ChineseHK, didAppendBits, aStyle);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AppendPrefFonts(aFonts, eFontPrefLang_ChineseTW, didAppendBits, aStyle);
    return rv;
}

static nsresult
AppendAllPrefFonts(nsTArray<nsRefPtr<gfxFont> > *aFonts,
                eFontPrefLang aLang,
                PRUint32& didAppendBits,
                const gfxFontStyle *aStyle)
{
    nsresult rv;
    
    if (aLang == eFontPrefLang_CJKSet)
        rv = AppendCJKPrefFonts(aFonts, didAppendBits, aStyle);
    else
        rv = AppendPrefFonts(aFonts, aLang, didAppendBits, aStyle);
    
    if (NS_FAILED(rv)) return rv;

    rv = AppendPrefFonts(aFonts, eFontPrefLang_Others, didAppendBits, aStyle);
    return rv;
}

static void
DisableOptionalLigaturesInStyle(ATSUStyle aStyle)
{
    static ATSUFontFeatureType selectors[] = {
        kCommonLigaturesOffSelector,
        kRareLigaturesOffSelector,
        kLogosOffSelector,
        kRebusPicturesOffSelector,
        kDiphthongLigaturesOffSelector,
        kSquaredLigaturesOffSelector,
        kAbbrevSquaredLigaturesOffSelector,
        kSymbolLigaturesOffSelector
    };
    static ATSUFontFeatureType types[NS_ARRAY_LENGTH(selectors)] = {
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType
    };
    ATSUSetFontFeatures(aStyle, NS_ARRAY_LENGTH(selectors), types, selectors);
}
















static void MirrorSubstring(ATSUTextLayout layout, nsAutoArrayPtr<PRUnichar>& mirroredStr,
                            const PRUnichar *aString, PRUint32 aLength,
                            UniCharArrayOffset runStart, UniCharCount runLength)
{
    UniCharArrayOffset  off;
    
    
    for (off = runStart; off < runStart + runLength; off++) {
        PRUnichar  mirroredChar;
        
        mirroredChar = (PRUnichar) SymmSwap(aString[off]);
        if (mirroredChar != aString[off]) {
            
            if (mirroredStr == NULL) {
            
                
                mirroredStr = new PRUnichar[aLength];
                memcpy(mirroredStr, aString, sizeof(PRUnichar) * aLength);
                
                
                ATSUTextMoved(layout, mirroredStr);
                
            }
            mirroredStr[off] = mirroredChar;
        }
    }
}


class CmapFontMatcher {
public:
    CmapFontMatcher(const PRUnichar *aString, PRUint32 aBeginOffset, PRUint32 aEndOffset, gfxAtsuiFontGroup* aFontGroup) :
        mString(aString), mOffset(aBeginOffset), mPrevOffset(aBeginOffset), mEndOffset(aEndOffset), mPrevCh(0), mFirstRange(PR_TRUE), mFontGroup(aFontGroup), mMatchedFont(0), mNextMatchedFont(0)
    {}
    
    
    PRUint32 MatchNextRange() 
    { 
        PRUint32                matchStartOffset, chStartOffset, ch, nextCh;
        nsRefPtr<gfxAtsuiFont>  font;
        
        matchStartOffset = mPrevOffset;

        if ( !mFirstRange ) {
            mMatchedFont = mNextMatchedFont;
        }
        
        while ( mOffset < mEndOffset ) {
            chStartOffset = mOffset;
            
            
            ch = mString[mOffset];
            if ((mOffset+1 < mEndOffset) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(mString[mOffset+1])) {
                mOffset++;
                ch = SURROGATE_TO_UCS4(ch, mString[mOffset]);
            }
            
            
            nextCh = 0;
            if (mOffset+1 < mEndOffset) {
                nextCh = mString[mOffset+1];
                if ((mOffset+2 < mEndOffset) && NS_IS_HIGH_SURROGATE(nextCh) && NS_IS_LOW_SURROGATE(mString[mOffset+2]))
                    nextCh = SURROGATE_TO_UCS4(nextCh, mString[mOffset+2]);
            }

            
            font = mFontGroup->FindFontForChar(ch, mPrevCh, nextCh, mMatchedFont);
            mOffset++;
            mPrevCh = ch;
            
            
            if ( mFirstRange ) {
                mMatchedFont = font;
                mFirstRange = PR_FALSE;
            } else if ( font != mMatchedFont ) {
                mPrevOffset = chStartOffset;
                mNextMatchedFont = font;
                return chStartOffset - matchStartOffset;
            }
            
        }
        
        
        mPrevOffset = mEndOffset;
        mNextMatchedFont = nsnull;
        return mOffset - matchStartOffset;
    }
    
    inline gfxAtsuiFont* MatchedFont() { return mMatchedFont.get(); }

private:
    const PRUnichar         *mString;
    PRUint32                mOffset;
    PRUint32                mPrevOffset;
    PRUint32                mEndOffset;
    PRUint32                mPrevCh;
    PRBool                  mFirstRange;
    gfxAtsuiFontGroup       *mFontGroup;
    nsRefPtr<gfxAtsuiFont>  mMatchedFont;
    nsRefPtr<gfxAtsuiFont>  mNextMatchedFont;
};


static ATSUStyle
SetLayoutRangeToFont(ATSUTextLayout layout, ATSUStyle mainStyle, UniCharArrayOffset offset,
                      UniCharCount length, ATSUFontID fontID)
{
    ATSUStyle subStyle;
    ATSUCreateStyle (&subStyle);
    ATSUCopyAttributes (mainStyle, subStyle);

    ATSUAttributeTag fontTags[] = { kATSUFontTag };
    ByteCount fontArgSizes[] = { sizeof(ATSUFontID) };
    ATSUAttributeValuePtr fontArgs[] = { &fontID };

    ATSUSetAttributes (subStyle, 1, fontTags, fontArgSizes, fontArgs);

    
    ATSUSetRunStyle (layout, subStyle, offset, length);

    return subStyle;
}

#ifdef DUMP_TEXT_RUNS
static PRLogModuleInfo *gAtsuiTextRunLog = PR_NewLogModule("atsuiTextRun");
#endif

PRBool
gfxAtsuiFontGroup::InitTextRun(gfxTextRun *aRun,
                               const PRUnichar *aString, PRUint32 aLength,
                               PRBool aWrapped, PRUint32 aSegmentStart,
                               PRUint32 aSegmentLength)
{
    OSStatus status;
    gfxAtsuiFont *firstFont = GetFontAt(0);
    ATSUStyle mainStyle = firstFont->GetATSUStyle();
    nsTArray<ATSUStyle> stylesToDispose;
    PRUint32 headerChars = aWrapped ? 1 : 0;
    const PRUnichar *realString = aString + headerChars;
    NS_ASSERTION(aSegmentLength == aLength - (aWrapped ? 3 : 0),
                 "Length mismatch");

#ifdef DUMP_TEXT_RUNS
    NS_ConvertUTF16toUTF8 str(realString, aSegmentLength);
    NS_ConvertUTF16toUTF8 families(mFamilies);
    PR_LOG(gAtsuiTextRunLog, PR_LOG_DEBUG, ("InitTextRun %p fontgroup %p (%s) len %d TEXTRUN \"%s\" ENDTEXTRUN\n", aRun, this, families.get(), aSegmentLength, str.get()) );
    PR_LOG(gAtsuiTextRunLog, PR_LOG_DEBUG, ("InitTextRun font: %s\n", NS_ConvertUTF16toUTF8(firstFont->GetUniqueName()).get()) );
#endif

    if (aRun->GetFlags() & TEXT_DISABLE_OPTIONAL_LIGATURES) {
        status = ATSUCreateAndCopyStyle(mainStyle, &mainStyle);
        if (status == noErr) {
            stylesToDispose.AppendElement(mainStyle);
            DisableOptionalLigaturesInStyle(mainStyle);
        }
    }

    UniCharCount runLengths = aSegmentLength;
    ATSUTextLayout layout;
    
    
    
    
    status = ATSUCreateTextLayoutWithTextPtr
        (aString,
         headerChars,
         aSegmentLength + (aWrapped ? 1 : 0),
         aLength,
         1,
         &runLengths,
         &mainStyle,
         &layout);
    

    PostLayoutCallbackClosure closure;
    closure.mTextRun = aRun;
    closure.mString = realString;
    closure.mWrapped = aWrapped;
    closure.mSegmentStart = aSegmentStart;
    closure.mSegmentLength = aSegmentLength;
    NS_ASSERTION(!gCallbackClosure, "Reentering InitTextRun? Expect disaster!");
    gCallbackClosure = &closure;

    ATSULayoutOperationOverrideSpecifier override;
    override.operationSelector = kATSULayoutOperationPostLayoutAdjustment;
    override.overrideUPP = PostLayoutOperationCallback;

    
    ATSLineLayoutOptions lineLayoutOptions = kATSLineKeepSpacesOutOfMargin | kATSLineHasNoHangers;

    static ATSUAttributeTag layoutTags[] = {
        kATSULineLayoutOptionsTag,
        kATSULayoutOperationOverrideTag
    };
    static ByteCount layoutArgSizes[] = {
        sizeof(ATSLineLayoutOptions),
        sizeof(ATSULayoutOperationOverrideSpecifier)
    };

    ATSUAttributeValuePtr layoutArgs[] = {
        &lineLayoutOptions,
        &override
    };
    ATSUSetLayoutControls(layout,
                          NS_ARRAY_LENGTH(layoutTags),
                          layoutTags,
                          layoutArgSizes,
                          layoutArgs);

    

    nsAutoArrayPtr<PRUnichar> mirroredStr;

    UniCharArrayOffset runStart = headerChars;
    UniCharCount runLength = aSegmentLength;
    UniCharCount totalLength = headerChars + aSegmentLength;
    
    
    
    CmapFontMatcher fontMatcher(aString, runStart, runStart + runLength, this);
    
    while (runStart < totalLength) {
        gfxAtsuiFont *matchedFont;
        UniCharCount  matchedLength;
        
        
        matchedLength = fontMatcher.MatchNextRange();
        matchedFont = fontMatcher.MatchedFont();

#ifdef DUMP_TEXT_RUNS
        PR_LOG(gAtsuiTextRunLog, PR_LOG_DEBUG, ("InitTextRun %p fontgroup %p font %p match %s (%d-%d)", aRun, this, matchedFont, (matchedFont ? NS_ConvertUTF16toUTF8(matchedFont->GetUniqueName()).get() : "<null>"), runStart, matchedLength));
#endif
        
        
        
        if (aRun->IsRightToLeft() && matchedFont && !matchedFont->HasMirroringInfo()) {
            MirrorSubstring(layout, mirroredStr, aString, aLength, runStart, runLength);
        }       

        
        if (!matchedFont) {
        
            aRun->AddGlyphRun(firstFont, aSegmentStart + runStart - headerChars, PR_TRUE);
            
            if (!closure.mUnmatchedChars) {
                closure.mUnmatchedChars = new PRPackedBool[aLength];
                if (closure.mUnmatchedChars) {
                    
                    memset(closure.mUnmatchedChars.get(), PR_FALSE, aLength);
                }
            }
    
            if (closure.mUnmatchedChars) {
                
                memset(closure.mUnmatchedChars.get() + runStart - headerChars,
                       PR_TRUE, matchedLength);
            }
            
        } else {
        
            if (matchedFont != firstFont) {
                
                ATSUStyle subStyle = SetLayoutRangeToFont(layout, mainStyle, runStart, matchedLength, matchedFont->GetATSUFontID());
                stylesToDispose.AppendElement(subStyle);
            }

            
            aRun->AddGlyphRun(matchedFont, aSegmentStart + runStart - headerChars, PR_TRUE);
        }
        
        runStart += matchedLength;
        runLength -= matchedLength;    
    }
    

    

    
    
    aRun->SortGlyphRuns();

    
    
    ATSTrapezoid trap;
    ItemCount trapCount;
    ATSUGetGlyphBounds(layout, 0, 0, headerChars, aSegmentLength,
                       kATSUseFractionalOrigins, 1, &trap, &trapCount); 

    ATSUDisposeTextLayout(layout);

    PRUint32 i;
    for (i = 0; i < stylesToDispose.Length(); ++i) {
        ATSUDisposeStyle(stylesToDispose[i]);
    }
    gCallbackClosure = nsnull;
    return !closure.mOverrunningGlyphs;
}
