




#include "mozilla/Util.h"

#include "nsAlgorithm.h"
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

using namespace mozilla;


CTFontDescriptorRef gfxCoreTextShaper::sDefaultFeaturesDescriptor = NULL;
CTFontDescriptorRef gfxCoreTextShaper::sDisableLigaturesDescriptor = NULL;

gfxCoreTextShaper::gfxCoreTextShaper(gfxMacFont *aFont)
    : gfxFontShaper(aFont)
{
    
    mCTFont = ::CTFontCreateWithGraphicsFont(aFont->GetCGFontRef(),
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

bool
gfxCoreTextShaper::ShapeWord(gfxContext      *aContext,
                             gfxShapedWord   *aShapedWord,
                             const PRUnichar *aText)
{
    

    bool isRightToLeft = aShapedWord->IsRightToLeft();
    uint32_t length = aShapedWord->Length();

    
    
    bool bidiWrap = isRightToLeft;
    if (!bidiWrap && !aShapedWord->TextIs8Bit()) {
        const PRUnichar *text = aShapedWord->TextUnicode();
        uint32_t i;
        for (i = 0; i < length; ++i) {
            if (gfxFontUtils::PotentialRTLChar(text[i])) {
                bidiWrap = true;
                break;
            }
        }
    }

    
    
    const UniChar beginLTR[]    = { 0x202d, 0x20 };
    const UniChar beginRTL[]    = { 0x202e, 0x20 };
    const UniChar endBidiWrap[] = { 0x20, 0x2e, 0x202c };

    uint32_t startOffset;
    CFStringRef stringObj;
    if (bidiWrap) {
        startOffset = isRightToLeft ?
            mozilla::ArrayLength(beginRTL) : mozilla::ArrayLength(beginLTR);
        CFMutableStringRef mutableString =
            ::CFStringCreateMutable(kCFAllocatorDefault,
                                    length + startOffset + mozilla::ArrayLength(endBidiWrap));
        ::CFStringAppendCharacters(mutableString,
                                   isRightToLeft ? beginRTL : beginLTR,
                                   startOffset);
        ::CFStringAppendCharacters(mutableString, aText, length);
        ::CFStringAppendCharacters(mutableString,
                                   endBidiWrap, mozilla::ArrayLength(endBidiWrap));
        stringObj = mutableString;
    } else {
        startOffset = 0;
        stringObj = ::CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                                         aText, length,
                                                         kCFAllocatorNull);
    }

    CFDictionaryRef attrObj;
    if (aShapedWord->DisableLigatures()) {
        
        
        CTFontRef ctFont =
            CreateCTFontWithDisabledLigatures(::CTFontGetSize(mCTFont));

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
    uint32_t numRuns = ::CFArrayGetCount(glyphRuns);

    
    
    
    bool success = true;
    for (uint32_t runIndex = 0; runIndex < numRuns; runIndex++) {
        CTRunRef aCTRun =
            (CTRunRef)::CFArrayGetValueAtIndex(glyphRuns, runIndex);
        if (SetGlyphsFromRun(aShapedWord, aCTRun, startOffset) != NS_OK) {
            success = false;
            break;
        }
    }

    ::CFRelease(line);

    return success;
}

#define SMALL_GLYPH_RUN 128 // preallocated size of our auto arrays for per-glyph data;
                            
                            

nsresult
gfxCoreTextShaper::SetGlyphsFromRun(gfxShapedWord *aShapedWord,
                                    CTRunRef aCTRun,
                                    int32_t aStringOffset)
{
    
    
    

    int32_t direction = aShapedWord->IsRightToLeft() ? -1 : 1;

    int32_t numGlyphs = ::CTRunGetGlyphCount(aCTRun);
    if (numGlyphs == 0) {
        return NS_OK;
    }

    int32_t wordLength = aShapedWord->Length();

    
    
    
    
    
    
    

    
    CFRange stringRange = ::CTRunGetStringRange(aCTRun);
    
    if (stringRange.location - aStringOffset + stringRange.length <= 0 ||
        stringRange.location - aStringOffset >= wordLength) {
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

    double runWidth = ::CTRunGetTypographicBounds(aCTRun, ::CFRangeMake(0, 0),
                                                  NULL, NULL, NULL);

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;
    gfxTextRun::CompressedGlyph g;

    
    
    
    
    
    
    

    

    static const int32_t NO_GLYPH = -1;
    nsAutoTArray<int32_t,SMALL_GLYPH_RUN> charToGlyphArray;
    if (!charToGlyphArray.SetLength(stringRange.length)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    int32_t *charToGlyph = charToGlyphArray.Elements();
    for (int32_t offset = 0; offset < stringRange.length; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }
    for (int32_t i = 0; i < numGlyphs; ++i) {
        int32_t loc = glyphToChar[i] - stringRange.location;
        if (loc >= 0 && loc < stringRange.length) {
            charToGlyph[loc] = i;
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    

    
    

    bool isRightToLeft = aShapedWord->IsRightToLeft();
    int32_t glyphStart = 0; 
    int32_t charStart = isRightToLeft ?
        stringRange.length - 1 : 0; 

    while (glyphStart < numGlyphs) { 
        bool inOrder = true;
        int32_t charEnd = glyphToChar[glyphStart] - stringRange.location;
        NS_WARN_IF_FALSE(charEnd >= 0 && charEnd < stringRange.length,
                         "glyph-to-char mapping points outside string range");
        
        charEnd = NS_MAX(charEnd, 0);
        charEnd = NS_MIN(charEnd, int32_t(stringRange.length));

        int32_t glyphEnd = glyphStart;
        int32_t charLimit = isRightToLeft ? -1 : stringRange.length;
        do {
            
            
            
            
            
            NS_ASSERTION((direction > 0 && charEnd < charLimit) ||
                         (direction < 0 && charEnd > charLimit),
                         "no characters left in range?");
            charEnd += direction;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += direction;
            }

            
            if (isRightToLeft) {
                for (int32_t i = charStart; i > charEnd; --i) {
                    if (charToGlyph[i] != NO_GLYPH) {
                        
                        glyphEnd = NS_MAX(glyphEnd, charToGlyph[i] + 1);
                    }
                }
            } else {
                for (int32_t i = charStart; i < charEnd; ++i) {
                    if (charToGlyph[i] != NO_GLYPH) {
                        
                        glyphEnd = NS_MAX(glyphEnd, charToGlyph[i] + 1);
                    }
                }
            }

            if (glyphEnd == glyphStart + 1) {
                
                break;
            }

            if (glyphEnd == glyphStart) {
                
                continue;
            }

            
            
            
            bool allGlyphsAreWithinCluster = true;
            int32_t prevGlyphCharIndex = charStart;
            for (int32_t i = glyphStart; i < glyphEnd; ++i) {
                int32_t glyphCharIndex = glyphToChar[i] - stringRange.location;
                if (isRightToLeft) {
                    if (glyphCharIndex > charStart || glyphCharIndex <= charEnd) {
                        allGlyphsAreWithinCluster = false;
                        break;
                    }
                    if (glyphCharIndex > prevGlyphCharIndex) {
                        inOrder = false;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                } else {
                    if (glyphCharIndex < charStart || glyphCharIndex >= charEnd) {
                        allGlyphsAreWithinCluster = false;
                        break;
                    }
                    if (glyphCharIndex < prevGlyphCharIndex) {
                        inOrder = false;
                    }
                    prevGlyphCharIndex = glyphCharIndex;
                }
            }
            if (allGlyphsAreWithinCluster) {
                break;
            }
        } while (charEnd != charLimit);

        NS_WARN_IF_FALSE(glyphStart < glyphEnd,
                         "character/glyph clump contains no glyphs!");
        if (glyphStart == glyphEnd) {
            ++glyphStart; 
            charStart = charEnd;
            continue;
        }

        NS_WARN_IF_FALSE(charStart != charEnd,
                         "character/glyph clump contains no characters!");
        if (charStart == charEnd) {
            glyphStart = glyphEnd; 
                                   
            continue;
        }

        
        
        
        
        int32_t baseCharIndex, endCharIndex;
        if (isRightToLeft) {
            while (charEnd >= 0 && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd--;
            }
            baseCharIndex = charEnd + stringRange.location - aStringOffset + 1;
            endCharIndex = charStart + stringRange.location - aStringOffset + 1;
        } else {
            while (charEnd < stringRange.length && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd++;
            }
            baseCharIndex = charStart + stringRange.location - aStringOffset;
            endCharIndex = charEnd + stringRange.location - aStringOffset;
        }

        
        if (endCharIndex <= 0 || baseCharIndex >= wordLength) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        baseCharIndex = NS_MAX(baseCharIndex, 0);
        endCharIndex = NS_MIN(endCharIndex, wordLength);

        
        
        int32_t appUnitsPerDevUnit = aShapedWord->AppUnitsPerDevUnit();
        double toNextGlyph;
        if (glyphStart < numGlyphs-1) {
            toNextGlyph = positions[glyphStart+1].x - positions[glyphStart].x;
        } else {
            toNextGlyph = positions[0].x + runWidth - positions[glyphStart].x;
        }
        int32_t advance = int32_t(toNextGlyph * appUnitsPerDevUnit);

        
        int32_t glyphsInClump = glyphEnd - glyphStart;
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(glyphs[glyphStart]) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            aShapedWord->IsClusterStart(baseCharIndex) &&
            positions[glyphStart].y == 0.0)
        {
            aShapedWord->SetSimpleGlyph(baseCharIndex,
                                        g.SetSimpleGlyph(advance,
                                                         glyphs[glyphStart]));
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
                advance = int32_t(toNextGlyph * appUnitsPerDevUnit);
            }

            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aShapedWord->IsClusterStart(baseCharIndex),
                         true, detailedGlyphs.Length());
            aShapedWord->SetGlyphs(baseCharIndex, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex && baseCharIndex < wordLength) {
            g.SetComplex(inOrder && aShapedWord->IsClusterStart(baseCharIndex),
                         false, 0);
            aShapedWord->SetGlyphs(baseCharIndex, g, nullptr);
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
                             ArrayLength(keys),
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
                             ArrayLength(keys),
                             &kCFTypeDictionaryKeyCallBacks,
                             &kCFTypeDictionaryValueCallBacks);
    ::CFRelease(lineFinalsOffSelector);
    ::CFRelease(swashesType);

    CFArrayRef featuresArray =
        ::CFArrayCreate(kCFAllocatorDefault,
                        (const void **) featureSettings,
                        ArrayLength(featureSettings),
                        &kCFTypeArrayCallBacks);
    ::CFRelease(featureSettings[0]);
    ::CFRelease(featureSettings[1]);

    const CFTypeRef attrKeys[]   = { kCTFontFeatureSettingsAttribute };
    const CFTypeRef attrValues[] = { featuresArray };
    CFDictionaryRef attributesDict =
        ::CFDictionaryCreate(kCFAllocatorDefault,
                             (const void **) attrKeys,
                             (const void **) attrValues,
                             ArrayLength(attrKeys),
                             &kCFTypeDictionaryKeyCallBacks,
                             &kCFTypeDictionaryValueCallBacks);
    ::CFRelease(featuresArray);

    sDefaultFeaturesDescriptor =
        ::CTFontDescriptorCreateWithAttributes(attributesDict);
    ::CFRelease(attributesDict);
}


CTFontRef
gfxCoreTextShaper::CreateCTFontWithDisabledLigatures(CGFloat aSize)
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
                                 ArrayLength(keys),
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
            ::CTFontDescriptorCreateCopyWithAttributes(GetDefaultFeaturesDescriptor(),
                                                       attributesDict);
        ::CFRelease(attributesDict);
    }

    gfxMacFont *f = static_cast<gfxMacFont*>(mFont);
    return ::CTFontCreateWithGraphicsFont(f->GetCGFontRef(), aSize, NULL,
                                          sDisableLigaturesDescriptor);
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
