







































#ifndef __LP64__ 

#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"

#include "gfxTypes.h"

#include "nsPromiseFlatString.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxPlatformMac.h"
#include "gfxAtsuiFonts.h"

#include "gfxFontTest.h"
#include "gfxFontUtils.h"

#include "cairo-quartz.h"

#include "gfxQuartzSurface.h"
#include "gfxMacPlatformFontList.h"
#include "gfxUserFontSet.h"

#include "nsUnicodeRange.h"




#ifdef DUMP_TEXT_RUNS
static PRLogModuleInfo *gAtsuiTextRunLog = PR_NewLogModule("atsuiTextRun");
#endif

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

gfxAtsuiFont::gfxAtsuiFont(MacOSFontEntry *aFontEntry,
                           const gfxFontStyle *fontStyle, PRBool aNeedsBold)
    : gfxFont(aFontEntry, fontStyle),
      mFontStyle(fontStyle), mATSUStyle(nsnull),
      mHasMirroring(PR_FALSE), mHasMirroringLookedUp(PR_FALSE),
      mFontFace(nsnull), mScaledFont(nsnull), mAdjustedSize(0.0f)
{
    ATSFontRef fontRef = aFontEntry->GetFontRef();
    ATSUFontID fontID = FMGetFontFromATSFontRef(fontRef);

    
    PRInt8 baseWeight, weightDistance;
    mFontStyle->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    PRUint16 targetWeight = (baseWeight * 100) + (weightDistance * 100);

    
    
    
    
    if (!aFontEntry->IsBold()
        && ((weightDistance == 0 && targetWeight >= 600) || (weightDistance > 0 && aNeedsBold))) 
    {
        mSyntheticBoldOffset = 1;  
    }

    InitMetrics(fontID, fontRef);
    if (!mIsValid) {
        return;
    }

    mFontFace = cairo_quartz_font_face_create_for_atsu_font_id(fontID);

    cairo_matrix_t sizeMatrix, ctm;
    cairo_matrix_init_identity(&ctm);
    cairo_matrix_init_scale(&sizeMatrix, mAdjustedSize, mAdjustedSize);

    
    PRBool needsOblique = (!aFontEntry->IsItalic() && (mFontStyle->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

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

    cairo_status_t cairoerr = cairo_scaled_font_status(mScaledFont);
    if (cairoerr != CAIRO_STATUS_SUCCESS) {
        mIsValid = PR_FALSE;

#ifdef DEBUG        
        char warnBuf[1024];
        sprintf(warnBuf, "Failed to create scaled font: %s status: %d", NS_ConvertUTF16toUTF8(GetName()).get(), cairoerr);
        NS_WARNING(warnBuf);
#endif
    }
}


ATSFontRef gfxAtsuiFont::GetATSFontRef()
{
    return GetFontEntry()->GetFontRef();
}

static void
DisableUncommonLigaturesAndLineBoundarySwashes(ATSUStyle aStyle)
{
    static const ATSUFontFeatureType types[] = {
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kLigaturesType,
        kSmartSwashType,
        kSmartSwashType
    };
    static const ATSUFontFeatureType selectors[NS_ARRAY_LENGTH(types)] = {
        kRareLigaturesOffSelector,
        kLogosOffSelector,
        kRebusPicturesOffSelector,
        kDiphthongLigaturesOffSelector,
        kSquaredLigaturesOffSelector,
        kAbbrevSquaredLigaturesOffSelector,
        kLineInitialSwashesOffSelector,
        kLineFinalSwashesOffSelector
    };
    ATSUSetFontFeatures(aStyle, NS_ARRAY_LENGTH(types), types, selectors);
}

static void
DisableCommonLigatures(ATSUStyle aStyle)
{
    static const ATSUFontFeatureType types[] = {
        kLigaturesType
    };
    static const ATSUFontFeatureType selectors[NS_ARRAY_LENGTH(types)] = {
        kCommonLigaturesOffSelector
    };
    ATSUSetFontFeatures(aStyle, NS_ARRAY_LENGTH(types), types, selectors);
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
  return floor(aValue/aFraction + 0.5)*aFraction;
}

void
gfxAtsuiFont::InitMetrics(ATSUFontID aFontID, ATSFontRef aFontRef)
{
    

    gfxFloat size =
        PR_MAX(((mAdjustedSize != 0.0f) ? mAdjustedSize : GetStyle()->size), 1.0f);

    

    if (mATSUStyle)
      ATSUDisposeStyle(mATSUStyle);

    ATSUFontID fid = aFontID;
    
    Fixed fSize = FloatToFixed(size);
    
    CGAffineTransform transform = CGAffineTransformMakeScale(1, -1);

    static const ATSUAttributeTag styleTags[] = {
        kATSUFontTag,
        kATSUSizeTag,
        kATSUFontMatrixTag
    };
    const ATSUAttributeValuePtr styleArgs[NS_ARRAY_LENGTH(styleTags)] = {
        &fid,
        &fSize,
        &transform
    };
    static const ByteCount styleArgSizes[NS_ARRAY_LENGTH(styleTags)] = {
        sizeof(ATSUFontID),
        sizeof(Fixed),
        sizeof(CGAffineTransform)
    };

    ATSUCreateStyle(&mATSUStyle);
    ATSUSetAttributes(mATSUStyle,
                      NS_ARRAY_LENGTH(styleTags),
                      styleTags,
                      styleArgSizes,
                      styleArgs);
    
    
    
    
    
    
    
    DisableUncommonLigaturesAndLineBoundarySwashes(mATSUStyle);

    

    ATSFontMetrics atsMetrics;
    OSStatus err;
    
    err = ATSFontGetHorizontalMetrics(aFontRef, kATSOptionFlagsDefault,
                                &atsMetrics);
                                
    if (err != noErr) {
        mIsValid = PR_FALSE;
        
#ifdef DEBUG        
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s err: %8.8x", NS_ConvertUTF16toUTF8(GetName()).get(), PRUint32(err));
        NS_WARNING(warnBuf);
#endif
        return;
    }

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

    mMetrics.maxAscent =
      NS_ceil(RoundToNearestMultiple(atsMetrics.ascent*size, 1/1024.0));
    mMetrics.maxDescent =
      NS_ceil(-RoundToNearestMultiple(atsMetrics.descent*size, 1/1024.0));

    mMetrics.maxHeight = mMetrics.maxAscent + mMetrics.maxDescent;

    if (mMetrics.maxHeight - mMetrics.emHeight > 0)
        mMetrics.internalLeading = mMetrics.maxHeight - mMetrics.emHeight; 
    else
        mMetrics.internalLeading = 0.0;
    mMetrics.externalLeading = atsMetrics.leading * size;

    mMetrics.emAscent = mMetrics.maxAscent * mMetrics.emHeight / mMetrics.maxHeight;
    mMetrics.emDescent = mMetrics.emHeight - mMetrics.emAscent;

    mMetrics.maxAdvance = atsMetrics.maxAdvanceWidth * size + mSyntheticBoldOffset;

    float xWidth = GetCharWidth('x');
    if (atsMetrics.avgAdvanceWidth != 0.0)
        mMetrics.aveCharWidth =
            PR_MIN(atsMetrics.avgAdvanceWidth * size, xWidth);
    else
        mMetrics.aveCharWidth = xWidth;

    mMetrics.aveCharWidth += mSyntheticBoldOffset;

    if (GetFontEntry()->IsFixedPitch()) {
        
        
        
        mMetrics.maxAdvance = mMetrics.aveCharWidth;
    }

    mMetrics.underlineOffset = atsMetrics.underlinePosition * size;
    mMetrics.underlineSize = atsMetrics.underlineThickness * size;

    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.superscriptOffset = mMetrics.xHeight;

    mMetrics.strikeoutOffset = mMetrics.xHeight / 2.0;
    mMetrics.strikeoutSize = mMetrics.underlineSize;

    PRUint32 glyphID;
    mMetrics.spaceWidth = GetCharWidth(' ', &glyphID);
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth('0', &glyphID);
    if (glyphID == 0) 
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;

    SanitizeMetrics(&mMetrics, GetFontEntry()->mIsBadUnderlineFont);

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
    return GetName();
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
    if (mScaledFont)
        cairo_scaled_font_destroy(mScaledFont);
    if (mFontFace)
        cairo_font_face_destroy(mFontFace);

    if (mATSUStyle)
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
        
        
        status = ATSFontGetTable(GetATSFontRef(), TRUETYPE_TAG('p','r','o','p'), 0, 0, 0, &size);
        mHasMirroring = (status == noErr);
        mHasMirroringLookedUp = PR_TRUE;
    }

    return mHasMirroring;
}

PRBool gfxAtsuiFont::TestCharacterMap(PRUint32 aCh) {
    if (!mIsValid) return PR_FALSE;
    return GetFontEntry()->TestCharacterMap(aCh);
}

MacOSFontEntry*
gfxAtsuiFont::GetFontEntry()
{
    return static_cast<MacOSFontEntry*> (mFontEntry.get());
}






 
static already_AddRefed<gfxAtsuiFont>
GetOrMakeFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aStyle, PRBool aNeedsBold)
{
    
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aFontEntry->Name(), aStyle);
    if (!font) {
        gfxAtsuiFont *newFont = new gfxAtsuiFont(aFontEntry, aStyle, aNeedsBold);
        if (!newFont)
            return nsnull;
        if (!newFont->Valid()) {
            delete newFont;
            return nsnull;
        }
        font = newFont;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxAtsuiFont *>(f);
}


gfxAtsuiFontGroup::gfxAtsuiFontGroup(const nsAString& families,
                                     const gfxFontStyle *aStyle,
                                     gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet)
{
    mPageLang = gfxPlatform::GetFontPrefLangFor(mStyle.langGroup.get());

    InitFontList();
}

PRBool
gfxAtsuiFontGroup::FindATSFont(const nsAString& aName,
                               const nsACString& aGenericName,
                               void *aClosure)
{
    gfxAtsuiFontGroup *fontGroup = static_cast<gfxAtsuiFontGroup*>(aClosure);
    const gfxFontStyle *fontStyle = fontGroup->GetStyle();


    PRBool needsBold;
    MacOSFontEntry *fe = nsnull;
    
    
    gfxUserFontSet *fs = fontGroup->GetUserFontSet();
    gfxFontEntry *gfe;
    if (fs && (gfe = fs->FindFontEntry(aName, *fontStyle, needsBold))) {
        
        fe = static_cast<MacOSFontEntry*> (gfe);
    }
    
    
    if (!fe) {
        fe = static_cast<MacOSFontEntry*>
            (gfxMacPlatformFontList::PlatformFontList()->FindFontForFamily(aName, fontStyle, needsBold));
    }

    if (fe && !fontGroup->HasFont(fe->GetFontRef())) {
        nsRefPtr<gfxAtsuiFont> font = GetOrMakeFont(fe, fontStyle, needsBold);
        if (font) {
            fontGroup->mFonts.AppendElement(font);
        }
    }

    return PR_TRUE;
}

gfxFontGroup *
gfxAtsuiFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxAtsuiFontGroup(mFamilies, aStyle, mUserFontSet);
}

#define UNICODE_LRO 0x202d
#define UNICODE_RLO 0x202e
#define UNICODE_PDF 0x202c

static void
AppendDirectionalIndicatorStart(PRUint32 aFlags, nsAString& aString)
{
    static const PRUnichar overrides[2] = { UNICODE_LRO, UNICODE_RLO };
    aString.Append(overrides[(aFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0]);
}



static PRUint32
AppendDirectionalIndicatorEnd(PRBool aNeedDirection, nsAString& aString)
{
    
    
    
    
    
    
    
    aString.Append(' ');
    if (!aNeedDirection)
        return 1;

    
    
    aString.Append('.');
    aString.Append(UNICODE_PDF);
    return 2;
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
    
    PRUint32 realGuessMax = PR_MAX(1, chars);
    
    
    
    
    
    realGuessMax = PR_MIN(500, realGuessMax);

    return realGuessMax;
}
















gfxTextRun *
gfxAtsuiFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    gfxPlatformMac::SetupClusterBoundaries(textRun, aString);

    PRUint32 maxLen;
    nsAutoString utf16;
    for (maxLen = GuessMaximumStringLength(); maxLen > 0; maxLen /= 2) {
        PRUint32 start = 0;
        while (start < aLength) {
            PRUint32 len = FindTextRunSegmentLength(textRun, start, maxLen);

            utf16.Truncate();
            AppendDirectionalIndicatorStart(aFlags, utf16);
            PRUint32 layoutStart = utf16.Length();
            utf16.Append(aString + start, len);
            
            
            PRUint32 trailingCharsToIgnore =
                AppendDirectionalIndicatorEnd(PR_TRUE, utf16);
            PRUint32 layoutLength = len + trailingCharsToIgnore;
            if (!InitTextRun(textRun, utf16.get(), utf16.Length(),
                             layoutStart, layoutLength,
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
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
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
                AppendDirectionalIndicatorStart(aFlags, utf16);
            }
            PRUint32 layoutStart = utf16.Length();
            AppendASCIItoUTF16(cString, utf16);
            PRUint32 trailingCharsToIgnore =
                AppendDirectionalIndicatorEnd(wrapBidi, utf16);
            PRUint32 layoutLength = len + trailingCharsToIgnore;
            if (!InitTextRun(textRun, utf16.get(), utf16.Length(),
                             layoutStart, layoutLength,
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

PRBool
gfxAtsuiFontGroup::HasFont(ATSFontRef aFontRef)
{
    for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
        if (aFontRef == static_cast<gfxAtsuiFont *>(mFonts.ElementAt(i).get())->GetATSFontRef())
            return PR_TRUE;
    }
    return PR_FALSE;
}

struct PrefFontCallbackData {
    PrefFontCallbackData(nsTArray<nsRefPtr<gfxFontFamily> >& aFamiliesArray) 
        : mPrefFamilies(aFamiliesArray)
    {}

    nsTArray<nsRefPtr<gfxFontFamily> >& mPrefFamilies;

    static PRBool AddFontFamilyEntry(eFontPrefLang aLang, const nsAString& aName, void *aClosure)
    {
        PrefFontCallbackData *prefFontData = static_cast<PrefFontCallbackData*>(aClosure);
        
        gfxFontFamily *family = gfxMacPlatformFontList::PlatformFontList()->FindFamily(aName);
        if (family) {
            prefFontData->mPrefFamilies.AppendElement(family);
        }
        return PR_TRUE;
    }
};


already_AddRefed<gfxFont>
gfxAtsuiFontGroup::WhichPrefFontSupportsChar(PRUint32 aCh)
{
    gfxFont *font;

    
    if (aCh > 0xFFFF)
        return nsnull;

    
    PRUint32 unicodeRange = FindCharUnicodeRange(aCh);
    eFontPrefLang charLang = gfxPlatformMac::GetFontPrefLangFor(unicodeRange);

    
    if (mLastPrefFont && charLang == mLastPrefLang && mLastPrefFirstFont && mLastPrefFont->TestCharacterMap(aCh)) {
        font = mLastPrefFont;
        NS_ADDREF(font);
        return font;
    }

    
    eFontPrefLang prefLangs[kMaxLenPrefLangList];
    PRUint32 i, numLangs = 0;

    gfxPlatformMac *macPlatform = gfxPlatformMac::GetPlatform();
    macPlatform->GetLangPrefs(prefLangs, numLangs, charLang, mPageLang);

    for (i = 0; i < numLangs; i++) {
        nsAutoTArray<nsRefPtr<gfxFontFamily>, 5> families;
        eFontPrefLang currentLang = prefLangs[i];
        
        gfxMacPlatformFontList *fc = gfxMacPlatformFontList::PlatformFontList();

        
        if (!fc->GetPrefFontFamilyEntries(currentLang, &families)) {
            eFontPrefLang prefLangsToSearch[1] = { currentLang };
            PrefFontCallbackData prefFontData(families);
            gfxPlatform::ForEachPrefFont(prefLangsToSearch, 1, PrefFontCallbackData::AddFontFamilyEntry,
                                           &prefFontData);
            fc->SetPrefFontFamilyEntries(currentLang, families);
        }

        
        PRUint32  i, numPrefs;
        numPrefs = families.Length();
        for (i = 0; i < numPrefs; i++) {
            
            gfxFontFamily *family = families[i];
            if (!family) continue;
            
            
            
            
            
            if (family == mLastPrefFamily && mLastPrefFont->TestCharacterMap(aCh)) {
                font = mLastPrefFont;
                NS_ADDREF(font);
                return font;
            }
            
            PRBool needsBold;
            MacOSFontEntry *fe =
                static_cast<MacOSFontEntry*>(family->FindFontForStyle(mStyle, needsBold));
            
            if (fe && fe->TestCharacterMap(aCh)) {
                nsRefPtr<gfxAtsuiFont> prefFont = GetOrMakeFont(fe, &mStyle, needsBold);
                if (!prefFont) continue;
                mLastPrefFamily = family;
                mLastPrefFont = prefFont;
                mLastPrefLang = charLang;
                mLastPrefFirstFont = (i == 0);
                nsRefPtr<gfxFont> font2 = prefFont.get();
                return font2.forget();
            }

        }
    }

    return nsnull;
}

already_AddRefed<gfxFont> 
gfxAtsuiFontGroup::WhichSystemFontSupportsChar(PRUint32 aCh)
{
    MacOSFontEntry *fe;

    fe = static_cast<MacOSFontEntry*>
        (gfxMacPlatformFontList::PlatformFontList()->FindFontForChar(aCh, GetFontAt(0)));
    if (fe) {
        nsRefPtr<gfxAtsuiFont> atsuiFont = GetOrMakeFont(fe, &mStyle, PR_FALSE); 
        nsRefPtr<gfxFont> font = atsuiFont.get(); 
        return font.forget();
    }

    return nsnull;
}

void
gfxAtsuiFontGroup::UpdateFontList()
{
    
    if (mUserFontSet && mCurrGeneration != GetGeneration()) {
        
        mFonts.Clear();
        mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
        InitFontList();
        mCurrGeneration = GetGeneration();
    }
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





#define ATSUI_OVERRUNNING_GLYPH_FLAG 0x100000




static PRBool
PostLayoutCallback(ATSULineRef aLine, gfxTextRun *aTextRun,
                   const PRUnichar *aString, PRInt32 aLayoutLength,
                   PRInt32 aOffsetInTextRun, PRInt32 aLengthInTextRun,
                   const PRPackedBool *aUnmatched)
{
    AutoLayoutDataArrayPtr baselineDeltasArray(aLine, kATSUDirectDataBaselineDeltaFixedArray);
    Fixed *baselineDeltas = static_cast<Fixed *>(baselineDeltasArray.mArray);
    AutoLayoutDataArrayPtr glyphRecordsArray(aLine, kATSUDirectDataLayoutRecordATSLayoutRecordCurrent);

    PRInt32 numGlyphs = glyphRecordsArray.mItemCount;
    if (numGlyphs == 0 || !glyphRecordsArray.mArray) {
        NS_WARNING("Failed to retrieve key glyph data");
        return PR_FALSE;
    }
    ATSLayoutRecord *glyphRecords = static_cast<ATSLayoutRecord *>(glyphRecordsArray.mArray);
    NS_ASSERTION(!baselineDeltas || baselineDeltasArray.mItemCount == (PRUint32)numGlyphs,
                 "Mismatched glyph counts");
    NS_ASSERTION(glyphRecords[numGlyphs - 1].flags & kATSGlyphInfoTerminatorGlyph,
                 "Last glyph should be a terminator glyph");
    --numGlyphs;
    if (numGlyphs == 0)
        return PR_FALSE;

    PRBool isLTR = !aTextRun->IsRightToLeft();

    PRUint32 trailingCharactersToIgnore = aLayoutLength - aLengthInTextRun;
    if (trailingCharactersToIgnore > 0) {
        
        
        if (isLTR) {
            NS_ASSERTION((PRInt32)glyphRecords[numGlyphs - trailingCharactersToIgnore].originalOffset == aLengthInTextRun*2,
                         "Couldn't find glyph for trailing marker");
        } else {
            NS_ASSERTION((PRInt32)glyphRecords[trailingCharactersToIgnore - 1].originalOffset == aLengthInTextRun*2,
                         "Couldn't find glyph for trailing marker");
            glyphRecords += trailingCharactersToIgnore;
        }
        numGlyphs -= trailingCharactersToIgnore;
        if (numGlyphs == 0)
            return PR_FALSE;
    }

    PRUint32 allFlags = 0;
    PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    nsAutoTArray<gfxTextRun::DetailedGlyph,8> detailedGlyphs;
    gfxTextRun::CompressedGlyph g;

    Fixed runWidth = glyphRecords[numGlyphs].realPos - glyphRecords[0].realPos;

    static const PRInt32 NO_GLYPH = -1;
    nsAutoTArray<PRInt32,128> charToGlyphArray;
    if (!charToGlyphArray.SetLength(aLengthInTextRun))
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 *charToGlyph = charToGlyphArray.Elements();
    for (PRInt32 offset = 0; offset < aLengthInTextRun; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }
    for (PRInt32 g = 0; g < numGlyphs; ++g) {
        
        
        charToGlyph[glyphRecords[g].originalOffset/2] = g;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRInt32 glyphStart = 0; 
    PRInt32 charStart = isLTR ? 0 : aLengthInTextRun-1; 
    PRInt32 direction = isLTR ? 1 : -1; 

    while (glyphStart < numGlyphs) { 

        PRInt32 charEnd = (PRInt32)glyphRecords[glyphStart].originalOffset/2;
        PRInt32 charLimit = isLTR ? aLengthInTextRun : -1;
        PRInt32 glyphEnd = glyphStart;
        PRBool inOrder = PR_TRUE;
        do {
            
            
            
            
            
            charEnd += direction;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += direction;
            }
            
            
            if (!isLTR && charToGlyph[charEnd+1] == NO_GLYPH &&
                NS_IS_LOW_SURROGATE(aString[charEnd+1])) {
                charEnd += 1;
            }

            
            for (PRInt32 i = charStart; i != charEnd; i += direction) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = PR_MAX(glyphEnd, charToGlyph[i] + 1); 
                }
            }

            
            PRBool extendedCharRange = PR_FALSE;
            while (glyphEnd < numGlyphs && glyphRecords[glyphEnd].glyphID == 0xffff) {
                if (isLTR) {
                    if ((PRInt32)glyphRecords[glyphEnd].originalOffset/2 >= charEnd) {
                        
                        
                        charEnd = (PRInt32)glyphRecords[glyphEnd].originalOffset/2;
                        extendedCharRange = PR_TRUE;
                    }
                } else {
                    if ((PRInt32)glyphRecords[glyphEnd].originalOffset/2 <= charEnd) {
                        charEnd = (PRInt32)glyphRecords[glyphEnd].originalOffset/2;
                        extendedCharRange = PR_TRUE;
                    }
                }
                ++glyphEnd;
            }
            if (extendedCharRange) {
                
                
                continue;
            }

            if (glyphEnd == glyphStart + 1) {
                
                break;
            }

            
            
            
            PRBool allGlyphsAreWithinCluster = PR_TRUE;
            PRInt32 prevGlyphCharIndex = charStart;
            for (PRInt32 i = glyphStart; i < glyphEnd; ++i) {
                PRInt32 glyphCharIndex = (PRInt32)glyphRecords[i].originalOffset/2;
                if (isLTR) {
                    if (glyphCharIndex < charStart || glyphCharIndex >= charEnd) {
                        allGlyphsAreWithinCluster = PR_FALSE;
                        break;
                    }
                    if (glyphCharIndex < prevGlyphCharIndex) {
                        inOrder = PR_FALSE;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                } else {
                    if (glyphCharIndex > charStart || glyphCharIndex <= charEnd) {
                        allGlyphsAreWithinCluster = PR_FALSE;
                        break;
                    }
                    if (glyphCharIndex > prevGlyphCharIndex) {
                        inOrder = PR_FALSE;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                }
            }
            if (allGlyphsAreWithinCluster) {
                break;
            }
        } while (charEnd != charLimit);

        NS_ASSERTION(glyphStart < glyphEnd, "character/glyph clump contains no glyphs!");
        NS_ASSERTION(charStart != charEnd, "character/glyph contains no characters!");

        
        
        
        PRInt32 baseCharIndex, endCharIndex;
        if (isLTR) {
            baseCharIndex = charStart;
            endCharIndex = charEnd;
        } else {
            baseCharIndex = charEnd + 1;
            endCharIndex = charStart + 1;
        }

        
        if (baseCharIndex >= aLayoutLength || endCharIndex <= 0) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        
        baseCharIndex = PR_MAX(baseCharIndex, 0);
        endCharIndex = PR_MIN(endCharIndex, aLayoutLength);

        
        if (aUnmatched && aUnmatched[baseCharIndex]) {
            for (PRInt32 i = baseCharIndex; i < endCharIndex; ++i) {
                if (NS_IS_HIGH_SURROGATE(aString[i]) &&
                    i + 1 < aLayoutLength &&
                    NS_IS_LOW_SURROGATE(aString[i + 1])) {
                    aTextRun->SetMissingGlyph(aOffsetInTextRun + i,
                                              SURROGATE_TO_UCS4(aString[i],
                                                                aString[i + 1]));
                    ++i;
                } else {
                    aTextRun->SetMissingGlyph(aOffsetInTextRun + i, aString[i]);
                }
            }
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        
        double toNextGlyph;
        if (glyphStart < numGlyphs-1) {
            toNextGlyph = FixedToFloat(glyphRecords[glyphStart+1].realPos -
                                       glyphRecords[glyphStart].realPos);
        } else {
            toNextGlyph = FixedToFloat(glyphRecords[0].realPos + runWidth -
                                       glyphRecords[glyphStart].realPos);
        }
        PRInt32 advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);

        
        
        baseCharIndex += aOffsetInTextRun;
        endCharIndex += aOffsetInTextRun;

        
        PRInt32 glyphsInClump = glyphEnd - glyphStart;
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyphRecords[glyphStart].glyphID) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            aTextRun->IsClusterStart(baseCharIndex) &&
            (!baselineDeltas || baselineDeltas[glyphStart] == 0))
        {
            aTextRun->SetSimpleGlyph(baseCharIndex,
                                     g.SetSimpleGlyph(advance, glyphRecords[glyphStart].glyphID));
        } else {
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph *details = detailedGlyphs.AppendElement();
                details->mGlyphID = glyphRecords[glyphStart].glyphID;
                details->mXOffset = 0;
                details->mYOffset = !baselineDeltas ? 0.0f
                    : - FixedToFloat(baselineDeltas[glyphStart]) * appUnitsPerDevUnit;
                details->mAdvance = advance;
                if (++glyphStart >= glyphEnd) {
                    break;
                }
                if (glyphStart < numGlyphs-1) {
                    toNextGlyph = FixedToFloat(glyphRecords[glyphStart+1].realPos -
                                               glyphRecords[glyphStart].realPos);
                } else {
                    toNextGlyph = FixedToFloat(glyphRecords[0].realPos + runWidth -
                                               glyphRecords[glyphStart].realPos);
                }
                advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);
            }

            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(baseCharIndex),
                         PR_TRUE, detailedGlyphs.Length());
            aTextRun->SetGlyphs(baseCharIndex, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex) {
            g.SetComplex(inOrder && aTextRun->IsClusterStart(baseCharIndex),
                         PR_FALSE, 0);
            aTextRun->SetGlyphs(baseCharIndex, g, nsnull);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return (allFlags & ATSUI_OVERRUNNING_GLYPH_FLAG) != 0;
}

struct PostLayoutCallbackClosure {
    gfxTextRun                  *mTextRun;
    const PRUnichar             *mString;
    PRUint32                     mLayoutLength;
    PRUint32                     mOffsetInTextRun;
    PRUint32                     mLengthInTextRun;
    
    
    nsAutoArrayPtr<PRPackedBool> mUnmatchedChars;
    
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
                           gCallbackClosure->mString,
                           gCallbackClosure->mLayoutLength,
                           gCallbackClosure->mOffsetInTextRun,
                           gCallbackClosure->mLengthInTextRun,
                           gCallbackClosure->mUnmatchedChars);
    *oCallbackStatus = kATSULayoutOperationCallbackStatusContinue;
    return noErr;
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

PRBool
gfxAtsuiFontGroup::InitTextRun(gfxTextRun *aRun,
                               const PRUnichar *aString, PRUint32 aLength,
                               PRUint32 aLayoutStart, PRUint32 aLayoutLength,
                               PRUint32 aOffsetInTextRun, PRUint32 aLengthInTextRun)
{
    OSStatus status;
    gfxAtsuiFont *firstFont = GetFontAt(0);
    ATSUStyle mainStyle = firstFont->GetATSUStyle();
    nsTArray<ATSUStyle> stylesToDispose;
    const PRUnichar *layoutString = aString + aLayoutStart;

#ifdef DUMP_TEXT_RUNS
    NS_ConvertUTF16toUTF8 str(layoutString, aLengthInTextRun);
    NS_ConvertUTF16toUTF8 families(mFamilies);
    PR_LOG(gAtsuiTextRunLog, PR_LOG_DEBUG,\
           ("InitTextRun %p fontgroup %p (%s) lang: %s len %d TEXTRUN \"%s\" ENDTEXTRUN\n",
            aRun, this, families.get(), mStyle.langGroup.get(), aLengthInTextRun, str.get()) );



#endif

    if (aRun->GetFlags() & TEXT_DISABLE_OPTIONAL_LIGATURES) {
        status = ATSUCreateAndCopyStyle(mainStyle, &mainStyle);
        if (status == noErr) {
            stylesToDispose.AppendElement(mainStyle);
            DisableCommonLigatures(mainStyle);
        }
    }

    UniCharCount runLengths = aLengthInTextRun;
    ATSUTextLayout layout;
    
    
    
    
    status = ATSUCreateTextLayoutWithTextPtr
        (aString,
         aLayoutStart,
         aLayoutLength,
         aLength,
         1,
         &runLengths,
         &mainStyle,
         &layout);
    

    PostLayoutCallbackClosure closure;
    closure.mTextRun = aRun;
    closure.mString = layoutString;
    closure.mLayoutLength = aLayoutLength;
    closure.mOffsetInTextRun = aOffsetInTextRun;
    closure.mLengthInTextRun = aLengthInTextRun;
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

    UniCharArrayOffset runStart = aLayoutStart;
    UniCharCount runLength = aLengthInTextRun;

    

    nsTArray<gfxTextRange> fontRanges;

    ComputeRanges(fontRanges, aString, runStart, runStart + runLength);

    PRUint32 r, numRanges = fontRanges.Length();

    
    
    
    
    
    
    nsTArray<PRUint32> hackForLiGothic;

    for (r = 0; r < numRanges; r++) {
        const gfxTextRange& range = fontRanges[r];
   
        gfxAtsuiFont *matchedFont;
        UniCharCount  matchedLength;
        
        
        matchedLength = range.Length();
        matchedFont = static_cast<gfxAtsuiFont*> (range.font ? range.font.get() : nsnull);

#ifdef DUMP_TEXT_RUNS
        PR_LOG(gAtsuiTextRunLog, PR_LOG_DEBUG, ("InitTextRun %p fontgroup %p font %p match %s (%d-%d)", aRun, this, matchedFont, (matchedFont ? NS_ConvertUTF16toUTF8(matchedFont->GetUniqueName()).get() : "<null>"), runStart, matchedLength));
#endif
        
        
        
        if (aRun->IsRightToLeft() && matchedFont && !matchedFont->HasMirroringInfo()) {
            MirrorSubstring(layout, mirroredStr, aString, aLength, runStart, runLength);
        }       

        
        if (!matchedFont) {
        
            aRun->AddGlyphRun(firstFont, aOffsetInTextRun + runStart - aLayoutStart, PR_TRUE);
            
            if (!closure.mUnmatchedChars) {
                closure.mUnmatchedChars = new PRPackedBool[aLength];
                if (closure.mUnmatchedChars) {
                    
                    memset(closure.mUnmatchedChars.get(), PR_FALSE, aLength);
                }
            }

            if (closure.mUnmatchedChars) {
                
                memset(closure.mUnmatchedChars.get() + runStart - aLayoutStart,
                       PR_TRUE, matchedLength);
            }
            
        } else {
        
            if (matchedFont != firstFont) {
                
                ATSUStyle subStyle = SetLayoutRangeToFont(layout, mainStyle, runStart,
                                                          matchedLength, matchedFont->GetATSFontRef());
                stylesToDispose.AppendElement(subStyle);
            }

            
            aRun->AddGlyphRun(matchedFont, aOffsetInTextRun + runStart - aLayoutStart, PR_TRUE);

            if (matchedFont->GetFontEntry()->UseLiGothicAtsuiHack()) {
                PRUnichar *text = const_cast<PRUnichar*>(layoutString);
                for (PRUint32 i = 0; i < matchedLength; ++i) {
                    if (text[aOffsetInTextRun + runStart + i] ==
                        kLiGothicBadCharUnicode) {
                        hackForLiGothic.AppendElement(aOffsetInTextRun +
                                                      runStart + i);
                        text[aOffsetInTextRun + runStart + i] =
                            kLiGothicBadCharUnicode + 1;
                    }
                }
            }
        }
        
        runStart += matchedLength;
        runLength -= matchedLength;    
    }


    

    
    
    aRun->SortGlyphRuns();

    
    
    ATSTrapezoid trap;
    ItemCount trapCount;
    ATSUGetGlyphBounds(layout, 0, 0, aLayoutStart, aLengthInTextRun,
                       kATSUseFractionalOrigins, 1, &trap, &trapCount); 

    ATSUDisposeTextLayout(layout);

    aRun->AdjustAdvancesForSyntheticBold(aOffsetInTextRun, aLengthInTextRun);

    for (PRUint32 i = 0; i < hackForLiGothic.Length(); ++i) {
        gfxTextRun::CompressedGlyph glyph =
            aRun->GetCharacterGlyphs()[hackForLiGothic[i]];
        if (glyph.IsSimpleGlyph()) {
            aRun->SetSimpleGlyph(hackForLiGothic[i],
                                 glyph.SetSimpleGlyph(glyph.GetSimpleAdvance(),
                                                      kLiGothicBadCharGlyph));
        }
    }

    PRUint32 i;
    for (i = 0; i < stylesToDispose.Length(); ++i) {
        ATSUDisposeStyle(stylesToDispose[i]);
    }
    gCallbackClosure = nsnull;
    return !closure.mOverrunningGlyphs;
}

void
gfxAtsuiFontGroup::InitFontList()
{
    ForEachFont(FindATSFont, this);

    if (mFonts.Length() == 0) {
        
        
        
        
        

        

        
        
        

        PRBool needsBold;
        MacOSFontEntry *defaultFont = static_cast<MacOSFontEntry*>
            (gfxMacPlatformFontList::PlatformFontList()->GetDefaultFont(&mStyle, needsBold));
        NS_ASSERTION(defaultFont, "invalid default font returned by GetDefaultFont");

        nsRefPtr<gfxAtsuiFont> font = GetOrMakeFont(defaultFont, &mStyle, needsBold);

        if (font) {
            mFonts.AppendElement(font);
        }
    }

    if (!mStyle.systemFont) {
        for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
            gfxAtsuiFont* font = static_cast<gfxAtsuiFont*>(mFonts[i].get());
            if (font->GetFontEntry()->mIsBadUnderlineFont) {
                gfxFloat first = mFonts[0]->GetMetrics().underlineOffset;
                gfxFloat bad = font->GetMetrics().underlineOffset;
                mUnderlineOffset = PR_MIN(first, bad);
                break;
            }
        }
    }
}

#endif 
