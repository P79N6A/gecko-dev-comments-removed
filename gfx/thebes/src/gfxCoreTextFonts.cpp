







































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
        ::CFDictionaryCreate(kCFAllocatorDefault,
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
    if (::CTFontGetGlyphsForCharacters(mCTFont, &c, &glyph, 1)) {
        CGSize advance;
        ::CTFontGetAdvancesForGlyphs(mCTFont,
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
    if (::CTFontGetGlyphsForCharacters(mCTFont, &c, &glyph, 1)) {
        CGRect boundingRect;
        ::CTFontGetBoundingRectsForGlyphs(mCTFont,
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
        ::CFRelease(mAttributesDict);
    if (mCTFont)
        ::CFRelease(mCTFont);
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
        ::CFRelease(mCTFont);
        mCTFont = NULL;
    }

    ATSFontMetrics atsMetrics;
    OSStatus err;

    err = ::ATSFontGetHorizontalMetrics(mATSFont, kATSOptionFlagsDefault,
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
        mCTFont = ::CTFontCreateWithPlatformFont(mATSFont, size, NULL, GetDefaultFeaturesDescriptor());
        mMetrics.xHeight = ::CTFontGetXHeight(mCTFont);
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
        mCTFont = ::CTFontCreateWithPlatformFont(mATSFont, size, NULL, GetDefaultFeaturesDescriptor());

    mMetrics.superscriptOffset = mMetrics.xHeight;
    mMetrics.subscriptOffset = mMetrics.xHeight;
    mMetrics.underlineSize = ::CTFontGetUnderlineThickness(mCTFont);
    mMetrics.underlineOffset = ::CTFontGetUnderlinePosition(mCTFont);
    mMetrics.strikeoutSize = mMetrics.underlineSize;
    mMetrics.strikeoutOffset = mMetrics.xHeight / 2;

    mMetrics.externalLeading = ::CTFontGetLeading(mCTFont);
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
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize,
                              mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif
}

void
gfxCoreTextFont::InitTextRun(gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength)
{
    
    

    PRBool disableLigatures = (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;

    

    PRBool isRTL = aTextRun->IsRightToLeft();

    
    
    PRBool bidiWrap = isRTL;
    if (!bidiWrap && (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) == 0) {
        PRUint32 i;
        for (i = aRunStart; i < aRunStart + aRunLength; ++i) {
            if (gfxFontUtils::PotentialRTLChar(aString[i])) {
                bidiWrap = PR_TRUE;
                break;
            }
        }
    }

    
    
    const UniChar beginLTR[]    = { 0x202d };
    const UniChar beginRTL[]    = { 0x202e };
    const UniChar endBidiWrap[] = { 0x202c };

    PRUint32 startOffset;
    CFStringRef stringObj;
    if (bidiWrap) {
        startOffset = isRTL ?
            sizeof(beginRTL) / sizeof(beginRTL[0]) : sizeof(beginLTR) / sizeof(beginLTR[0]);
        CFMutableStringRef mutableString =
            ::CFStringCreateMutable(kCFAllocatorDefault,
                                    aRunLength + startOffset +
                                    sizeof(endBidiWrap) / sizeof(endBidiWrap[0]));
        ::CFStringAppendCharacters(mutableString,
                                   isRTL ? beginRTL : beginLTR,
                                   startOffset);
        ::CFStringAppendCharacters(mutableString,
                                   aString + aRunStart, aRunLength);
        ::CFStringAppendCharacters(mutableString,
                                   endBidiWrap,
                                   sizeof(endBidiWrap) / sizeof(endBidiWrap[0]));
        stringObj = mutableString;
    } else {
        startOffset = 0;
        stringObj = ::CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                                         aString + aRunStart,
                                                         aRunLength,
                                                         kCFAllocatorNull);
    }

    CFDictionaryRef attrObj;
    if (disableLigatures) {
        
        
        CTFontRef ctFont =
            gfxCoreTextFont::CreateCTFontWithDisabledLigatures(GetATSFont(),
                                                               ::CTFontGetSize(GetCTFont()));

        attrObj =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void**) &kCTFontAttributeName,
                                 (const void**) &ctFont,
                                 1, 
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);
        
        ::CFRelease(ctFont);
    } else {
        attrObj = GetAttributesDictionary();
        ::CFRetain(attrObj);
    }

    
    CFAttributedStringRef attrStringObj =
        ::CFAttributedStringCreate(kCFAllocatorDefault, stringObj, attrObj);
    ::CFRelease(stringObj);
    ::CFRelease(attrObj);

    
    CTLineRef line = ::CTLineCreateWithAttributedString(attrStringObj);
    ::CFRelease(attrStringObj);

    
    CFArrayRef glyphRuns = ::CTLineGetGlyphRuns(line);
    PRUint32 numRuns = ::CFArrayGetCount(glyphRuns);

    
    
    
    for (PRUint32 runIndex = 0; runIndex < numRuns; runIndex++) {
        CTRunRef aCTRun = (CTRunRef)::CFArrayGetValueAtIndex(glyphRuns, runIndex);
        if (SetGlyphsFromRun(aTextRun, aCTRun, startOffset, aRunStart, aRunLength) != NS_OK)
            break;
    }

    ::CFRelease(line);
}

#define SMALL_GLYPH_RUN 128 // preallocated size of our auto arrays for per-glyph data;
                            
                            

nsresult
gfxCoreTextFont::SetGlyphsFromRun(gfxTextRun *aTextRun,
                                  CTRunRef aCTRun,
                                  PRInt32 aStringOffset, 
                                  PRInt32 aRunStart,     
                                  PRInt32 aRunLength)    
{
    
    
    
    
    

    PRBool isLTR = !aTextRun->IsRightToLeft();
    PRInt32 direction = isLTR ? 1 : -1;

    PRInt32 numGlyphs = ::CTRunGetGlyphCount(aCTRun);
    if (numGlyphs == 0)
        return NS_OK;

    
    
    
    
    
    
    
    
    

    
    CFRange stringRange = ::CTRunGetStringRange(aCTRun);
    
    if (stringRange.location - aStringOffset + stringRange.length <= 0 ||
        stringRange.location - aStringOffset >= aRunLength)
        return NS_OK;

    
    nsAutoArrayPtr<CGGlyph> glyphsArray;
    nsAutoArrayPtr<CGPoint> positionsArray;
    nsAutoArrayPtr<CFIndex> glyphToCharArray;
    const CGGlyph* glyphs = NULL;
    const CGPoint* positions = NULL;
    const CFIndex* glyphToChar = NULL;

    
    
    
    
    
    
    
    
    glyphs = ::CTRunGetGlyphsPtr(aCTRun);
    if (glyphs == NULL) {
        glyphsArray = new CGGlyph[numGlyphs];
        if (!glyphsArray)
            return NS_ERROR_OUT_OF_MEMORY;
        ::CTRunGetGlyphs(aCTRun, ::CFRangeMake(0, 0), glyphsArray.get());
        glyphs = glyphsArray.get();
    }

    positions = ::CTRunGetPositionsPtr(aCTRun);
    if (positions == NULL) {
        positionsArray = new CGPoint[numGlyphs];
        if (!positionsArray)
            return NS_ERROR_OUT_OF_MEMORY;
        ::CTRunGetPositions(aCTRun, ::CFRangeMake(0, 0), positionsArray.get());
        positions = positionsArray.get();
    }

    
    
    glyphToChar = ::CTRunGetStringIndicesPtr(aCTRun);
    if (glyphToChar == NULL) {
        glyphToCharArray = new CFIndex[numGlyphs];
        if (!glyphToCharArray)
            return NS_ERROR_OUT_OF_MEMORY;
        ::CTRunGetStringIndices(aCTRun, ::CFRangeMake(0, 0), glyphToCharArray.get());
        glyphToChar = glyphToCharArray.get();
    }

    double runWidth = ::CTRunGetTypographicBounds(aCTRun, ::CFRangeMake(0, 0), NULL, NULL, NULL);

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
    for (PRInt32 g = 0; g < numGlyphs; ++g) {
        PRInt32 loc = glyphToChar[g] - stringRange.location;
        if (loc == 0 && !isLTR)
            ++loc; 
        if (loc >= 0 && loc < stringRange.length) {
            charToGlyph[loc] = g;
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    

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
            while (charEnd != charLimit && charToGlyph[charEnd - stringRange.location] == NO_GLYPH) {
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
            baseCharIndex = charStart + stringRange.location - aStringOffset + aRunStart;
            endCharIndex = charEnd + stringRange.location - aStringOffset + aRunStart;
        } else {
            while (charEnd >= 0 && charToGlyph[charEnd] == NO_GLYPH)
                charEnd--;
            baseCharIndex = charEnd + stringRange.location - aStringOffset + aRunStart + 1;
            endCharIndex = charStart + stringRange.location - aStringOffset + aRunStart + 1;
        }

        
        if (endCharIndex <= aRunStart || baseCharIndex >= aRunStart + aRunLength) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        baseCharIndex = PR_MAX(baseCharIndex, aRunStart);
        endCharIndex = PR_MIN(endCharIndex, aRunStart + aRunLength);

        
        
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
            aTextRun->IsClusterStart(baseCharIndex) &&
            positions[glyphStart].y == 0.0)
        {
            aTextRun->SetSimpleGlyph(baseCharIndex,
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
            g.SetComplex(aTextRun->IsClusterStart(baseCharIndex),
                         PR_TRUE, detailedGlyphs.Length());
            aTextRun->SetGlyphs(baseCharIndex, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex && baseCharIndex < aRunStart + aRunLength) {
            g.SetComplex(inOrder && aTextRun->IsClusterStart(baseCharIndex),
                         PR_FALSE, 0);
            aTextRun->SetGlyphs(baseCharIndex, g, nsnull);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return NS_OK;
}




void
gfxCoreTextFont::CreateDefaultFeaturesDescriptor()
{
    if (sDefaultFeaturesDescriptor != NULL)
        return;

    SInt16 val = kSmartSwashType;
    CFNumberRef swashesType =
        ::CFNumberCreate(kCFAllocatorDefault,
                         kCFNumberSInt16Type,
                         &val);
    val = kLineInitialSwashesOffSelector;
    CFNumberRef lineInitialsOffSelector =
        ::CFNumberCreate(kCFAllocatorDefault,
                         kCFNumberSInt16Type,
                         &val);

    CFTypeRef keys[]   = { kCTFontFeatureTypeIdentifierKey,
                           kCTFontFeatureSelectorIdentifierKey };
    CFTypeRef values[] = { swashesType,
                           lineInitialsOffSelector };
    CFDictionaryRef featureSettings[2];
    featureSettings[0] =
        ::CFDictionaryCreate(kCFAllocatorDefault,
                             (const void **) keys,
                             (const void **) values,
                             NS_ARRAY_LENGTH(keys),
                             &kCFTypeDictionaryKeyCallBacks,
                             &kCFTypeDictionaryValueCallBacks);
    ::CFRelease(lineInitialsOffSelector);

    val = kLineFinalSwashesOffSelector;
    CFNumberRef lineFinalsOffSelector =
        ::CFNumberCreate(kCFAllocatorDefault,
                         kCFNumberSInt16Type,
                         &val);
    values[1] = lineFinalsOffSelector;
    featureSettings[1] =
        ::CFDictionaryCreate(kCFAllocatorDefault,
                             (const void **) keys,
                             (const void **) values,
                             NS_ARRAY_LENGTH(keys),
                             &kCFTypeDictionaryKeyCallBacks,
                             &kCFTypeDictionaryValueCallBacks);
    ::CFRelease(lineFinalsOffSelector);
    ::CFRelease(swashesType);

    CFArrayRef featuresArray =
        ::CFArrayCreate(kCFAllocatorDefault,
                        (const void **) featureSettings,
                        NS_ARRAY_LENGTH(featureSettings),
                        &kCFTypeArrayCallBacks);
    ::CFRelease(featureSettings[0]);
    ::CFRelease(featureSettings[1]);

    const CFTypeRef attrKeys[]   = { kCTFontFeatureSettingsAttribute };
    const CFTypeRef attrValues[] = { featuresArray };
    CFDictionaryRef attributesDict =
        ::CFDictionaryCreate(kCFAllocatorDefault,
                             (const void **) attrKeys,
                             (const void **) attrValues,
                             NS_ARRAY_LENGTH(attrKeys),
                             &kCFTypeDictionaryKeyCallBacks,
                             &kCFTypeDictionaryValueCallBacks);
    ::CFRelease(featuresArray);

    sDefaultFeaturesDescriptor =
        ::CTFontDescriptorCreateWithAttributes(attributesDict);
    ::CFRelease(attributesDict);
}


CTFontRef
gfxCoreTextFont::CreateCTFontWithDisabledLigatures(ATSFontRef aFontRef, CGFloat aSize)
{
    if (sDisableLigaturesDescriptor == NULL) {
        
        SInt16 val = kLigaturesType;
        CFNumberRef ligaturesType =
            ::CFNumberCreate(kCFAllocatorDefault,
                             kCFNumberSInt16Type,
                             &val);
        val = kCommonLigaturesOffSelector;
        CFNumberRef commonLigaturesOffSelector =
            ::CFNumberCreate(kCFAllocatorDefault,
                             kCFNumberSInt16Type,
                             &val);

        const CFTypeRef keys[]   = { kCTFontFeatureTypeIdentifierKey,
                                     kCTFontFeatureSelectorIdentifierKey };
        const CFTypeRef values[] = { ligaturesType,
                                     commonLigaturesOffSelector };
        CFDictionaryRef featureSettingDict =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void **) keys,
                                 (const void **) values,
                                 NS_ARRAY_LENGTH(keys),
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);
        ::CFRelease(ligaturesType);
        ::CFRelease(commonLigaturesOffSelector);

        CFArrayRef featuresArray =
            ::CFArrayCreate(kCFAllocatorDefault,
                            (const void **) &featureSettingDict,
                            1,
                            &kCFTypeArrayCallBacks);
        ::CFRelease(featureSettingDict);

        CFDictionaryRef attributesDict =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void **) &kCTFontFeatureSettingsAttribute,
                                 (const void **) &featuresArray,
                                 1, 
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);
        ::CFRelease(featuresArray);

        sDisableLigaturesDescriptor =
            ::CTFontDescriptorCreateCopyWithAttributes(GetDefaultFeaturesDescriptor(), attributesDict);
        ::CFRelease(attributesDict);
    }
    
    return ::CTFontCreateWithPlatformFont(aFontRef, aSize, NULL, sDisableLigaturesDescriptor);
}

void
gfxCoreTextFont::Shutdown() 
{
    if (sDisableLigaturesDescriptor != NULL) {
        ::CFRelease(sDisableLigaturesDescriptor);
        sDisableLigaturesDescriptor = NULL;
    }        
    if (sDefaultFeaturesDescriptor != NULL) {
        ::CFRelease(sDefaultFeaturesDescriptor);
        sDefaultFeaturesDescriptor = NULL;
    }
}
