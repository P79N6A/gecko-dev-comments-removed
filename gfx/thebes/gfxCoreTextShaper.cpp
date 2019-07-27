




#include "mozilla/ArrayUtils.h"
#include "gfxCoreTextShaper.h"
#include "gfxMacFont.h"
#include "gfxFontUtils.h"
#include "gfxTextRun.h"
#include "mozilla/gfx/2D.h"

#include <algorithm>

using namespace mozilla;


CTFontDescriptorRef gfxCoreTextShaper::sDefaultFeaturesDescriptor = nullptr;
CTFontDescriptorRef gfxCoreTextShaper::sDisableLigaturesDescriptor = nullptr;
CTFontDescriptorRef gfxCoreTextShaper::sIndicFeaturesDescriptor = nullptr;
CTFontDescriptorRef gfxCoreTextShaper::sIndicDisableLigaturesDescriptor = nullptr;

gfxCoreTextShaper::gfxCoreTextShaper(gfxMacFont *aFont)
    : gfxFontShaper(aFont)
{
    
    mCTFont = CreateCTFontWithFeatures(aFont->GetAdjustedSize(),
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

static bool
IsBuggyIndicScript(int32_t aScript)
{
    return aScript == MOZ_SCRIPT_BENGALI || aScript == MOZ_SCRIPT_KANNADA;
}

bool
gfxCoreTextShaper::ShapeText(gfxContext      *aContext,
                             const char16_t *aText,
                             uint32_t         aOffset,
                             uint32_t         aLength,
                             int32_t          aScript,
                             bool             aVertical,
                             gfxShapedText   *aShapedText)
{
    

    bool isRightToLeft = aShapedText->IsRightToLeft();
    uint32_t length = aLength;

    
    
    bool bidiWrap = isRightToLeft;
    if (!bidiWrap && !aShapedText->TextIs8Bit()) {
        uint32_t i;
        for (i = 0; i < length; ++i) {
            if (gfxFontUtils::PotentialRTLChar(aText[i])) {
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
        ::CFStringAppendCharacters(mutableString, reinterpret_cast<const UniChar*>(aText), length);
        ::CFStringAppendCharacters(mutableString,
                                   endBidiWrap, mozilla::ArrayLength(endBidiWrap));
        stringObj = mutableString;
    } else {
        startOffset = 0;
        stringObj = ::CFStringCreateWithCharactersNoCopy(kCFAllocatorDefault,
                                                         reinterpret_cast<const UniChar*>(aText),
                                                         length, kCFAllocatorNull);
    }

    CFDictionaryRef attrObj;
    CTFontRef tempCTFont = nullptr;

    if (IsBuggyIndicScript(aScript)) {
        
        
        
        
        
        tempCTFont =
            CreateCTFontWithFeatures(::CTFontGetSize(mCTFont),
                                     aShapedText->DisableLigatures()
                                         ? GetIndicDisableLigaturesDescriptor()
                                         : GetIndicFeaturesDescriptor());
    } else if (aShapedText->DisableLigatures()) {
        
        
        tempCTFont =
            CreateCTFontWithFeatures(::CTFontGetSize(mCTFont),
                                     GetDisableLigaturesDescriptor());
    }

    if (tempCTFont) {
        attrObj =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void**) &kCTFontAttributeName,
                                 (const void**) &tempCTFont,
                                 1, 
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);
        
        
        ::CFRelease(tempCTFont);
    } else {
        
        attrObj = mAttributesDict;
        ::CFRetain(attrObj);
    }

    
    CFAttributedStringRef attrStringObj =
        ::CFAttributedStringCreate(kCFAllocatorDefault, stringObj, attrObj);
    ::CFRelease(stringObj);

    
    CTLineRef line = ::CTLineCreateWithAttributedString(attrStringObj);
    ::CFRelease(attrStringObj);

    
    CFArrayRef glyphRuns = ::CTLineGetGlyphRuns(line);
    uint32_t numRuns = ::CFArrayGetCount(glyphRuns);

    
    
    
    bool success = true;
    for (uint32_t runIndex = 0; runIndex < numRuns; runIndex++) {
        CTRunRef aCTRun =
            (CTRunRef)::CFArrayGetValueAtIndex(glyphRuns, runIndex);
        
        CFRange range = ::CTRunGetStringRange(aCTRun);
        if (range.location + range.length <= startOffset ||
            range.location - startOffset >= aLength) {
            continue;
        }
        CFDictionaryRef runAttr = ::CTRunGetAttributes(aCTRun);
        if (runAttr != attrObj) {
            
            
            
            const void* font1 = ::CFDictionaryGetValue(attrObj, kCTFontAttributeName);
            const void* font2 = ::CFDictionaryGetValue(runAttr, kCTFontAttributeName);
            if (font1 != font2) {
                
                
                if (range.length == 1 &&
                    gfxFontUtils::IsVarSelector(aText[range.location -
                                                      startOffset])) {
                    continue;
                }
                NS_WARNING("unexpected font fallback in Core Text");
                success = false;
                break;
            }
        }
        if (SetGlyphsFromRun(aShapedText, aOffset, aLength, aCTRun, startOffset) != NS_OK) {
            success = false;
            break;
        }
    }

    ::CFRelease(attrObj);
    ::CFRelease(line);

    return success;
}

#define SMALL_GLYPH_RUN 128 // preallocated size of our auto arrays for per-glyph data;
                            
                            

nsresult
gfxCoreTextShaper::SetGlyphsFromRun(gfxShapedText *aShapedText,
                                    uint32_t       aOffset,
                                    uint32_t       aLength,
                                    CTRunRef       aCTRun,
                                    int32_t        aStringOffset)
{
    
    
    

    int32_t direction = aShapedText->IsRightToLeft() ? -1 : 1;

    int32_t numGlyphs = ::CTRunGetGlyphCount(aCTRun);
    if (numGlyphs == 0) {
        return NS_OK;
    }

    int32_t wordLength = aLength;

    
    
    
    
    
    
    

    
    CFRange stringRange = ::CTRunGetStringRange(aCTRun);
    
    if (stringRange.location - aStringOffset + stringRange.length <= 0 ||
        stringRange.location - aStringOffset >= wordLength) {
        return NS_OK;
    }

    
    nsAutoArrayPtr<CGGlyph> glyphsArray;
    nsAutoArrayPtr<CGPoint> positionsArray;
    nsAutoArrayPtr<CFIndex> glyphToCharArray;
    const CGGlyph* glyphs = nullptr;
    const CGPoint* positions = nullptr;
    const CFIndex* glyphToChar = nullptr;

    
    
    
    
    
    
    
    
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
                                                  nullptr, nullptr, nullptr);

    nsAutoTArray<gfxShapedText::DetailedGlyph,1> detailedGlyphs;
    gfxShapedText::CompressedGlyph g;
    gfxShapedText::CompressedGlyph *charGlyphs =
        aShapedText->GetCharacterGlyphs() + aOffset;

    
    
    
    
    
    
    

    

    static const int32_t NO_GLYPH = -1;
    AutoFallibleTArray<int32_t,SMALL_GLYPH_RUN> charToGlyphArray;
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

    
    
    
    
    
    
    
    
    
    
    
    

    
    

    bool isRightToLeft = aShapedText->IsRightToLeft();
    int32_t glyphStart = 0; 
    int32_t charStart = isRightToLeft ?
        stringRange.length - 1 : 0; 

    while (glyphStart < numGlyphs) { 
        bool inOrder = true;
        int32_t charEnd = glyphToChar[glyphStart] - stringRange.location;
        NS_WARN_IF_FALSE(charEnd >= 0 && charEnd < stringRange.length,
                         "glyph-to-char mapping points outside string range");
        
        charEnd = std::max(charEnd, 0);
        charEnd = std::min(charEnd, int32_t(stringRange.length));

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
                        
                        glyphEnd = std::max(glyphEnd, charToGlyph[i] + 1);
                    }
                }
            } else {
                for (int32_t i = charStart; i < charEnd; ++i) {
                    if (charToGlyph[i] != NO_GLYPH) {
                        
                        glyphEnd = std::max(glyphEnd, charToGlyph[i] + 1);
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
        
        baseCharIndex = std::max(baseCharIndex, 0);
        endCharIndex = std::min(endCharIndex, wordLength);

        
        
        int32_t appUnitsPerDevUnit = aShapedText->GetAppUnitsPerDevUnit();
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
            charGlyphs[baseCharIndex].IsClusterStart() &&
            positions[glyphStart].y == 0.0)
        {
            charGlyphs[baseCharIndex].SetSimpleGlyph(advance,
                                                     glyphs[glyphStart]);
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
            g.SetComplex(charGlyphs[baseCharIndex].IsClusterStart(),
                         true, detailedGlyphs.Length());
            aShapedText->SetGlyphs(aOffset + baseCharIndex, g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        while (++baseCharIndex != endCharIndex && baseCharIndex < wordLength) {
            gfxShapedText::CompressedGlyph &g = charGlyphs[baseCharIndex];
            NS_ASSERTION(!g.IsSimpleGlyph(), "overwriting a simple glyph");
            g.SetComplex(inOrder && g.IsClusterStart(), false, 0);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return NS_OK;
}

#undef SMALL_GLYPH_RUN








#define MAX_FEATURES  3 // max used by any of our Get*Descriptor functions

CTFontDescriptorRef
gfxCoreTextShaper::CreateFontFeaturesDescriptor(
    const std::pair<SInt16,SInt16> aFeatures[],
    size_t aCount)
{
    MOZ_ASSERT(aCount <= MAX_FEATURES);

    CFDictionaryRef featureSettings[MAX_FEATURES];

    for (size_t i = 0; i < aCount; i++) {
        CFNumberRef type = ::CFNumberCreate(kCFAllocatorDefault,
                                            kCFNumberSInt16Type,
                                            &aFeatures[i].first);
        CFNumberRef selector = ::CFNumberCreate(kCFAllocatorDefault,
                                                kCFNumberSInt16Type,
                                                &aFeatures[i].second);

        CFTypeRef keys[]   = { kCTFontFeatureTypeIdentifierKey,
                               kCTFontFeatureSelectorIdentifierKey };
        CFTypeRef values[] = { type, selector };
        featureSettings[i] =
            ::CFDictionaryCreate(kCFAllocatorDefault,
                                 (const void **) keys,
                                 (const void **) values,
                                 ArrayLength(keys),
                                 &kCFTypeDictionaryKeyCallBacks,
                                 &kCFTypeDictionaryValueCallBacks);

        ::CFRelease(selector);
        ::CFRelease(type);
    }

    CFArrayRef featuresArray =
        ::CFArrayCreate(kCFAllocatorDefault,
                        (const void **) featureSettings,
                        aCount, 
                                
                        &kCFTypeArrayCallBacks);

    for (size_t i = 0; i < aCount; i++) {
        ::CFRelease(featureSettings[i]);
    }

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

    CTFontDescriptorRef descriptor =
        ::CTFontDescriptorCreateWithAttributes(attributesDict);
    ::CFRelease(attributesDict);

    return descriptor;
}

CTFontDescriptorRef
gfxCoreTextShaper::GetDefaultFeaturesDescriptor()
{
    if (sDefaultFeaturesDescriptor == nullptr) {
        const std::pair<SInt16,SInt16> kDefaultFeatures[] = {
            { kSmartSwashType, kLineInitialSwashesOffSelector },
            { kSmartSwashType, kLineFinalSwashesOffSelector }
        };
        sDefaultFeaturesDescriptor =
            CreateFontFeaturesDescriptor(kDefaultFeatures,
                                         ArrayLength(kDefaultFeatures));
    }
    return sDefaultFeaturesDescriptor;
}

CTFontDescriptorRef
gfxCoreTextShaper::GetDisableLigaturesDescriptor()
{
    if (sDisableLigaturesDescriptor == nullptr) {
        const std::pair<SInt16,SInt16> kDisableLigatures[] = {
            { kSmartSwashType, kLineInitialSwashesOffSelector },
            { kSmartSwashType, kLineFinalSwashesOffSelector },
            { kLigaturesType, kCommonLigaturesOffSelector }
        };
        sDisableLigaturesDescriptor =
            CreateFontFeaturesDescriptor(kDisableLigatures,
                                         ArrayLength(kDisableLigatures));
    }
    return sDisableLigaturesDescriptor;
}

CTFontDescriptorRef
gfxCoreTextShaper::GetIndicFeaturesDescriptor()
{
    if (sIndicFeaturesDescriptor == nullptr) {
        const std::pair<SInt16,SInt16> kIndicFeatures[] = {
            { kSmartSwashType, kLineFinalSwashesOffSelector }
        };
        sIndicFeaturesDescriptor =
            CreateFontFeaturesDescriptor(kIndicFeatures,
                                         ArrayLength(kIndicFeatures));
    }
    return sIndicFeaturesDescriptor;
}

CTFontDescriptorRef
gfxCoreTextShaper::GetIndicDisableLigaturesDescriptor()
{
    if (sIndicDisableLigaturesDescriptor == nullptr) {
        const std::pair<SInt16,SInt16> kIndicDisableLigatures[] = {
            { kSmartSwashType, kLineFinalSwashesOffSelector },
            { kLigaturesType, kCommonLigaturesOffSelector }
        };
        sIndicDisableLigaturesDescriptor =
            CreateFontFeaturesDescriptor(kIndicDisableLigatures,
                                         ArrayLength(kIndicDisableLigatures));
    }
    return sIndicDisableLigaturesDescriptor;
}

CTFontRef
gfxCoreTextShaper::CreateCTFontWithFeatures(CGFloat aSize,
                                            CTFontDescriptorRef aDescriptor)
{
    gfxMacFont *f = static_cast<gfxMacFont*>(mFont);
    return ::CTFontCreateWithGraphicsFont(f->GetCGFontRef(), aSize, nullptr,
                                          aDescriptor);
}

void
gfxCoreTextShaper::Shutdown() 
{
    if (sIndicDisableLigaturesDescriptor != nullptr) {
        ::CFRelease(sIndicDisableLigaturesDescriptor);
        sIndicDisableLigaturesDescriptor = nullptr;
    }
    if (sIndicFeaturesDescriptor != nullptr) {
        ::CFRelease(sIndicFeaturesDescriptor);
        sIndicFeaturesDescriptor = nullptr;
    }
    if (sDisableLigaturesDescriptor != nullptr) {
        ::CFRelease(sDisableLigaturesDescriptor);
        sDisableLigaturesDescriptor = nullptr;
    }
    if (sDefaultFeaturesDescriptor != nullptr) {
        ::CFRelease(sDefaultFeaturesDescriptor);
        sDefaultFeaturesDescriptor = nullptr;
    }
}
