







































#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"

#include "gfxTypes.h"

#include "nsPromiseFlatString.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxPlatformMac.h"
#include "gfxCoreTextFonts.h"

#include "gfxFontTest.h"
#include "gfxFontUtils.h"

#include "cairo-quartz.h"

#include "gfxQuartzSurface.h"
#include "gfxMacPlatformFontList.h"
#include "gfxUserFontSet.h"

#include "nsUnicodeRange.h"




#ifdef DUMP_TEXT_RUNS
static PRLogModuleInfo *gCoreTextTextRunLog = PR_NewLogModule("coreTextTextRun");
#endif

#define ROUND(x) (floor((x) + 0.5))



CTFontDescriptorRef gfxCoreTextFont::sDefaultFeaturesDescriptor = NULL;
CTFontDescriptorRef gfxCoreTextFont::sDisableLigaturesDescriptor = NULL;

#ifdef DEBUG_jonathan
static void dumpFontDescCallback(const void *key, const void *value, void *context)
{
    CFStringRef attribute = (CFStringRef)key;
    CFTypeRef setting = (CFTypeRef)value;
    fprintf(stderr, "attr: "); CFShow(attribute);
    fprintf(stderr, "    = "); CFShow(setting);
    fprintf(stderr, "\n");
}

static void
dumpFontDescriptor(CTFontRef font)
{
    CTFontDescriptorRef desc = CTFontCopyFontDescriptor(font);
    CFDictionaryRef dict = CTFontDescriptorCopyAttributes(desc);
    CFRelease(desc);
    CFDictionaryApplyFunction(dict, &dumpFontDescCallback, 0);
    CFRelease(dict);
}
#endif

gfxCoreTextFont::gfxCoreTextFont(MacOSFontEntry *aFontEntry,
                                 const gfxFontStyle *aFontStyle,
                                 PRBool aNeedsBold)
    : gfxFont(aFontEntry, aFontStyle),
      mFontStyle(aFontStyle),
      mCTFont(nsnull),
      mAttributesDict(nsnull),
      mHasMetrics(PR_FALSE),
      mFontFace(nsnull),
      mScaledFont(nsnull),
      mAdjustedSize(0.0f)
{
    mATSFont = aFontEntry->GetFontRef();

    
    PRInt8 baseWeight, weightDistance;
    mFontStyle->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    PRUint16 targetWeight = (baseWeight * 100) + (weightDistance * 100);

    
    
    
    
    if (!aFontEntry->IsBold()
        && ((weightDistance == 0 && targetWeight >= 600) || (weightDistance > 0 && aNeedsBold)))
    {
        mSyntheticBoldOffset = 1;  
    }

    
    InitMetrics();
    if (!mIsValid) {
        return;
    }

    
    mAttributesDict =
        CFDictionaryCreate(kCFAllocatorDefault,
                           (const void**) &kCTFontAttributeName,
                           (const void**) &mCTFont,
                           1, 
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);

    
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
        (!mFontEntry->IsItalic() && (mFontStyle->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)));

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
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
  return floor(aValue/aFraction + 0.5)*aFraction;
}

PRBool
gfxCoreTextFont::SetupCairoFont(gfxContext *aContext)
{
    cairo_scaled_font_t *scaledFont = CairoScaledFont();
    if (cairo_scaled_font_status(scaledFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(aContext->GetCairo(), scaledFont);
    return PR_TRUE;
}

float
gfxCoreTextFont::GetCharWidth(PRUnichar aUniChar, PRUint32 *aGlyphID)
{
    UniChar c = aUniChar;
    CGGlyph glyph;
    if (CTFontGetGlyphsForCharacters(mCTFont, &c, &glyph, 1)) {
        CGSize advance;
        CTFontGetAdvancesForGlyphs(mCTFont,
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
gfxCoreTextFont::GetCharHeight(PRUnichar aUniChar)
{
    UniChar c = aUniChar;
    CGGlyph glyph;
    if (CTFontGetGlyphsForCharacters(mCTFont, &c, &glyph, 1)) {
        CGRect boundingRect;
        CTFontGetBoundingRectsForGlyphs(mCTFont,
                                        kCTFontHorizontalOrientation,
                                        &glyph,
                                        &boundingRect,
                                        1);
        return boundingRect.size.height;
    }

    
    return 0;
}

gfxCoreTextFont::~gfxCoreTextFont()
{
    if (mScaledFont)
        cairo_scaled_font_destroy(mScaledFont);
    if (mFontFace)
        cairo_font_face_destroy(mFontFace);

    if (mAttributesDict)
        CFRelease(mAttributesDict);
    if (mCTFont)
        CFRelease(mCTFont);
}

MacOSFontEntry*
gfxCoreTextFont::GetFontEntry()
{
    return static_cast<MacOSFontEntry*>(mFontEntry.get());
}

PRBool
gfxCoreTextFont::TestCharacterMap(PRUint32 aCh)
{
    return mIsValid && GetFontEntry()->TestCharacterMap(aCh);
}

void
gfxCoreTextFont::InitMetrics()
{
    if (mHasMetrics)
        return;

    gfxFloat size =
        PR_MAX(((mAdjustedSize != 0.0f) ? mAdjustedSize : GetStyle()->size), 1.0f);

    if (mCTFont != NULL) {
        CFRelease(mCTFont);
        mCTFont = NULL;
    }

    ATSFontMetrics atsMetrics;
    OSStatus err;

    err = ATSFontGetHorizontalMetrics(mATSFont, kATSOptionFlagsDefault,
                                      &atsMetrics);
    if (err != noErr) {
        mIsValid = PR_FALSE;

#ifdef DEBUG
        char warnBuf[1024];
        sprintf(warnBuf, "Bad font metrics for: %s err: %8.8x",
                NS_ConvertUTF16toUTF8(GetName()).get(), PRUint32(err));
        NS_WARNING(warnBuf);
#endif
        return;
    }

    
    
    if (atsMetrics.xHeight > 0) {
        mMetrics.xHeight = atsMetrics.xHeight * size;
    } else {
        mCTFont = CTFontCreateWithPlatformFont(mATSFont, size, NULL, GetDefaultFeaturesDescriptor());
        mMetrics.xHeight = CTFontGetXHeight(mCTFont);
    }

    if (mAdjustedSize == 0.0f) {
        if (mMetrics.xHeight != 0.0f && GetStyle()->sizeAdjust != 0.0f) {
            gfxFloat aspect = mMetrics.xHeight / size;
            mAdjustedSize = GetStyle()->GetAdjustedSize(aspect);

            
            
            InitMetrics();
            return;
        }
        mAdjustedSize = size;
    }

    
    if (mCTFont == NULL)
        mCTFont = CTFontCreateWithPlatformFont(mATSFont, size, NULL, GetDefaultFeaturesDescriptor());

    mMetrics.superscriptOffset = mMetrics.xHeight;
    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.underlineSize = CTFontGetUnderlineThickness(mCTFont);
    mMetrics.underlineOffset = CTFontGetUnderlinePosition(mCTFont);
    mMetrics.strikeoutSize = mMetrics.underlineSize;
    mMetrics.strikeoutOffset = mMetrics.xHeight / 2;

    mMetrics.externalLeading = CTFontGetLeading(mCTFont);
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
    float xWidth = GetCharWidth('x', &glyphID);
    if (atsMetrics.avgAdvanceWidth != 0.0)
        mMetrics.aveCharWidth = PR_MIN(atsMetrics.avgAdvanceWidth * size, xWidth);
    else if (glyphID != 0)
        mMetrics.aveCharWidth = xWidth;
    else
        mMetrics.aveCharWidth = mMetrics.maxAdvance;
    mMetrics.aveCharWidth += mSyntheticBoldOffset;

    if (GetFontEntry()->IsFixedPitch()) {
        
        
        
        mMetrics.maxAdvance = mMetrics.aveCharWidth;
    }

    mMetrics.spaceWidth = GetCharWidth(' ', &glyphID);
    mSpaceGlyph = glyphID;

    mMetrics.zeroOrAveCharWidth = GetCharWidth('0', &glyphID);
    if (glyphID == 0)
        mMetrics.zeroOrAveCharWidth = mMetrics.aveCharWidth;

    mHasMetrics = PR_TRUE;

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




void
gfxCoreTextFont::CreateDefaultFeaturesDescriptor()
{
    if (sDefaultFeaturesDescriptor != NULL)
        return;

    SInt16 val = kSmartSwashType;
    CFNumberRef swashesType =
        CFNumberCreate(kCFAllocatorDefault,
                       kCFNumberSInt16Type,
                       &val);
    val = kLineInitialSwashesOffSelector;
    CFNumberRef lineInitialsOffSelector =
        CFNumberCreate(kCFAllocatorDefault,
                       kCFNumberSInt16Type,
                       &val);

    CFTypeRef keys[]   = { kCTFontFeatureTypeIdentifierKey,
                           kCTFontFeatureSelectorIdentifierKey };
    CFTypeRef values[] = { swashesType,
                           lineInitialsOffSelector };
    CFDictionaryRef featureSettings[2];
    featureSettings[0] =
        CFDictionaryCreate(kCFAllocatorDefault,
                           (const void **) keys,
                           (const void **) values,
                           NS_ARRAY_LENGTH(keys),
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);
    CFRelease(lineInitialsOffSelector);

    val = kLineFinalSwashesOffSelector;
    CFNumberRef lineFinalsOffSelector =
        CFNumberCreate(kCFAllocatorDefault,
                       kCFNumberSInt16Type,
                       &val);
    values[1] = lineFinalsOffSelector;
    featureSettings[1] =
        CFDictionaryCreate(kCFAllocatorDefault,
                           (const void **) keys,
                           (const void **) values,
                           NS_ARRAY_LENGTH(keys),
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);
    CFRelease(lineFinalsOffSelector);
    CFRelease(swashesType);

    CFArrayRef featuresArray =
        CFArrayCreate(kCFAllocatorDefault,
                      (const void **) featureSettings,
                      NS_ARRAY_LENGTH(featureSettings),
                      &kCFTypeArrayCallBacks);
    CFRelease(featureSettings[0]);
    CFRelease(featureSettings[1]);

    const CFTypeRef attrKeys[]   = { kCTFontFeatureSettingsAttribute };
    const CFTypeRef attrValues[] = { featuresArray };
    CFDictionaryRef attributesDict =
        CFDictionaryCreate(kCFAllocatorDefault,
                           (const void **) attrKeys,
                           (const void **) attrValues,
                           NS_ARRAY_LENGTH(attrKeys),
                           &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks);
    CFRelease(featuresArray);

    sDefaultFeaturesDescriptor =
        CTFontDescriptorCreateWithAttributes(attributesDict);
    CFRelease(attributesDict);
}


CTFontRef
gfxCoreTextFont::CreateCTFontWithDisabledLigatures(ATSFontRef aFontRef, CGFloat aSize)
{
    if (sDisableLigaturesDescriptor == NULL) {
        
        SInt16 val = kLigaturesType;
        CFNumberRef ligaturesType =
            CFNumberCreate(kCFAllocatorDefault,
                           kCFNumberSInt16Type,
                           &val);
        val = kCommonLigaturesOffSelector;
        CFNumberRef commonLigaturesOffSelector =
            CFNumberCreate(kCFAllocatorDefault,
                           kCFNumberSInt16Type,
                           &val);

        const CFTypeRef keys[]   = { kCTFontFeatureTypeIdentifierKey,
                                     kCTFontFeatureSelectorIdentifierKey };
        const CFTypeRef values[] = { ligaturesType,
                                     commonLigaturesOffSelector };
        CFDictionaryRef featureSettingDict =
            CFDictionaryCreate(kCFAllocatorDefault,
                               (const void **) keys,
                               (const void **) values,
                               NS_ARRAY_LENGTH(keys),
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);
        CFRelease(ligaturesType);
        CFRelease(commonLigaturesOffSelector);

        CFArrayRef featuresArray =
            CFArrayCreate(kCFAllocatorDefault,
                          (const void **) &featureSettingDict,
                          1,
                          &kCFTypeArrayCallBacks);
        CFRelease(featureSettingDict);

        CFDictionaryRef attributesDict =
            CFDictionaryCreate(kCFAllocatorDefault,
                               (const void **) &kCTFontFeatureSettingsAttribute,
                               (const void **) &featuresArray,
                               1, 
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);
        CFRelease(featuresArray);

        sDisableLigaturesDescriptor =
            CTFontDescriptorCreateCopyWithAttributes(GetDefaultFeaturesDescriptor(), attributesDict);
        CFRelease(attributesDict);
    }

    return CTFontCreateWithPlatformFont(aFontRef, aSize, NULL, sDisableLigaturesDescriptor);
}

void
gfxCoreTextFont::Shutdown() 
{
    if (sDisableLigaturesDescriptor != NULL) {
        CFRelease(sDisableLigaturesDescriptor);
        sDisableLigaturesDescriptor = NULL;
    }        
    if (sDefaultFeaturesDescriptor != NULL) {
        CFRelease(sDefaultFeaturesDescriptor);
        sDefaultFeaturesDescriptor = NULL;
    }
}








static already_AddRefed<gfxCoreTextFont>
GetOrMakeCTFont(MacOSFontEntry *aFontEntry, const gfxFontStyle *aStyle, PRBool aNeedsBold)
{
    
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aFontEntry->Name(), aStyle);
    if (!font) {
        gfxCoreTextFont *newFont = new gfxCoreTextFont(aFontEntry, aStyle, aNeedsBold);
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
    return static_cast<gfxCoreTextFont *>(f);
}


gfxCoreTextFontGroup::gfxCoreTextFontGroup(const nsAString& families,
                                           const gfxFontStyle *aStyle,
                                           gfxUserFontSet *aUserFontSet)
    : gfxFontGroup(families, aStyle, aUserFontSet)
{
    ForEachFont(FindCTFont, this);

    if (mFonts.Length() == 0) {
        
        
        
        
        

        

        
        
        

        PRBool needsBold;
        MacOSFontEntry *defaultFont = static_cast<MacOSFontEntry*>
            (gfxMacPlatformFontList::PlatformFontList()->GetDefaultFont(aStyle, needsBold));
        NS_ASSERTION(defaultFont, "invalid default font returned by GetDefaultFont");

        nsRefPtr<gfxCoreTextFont> font = GetOrMakeCTFont(defaultFont, aStyle, needsBold);
        if (font) {
            mFonts.AppendElement(font);
        }
    }

    mPageLang = gfxPlatform::GetFontPrefLangFor(mStyle.langGroup.get());

    if (!mStyle.systemFont) {
        for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
            gfxCoreTextFont* font = static_cast<gfxCoreTextFont*>(mFonts[i].get());
            if (font->GetFontEntry()->mIsBadUnderlineFont) {
                gfxFloat first = mFonts[0]->GetMetrics().underlineOffset;
                gfxFloat bad = font->GetMetrics().underlineOffset;
                mUnderlineOffset = PR_MIN(first, bad);
                break;
            }
        }
    }
}

PRBool
gfxCoreTextFontGroup::FindCTFont(const nsAString& aName,
                                 const nsACString& aGenericName,
                                 void *aClosure)
{
    gfxCoreTextFontGroup *fontGroup = static_cast<gfxCoreTextFontGroup*>(aClosure);
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
        nsRefPtr<gfxCoreTextFont> font = GetOrMakeCTFont(fe, fontStyle, needsBold);
        if (font) {
            fontGroup->mFonts.AppendElement(font);
        }
    }

    return PR_TRUE;
}

gfxFontGroup *
gfxCoreTextFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxCoreTextFontGroup(mFamilies, aStyle, mUserFontSet);
}

#define UNICODE_LRO 0x202d
#define UNICODE_RLO 0x202e
#define UNICODE_PDF 0x202c

inline void
AppendDirectionalIndicatorStart(PRUint32 aFlags, nsAString& aString)
{
    static const PRUnichar overrides[2] = { UNICODE_LRO, UNICODE_RLO };
    aString.Append(overrides[(aFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0]);    
    aString.Append(' ');
}

inline void
AppendDirectionalIndicatorEnd(PRBool aNeedDirection, nsAString& aString)
{
    
    
    
    
    aString.Append(' ');
    if (!aNeedDirection)
        return;

    aString.Append('.');
    aString.Append(UNICODE_PDF);
}

gfxTextRun *
gfxCoreTextFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                  const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "should be marked 8bit");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    nsDependentCSubstring cString(reinterpret_cast<const char*>(aString),
                                  reinterpret_cast<const char*>(aString) + aLength);

    nsAutoString utf16;
    PRBool wrapBidi = (aFlags & TEXT_IS_RTL) != 0;
    if (wrapBidi)
        AppendDirectionalIndicatorStart(aFlags, utf16);
    PRUint32 startOffset = utf16.Length();
    AppendASCIItoUTF16(cString, utf16);
    AppendDirectionalIndicatorEnd(wrapBidi, utf16);

    InitTextRun(textRun, utf16.get(), utf16.Length(), startOffset, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxCoreTextFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                  const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    gfxPlatformMac::SetupClusterBoundaries(textRun, aString);

    nsAutoString utf16;
    AppendDirectionalIndicatorStart(aFlags, utf16);
    PRUint32 startOffset = utf16.Length();
    utf16.Append(aString, aLength);
    
    
    AppendDirectionalIndicatorEnd(PR_TRUE, utf16);

    InitTextRun(textRun, utf16.get(), utf16.Length(), startOffset, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

#define SMALL_GLYPH_RUN 128 // preallocated size of our auto arrays for per-glyph data;
                            
                            

void
gfxCoreTextFontGroup::InitTextRun(gfxTextRun *aTextRun,
                                  const PRUnichar *aString,
                                  PRUint32 aTotalLength,
                                  PRUint32 aLayoutStart,
                                  PRUint32 aLayoutLength)
{
    PRBool disableLigatures = (aTextRun->GetFlags() & TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;

    gfxCoreTextFont *mainFont = static_cast<gfxCoreTextFont*>(mFonts[0].get());

#ifdef DUMP_TEXT_RUNS
    NS_ConvertUTF16toUTF8 str(aString, aTotalLength);
    NS_ConvertUTF16toUTF8 families(mFamilies);
    PR_LOG(gCoreTextTextRunLog, PR_LOG_DEBUG,\
           ("MakeTextRun %p fontgroup %p (%s) lang: %s len %d TEXTRUN \"%s\" ENDTEXTRUN\n",
            aTextRun, this, families.get(), mStyle.langGroup.get(), aTotalLength, str.get()));
#endif

    
    
    CFStringRef stringObj =
        CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                           aString,
                                           aTotalLength,
                                           kCFAllocatorNull);

    CFDictionaryRef attrObj;
    if (disableLigatures) {
        
        
        CTFontRef ctFont =
            gfxCoreTextFont::CreateCTFontWithDisabledLigatures(mainFont->GetATSFont(),
                                                               CTFontGetSize(mainFont->GetCTFont()));

        
        attrObj =
            CFDictionaryCreate(kCFAllocatorDefault,
                               (const void**) &kCTFontAttributeName,
                               (const void**) &ctFont,
                               1, 
                               &kCFTypeDictionaryKeyCallBacks,
                               &kCFTypeDictionaryValueCallBacks);
        
        CFRelease(ctFont);
    } else {
        attrObj = mainFont->GetAttributesDictionary();
        CFRetain(attrObj);
    }

    
    CFAttributedStringRef attrStringObj =
        CFAttributedStringCreate(kCFAllocatorDefault, stringObj, attrObj);
    CFRelease(stringObj);
    CFRelease(attrObj);

    
    
    CFMutableAttributedStringRef mutableStringObj = NULL;

    PRUint32 runStart = aLayoutStart;
    nsAutoTArray<gfxTextRange,3> fontRanges;
    ComputeRanges(fontRanges, aString, runStart, runStart + aLayoutLength);
    PRUint32 numRanges = fontRanges.Length();

    nsAutoTArray<PRPackedBool,SMALL_GLYPH_RUN> unmatchedArray;
    PRPackedBool *unmatched = NULL;

    for (PRUint32 r = 0; r < numRanges; r++) {
        const gfxTextRange& range = fontRanges[r];
        UniCharCount matchedLength = range.Length();
        gfxCoreTextFont *matchedFont =
            static_cast<gfxCoreTextFont*> (range.font ? range.font.get() : nsnull);

#ifdef DUMP_TEXT_RUNS
        PR_LOG(gCoreTextTextRunLog, PR_LOG_DEBUG,
               ("InitTextRun %p fontgroup %p font %p match %s (%d-%d)", aTextRun, this, matchedFont,
                   (matchedFont ? NS_ConvertUTF16toUTF8(matchedFont->GetUniqueName()).get() : "<null>"),
                   runStart, matchedLength));
#endif

        if (matchedFont) {
            
            if (matchedFont != mainFont) {
                CTFontRef matchedCTFont = matchedFont->GetCTFont();
                if (disableLigatures)
                    matchedCTFont = gfxCoreTextFont::CreateCTFontWithDisabledLigatures(matchedFont->GetATSFont(),
                                                                                       CTFontGetSize(matchedCTFont));
                
                if (!mutableStringObj) {
                    mutableStringObj =
                        CFAttributedStringCreateMutableCopy(kCFAllocatorDefault,
                                                            0,
                                                            attrStringObj);
                    CFRelease(attrStringObj);
                }
                CFAttributedStringSetAttribute(mutableStringObj,
                                               CFRangeMake(runStart, matchedLength),
                                               kCTFontAttributeName,
                                               matchedCTFont);
                if (disableLigatures)
                    CFRelease(matchedCTFont);
            }

            aTextRun->AddGlyphRun(matchedFont, runStart-aLayoutStart, (matchedLength > 0));

        } else {
            
            if (unmatched == NULL) {
                if (unmatchedArray.SetLength(aTotalLength)) {
                    unmatched = unmatchedArray.Elements();
                    memset(unmatched, PR_FALSE, aTotalLength*sizeof(PRPackedBool));
                }
            }

            
            aTextRun->AddGlyphRun(mainFont, runStart-aLayoutStart, matchedLength);

            for (PRUint32 index = runStart; index < runStart + matchedLength; index++) {
                
                
                if (!mutableStringObj) {
                    mutableStringObj =
                        CFAttributedStringCreateMutableCopy(kCFAllocatorDefault,
                                                            0,
                                                            attrStringObj);
                    CFRelease(attrStringObj);
                }

                if (NS_IS_HIGH_SURROGATE(aString[index]) &&
                    index + 1 < aTotalLength &&
                    NS_IS_LOW_SURROGATE(aString[index+1])) {
                    aTextRun->SetMissingGlyph(index-aLayoutStart,
                                              SURROGATE_TO_UCS4(aString[index],
                                                                aString[index+1]));
                    CFAttributedStringReplaceString(mutableStringObj,
                                                    CFRangeMake(index, 2),
                                                    CFSTR("  "));
                    index++;
                } else {
                    aTextRun->SetMissingGlyph(index-aLayoutStart, aString[index]);
                    CFAttributedStringReplaceString(mutableStringObj,
                                                    CFRangeMake(index, 1),
                                                    CFSTR(" "));
                }
            }

            
            
            
            if (unmatched)
                memset(unmatched + runStart, PR_TRUE, matchedLength);
        }

        runStart += matchedLength;
    }

    
    CTLineRef line;
    if (mutableStringObj) {
        line = CTLineCreateWithAttributedString(mutableStringObj);
        CFRelease(mutableStringObj);
    } else {
        line = CTLineCreateWithAttributedString(attrStringObj);
        CFRelease(attrStringObj);
    }

    
    CFArrayRef glyphRuns = CTLineGetGlyphRuns(line);
    PRUint32 numRuns = CFArrayGetCount(glyphRuns);

    
    
    for (PRUint32 runIndex = 0; runIndex < numRuns; runIndex++) {
        CTRunRef aCTRun = (CTRunRef)CFArrayGetValueAtIndex(glyphRuns, runIndex);
        if (SetGlyphsFromRun(aTextRun, aCTRun, unmatched, aLayoutStart, aLayoutLength) != NS_OK)
            break;
    } 

    CFRelease(line);

    
    
    
    
    aTextRun->SanitizeGlyphRuns();

    
    
    aTextRun->SortGlyphRuns();
}

nsresult
gfxCoreTextFontGroup::SetGlyphsFromRun(gfxTextRun *aTextRun,
                                       CTRunRef aCTRun,
                                       const PRPackedBool *aUnmatched,
                                       PRInt32 aLayoutStart,
                                       PRInt32 aLayoutLength)
{
    
    
    
    

    PRBool isLTR = !aTextRun->IsRightToLeft();
    PRInt32 direction = isLTR ? 1 : -1;

    PRInt32 numGlyphs = CTRunGetGlyphCount(aCTRun);
    if (numGlyphs == 0)
        return NS_OK;

    
    CFRange stringRange = CTRunGetStringRange(aCTRun);
    if (stringRange.location >= aLayoutStart + aLayoutLength ||
        stringRange.location + stringRange.length <= aLayoutStart)
        return NS_OK;

    
    nsAutoArrayPtr<CGGlyph> glyphsArray;
    nsAutoArrayPtr<CGPoint> positionsArray;
    nsAutoArrayPtr<CFIndex> glyphToCharArray;
    const CGGlyph* glyphs = NULL;
    const CGPoint* positions = NULL;
    const CFIndex* glyphToChar = NULL;

    
    
    
    
    
    
    
    
    glyphs = CTRunGetGlyphsPtr(aCTRun);
    if (glyphs == NULL) {
        glyphsArray = new CGGlyph[numGlyphs];
        if (!glyphsArray)
            return NS_ERROR_OUT_OF_MEMORY;
        CTRunGetGlyphs(aCTRun, CFRangeMake(0, 0), glyphsArray.get());
        glyphs = glyphsArray.get();
    }

    positions = CTRunGetPositionsPtr(aCTRun);
    if (positions == NULL) {
        positionsArray = new CGPoint[numGlyphs];
        if (!positionsArray)
            return NS_ERROR_OUT_OF_MEMORY;
        CTRunGetPositions(aCTRun, CFRangeMake(0, 0), positionsArray.get());
        positions = positionsArray.get();
    }

    
    glyphToChar = CTRunGetStringIndicesPtr(aCTRun);
    if (glyphToChar == NULL) {
        glyphToCharArray = new CFIndex[numGlyphs];
        if (!glyphToCharArray)
            return NS_ERROR_OUT_OF_MEMORY;
        CTRunGetStringIndices(aCTRun, CFRangeMake(0, 0), glyphToCharArray.get());
        glyphToChar = glyphToCharArray.get();
    }

    double runWidth = CTRunGetTypographicBounds(aCTRun, CFRangeMake(0, 0), NULL, NULL, NULL);

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    
    
    
    
    
    
    
    
    
    static const PRInt32 NO_GLYPH = -1;
    nsAutoTArray<PRInt32,SMALL_GLYPH_RUN> charToGlyphArray;
    if (!charToGlyphArray.SetLength(stringRange.length))
        return NS_ERROR_OUT_OF_MEMORY;
    PRInt32 *charToGlyph = charToGlyphArray.Elements();
    for (PRInt32 offset = 0; offset < stringRange.length; ++offset)
        charToGlyph[offset] = NO_GLYPH;
    for (PRInt32 g = 0; g < numGlyphs; ++g)
        
        charToGlyph[glyphToChar[g]-stringRange.location] = g;

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRInt32 glyphStart = 0; 
    PRInt32 charStart = isLTR ?
        0 : stringRange.length-1; 

    while (glyphStart < numGlyphs) { 

        PRBool inOrder = PR_TRUE;
        PRInt32 charEnd = glyphToChar[glyphStart] - stringRange.location;
        PRInt32 glyphEnd = glyphStart;
        PRInt32 charLimit = isLTR ? stringRange.length : -1;
        do {
            
            
            
            
            
            charEnd += direction;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += direction;
            }

            
            for (PRInt32 i = charStart; i != charEnd; i += direction) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = PR_MAX(glyphEnd, charToGlyph[i] + 1); 
                }
            }

            if (glyphEnd == glyphStart + 1) {
                
                break;
            }

            if (glyphEnd == glyphStart) {
                
                continue;
            }

            
            
            
            PRBool allGlyphsAreWithinCluster = PR_TRUE;
            PRInt32 prevGlyphCharIndex = charStart;
            for (PRInt32 i = glyphStart; i < glyphEnd; ++i) {
                PRInt32 glyphCharIndex = glyphToChar[i] - stringRange.location;
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
            while (charEnd < stringRange.length && charToGlyph[charEnd] == NO_GLYPH)
                charEnd++;
            baseCharIndex = charStart + stringRange.location;
            endCharIndex = charEnd + stringRange.location;
        } else {
            while (charEnd >= 0 && charToGlyph[charEnd] == NO_GLYPH)
                charEnd--;
            baseCharIndex = charEnd + stringRange.location + 1;
            endCharIndex = charStart + stringRange.location + 1;
        }

        
        if (endCharIndex <= aLayoutStart || baseCharIndex >= aLayoutStart + aLayoutLength) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        baseCharIndex = PR_MAX(baseCharIndex, aLayoutStart);
        endCharIndex = PR_MIN(endCharIndex, aLayoutStart + aLayoutLength);

        
        if (aUnmatched && aUnmatched[baseCharIndex]) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        
        double toNextGlyph;
        if (glyphStart < numGlyphs-1)
            toNextGlyph = positions[glyphStart+1].x - positions[glyphStart].x;
        else
            toNextGlyph = positions[0].x + runWidth - positions[glyphStart].x;
        PRInt32 advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);

        
        PRInt32 glyphsInClump = glyphEnd - glyphStart;
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyphs[glyphStart]) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            aTextRun->IsClusterStart(baseCharIndex - aLayoutStart) &&
            positions[glyphStart].y == 0.0)
        {
            aTextRun->SetSimpleGlyph(baseCharIndex - aLayoutStart,
                                     g.SetSimpleGlyph(advance, glyphs[glyphStart]));
        } else {
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph *details = detailedGlyphs.AppendElement();
                details->mGlyphID = glyphs[glyphStart];
                details->mXOffset = 0;
                details->mYOffset = -positions[glyphStart].y * appUnitsPerDevUnit;
                details->mAdvance = advance;
                if (++glyphStart >= glyphEnd)
                   break;
                if (glyphStart < numGlyphs-1)
                    toNextGlyph = positions[glyphStart+1].x - positions[glyphStart].x;
                else
                    toNextGlyph = positions[0].x + runWidth - positions[glyphStart].x;
                advance = PRInt32(toNextGlyph * appUnitsPerDevUnit);
            }

            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(baseCharIndex - aLayoutStart),
                         PR_TRUE, detailedGlyphs.Length());
            aTextRun->SetGlyphs(baseCharIndex - aLayoutStart, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex &&
            (baseCharIndex - aLayoutStart) < aLayoutLength) {
            g.SetComplex(inOrder && aTextRun->IsClusterStart(baseCharIndex - aLayoutStart),
                         PR_FALSE, 0);
            aTextRun->SetGlyphs(baseCharIndex - aLayoutStart, g, nsnull);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return NS_OK;
}

PRBool
gfxCoreTextFontGroup::HasFont(ATSFontRef aFontRef)
{
    for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
        if (aFontRef == static_cast<gfxCoreTextFont *>(mFonts.ElementAt(i).get())->GetATSFont())
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
gfxCoreTextFontGroup::WhichPrefFontSupportsChar(PRUint32 aCh)
{
    gfxFont *font;

    
    if (aCh > 0xFFFF)
        return nsnull;

    
    PRUint32 unicodeRange = FindCharUnicodeRange(aCh);
    eFontPrefLang charLang = gfxPlatformMac::GetFontPrefLangFor(unicodeRange);

    
    if (mLastPrefFont && charLang == mLastPrefLang &&
        mLastPrefFirstFont && mLastPrefFont->TestCharacterMap(aCh)) {
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
                nsRefPtr<gfxCoreTextFont> prefFont = GetOrMakeCTFont(fe, &mStyle, needsBold);
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
gfxCoreTextFontGroup::WhichSystemFontSupportsChar(PRUint32 aCh)
{
    MacOSFontEntry *fe = static_cast<MacOSFontEntry*>
        (gfxMacPlatformFontList::PlatformFontList()->FindFontForChar(aCh, GetFontAt(0)));
    if (fe) {
        nsRefPtr<gfxCoreTextFont> ctFont = GetOrMakeCTFont(fe, &mStyle, PR_FALSE); 
        nsRefPtr<gfxFont> font = ctFont.get();
        return font.forget();
    }

    return nsnull;
}

void
gfxCoreTextFontGroup::UpdateFontList()
{
    
    if (mUserFontSet && mCurrGeneration != GetGeneration()) {
        
        mFonts.Clear();
        mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
        ForEachFont(FindCTFont, this);
        mCurrGeneration = GetGeneration();
    }
}
