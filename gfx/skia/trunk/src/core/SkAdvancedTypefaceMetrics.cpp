








#include "SkAdvancedTypefaceMetrics.h"
#include "SkTypes.h"

#if defined(SK_BUILD_FOR_WIN)
#include <dwrite.h>
#endif

#if defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_ANDROID)

struct FT_FaceRec;
typedef struct FT_FaceRec_* FT_Face;
#endif

#ifdef SK_BUILD_FOR_MAC
#import <ApplicationServices/ApplicationServices.h>
#endif

#ifdef SK_BUILD_FOR_IOS
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace skia_advanced_typeface_metrics_utils {

const int16_t kInvalidAdvance = SK_MinS16;
const int16_t kDontCareAdvance = SK_MinS16 + 1;

template <typename Data>
void stripUninterestingTrailingAdvancesFromRange(
                                                 SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range) {
    SkASSERT(false);
}

template <>
void stripUninterestingTrailingAdvancesFromRange<int16_t>(
                                                          SkAdvancedTypefaceMetrics::AdvanceMetric<int16_t>* range) {
    SkASSERT(range);

    int expectedAdvanceCount = range->fEndId - range->fStartId + 1;
    if (range->fAdvance.count() < expectedAdvanceCount) {
        return;
    }

    for (int i = expectedAdvanceCount - 1; i >= 0; --i) {
        if (range->fAdvance[i] != kDontCareAdvance &&
            range->fAdvance[i] != kInvalidAdvance &&
            range->fAdvance[i] != 0) {
            range->fEndId = range->fStartId + i;
            break;
        }
    }
}

template <typename Data>
void resetRange(SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range,
                int startId) {
    range->fStartId = startId;
    range->fAdvance.setCount(0);
}

template <typename Data>
SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* appendRange(
        SkAutoTDelete<SkAdvancedTypefaceMetrics::AdvanceMetric<Data> >* nextSlot,
        int startId) {
    nextSlot->reset(new SkAdvancedTypefaceMetrics::AdvanceMetric<Data>);
    resetRange(nextSlot->get(), startId);
    return nextSlot->get();
}

template <typename Data>
void zeroWildcardsInRange(
                          SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range) {
    SkASSERT(false);
}

template <>
void zeroWildcardsInRange<int16_t>(
                                   SkAdvancedTypefaceMetrics::AdvanceMetric<int16_t>* range) {
    SkASSERT(range);
    if (range->fType != SkAdvancedTypefaceMetrics::WidthRange::kRange) {
        return;
    }
    SkASSERT(range->fAdvance.count() == range->fEndId - range->fStartId + 1);

    
    for (int i = 0; i < range->fAdvance.count(); ++i) {
        if (range->fAdvance[i] == kDontCareAdvance) {
            range->fAdvance[i] = 0;
        }
    }
}

template <typename Data>
void finishRange(
        SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* range,
        int endId,
        typename SkAdvancedTypefaceMetrics::AdvanceMetric<Data>::MetricType
                type) {
    range->fEndId = endId;
    range->fType = type;
    stripUninterestingTrailingAdvancesFromRange(range);
    int newLength;
    if (type == SkAdvancedTypefaceMetrics::AdvanceMetric<Data>::kRange) {
        newLength = range->fEndId - range->fStartId + 1;
    } else {
        if (range->fEndId == range->fStartId) {
            range->fType =
                SkAdvancedTypefaceMetrics::AdvanceMetric<Data>::kRange;
        }
        newLength = 1;
    }
    SkASSERT(range->fAdvance.count() >= newLength);
    range->fAdvance.setCount(newLength);
    zeroWildcardsInRange(range);
}

template <typename Data, typename FontHandle>
SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* getAdvanceData(
        FontHandle fontHandle,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(FontHandle fontHandle, int gId, Data* data)) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    SkAutoTDelete<SkAdvancedTypefaceMetrics::AdvanceMetric<Data> > result;
    SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* curRange;
    SkAdvancedTypefaceMetrics::AdvanceMetric<Data>* prevRange = NULL;
    Data lastAdvance = kInvalidAdvance;
    int repeatedAdvances = 0;
    int wildCardsInRun = 0;
    int trailingWildCards = 0;
    uint32_t subsetIndex = 0;

    
    int firstIndex = 0;
    int lastIndex = num_glyphs;
    if (subsetGlyphIDs) {
        firstIndex = static_cast<int>(subsetGlyphIDs[0]);
        lastIndex =
                static_cast<int>(subsetGlyphIDs[subsetGlyphIDsLength - 1]) + 1;
    }
    curRange = appendRange(&result, firstIndex);

    for (int gId = firstIndex; gId <= lastIndex; gId++) {
        Data advance = kInvalidAdvance;
        if (gId < lastIndex) {
            
            SkASSERT(!subsetGlyphIDs || (subsetIndex < subsetGlyphIDsLength &&
                    static_cast<uint32_t>(gId) <= subsetGlyphIDs[subsetIndex]));
            if (!subsetGlyphIDs ||
                (subsetIndex < subsetGlyphIDsLength &&
                 static_cast<uint32_t>(gId) == subsetGlyphIDs[subsetIndex])) {
                SkAssertResult(getAdvance(fontHandle, gId, &advance));
                ++subsetIndex;
            } else {
                advance = kDontCareAdvance;
            }
        }
        if (advance == lastAdvance) {
            repeatedAdvances++;
            trailingWildCards = 0;
        } else if (advance == kDontCareAdvance) {
            wildCardsInRun++;
            trailingWildCards++;
        } else if (curRange->fAdvance.count() ==
                   repeatedAdvances + 1 + wildCardsInRun) {  
            if (lastAdvance == 0) {
                resetRange(curRange, gId);
                trailingWildCards = 0;
            } else if (repeatedAdvances + 1 >= 2 || trailingWildCards >= 4) {
                finishRange(curRange, gId - 1,
                            SkAdvancedTypefaceMetrics::WidthRange::kRun);
                prevRange = curRange;
                curRange = appendRange(&curRange->fNext, gId);
                trailingWildCards = 0;
            }
            repeatedAdvances = 0;
            wildCardsInRun = trailingWildCards;
            trailingWildCards = 0;
        } else {
            if (lastAdvance == 0 &&
                    repeatedAdvances + 1 + wildCardsInRun >= 4) {
                finishRange(curRange,
                            gId - repeatedAdvances - wildCardsInRun - 2,
                            SkAdvancedTypefaceMetrics::WidthRange::kRange);
                prevRange = curRange;
                curRange = appendRange(&curRange->fNext, gId);
                trailingWildCards = 0;
            } else if (trailingWildCards >= 4 && repeatedAdvances + 1 < 2) {
                finishRange(curRange,
                            gId - trailingWildCards - 1,
                            SkAdvancedTypefaceMetrics::WidthRange::kRange);
                prevRange = curRange;
                curRange = appendRange(&curRange->fNext, gId);
                trailingWildCards = 0;
            } else if (lastAdvance != 0 &&
                       (repeatedAdvances + 1 >= 3 ||
                        (repeatedAdvances + 1 >= 2 && wildCardsInRun >= 3))) {
                finishRange(curRange,
                            gId - repeatedAdvances - wildCardsInRun - 2,
                            SkAdvancedTypefaceMetrics::WidthRange::kRange);
                curRange =
                    appendRange(&curRange->fNext,
                                gId - repeatedAdvances - wildCardsInRun - 1);
                curRange->fAdvance.append(1, &lastAdvance);
                finishRange(curRange, gId - 1,
                            SkAdvancedTypefaceMetrics::WidthRange::kRun);
                prevRange = curRange;
                curRange = appendRange(&curRange->fNext, gId);
                trailingWildCards = 0;
            }
            repeatedAdvances = 0;
            wildCardsInRun = trailingWildCards;
            trailingWildCards = 0;
        }
        curRange->fAdvance.append(1, &advance);
        if (advance != kDontCareAdvance) {
            lastAdvance = advance;
        }
    }
    if (curRange->fStartId == lastIndex) {
        SkASSERT(prevRange);
        SkASSERT(prevRange->fNext->fStartId == lastIndex);
        prevRange->fNext.free();
    } else {
        finishRange(curRange, lastIndex - 1,
                    SkAdvancedTypefaceMetrics::WidthRange::kRange);
    }
    return result.detach();
}



#if defined(SK_BUILD_FOR_WIN)
template SkAdvancedTypefaceMetrics::WidthRange* getAdvanceData(
        HDC hdc,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(HDC hdc, int gId, int16_t* data));
template SkAdvancedTypefaceMetrics::WidthRange* getAdvanceData(
        IDWriteFontFace* fontFace,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(IDWriteFontFace* fontFace, int gId, int16_t* data));
#elif defined(SK_BUILD_FOR_UNIX) || defined(SK_BUILD_FOR_ANDROID)
template SkAdvancedTypefaceMetrics::WidthRange* getAdvanceData(
        FT_Face face,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(FT_Face face, int gId, int16_t* data));
#elif defined(SK_BUILD_FOR_MAC) || defined(SK_BUILD_FOR_IOS)
template SkAdvancedTypefaceMetrics::WidthRange* getAdvanceData(
        CTFontRef ctFont,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(CTFontRef ctFont, int gId, int16_t* data));
#endif
template void resetRange(
        SkAdvancedTypefaceMetrics::WidthRange* range,
        int startId);
template SkAdvancedTypefaceMetrics::WidthRange* appendRange(
        SkAutoTDelete<SkAdvancedTypefaceMetrics::WidthRange >* nextSlot,
        int startId);
template void finishRange<int16_t>(
        SkAdvancedTypefaceMetrics::WidthRange* range,
        int endId,
        SkAdvancedTypefaceMetrics::WidthRange::MetricType type);

template void resetRange(
        SkAdvancedTypefaceMetrics::VerticalAdvanceRange* range,
        int startId);
template SkAdvancedTypefaceMetrics::VerticalAdvanceRange* appendRange(
        SkAutoTDelete<SkAdvancedTypefaceMetrics::VerticalAdvanceRange >*
            nextSlot,
        int startId);
template void finishRange<SkAdvancedTypefaceMetrics::VerticalMetric>(
        SkAdvancedTypefaceMetrics::VerticalAdvanceRange* range,
        int endId,
        SkAdvancedTypefaceMetrics::VerticalAdvanceRange::MetricType type);


template SkAdvancedTypefaceMetrics::WidthRange* getAdvanceData(
        void* fontData,
        int num_glyphs,
        const uint32_t* subsetGlyphIDs,
        uint32_t subsetGlyphIDsLength,
        bool (*getAdvance)(void* fontData, int gId, int16_t* data));

} 
