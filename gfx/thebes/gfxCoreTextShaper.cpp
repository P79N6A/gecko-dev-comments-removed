







































#include "prtypes.h"
#include "nsAlgorithm.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"

#include "gfxTypes.h"

#include "nsPromiseFlatString.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxPlatformMac.h"
#include "gfxCoreTextShaper.h"
#include "gfxMacFont.h"

#include "gfxFontTest.h"
#include "gfxFontUtils.h"

#include "gfxQuartzSurface.h"
#include "gfxMacPlatformFontList.h"
#include "gfxUserFontSet.h"

#include "nsUnicodeRange.h"


CTFontDescriptorRef gfxCoreTextShaper::sDefaultFeaturesDescriptor = NULL;
CTFontDescriptorRef gfxCoreTextShaper::sDisableLigaturesDescriptor = NULL;

gfxCoreTextShaper::gfxCoreTextShaper(gfxMacFont *aFont)
    : gfxFontShaper(aFont)
{
    
    mCTFont = ::CTFontCreateWithPlatformFont(aFont->GetATSFontRef(),
                                             aFont->GetAdjustedSize(),
                                             NULL,
                                             GetDefaultFeaturesDescriptor());

    
    mAttributesDict = ::CFDictionaryCreate(kCFAllocatorDefault,
                                           (const void**) &kCTFontAttributeName,
                                           (const void**) &mCTFont,
                                           1, 
                                           &kCFTypeDictionaryKeyCallBacks,
                                           &kCFTypeDictionaryValueCallBacks);
}

gfxCoreTextShaper::~gfxCoreTextShaper()
{
    if (mAttributesDict) {
        ::CFRelease(mAttributesDict);
    }
    if (mCTFont) {
        ::CFRelease(mCTFont);
    }
}

PRBool
gfxCoreTextShaper::InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript)
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

    
    
    const UniChar beginLTR[]    = { 0x202d, 0x20 };
    const UniChar beginRTL[]    = { 0x202e, 0x20 };
    const UniChar endBidiWrap[] = { 0x20, 0x2e, 0x202c };

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
        
        
        gfxMacFont *font = static_cast<gfxMacFont*>(mFont);
        CTFontRef ctFont =
            CreateCTFontWithDisabledLigatures(font->GetATSFontRef(),
                                              ::CTFontGetSize(mCTFont));

        attrObj =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void**) &kCTFontAttributeName,
                                 (const void**) &ctFont,
                                 1, 
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);
        
        ::CFRelease(ctFont);
    } else {
        attrObj = mAttributesDict;
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

    
    
    
    PRBool success = PR_TRUE;
    for (PRUint32 runIndex = 0; runIndex < numRuns; runIndex++) {
        CTRunRef aCTRun = (CTRunRef)::CFArrayGetValueAtIndex(glyphRuns, runIndex);
        if (SetGlyphsFromRun(aTextRun, aCTRun, startOffset,
                             aRunStart, aRunLength) != NS_OK) {
            success = PR_FALSE;
            break;
        }
    }

    ::CFRelease(line);

    return success;
}

#define SMALL_GLYPH_RUN 128 // preallocated size of our auto arrays for per-glyph data;
                            
                            

nsresult
gfxCoreTextShaper::SetGlyphsFromRun(gfxTextRun *aTextRun,
                                    CTRunRef aCTRun,
                                    PRInt32 aStringOffset, 
                                    PRInt32 aRunStart,     
                                    PRInt32 aRunLength)    
{
    
    
    
    
    

    PRBool isLTR = !aTextRun->IsRightToLeft();
    PRInt32 direction = isLTR ? 1 : -1;

    PRInt32 numGlyphs = ::CTRunGetGlyphCount(aCTRun);
    if (numGlyphs == 0) {
        return NS_OK;
    }

    
    
    
    
    
    
    
    
    

    
    CFRange stringRange = ::CTRunGetStringRange(aCTRun);
    
    if (stringRange.location - aStringOffset + stringRange.length <= 0 ||
        stringRange.location - aStringOffset >= aRunLength) {
        return NS_OK;
    }

    
    nsAutoArrayPtr<CGGlyph> glyphsArray;
    nsAutoArrayPtr<CGPoint> positionsArray;
    nsAutoArrayPtr<CFIndex> glyphToCharArray;
    const CGGlyph* glyphs = NULL;
    const CGPoint* positions = NULL;
    const CFIndex* glyphToChar = NULL;

    
    
    
    
    
    
    
    
    glyphs = ::CTRunGetGlyphsPtr(aCTRun);
    if (!glyphs) {
        glyphsArray = new (std::nothrow) CGGlyph[numGlyphs];
        if (!glyphsArray) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        ::CTRunGetGlyphs(aCTRun, ::CFRangeMake(0, 0), glyphsArray.get());
        glyphs = glyphsArray.get();
    }

    positions = ::CTRunGetPositionsPtr(aCTRun);
    if (!positions) {
        positionsArray = new (std::nothrow) CGPoint[numGlyphs];
        if (!positionsArray) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        ::CTRunGetPositions(aCTRun, ::CFRangeMake(0, 0), positionsArray.get());
        positions = positionsArray.get();
    }

    
    
    glyphToChar = ::CTRunGetStringIndicesPtr(aCTRun);
    if (!glyphToChar) {
        glyphToCharArray = new (std::nothrow) CFIndex[numGlyphs];
        if (!glyphToCharArray) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
        ::CTRunGetStringIndices(aCTRun, ::CFRangeMake(0, 0), glyphToCharArray.get());
        glyphToChar = glyphToCharArray.get();
    }

    double runWidth = ::CTRunGetTypographicBounds(aCTRun, ::CFRangeMake(0, 0), NULL, NULL, NULL);

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
    gfxTextRun::CompressedGlyph g;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    
    
    
    
    
    
    

    

    static const PRInt32 NO_GLYPH = -1;
    nsAutoTArray<PRInt32,SMALL_GLYPH_RUN> charToGlyphArray;
    if (!charToGlyphArray.SetLength(stringRange.length)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    PRInt32 *charToGlyph = charToGlyphArray.Elements();
    for (PRInt32 offset = 0; offset < stringRange.length; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }
    for (PRInt32 i = 0; i < numGlyphs; ++i) {
        PRInt32 loc = glyphToChar[i] - stringRange.location;
        if (loc >= 0 && loc < stringRange.length) {
            charToGlyph[loc] = i;
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRInt32 glyphStart = 0; 
    PRInt32 charStart = isLTR ?
        0 : stringRange.length-1; 

    while (glyphStart < numGlyphs) { 

        PRBool inOrder = PR_TRUE;
        PRInt32 charEnd = glyphToChar[glyphStart] - stringRange.location;
        NS_ASSERTION(charEnd >= 0 && charEnd < stringRange.length,
                     "glyph-to-char mapping points outside string range");
        PRInt32 glyphEnd = glyphStart;
        PRInt32 charLimit = isLTR ? stringRange.length : -1;
        do {
            
            
            
            
            
            NS_ASSERTION((direction > 0 && charEnd < charLimit) ||
                         (direction < 0 && charEnd > charLimit),
                         "no characters left in range?");
            charEnd += direction;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += direction;
            }

            
            for (PRInt32 i = charStart; i != charEnd; i += direction) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = NS_MAX(glyphEnd, charToGlyph[i] + 1); 
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
            while (charEnd < stringRange.length && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd++;
            }
            baseCharIndex = charStart + stringRange.location - aStringOffset + aRunStart;
            endCharIndex = charEnd + stringRange.location - aStringOffset + aRunStart;
        } else {
            while (charEnd >= 0 && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd--;
            }
            baseCharIndex = charEnd + stringRange.location - aStringOffset + aRunStart + 1;
            endCharIndex = charStart + stringRange.location - aStringOffset + aRunStart + 1;
        }

        
        if (endCharIndex <= aRunStart || baseCharIndex >= aRunStart + aRunLength) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        baseCharIndex = NS_MAX(baseCharIndex, aRunStart);
        endCharIndex = NS_MIN(endCharIndex, aRunStart + aRunLength);

        
        
        double toNextGlyph;
        if (glyphStart < numGlyphs-1) {
            toNextGlyph = positions[glyphStart+1].x - positions[glyphStart].x;
        } else {
            toNextGlyph = positions[0].x + runWidth - positions[glyphStart].x;
        }
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
                if (++glyphStart >= glyphEnd) {
                   break;
                }
                if (glyphStart < numGlyphs-1) {
                    toNextGlyph = positions[glyphStart+1].x - positions[glyphStart].x;
                } else {
                    toNextGlyph = positions[0].x + runWidth - positions[glyphStart].x;
                }
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
gfxCoreTextShaper::CreateDefaultFeaturesDescriptor()
{
    if (sDefaultFeaturesDescriptor != NULL) {
        return;
    }

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
gfxCoreTextShaper::CreateCTFontWithDisabledLigatures(ATSFontRef aFontRef, CGFloat aSize)
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
gfxCoreTextShaper::Shutdown() 
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
