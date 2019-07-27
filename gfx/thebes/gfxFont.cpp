




#include "gfxFont.h"

#include "mozilla/BinarySearch.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/MathAlgorithms.h"

#include "prlog.h"

#include "nsExpirationTracker.h"
#include "nsITimer.h"

#include "gfxGlyphExtents.h"
#include "gfxPlatform.h"
#include "gfxTextRun.h"
#include "nsGkAtoms.h"

#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxFontMissingGlyphs.h"
#include "gfxGraphiteShaper.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxUserFontSet.h"
#include "nsSpecialCasingData.h"
#include "nsTextRunTransformations.h"
#include "nsUnicodeProperties.h"
#include "nsStyleConsts.h"
#include "mozilla/AppUnits.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "gfxSVGGlyphs.h"
#include "gfx2DGlue.h"

#include "GreekCasing.h"

#include "cairo.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ot.h"
#include "graphite2/Font.h"

#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;
using mozilla::services::GetObserverService;

gfxFontCache *gfxFontCache::gGlobalCache = nullptr;

#ifdef DEBUG_roc
#define DEBUG_TEXT_RUN_STORAGE_METRICS
#endif

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
uint32_t gTextRunStorageHighWaterMark = 0;
uint32_t gTextRunStorage = 0;
uint32_t gFontCount = 0;
uint32_t gGlyphExtentsCount = 0;
uint32_t gGlyphExtentsWidthsTotalSize = 0;
uint32_t gGlyphExtentsSetupEagerSimple = 0;
uint32_t gGlyphExtentsSetupEagerTight = 0;
uint32_t gGlyphExtentsSetupLazyTight = 0;
uint32_t gGlyphExtentsSetupFallBackToTight = 0;
#endif

#ifdef PR_LOGGING
#define LOG_FONTINIT(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontinit), \
                                  PR_LOG_DEBUG, args)
#define LOG_FONTINIT_ENABLED() PR_LOG_TEST( \
                                        gfxPlatform::GetLog(eGfxLog_fontinit), \
                                        PR_LOG_DEBUG)
#endif 










MOZ_DEFINE_MALLOC_SIZE_OF(FontCacheMallocSizeOf)

NS_IMPL_ISUPPORTS(gfxFontCache::MemoryReporter, nsIMemoryReporter)

NS_IMETHODIMP
gfxFontCache::MemoryReporter::CollectReports(
    nsIMemoryReporterCallback* aCb, nsISupports* aClosure, bool aAnonymize)
{
    FontCacheSizes sizes;

    gfxFontCache::GetCache()->AddSizeOfIncludingThis(&FontCacheMallocSizeOf,
                                                     &sizes);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-cache"),
                  KIND_HEAP, UNITS_BYTES, sizes.mFontInstances,
                  NS_LITERAL_CSTRING("Memory used for active font instances."),
                  aClosure);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-shaped-words"),
                  KIND_HEAP, UNITS_BYTES, sizes.mShapedWords,
                  NS_LITERAL_CSTRING("Memory used to cache shaped glyph data."),
                  aClosure);

    return NS_OK;
}

NS_IMPL_ISUPPORTS(gfxFontCache::Observer, nsIObserver)

NS_IMETHODIMP
gfxFontCache::Observer::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const char16_t *someData)
{
    if (!nsCRT::strcmp(aTopic, "memory-pressure")) {
        gfxFontCache *fontCache = gfxFontCache::GetCache();
        if (fontCache) {
            fontCache->FlushShapedWordCaches();
        }
    } else {
        NS_NOTREACHED("unexpected notification topic");
    }
    return NS_OK;
}

nsresult
gfxFontCache::Init()
{
    NS_ASSERTION(!gGlobalCache, "Where did this come from?");
    gGlobalCache = new gfxFontCache();
    if (!gGlobalCache) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    RegisterStrongMemoryReporter(new MemoryReporter());
    return NS_OK;
}

void
gfxFontCache::Shutdown()
{
    delete gGlobalCache;
    gGlobalCache = nullptr;

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    printf("Textrun storage high water mark=%d\n", gTextRunStorageHighWaterMark);
    printf("Total number of fonts=%d\n", gFontCount);
    printf("Total glyph extents allocated=%d (size %d)\n", gGlyphExtentsCount,
            int(gGlyphExtentsCount*sizeof(gfxGlyphExtents)));
    printf("Total glyph extents width-storage size allocated=%d\n", gGlyphExtentsWidthsTotalSize);
    printf("Number of simple glyph extents eagerly requested=%d\n", gGlyphExtentsSetupEagerSimple);
    printf("Number of tight glyph extents eagerly requested=%d\n", gGlyphExtentsSetupEagerTight);
    printf("Number of tight glyph extents lazily requested=%d\n", gGlyphExtentsSetupLazyTight);
    printf("Number of simple glyph extent setups that fell back to tight=%d\n", gGlyphExtentsSetupFallBackToTight);
#endif
}

gfxFontCache::gfxFontCache()
    : nsExpirationTracker<gfxFont,3>(FONT_TIMEOUT_SECONDS * 1000)
{
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs) {
        obs->AddObserver(new Observer, "memory-pressure", false);
    }

#ifndef RELEASE_BUILD
    
    
    mWordCacheExpirationTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mWordCacheExpirationTimer) {
        mWordCacheExpirationTimer->
            InitWithFuncCallback(WordCacheExpirationTimerCallback, this,
                                 SHAPED_WORD_TIMEOUT_SECONDS * 1000,
                                 nsITimer::TYPE_REPEATING_SLACK);
    }
#endif
}

gfxFontCache::~gfxFontCache()
{
    
    
    
    gfxUserFontSet::UserFontCache::Shutdown();

    if (mWordCacheExpirationTimer) {
        mWordCacheExpirationTimer->Cancel();
        mWordCacheExpirationTimer = nullptr;
    }

    
    AgeAllGenerations();
    
    NS_WARN_IF_FALSE(mFonts.Count() == 0,
                     "Fonts still alive while shutting down gfxFontCache");
    
    
    
}

bool
gfxFontCache::HashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    const gfxCharacterMap* fontUnicodeRangeMap = mFont->GetUnicodeRangeMap();
    return aKey->mFontEntry == mFont->GetFontEntry() &&
           aKey->mStyle->Equals(*mFont->GetStyle()) &&
           ((!aKey->mUnicodeRangeMap && !fontUnicodeRangeMap) ||
            (aKey->mUnicodeRangeMap && fontUnicodeRangeMap &&
             aKey->mUnicodeRangeMap->Equals(fontUnicodeRangeMap)));
}

already_AddRefed<gfxFont>
gfxFontCache::Lookup(const gfxFontEntry* aFontEntry,
                     const gfxFontStyle* aStyle,
                     const gfxCharacterMap* aUnicodeRangeMap)
{
    Key key(aFontEntry, aStyle, aUnicodeRangeMap);
    HashEntry *entry = mFonts.GetEntry(key);

    Telemetry::Accumulate(Telemetry::FONT_CACHE_HIT, entry != nullptr);
    if (!entry)
        return nullptr;

    nsRefPtr<gfxFont> font = entry->mFont;
    return font.forget();
}

void
gfxFontCache::AddNew(gfxFont *aFont)
{
    Key key(aFont->GetFontEntry(), aFont->GetStyle(),
            aFont->GetUnicodeRangeMap());
    HashEntry *entry = mFonts.PutEntry(key);
    if (!entry)
        return;
    gfxFont *oldFont = entry->mFont;
    entry->mFont = aFont;
    
    
    MOZ_ASSERT(entry == mFonts.GetEntry(key));
    
    
    if (oldFont && oldFont->GetExpirationState()->IsTracked()) {
        
        
        NS_ASSERTION(aFont != oldFont, "new font is tracked for expiry!");
        NotifyExpired(oldFont);
    }
}

void
gfxFontCache::NotifyReleased(gfxFont *aFont)
{
    nsresult rv = AddObject(aFont);
    if (NS_FAILED(rv)) {
        
        DestroyFont(aFont);
    }
    
    
    
    
    
}

void
gfxFontCache::NotifyExpired(gfxFont *aFont)
{
    aFont->ClearCachedWords();
    RemoveObject(aFont);
    DestroyFont(aFont);
}

void
gfxFontCache::DestroyFont(gfxFont *aFont)
{
    Key key(aFont->GetFontEntry(), aFont->GetStyle(),
            aFont->GetUnicodeRangeMap());
    HashEntry *entry = mFonts.GetEntry(key);
    if (entry && entry->mFont == aFont) {
        mFonts.RemoveEntry(key);
    }
    NS_ASSERTION(aFont->GetRefCount() == 0,
                 "Destroying with non-zero ref count!");
    delete aFont;
}


PLDHashOperator
gfxFontCache::AgeCachedWordsForFont(HashEntry* aHashEntry, void* aUserData)
{
    aHashEntry->mFont->AgeCachedWords();
    return PL_DHASH_NEXT;
}


void
gfxFontCache::WordCacheExpirationTimerCallback(nsITimer* aTimer, void* aCache)
{
    gfxFontCache* cache = static_cast<gfxFontCache*>(aCache);
    cache->mFonts.EnumerateEntries(AgeCachedWordsForFont, nullptr);
}


PLDHashOperator
gfxFontCache::ClearCachedWordsForFont(HashEntry* aHashEntry, void* aUserData)
{
    aHashEntry->mFont->ClearCachedWords();
    return PL_DHASH_NEXT;
}


size_t
gfxFontCache::AddSizeOfFontEntryExcludingThis(HashEntry* aHashEntry,
                                              MallocSizeOf aMallocSizeOf,
                                              void* aUserArg)
{
    HashEntry *entry = static_cast<HashEntry*>(aHashEntry);
    FontCacheSizes *sizes = static_cast<FontCacheSizes*>(aUserArg);
    entry->mFont->AddSizeOfExcludingThis(aMallocSizeOf, sizes);

    
    
    return 0;
}

void
gfxFontCache::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                     FontCacheSizes* aSizes) const
{
    

    aSizes->mFontInstances +=
        mFonts.SizeOfExcludingThis(AddSizeOfFontEntryExcludingThis,
                                   aMallocSizeOf, aSizes);
}

void
gfxFontCache::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                     FontCacheSizes* aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}

#define MAX_SSXX_VALUE 99
#define MAX_CVXX_VALUE 99

static void
LookupAlternateValues(gfxFontFeatureValueSet *featureLookup,
                      const nsAString& aFamily,
                      const nsTArray<gfxAlternateValue>& altValue,
                      nsTArray<gfxFontFeature>& aFontFeatures)
{
    uint32_t numAlternates = altValue.Length();
    for (uint32_t i = 0; i < numAlternates; i++) {
        const gfxAlternateValue& av = altValue.ElementAt(i);
        nsAutoTArray<uint32_t,4> values;

        
        bool found =
            featureLookup->GetFontFeatureValuesFor(aFamily, av.alternate,
                                                   av.value, values);
        uint32_t numValues = values.Length();

        
        if (!found || numValues == 0) {
            continue;
        }

        gfxFontFeature feature;
        if (av.alternate == NS_FONT_VARIANT_ALTERNATES_CHARACTER_VARIANT) {
            NS_ASSERTION(numValues <= 2,
                         "too many values allowed for character-variant");
            
            uint32_t nn = values.ElementAt(0);
            
            if (nn == 0 || nn > MAX_CVXX_VALUE) {
                continue;
            }
            feature.mValue = 1;
            if (numValues > 1) {
                feature.mValue = values.ElementAt(1);
            }
            feature.mTag = HB_TAG('c','v',('0' + nn / 10), ('0' + nn % 10));
            aFontFeatures.AppendElement(feature);

        } else if (av.alternate == NS_FONT_VARIANT_ALTERNATES_STYLESET) {
            
            feature.mValue = 1;
            for (uint32_t v = 0; v < numValues; v++) {
                uint32_t nn = values.ElementAt(v);
                if (nn == 0 || nn > MAX_SSXX_VALUE) {
                    continue;
                }
                feature.mTag = HB_TAG('s','s',('0' + nn / 10), ('0' + nn % 10));
                aFontFeatures.AppendElement(feature);
            }

        } else {
            NS_ASSERTION(numValues == 1,
                   "too many values for font-specific font-variant-alternates");
            feature.mValue = values.ElementAt(0);

            switch (av.alternate) {
                case NS_FONT_VARIANT_ALTERNATES_STYLISTIC:  
                    feature.mTag = HB_TAG('s','a','l','t');
                    break;
                case NS_FONT_VARIANT_ALTERNATES_SWASH:  
                    feature.mTag = HB_TAG('s','w','s','h');
                    aFontFeatures.AppendElement(feature);
                    feature.mTag = HB_TAG('c','s','w','h');
                    break;
                case NS_FONT_VARIANT_ALTERNATES_ORNAMENTS: 
                    feature.mTag = HB_TAG('o','r','n','m');
                    break;
                case NS_FONT_VARIANT_ALTERNATES_ANNOTATION: 
                    feature.mTag = HB_TAG('n','a','l','t');
                    break;
                default:
                    feature.mTag = 0;
                    break;
            }

            NS_ASSERTION(feature.mTag, "unsupported alternate type");
            if (!feature.mTag) {
                continue;
            }
            aFontFeatures.AppendElement(feature);
        }
    }
}

 void
gfxFontShaper::MergeFontFeatures(
    const gfxFontStyle *aStyle,
    const nsTArray<gfxFontFeature>& aFontFeatures,
    bool aDisableLigatures,
    const nsAString& aFamilyName,
    bool aAddSmallCaps,
    PLDHashOperator (*aHandleFeature)(const uint32_t&, uint32_t&, void*),
    void* aHandleFeatureData)
{
    uint32_t numAlts = aStyle->alternateValues.Length();
    const nsTArray<gfxFontFeature>& styleRuleFeatures =
        aStyle->featureSettings;

    
    if (styleRuleFeatures.IsEmpty() &&
        aFontFeatures.IsEmpty() &&
        !aDisableLigatures &&
        aStyle->variantCaps == NS_FONT_VARIANT_CAPS_NORMAL &&
        aStyle->variantSubSuper == NS_FONT_VARIANT_POSITION_NORMAL &&
        numAlts == 0) {
        return;
    }

    nsDataHashtable<nsUint32HashKey,uint32_t> mergedFeatures;

    
    
    if (aDisableLigatures) {
        mergedFeatures.Put(HB_TAG('l','i','g','a'), 0);
        mergedFeatures.Put(HB_TAG('c','l','i','g'), 0);
    }

    
    uint32_t i, count;

    count = aFontFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = aFontFeatures.ElementAt(i);
        mergedFeatures.Put(feature.mTag, feature.mValue);
    }

    
    
    uint32_t variantCaps = aStyle->variantCaps;
    switch (variantCaps) {
        case NS_FONT_VARIANT_CAPS_ALLSMALL:
            mergedFeatures.Put(HB_TAG('c','2','s','c'), 1);
            
        case NS_FONT_VARIANT_CAPS_SMALLCAPS:
            mergedFeatures.Put(HB_TAG('s','m','c','p'), 1);
            break;

        case NS_FONT_VARIANT_CAPS_ALLPETITE:
            mergedFeatures.Put(aAddSmallCaps ? HB_TAG('c','2','s','c') :
                                               HB_TAG('c','2','p','c'), 1);
        
        case NS_FONT_VARIANT_CAPS_PETITECAPS:
            mergedFeatures.Put(aAddSmallCaps ? HB_TAG('s','m','c','p') :
                                               HB_TAG('p','c','a','p'), 1);
        break;

        case NS_FONT_VARIANT_CAPS_TITLING:
            mergedFeatures.Put(HB_TAG('t','i','t','l'), 1);
            break;

        case NS_FONT_VARIANT_CAPS_UNICASE:
            mergedFeatures.Put(HB_TAG('u','n','i','c'), 1);
            break;

        default:
            break;
    }

    
    switch (aStyle->variantSubSuper) {
        case NS_FONT_VARIANT_POSITION_SUPER:
            mergedFeatures.Put(HB_TAG('s','u','p','s'), 1);
            break;
        case NS_FONT_VARIANT_POSITION_SUB:
            mergedFeatures.Put(HB_TAG('s','u','b','s'), 1);
            break;
        default:
            break;
    }

    
    if (aStyle->featureValueLookup && numAlts > 0) {
        nsAutoTArray<gfxFontFeature,4> featureList;

        
        LookupAlternateValues(aStyle->featureValueLookup, aFamilyName,
                              aStyle->alternateValues, featureList);

        count = featureList.Length();
        for (i = 0; i < count; i++) {
            const gfxFontFeature& feature = featureList.ElementAt(i);
            mergedFeatures.Put(feature.mTag, feature.mValue);
        }
    }

    
    count = styleRuleFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = styleRuleFeatures.ElementAt(i);
        mergedFeatures.Put(feature.mTag, feature.mValue);
    }

    if (mergedFeatures.Count() != 0) {
        mergedFeatures.Enumerate(aHandleFeature, aHandleFeatureData);
    }
}

void
gfxShapedText::SetupClusterBoundaries(uint32_t         aOffset,
                                      const char16_t *aString,
                                      uint32_t         aLength)
{
    CompressedGlyph *glyphs = GetCharacterGlyphs() + aOffset;

    gfxTextRun::CompressedGlyph extendCluster;
    extendCluster.SetComplex(false, true, 0);

    ClusterIterator iter(aString, aLength);

    
    
    if (aLength && IsClusterExtender(*aString)) {
        *glyphs = extendCluster;
    }

    while (!iter.AtEnd()) {
        if (*iter == char16_t(' ')) {
            glyphs->SetIsSpace();
        }
        
        iter.Next();
        
        aString++;
        glyphs++;
        
        while (aString < iter) {
            *glyphs = extendCluster;
            if (NS_IS_LOW_SURROGATE(*aString)) {
                glyphs->SetIsLowSurrogate();
            }
            glyphs++;
            aString++;
        }
    }
}

void
gfxShapedText::SetupClusterBoundaries(uint32_t       aOffset,
                                      const uint8_t *aString,
                                      uint32_t       aLength)
{
    CompressedGlyph *glyphs = GetCharacterGlyphs() + aOffset;
    const uint8_t *limit = aString + aLength;

    while (aString < limit) {
        if (*aString == uint8_t(' ')) {
            glyphs->SetIsSpace();
        }
        aString++;
        glyphs++;
    }
}

gfxShapedText::DetailedGlyph *
gfxShapedText::AllocateDetailedGlyphs(uint32_t aIndex, uint32_t aCount)
{
    NS_ASSERTION(aIndex < GetLength(), "Index out of range");

    if (!mDetailedGlyphs) {
        mDetailedGlyphs = new DetailedGlyphStore();
    }

    return mDetailedGlyphs->Allocate(aIndex, aCount);
}

void
gfxShapedText::SetGlyphs(uint32_t aIndex, CompressedGlyph aGlyph,
                         const DetailedGlyph *aGlyphs)
{
    NS_ASSERTION(!aGlyph.IsSimpleGlyph(), "Simple glyphs not handled here");
    NS_ASSERTION(aIndex > 0 || aGlyph.IsLigatureGroupStart(),
                 "First character can't be a ligature continuation!");

    uint32_t glyphCount = aGlyph.GetGlyphCount();
    if (glyphCount > 0) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, glyphCount);
        memcpy(details, aGlyphs, sizeof(DetailedGlyph)*glyphCount);
    }
    GetCharacterGlyphs()[aIndex] = aGlyph;
}

#define ZWNJ 0x200C
#define ZWJ  0x200D


#define ALM  0x061C
static inline bool
IsDefaultIgnorable(uint32_t aChar)
{
    return GetIdentifierModification(aChar) == XIDMOD_DEFAULT_IGNORABLE ||
           aChar == ZWNJ || aChar == ZWJ || aChar == ALM;
}

void
gfxShapedText::SetMissingGlyph(uint32_t aIndex, uint32_t aChar, gfxFont *aFont)
{
    uint8_t category = GetGeneralCategory(aChar);
    if (category >= HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK &&
        category <= HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)
    {
        GetCharacterGlyphs()[aIndex].SetComplex(false, true, 0);
    }

    DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);

    details->mGlyphID = aChar;
    if (IsDefaultIgnorable(aChar)) {
        
        details->mAdvance = 0;
    } else {
        gfxFloat width =
            std::max(aFont->GetMetrics(gfxFont::eHorizontal).aveCharWidth,
                     gfxFloat(gfxFontMissingGlyphs::GetDesiredMinWidth(aChar,
                                mAppUnitsPerDevUnit)));
        details->mAdvance = uint32_t(width * mAppUnitsPerDevUnit);
    }
    details->mXOffset = 0;
    details->mYOffset = 0;
    GetCharacterGlyphs()[aIndex].SetMissing(1);
}

bool
gfxShapedText::FilterIfIgnorable(uint32_t aIndex, uint32_t aCh)
{
    if (IsDefaultIgnorable(aCh)) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
        details->mGlyphID = aCh;
        details->mAdvance = 0;
        details->mXOffset = 0;
        details->mYOffset = 0;
        GetCharacterGlyphs()[aIndex].SetMissing(1);
        return true;
    }
    return false;
}

void
gfxShapedText::AdjustAdvancesForSyntheticBold(float aSynBoldOffset,
                                              uint32_t aOffset,
                                              uint32_t aLength)
{
    uint32_t synAppUnitOffset = aSynBoldOffset * mAppUnitsPerDevUnit;
    CompressedGlyph *charGlyphs = GetCharacterGlyphs();
    for (uint32_t i = aOffset; i < aOffset + aLength; ++i) {
         CompressedGlyph *glyphData = charGlyphs + i;
         if (glyphData->IsSimpleGlyph()) {
             
             int32_t advance = glyphData->GetSimpleAdvance() + synAppUnitOffset;
             if (CompressedGlyph::IsSimpleAdvance(advance)) {
                 glyphData->SetSimpleGlyph(advance, glyphData->GetSimpleGlyph());
             } else {
                 
                 uint32_t glyphIndex = glyphData->GetSimpleGlyph();
                 glyphData->SetComplex(true, true, 1);
                 DetailedGlyph detail = {glyphIndex, advance, 0, 0};
                 SetGlyphs(i, *glyphData, &detail);
             }
         } else {
             
             uint32_t detailedLength = glyphData->GetGlyphCount();
             if (detailedLength) {
                 DetailedGlyph *details = GetDetailedGlyphs(i);
                 if (!details) {
                     continue;
                 }
                 if (IsRightToLeft()) {
                     details[0].mAdvance += synAppUnitOffset;
                 } else {
                     details[detailedLength - 1].mAdvance += synAppUnitOffset;
                 }
             }
         }
    }
}

void
gfxFont::RunMetrics::CombineWith(const RunMetrics& aOther, bool aOtherIsOnLeft)
{
    mAscent = std::max(mAscent, aOther.mAscent);
    mDescent = std::max(mDescent, aOther.mDescent);
    if (aOtherIsOnLeft) {
        mBoundingBox =
            (mBoundingBox + gfxPoint(aOther.mAdvanceWidth, 0)).Union(aOther.mBoundingBox);
    } else {
        mBoundingBox =
            mBoundingBox.Union(aOther.mBoundingBox + gfxPoint(mAdvanceWidth, 0));
    }
    mAdvanceWidth += aOther.mAdvanceWidth;
}

gfxFont::gfxFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                 AntialiasOption anAAOption, cairo_scaled_font_t *aScaledFont) :
    mScaledFont(aScaledFont),
    mFontEntry(aFontEntry), mIsValid(true),
    mApplySyntheticBold(false),
    mStyle(*aFontStyle),
    mAdjustedSize(0.0),
    mFUnitsConvFactor(0.0f),
    mAntialiasOption(anAAOption)
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    ++gFontCount;
#endif
    mKerningSet = HasFeatureSet(HB_TAG('k','e','r','n'), mKerningEnabled);
}

static PLDHashOperator
NotifyFontDestroyed(nsPtrHashKey<gfxFont::GlyphChangeObserver>* aKey,
                    void* aClosure)
{
    aKey->GetKey()->ForgetFont();
    return PL_DHASH_NEXT;
}

gfxFont::~gfxFont()
{
    uint32_t i, count = mGlyphExtentsArray.Length();
    
    
    
    for (i = 0; i < count; ++i) {
        delete mGlyphExtentsArray[i];
    }

    mFontEntry->NotifyFontDestroyed(this);

    if (mGlyphChangeObservers) {
        mGlyphChangeObservers->EnumerateEntries(NotifyFontDestroyed, nullptr);
    }
}

gfxFloat
gfxFont::GetGlyphHAdvance(gfxContext *aCtx, uint16_t aGID)
{
    if (!SetupCairoFont(aCtx)) {
        return 0;
    }
    if (ProvidesGlyphWidths()) {
        return GetGlyphWidth(*aCtx->GetDrawTarget(), aGID) / 65536.0;
    }
    if (mFUnitsConvFactor == 0.0f) {
        GetMetrics(eHorizontal);
    }
    NS_ASSERTION(mFUnitsConvFactor > 0.0f,
                 "missing font unit conversion factor");
    if (!mHarfBuzzShaper) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
    gfxHarfBuzzShaper* shaper =
        static_cast<gfxHarfBuzzShaper*>(mHarfBuzzShaper.get());
    if (!shaper->Initialize()) {
        return 0;
    }
    return shaper->GetGlyphHAdvance(aGID) / 65536.0;
}


PLDHashOperator
gfxFont::AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData)
{
    if (!aEntry->mShapedWord) {
        NS_ASSERTION(aEntry->mShapedWord, "cache entry has no gfxShapedWord!");
        return PL_DHASH_REMOVE;
    }
    if (aEntry->mShapedWord->IncrementAge() == kShapedWordCacheMaxAge) {
        return PL_DHASH_REMOVE;
    }
    return PL_DHASH_NEXT;
}

static void
CollectLookupsByFeature(hb_face_t *aFace, hb_tag_t aTableTag,
                        uint32_t aFeatureIndex, hb_set_t *aLookups)
{
    uint32_t lookups[32];
    uint32_t i, len, offset;

    offset = 0;
    do {
        len = ArrayLength(lookups);
        hb_ot_layout_feature_get_lookups(aFace, aTableTag, aFeatureIndex,
                                         offset, &len, lookups);
        for (i = 0; i < len; i++) {
            hb_set_add(aLookups, lookups[i]);
        }
        offset += len;
    } while (len == ArrayLength(lookups));
}

static void
CollectLookupsByLanguage(hb_face_t *aFace, hb_tag_t aTableTag,
                         const nsTHashtable<nsUint32HashKey>&
                             aSpecificFeatures,
                         hb_set_t *aOtherLookups,
                         hb_set_t *aSpecificFeatureLookups,
                         uint32_t aScriptIndex, uint32_t aLangIndex)
{
    uint32_t reqFeatureIndex;
    if (hb_ot_layout_language_get_required_feature_index(aFace, aTableTag,
                                                         aScriptIndex,
                                                         aLangIndex,
                                                         &reqFeatureIndex)) {
        CollectLookupsByFeature(aFace, aTableTag, reqFeatureIndex,
                                aOtherLookups);
    }

    uint32_t featureIndexes[32];
    uint32_t i, len, offset;

    offset = 0;
    do {
        len = ArrayLength(featureIndexes);
        hb_ot_layout_language_get_feature_indexes(aFace, aTableTag,
                                                  aScriptIndex, aLangIndex,
                                                  offset, &len, featureIndexes);

        for (i = 0; i < len; i++) {
            uint32_t featureIndex = featureIndexes[i];

            
            hb_tag_t featureTag;
            uint32_t tagLen = 1;
            hb_ot_layout_language_get_feature_tags(aFace, aTableTag,
                                                   aScriptIndex, aLangIndex,
                                                   offset + i, &tagLen,
                                                   &featureTag);

            
            hb_set_t *lookups = aSpecificFeatures.GetEntry(featureTag) ?
                                    aSpecificFeatureLookups : aOtherLookups;
            CollectLookupsByFeature(aFace, aTableTag, featureIndex, lookups);
        }
        offset += len;
    } while (len == ArrayLength(featureIndexes));
}

static bool
HasLookupRuleWithGlyphByScript(hb_face_t *aFace, hb_tag_t aTableTag,
                               hb_tag_t aScriptTag, uint32_t aScriptIndex,
                               uint16_t aGlyph,
                               const nsTHashtable<nsUint32HashKey>&
                                   aDefaultFeatures,
                               bool& aHasDefaultFeatureWithGlyph)
{
    uint32_t numLangs, lang;
    hb_set_t *defaultFeatureLookups = hb_set_create();
    hb_set_t *nonDefaultFeatureLookups = hb_set_create();

    
    CollectLookupsByLanguage(aFace, aTableTag, aDefaultFeatures,
                             nonDefaultFeatureLookups, defaultFeatureLookups,
                             aScriptIndex,
                             HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX);

    
    numLangs = hb_ot_layout_script_get_language_tags(aFace, aTableTag,
                                                     aScriptIndex, 0,
                                                     nullptr, nullptr);
    for (lang = 0; lang < numLangs; lang++) {
        CollectLookupsByLanguage(aFace, aTableTag, aDefaultFeatures,
                                 nonDefaultFeatureLookups,
                                 defaultFeatureLookups,
                                 aScriptIndex, lang);
    }

    
    aHasDefaultFeatureWithGlyph = false;
    hb_set_t *glyphs = hb_set_create();
    hb_codepoint_t index = -1;
    while (hb_set_next(defaultFeatureLookups, &index)) {
        hb_ot_layout_lookup_collect_glyphs(aFace, aTableTag, index,
                                           glyphs, glyphs, glyphs,
                                           nullptr);
        if (hb_set_has(glyphs, aGlyph)) {
            aHasDefaultFeatureWithGlyph = true;
            break;
        }
    }

    
    
    bool hasNonDefaultFeatureWithGlyph = false;
    if (!aHasDefaultFeatureWithGlyph) {
        hb_set_clear(glyphs);
        index = -1;
        while (hb_set_next(nonDefaultFeatureLookups, &index)) {
            hb_ot_layout_lookup_collect_glyphs(aFace, aTableTag, index,
                                               glyphs, glyphs, glyphs,
                                               nullptr);
            if (hb_set_has(glyphs, aGlyph)) {
                hasNonDefaultFeatureWithGlyph = true;
                break;
            }
        }
    }

    hb_set_destroy(glyphs);
    hb_set_destroy(defaultFeatureLookups);
    hb_set_destroy(nonDefaultFeatureLookups);

    return aHasDefaultFeatureWithGlyph || hasNonDefaultFeatureWithGlyph;
}

static void
HasLookupRuleWithGlyph(hb_face_t *aFace, hb_tag_t aTableTag, bool& aHasGlyph,
                       hb_tag_t aSpecificFeature, bool& aHasGlyphSpecific,
                       uint16_t aGlyph)
{
    
    uint32_t numScripts, numLangs, script, lang;
    hb_set_t *otherLookups = hb_set_create();
    hb_set_t *specificFeatureLookups = hb_set_create();
    nsTHashtable<nsUint32HashKey> specificFeature;

    specificFeature.PutEntry(aSpecificFeature);

    numScripts = hb_ot_layout_table_get_script_tags(aFace, aTableTag, 0,
                                                    nullptr, nullptr);

    for (script = 0; script < numScripts; script++) {
        
        CollectLookupsByLanguage(aFace, aTableTag, specificFeature,
                                 otherLookups, specificFeatureLookups,
                                 script, HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX);

        
        numLangs = hb_ot_layout_script_get_language_tags(aFace, HB_OT_TAG_GPOS,
                                                         script, 0,
                                                         nullptr, nullptr);
        for (lang = 0; lang < numLangs; lang++) {
            CollectLookupsByLanguage(aFace, aTableTag, specificFeature,
                                     otherLookups, specificFeatureLookups,
                                     script, lang);
        }
    }

    
    hb_set_t *glyphs = hb_set_create();
    hb_codepoint_t index = -1;
    while (hb_set_next(otherLookups, &index)) {
        hb_ot_layout_lookup_collect_glyphs(aFace, aTableTag, index,
                                           glyphs, glyphs, glyphs,
                                           nullptr);
        if (hb_set_has(glyphs, aGlyph)) {
            aHasGlyph = true;
            break;
        }
    }

    
    hb_set_clear(glyphs);
    index = -1;
    while (hb_set_next(specificFeatureLookups, &index)) {
        hb_ot_layout_lookup_collect_glyphs(aFace, aTableTag, index,
                                           glyphs, glyphs, glyphs,
                                           nullptr);
        if (hb_set_has(glyphs, aGlyph)) {
            aHasGlyphSpecific = true;
            break;
        }
    }

    hb_set_destroy(glyphs);
    hb_set_destroy(specificFeatureLookups);
    hb_set_destroy(otherLookups);
}

nsDataHashtable<nsUint32HashKey, int32_t> *gfxFont::sScriptTagToCode = nullptr;
nsTHashtable<nsUint32HashKey>             *gfxFont::sDefaultFeatures = nullptr;

static inline bool
HasSubstitution(uint32_t *aBitVector, uint32_t aBit) {
    return (aBitVector[aBit >> 5] & (1 << (aBit & 0x1f))) != 0;
}


static const hb_tag_t defaultFeatures[] = {
    HB_TAG('a','b','v','f'),
    HB_TAG('a','b','v','s'),
    HB_TAG('a','k','h','n'),
    HB_TAG('b','l','w','f'),
    HB_TAG('b','l','w','s'),
    HB_TAG('c','a','l','t'),
    HB_TAG('c','c','m','p'),
    HB_TAG('c','f','a','r'),
    HB_TAG('c','j','c','t'),
    HB_TAG('c','l','i','g'),
    HB_TAG('f','i','n','2'),
    HB_TAG('f','i','n','3'),
    HB_TAG('f','i','n','a'),
    HB_TAG('h','a','l','f'),
    HB_TAG('h','a','l','n'),
    HB_TAG('i','n','i','t'),
    HB_TAG('i','s','o','l'),
    HB_TAG('l','i','g','a'),
    HB_TAG('l','j','m','o'),
    HB_TAG('l','o','c','l'),
    HB_TAG('l','t','r','a'),
    HB_TAG('l','t','r','m'),
    HB_TAG('m','e','d','2'),
    HB_TAG('m','e','d','i'),
    HB_TAG('m','s','e','t'),
    HB_TAG('n','u','k','t'),
    HB_TAG('p','r','e','f'),
    HB_TAG('p','r','e','s'),
    HB_TAG('p','s','t','f'),
    HB_TAG('p','s','t','s'),
    HB_TAG('r','c','l','t'),
    HB_TAG('r','l','i','g'),
    HB_TAG('r','k','r','f'),
    HB_TAG('r','p','h','f'),
    HB_TAG('r','t','l','a'),
    HB_TAG('r','t','l','m'),
    HB_TAG('t','j','m','o'),
    HB_TAG('v','a','t','u'),
    HB_TAG('v','e','r','t'),
    HB_TAG('v','j','m','o')
};

void
gfxFont::CheckForFeaturesInvolvingSpace()
{
    mFontEntry->mHasSpaceFeaturesInitialized = true;

#ifdef PR_LOGGING
    bool log = LOG_FONTINIT_ENABLED();
    TimeStamp start;
    if (MOZ_UNLIKELY(log)) {
        start = TimeStamp::Now();
    }
#endif

    bool result = false;

    uint32_t spaceGlyph = GetSpaceGlyph();
    if (!spaceGlyph) {
        return;
    }

    hb_face_t *face = GetFontEntry()->GetHBFace();

    
    if (hb_ot_layout_has_substitution(face)) {

        
        if (!sScriptTagToCode) {
            sScriptTagToCode =
                new nsDataHashtable<nsUint32HashKey,
                                    int32_t>(MOZ_NUM_SCRIPT_CODES);
            sScriptTagToCode->Put(HB_TAG('D','F','L','T'), MOZ_SCRIPT_COMMON);
            for (int32_t s = MOZ_SCRIPT_ARABIC; s < MOZ_NUM_SCRIPT_CODES; s++) {
                hb_script_t scriptTag = hb_script_t(GetScriptTagForCode(s));
                hb_tag_t s1, s2;
                hb_ot_tags_from_script(scriptTag, &s1, &s2);
                sScriptTagToCode->Put(s1, s);
                if (s2 != HB_OT_TAG_DEFAULT_SCRIPT) {
                    sScriptTagToCode->Put(s2, s);
                }
            }

            uint32_t numDefaultFeatures = ArrayLength(defaultFeatures);
            sDefaultFeatures =
                new nsTHashtable<nsUint32HashKey>(numDefaultFeatures);
            for (uint32_t i = 0; i < numDefaultFeatures; i++) {
                sDefaultFeatures->PutEntry(defaultFeatures[i]);
            }
        }

        
        hb_tag_t scriptTags[8];

        uint32_t len, offset = 0;
        do {
            len = ArrayLength(scriptTags);
            hb_ot_layout_table_get_script_tags(face, HB_OT_TAG_GSUB, offset,
                                               &len, scriptTags);
            for (uint32_t i = 0; i < len; i++) {
                bool isDefaultFeature = false;
                int32_t s;
                if (!HasLookupRuleWithGlyphByScript(face, HB_OT_TAG_GSUB,
                                                    scriptTags[i], offset + i,
                                                    spaceGlyph,
                                                    *sDefaultFeatures,
                                                    isDefaultFeature) ||
                    !sScriptTagToCode->Get(scriptTags[i], &s))
                {
                    continue;
                }
                result = true;
                uint32_t index = s >> 5;
                uint32_t bit = s & 0x1f;
                if (isDefaultFeature) {
                    mFontEntry->mDefaultSubSpaceFeatures[index] |= (1 << bit);
                } else {
                    mFontEntry->mNonDefaultSubSpaceFeatures[index] |= (1 << bit);
                }
            }
            offset += len;
        } while (len == ArrayLength(scriptTags));
    }

    
    
    bool canUseWordCache = true;
    if (HasSubstitution(mFontEntry->mDefaultSubSpaceFeatures,
                        MOZ_SCRIPT_COMMON)) {
        canUseWordCache = false;
    }

    
    mFontEntry->mHasSpaceFeaturesKerning = false;
    mFontEntry->mHasSpaceFeaturesNonKerning = false;

    if (canUseWordCache && hb_ot_layout_has_positioning(face)) {
        bool hasKerning = false, hasNonKerning = false;
        HasLookupRuleWithGlyph(face, HB_OT_TAG_GPOS, hasNonKerning,
                               HB_TAG('k','e','r','n'), hasKerning, spaceGlyph);
        if (hasKerning || hasNonKerning) {
            result = true;
        }
        mFontEntry->mHasSpaceFeaturesKerning = hasKerning;
        mFontEntry->mHasSpaceFeaturesNonKerning = hasNonKerning;
    }

    hb_face_destroy(face);
    mFontEntry->mHasSpaceFeatures = result;

#ifdef PR_LOGGING
    if (MOZ_UNLIKELY(log)) {
        TimeDuration elapsed = TimeStamp::Now() - start;
        LOG_FONTINIT((
            "(fontinit-spacelookups) font: %s - "
            "subst default: %8.8x %8.8x %8.8x %8.8x "
            "subst non-default: %8.8x %8.8x %8.8x %8.8x "
            "kerning: %s non-kerning: %s time: %6.3f\n",
            NS_ConvertUTF16toUTF8(mFontEntry->Name()).get(),
            mFontEntry->mDefaultSubSpaceFeatures[3],
            mFontEntry->mDefaultSubSpaceFeatures[2],
            mFontEntry->mDefaultSubSpaceFeatures[1],
            mFontEntry->mDefaultSubSpaceFeatures[0],
            mFontEntry->mNonDefaultSubSpaceFeatures[3],
            mFontEntry->mNonDefaultSubSpaceFeatures[2],
            mFontEntry->mNonDefaultSubSpaceFeatures[1],
            mFontEntry->mNonDefaultSubSpaceFeatures[0],
            (mFontEntry->mHasSpaceFeaturesKerning ? "true" : "false"),
            (mFontEntry->mHasSpaceFeaturesNonKerning ? "true" : "false"),
            elapsed.ToMilliseconds()
        ));
    }
#endif
}

bool
gfxFont::HasSubstitutionRulesWithSpaceLookups(int32_t aRunScript)
{
    NS_ASSERTION(GetFontEntry()->mHasSpaceFeaturesInitialized,
                 "need to initialize space lookup flags");
    NS_ASSERTION(aRunScript < MOZ_NUM_SCRIPT_CODES, "weird script code");
    if (aRunScript == MOZ_SCRIPT_INVALID ||
        aRunScript >= MOZ_NUM_SCRIPT_CODES) {
        return false;
    }

    
    if (HasSubstitution(mFontEntry->mDefaultSubSpaceFeatures,
                        MOZ_SCRIPT_COMMON) ||
        HasSubstitution(mFontEntry->mDefaultSubSpaceFeatures,
                        aRunScript))
    {
        return true;
    }

    
    
    if ((HasSubstitution(mFontEntry->mNonDefaultSubSpaceFeatures,
                         MOZ_SCRIPT_COMMON) ||
         HasSubstitution(mFontEntry->mNonDefaultSubSpaceFeatures,
                         aRunScript)) &&
        (!mStyle.featureSettings.IsEmpty() ||
         !mFontEntry->mFeatureSettings.IsEmpty()))
    {
        return true;
    }

    return false;
}

bool
gfxFont::SpaceMayParticipateInShaping(int32_t aRunScript)
{
    
    if (MOZ_UNLIKELY(mFontEntry->mSkipDefaultFeatureSpaceCheck)) {
        if (!mKerningSet && mStyle.featureSettings.IsEmpty() &&
            mFontEntry->mFeatureSettings.IsEmpty()) {
            return false;
        }
    }

    
    
    
    
    
    if (!mFontEntry->mHasSpaceFeaturesInitialized) {
        CheckForFeaturesInvolvingSpace();
    }

    if (!mFontEntry->mHasSpaceFeatures) {
        return false;
    }

    
    
    if (HasSubstitutionRulesWithSpaceLookups(aRunScript) ||
        mFontEntry->mHasSpaceFeaturesNonKerning) {
        return true;
    }

    
    
    if (mKerningSet && mFontEntry->mHasSpaceFeaturesKerning) {
        return mKerningEnabled;
    }

    return false;
}

bool
gfxFont::SupportsFeature(int32_t aScript, uint32_t aFeatureTag)
{
    if (mGraphiteShaper && gfxPlatform::GetPlatform()->UseGraphiteShaping()) {
        return GetFontEntry()->SupportsGraphiteFeature(aFeatureTag);
    }
    return GetFontEntry()->SupportsOpenTypeFeature(aScript, aFeatureTag);
}

bool
gfxFont::SupportsVariantCaps(int32_t aScript,
                             uint32_t aVariantCaps,
                             bool& aFallbackToSmallCaps,
                             bool& aSyntheticLowerToSmallCaps,
                             bool& aSyntheticUpperToSmallCaps)
{
    bool ok = true;  
    aFallbackToSmallCaps = false;
    aSyntheticLowerToSmallCaps = false;
    aSyntheticUpperToSmallCaps = false;
    switch (aVariantCaps) {
        case NS_FONT_VARIANT_CAPS_SMALLCAPS:
            ok = SupportsFeature(aScript, HB_TAG('s','m','c','p'));
            if (!ok) {
                aSyntheticLowerToSmallCaps = true;
            }
            break;
        case NS_FONT_VARIANT_CAPS_ALLSMALL:
            ok = SupportsFeature(aScript, HB_TAG('s','m','c','p')) &&
                 SupportsFeature(aScript, HB_TAG('c','2','s','c'));
            if (!ok) {
                aSyntheticLowerToSmallCaps = true;
                aSyntheticUpperToSmallCaps = true;
            }
            break;
        case NS_FONT_VARIANT_CAPS_PETITECAPS:
            ok = SupportsFeature(aScript, HB_TAG('p','c','a','p'));
            if (!ok) {
                ok = SupportsFeature(aScript, HB_TAG('s','m','c','p'));
                aFallbackToSmallCaps = ok;
            }
            if (!ok) {
                aSyntheticLowerToSmallCaps = true;
            }
            break;
        case NS_FONT_VARIANT_CAPS_ALLPETITE:
            ok = SupportsFeature(aScript, HB_TAG('p','c','a','p')) &&
                 SupportsFeature(aScript, HB_TAG('c','2','p','c'));
            if (!ok) {
                ok = SupportsFeature(aScript, HB_TAG('s','m','c','p')) &&
                     SupportsFeature(aScript, HB_TAG('c','2','s','c'));
                aFallbackToSmallCaps = ok;
            }
            if (!ok) {
                aSyntheticLowerToSmallCaps = true;
                aSyntheticUpperToSmallCaps = true;
            }
            break;
        default:
            break;
    }

    NS_ASSERTION(!(ok && (aSyntheticLowerToSmallCaps ||
                          aSyntheticUpperToSmallCaps)),
                 "shouldn't use synthetic features if we found real ones");

    NS_ASSERTION(!(!ok && aFallbackToSmallCaps),
                 "if we found a usable fallback, that counts as ok");

    return ok;
}

bool
gfxFont::SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const uint8_t *aString,
                                uint32_t aLength, int32_t aRunScript)
{
    NS_ConvertASCIItoUTF16 unicodeString(reinterpret_cast<const char*>(aString),
                                         aLength);
    return SupportsSubSuperscript(aSubSuperscript, unicodeString.get(),
                                  aLength, aRunScript);
}

bool
gfxFont::SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const char16_t *aString,
                                uint32_t aLength, int32_t aRunScript)
{
    NS_ASSERTION(aSubSuperscript == NS_FONT_VARIANT_POSITION_SUPER ||
                 aSubSuperscript == NS_FONT_VARIANT_POSITION_SUB,
                 "unknown value of font-variant-position");

    uint32_t feature = aSubSuperscript == NS_FONT_VARIANT_POSITION_SUPER ?
                       HB_TAG('s','u','p','s') : HB_TAG('s','u','b','s');

    if (!SupportsFeature(aRunScript, feature)) {
        return false;
    }

    
    if (mGraphiteShaper && gfxPlatform::GetPlatform()->UseGraphiteShaping()) {
        return true;
    }

    if (!mHarfBuzzShaper) {
        mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
    }
    gfxHarfBuzzShaper* shaper =
        static_cast<gfxHarfBuzzShaper*>(mHarfBuzzShaper.get());
    if (!shaper->Initialize()) {
        return false;
    }

    
    const hb_set_t *inputGlyphs = mFontEntry->InputsForOpenTypeFeature(aRunScript, feature);

    
    hb_set_t *defaultGlyphsInRun = hb_set_create();

    
    for (uint32_t i = 0; i < aLength; i++) {
        uint32_t ch = aString[i];

        if ((i + 1 < aLength) && NS_IS_HIGH_SURROGATE(ch) &&
                             NS_IS_LOW_SURROGATE(aString[i + 1])) {
            i++;
            ch = SURROGATE_TO_UCS4(ch, aString[i]);
        }

        if (ch == 0xa0) {
            ch = ' ';
        }

        hb_codepoint_t gid = shaper->GetGlyph(ch, 0);
        hb_set_add(defaultGlyphsInRun, gid);
    }

    
    uint32_t origSize = hb_set_get_population(defaultGlyphsInRun);
    hb_set_intersect(defaultGlyphsInRun, inputGlyphs);
    uint32_t intersectionSize = hb_set_get_population(defaultGlyphsInRun);
    hb_set_destroy(defaultGlyphsInRun);

    return origSize == intersectionSize;
}

bool
gfxFont::HasFeatureSet(uint32_t aFeature, bool& aFeatureOn)
{
    aFeatureOn = false;

    if (mStyle.featureSettings.IsEmpty() &&
        GetFontEntry()->mFeatureSettings.IsEmpty()) {
        return false;
    }

    
    bool featureSet = false;
    uint32_t i, count;

    nsTArray<gfxFontFeature>& fontFeatures = GetFontEntry()->mFeatureSettings;
    count = fontFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = fontFeatures.ElementAt(i);
        if (feature.mTag == aFeature) {
            featureSet = true;
            aFeatureOn = (feature.mValue != 0);
        }
    }

    
    nsTArray<gfxFontFeature>& styleFeatures = mStyle.featureSettings;
    count = styleFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = styleFeatures.ElementAt(i);
        if (feature.mTag == aFeature) {
            featureSet = true;
            aFeatureOn = (feature.mValue != 0);
        }
    }

    return featureSet;
}





#define ToDeviceUnits(aAppUnits, aDevUnitsPerAppUnit) \
    (double(aAppUnits)*double(aDevUnitsPerAppUnit))

static AntialiasMode Get2DAAMode(gfxFont::AntialiasOption aAAOption) {
  switch (aAAOption) {
  case gfxFont::kAntialiasSubpixel:
    return AntialiasMode::SUBPIXEL;
  case gfxFont::kAntialiasGrayscale:
    return AntialiasMode::GRAY;
  case gfxFont::kAntialiasNone:
    return AntialiasMode::NONE;
  default:
    return AntialiasMode::DEFAULT;
  }
}

class GlyphBufferAzure
{
public:
    GlyphBufferAzure(const TextRunDrawParams& aRunParams,
                     const FontDrawParams&    aFontParams)
        : mRunParams(aRunParams)
        , mFontParams(aFontParams)
        , mNumGlyphs(0)
    {
    }

    ~GlyphBufferAzure()
    {
        Flush(true); 
    }

    void OutputGlyph(uint32_t aGlyphID, const gfxPoint& aPt)
    {
        Glyph *glyph = AppendGlyph();
        glyph->mIndex = aGlyphID;
        glyph->mPosition.x = aPt.x;
        glyph->mPosition.y = aPt.y;
        glyph->mPosition = mFontParams.matInv * glyph->mPosition;
        Flush(false); 
    }

    const TextRunDrawParams& mRunParams;
    const FontDrawParams& mFontParams;

private:
#define GLYPH_BUFFER_SIZE (2048/sizeof(Glyph))

    Glyph *AppendGlyph()
    {
        return &mGlyphBuffer[mNumGlyphs++];
    }

    
    
    
    void Flush(bool aFinish)
    {
        
        if ((!aFinish && mNumGlyphs < GLYPH_BUFFER_SIZE) || !mNumGlyphs) {
            return;
        }

        if (mRunParams.isRTL) {
            Glyph *begin = &mGlyphBuffer[0];
            Glyph *end = &mGlyphBuffer[mNumGlyphs];
            std::reverse(begin, end);
        }

        gfx::GlyphBuffer buf;
        buf.mGlyphs = mGlyphBuffer;
        buf.mNumGlyphs = mNumGlyphs;

        gfxContext::AzureState state = mRunParams.context->CurrentState();
        if ((int(mRunParams.drawMode) &
            (int(DrawMode::GLYPH_STROKE) | int(DrawMode::GLYPH_STROKE_UNDERNEATH))) ==
            (int(DrawMode::GLYPH_STROKE) | int(DrawMode::GLYPH_STROKE_UNDERNEATH))) {
            FlushStroke(buf, state);
        }
        if (int(mRunParams.drawMode) & int(DrawMode::GLYPH_FILL)) {
            if (state.pattern || mFontParams.contextPaint) {
                Pattern *pat;

                nsRefPtr<gfxPattern> fillPattern;
                if (!mFontParams.contextPaint ||
                    !(fillPattern = mFontParams.contextPaint->GetFillPattern(
                                        mRunParams.context->GetDrawTarget(),
                                        mRunParams.context->CurrentMatrix()))) {
                    if (state.pattern) {
                        pat = state.pattern->GetPattern(mRunParams.dt,
                                      state.patternTransformChanged ?
                                          &state.patternTransform : nullptr);
                    } else {
                        pat = nullptr;
                    }
                } else {
                    pat = fillPattern->GetPattern(mRunParams.dt);
                }

                if (pat) {
                    Matrix saved;
                    Matrix *mat = nullptr;
                    if (mFontParams.passedInvMatrix) {
                        
                        
                        

                        
                        
                        
                        if (pat->GetType() == PatternType::LINEAR_GRADIENT) {
                            mat = &static_cast<LinearGradientPattern*>(pat)->mMatrix;
                        } else if (pat->GetType() == PatternType::RADIAL_GRADIENT) {
                            mat = &static_cast<RadialGradientPattern*>(pat)->mMatrix;
                        } else if (pat->GetType() == PatternType::SURFACE) {
                            mat = &static_cast<SurfacePattern*>(pat)->mMatrix;
                        }

                        if (mat) {
                            saved = *mat;
                            *mat = (*mat) * (*mFontParams.passedInvMatrix);
                        }
                    }

                    mRunParams.dt->FillGlyphs(mFontParams.scaledFont, buf,
                                              *pat, mFontParams.drawOptions,
                                              mFontParams.renderingOptions);

                    if (mat) {
                        *mat = saved;
                    }
                }
            } else if (state.sourceSurface) {
                mRunParams.dt->FillGlyphs(mFontParams.scaledFont, buf,
                                          SurfacePattern(state.sourceSurface,
                                                         ExtendMode::CLAMP,
                                                         state.surfTransform),
                                          mFontParams.drawOptions,
                                          mFontParams.renderingOptions);
            } else {
                mRunParams.dt->FillGlyphs(mFontParams.scaledFont, buf,
                                          ColorPattern(state.color),
                                          mFontParams.drawOptions,
                                          mFontParams.renderingOptions);
            }
        }
        if (int(mRunParams.drawMode) & int(DrawMode::GLYPH_PATH)) {
            mRunParams.context->EnsurePathBuilder();
            Matrix mat = mRunParams.dt->GetTransform();
            mFontParams.scaledFont->CopyGlyphsToBuilder(
                buf, mRunParams.context->mPathBuilder,
                mRunParams.dt->GetBackendType(), &mat);
        }
        if ((int(mRunParams.drawMode) &
            (int(DrawMode::GLYPH_STROKE) | int(DrawMode::GLYPH_STROKE_UNDERNEATH))) ==
             int(DrawMode::GLYPH_STROKE)) {
            FlushStroke(buf, state);
        }

        mNumGlyphs = 0;
    }

    void FlushStroke(gfx::GlyphBuffer& aBuf, gfxContext::AzureState& aState)
    {
        RefPtr<Path> path =
            mFontParams.scaledFont->GetPathForGlyphs(aBuf, mRunParams.dt);
        if (mFontParams.contextPaint) {
            nsRefPtr<gfxPattern> strokePattern =
                mFontParams.contextPaint->GetStrokePattern(
                    mRunParams.context->GetDrawTarget(),
                    mRunParams.context->CurrentMatrix());
            if (strokePattern) {
                mRunParams.dt->Stroke(path,
                                      *strokePattern->GetPattern(mRunParams.dt),
                                      aState.strokeOptions);
            }
        }
    }

    Glyph        mGlyphBuffer[GLYPH_BUFFER_SIZE];
    unsigned int mNumGlyphs;

#undef GLYPH_BUFFER_SIZE
};







double
gfxFont::CalcXScale(gfxContext *aContext)
{
    
    gfxSize t = aContext->UserToDevice(gfxSize(1.0, 0.0));
    if (t.width == 1.0 && t.height == 0.0) {
        
        return 1.0;
    }

    double m = sqrt(t.width * t.width + t.height * t.height);

    NS_ASSERTION(m != 0.0, "degenerate transform while synthetic bolding");
    if (m == 0.0) {
        return 0.0; 
    }

    
    return 1.0 / m;
}

static DrawMode
ForcePaintingDrawMode(DrawMode aDrawMode)
{
    return aDrawMode == DrawMode::GLYPH_PATH ?
        DrawMode(int(DrawMode::GLYPH_FILL) | int(DrawMode::GLYPH_STROKE)) :
        aDrawMode;
}




void
gfxFont::DrawOneGlyph(uint32_t aGlyphID, double aAdvance, gfxPoint *aPt,
                      GlyphBufferAzure& aBuffer, bool *aEmittedGlyphs) const
{
    const TextRunDrawParams& runParams(aBuffer.mRunParams);
    const FontDrawParams& fontParams(aBuffer.mFontParams);

    double glyphX, glyphY;
    if (fontParams.isVerticalFont) {
        glyphX = aPt->x;
        if (runParams.isRTL) {
            aPt->y -= aAdvance;
            glyphY = aPt->y;
        } else {
            glyphY = aPt->y;
            aPt->y += aAdvance;
        }
    } else {
        glyphY = aPt->y;
        if (runParams.isRTL) {
            aPt->x -= aAdvance;
            glyphX = aPt->x;
        } else {
            glyphX = aPt->x;
            aPt->x += aAdvance;
        }
    }
    gfxPoint devPt(ToDeviceUnits(glyphX, runParams.devPerApp),
                   ToDeviceUnits(glyphY, runParams.devPerApp));

    if (fontParams.haveSVGGlyphs) {
        if (!runParams.paintSVGGlyphs) {
            return;
        }
        DrawMode mode = ForcePaintingDrawMode(runParams.drawMode);
        if (RenderSVGGlyph(runParams.context, devPt, mode,
                           aGlyphID, fontParams.contextPaint,
                           runParams.callbacks, *aEmittedGlyphs)) {
            return;
        }
    }

    if (fontParams.haveColorGlyphs &&
        RenderColorGlyph(runParams.context, fontParams.scaledFont,
                         fontParams.renderingOptions, fontParams.drawOptions,
                         fontParams.matInv * gfx::Point(devPt.x, devPt.y),
                         aGlyphID)) {
        return;
    }

    aBuffer.OutputGlyph(aGlyphID, devPt);

    
    for (int32_t i = 0; i < fontParams.extraStrikes; ++i) {
        if (fontParams.isVerticalFont) {
            devPt.y += fontParams.synBoldOnePixelOffset;
        } else {
            devPt.x += fontParams.synBoldOnePixelOffset;
        }
        aBuffer.OutputGlyph(aGlyphID, devPt);
    }

    *aEmittedGlyphs = true;
}



bool
gfxFont::DrawGlyphs(gfxShapedText            *aShapedText,
                    uint32_t                  aOffset, 
                    uint32_t                  aCount, 
                    gfxPoint                 *aPt,
                    const TextRunDrawParams&  aRunParams,
                    const FontDrawParams&     aFontParams)
{
    bool emittedGlyphs = false;
    GlyphBufferAzure buffer(aRunParams, aFontParams);

    gfxFloat& inlineCoord = aFontParams.isVerticalFont ? aPt->y : aPt->x;

    if (aRunParams.spacing) {
        inlineCoord += aRunParams.direction * aRunParams.spacing[0].mBefore;
    }

    const gfxShapedText::CompressedGlyph *glyphData =
        &aShapedText->GetCharacterGlyphs()[aOffset];

    for (uint32_t i = 0; i < aCount; ++i, ++glyphData) {
        if (glyphData->IsSimpleGlyph()) {
            DrawOneGlyph(glyphData->GetSimpleGlyph(),
                         glyphData->GetSimpleAdvance(),
                         aPt, buffer, &emittedGlyphs);
        } else {
            uint32_t glyphCount = glyphData->GetGlyphCount();
            if (glyphCount > 0) {
                const gfxShapedText::DetailedGlyph *details =
                    aShapedText->GetDetailedGlyphs(aOffset + i);
                NS_ASSERTION(details, "detailedGlyph should not be missing!");
                for (uint32_t j = 0; j < glyphCount; ++j, ++details) {
                    double advance = details->mAdvance;

                    if (glyphData->IsMissing()) {
                        
                        
                        if (aRunParams.drawMode != DrawMode::GLYPH_PATH &&
                            advance > 0) {
                            double glyphX = aPt->x;
                            double glyphY = aPt->y;
                            if (aRunParams.isRTL) {
                                if (aFontParams.isVerticalFont) {
                                    glyphY -= advance;
                                } else {
                                    glyphX -= advance;
                                }
                            }
                            Point pt(Float(ToDeviceUnits(glyphX, aRunParams.devPerApp)),
                                     Float(ToDeviceUnits(glyphY, aRunParams.devPerApp)));
                            Float advanceDevUnits =
                                Float(ToDeviceUnits(advance, aRunParams.devPerApp));
                            Float height = GetMetrics(eHorizontal).maxAscent;
                            Rect glyphRect = aFontParams.isVerticalFont ?
                                Rect(pt.x - height / 2, pt.y,
                                     height, advanceDevUnits) :
                                Rect(pt.x, pt.y - height,
                                     advanceDevUnits, height);

                            
                            
                            
                            Matrix oldMat;
                            if (aFontParams.passedInvMatrix) {
                                oldMat = aRunParams.dt->GetTransform();
                                aRunParams.dt->SetTransform(
                                    *aFontParams.passedInvMatrix * oldMat);
                            }

                            gfxFontMissingGlyphs::DrawMissingGlyph(
                                details->mGlyphID, glyphRect, *aRunParams.dt,
                                PatternFromState(aRunParams.context),
                                aShapedText->GetAppUnitsPerDevUnit());

                            
                            
                            if (aFontParams.passedInvMatrix) {
                                aRunParams.dt->SetTransform(oldMat);
                            }
                        }
                    } else {
                        gfxPoint glyphXY(*aPt);
                        if (aFontParams.isVerticalFont) {
                            glyphXY.x += details->mYOffset;
                            glyphXY.y += details->mXOffset;
                        } else {
                            glyphXY.x += details->mXOffset;
                            glyphXY.y += details->mYOffset;
                        }
                        DrawOneGlyph(details->mGlyphID, advance, &glyphXY,
                                     buffer, &emittedGlyphs);
                    }

                    inlineCoord += aRunParams.direction * advance;
                }
            }
        }

        if (aRunParams.spacing) {
            double space = aRunParams.spacing[i].mAfter;
            if (i + 1 < aCount) {
                space += aRunParams.spacing[i + 1].mBefore;
            }
            inlineCoord += aRunParams.direction * space;
        }
    }

    return emittedGlyphs;
}

void
gfxFont::Draw(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
              gfxPoint *aPt, const TextRunDrawParams& aRunParams,
              uint16_t aOrientation)
{
    NS_ASSERTION(aRunParams.drawMode == DrawMode::GLYPH_PATH ||
                 !(int(aRunParams.drawMode) & int(DrawMode::GLYPH_PATH)),
                 "GLYPH_PATH cannot be used with GLYPH_FILL, GLYPH_STROKE or GLYPH_STROKE_UNDERNEATH");

    if (aStart >= aEnd) {
        return;
    }

    FontDrawParams fontParams;

    fontParams.scaledFont = GetScaledFont(aRunParams.dt);
    if (!fontParams.scaledFont) {
        return;
    }

    fontParams.haveSVGGlyphs = GetFontEntry()->TryGetSVGData(this);
    fontParams.haveColorGlyphs = GetFontEntry()->TryGetColorGlyphs();
    fontParams.contextPaint = aRunParams.runContextPaint;
    fontParams.isVerticalFont =
        aOrientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT;

    bool sideways = false;
    gfxPoint origPt = *aPt;
    if (aRunParams.isVerticalRun && !fontParams.isVerticalFont) {
        sideways = true;
        aRunParams.context->Save();
        gfxPoint p(aPt->x * aRunParams.devPerApp,
                   aPt->y * aRunParams.devPerApp);
        const Metrics& metrics = GetMetrics(eHorizontal);
        
        
        gfxMatrix mat = aRunParams.context->CurrentMatrix().
            Translate(p).       
            Rotate(M_PI / 2.0). 
            Translate(-p);      

        
        
        
        
        
        
        
        
        
        if (aTextRun->UseCenterBaseline()) {
            gfxPoint baseAdj(0, (metrics.emAscent - metrics.emDescent) / 2);
            mat.Translate(baseAdj);
        }

        aRunParams.context->SetMatrix(mat);
    }

    nsAutoPtr<gfxTextContextPaint> contextPaint;
    if (fontParams.haveSVGGlyphs && !fontParams.contextPaint) {
        
        NS_ASSERTION((int(aRunParams.drawMode) & int(DrawMode::GLYPH_STROKE)) == 0,
                     "no pattern supplied for stroking text");
        nsRefPtr<gfxPattern> fillPattern = aRunParams.context->GetPattern();
        contextPaint =
            new SimpleTextContextPaint(fillPattern, nullptr,
                                       aRunParams.context->CurrentMatrix());
        fontParams.contextPaint = contextPaint;
    }

    
    
    if (IsSyntheticBold()) {
        double xscale = CalcXScale(aRunParams.context);
        fontParams.synBoldOnePixelOffset = aRunParams.direction * xscale;
        if (xscale != 0.0) {
            
            fontParams.extraStrikes =
                std::max(1, NS_lroundf(GetSyntheticBoldOffset() / xscale));
        }
    } else {
        fontParams.synBoldOnePixelOffset = 0;
        fontParams.extraStrikes = 0;
    }

    bool oldSubpixelAA = aRunParams.dt->GetPermitSubpixelAA();
    if (!AllowSubpixelAA()) {
        aRunParams.dt->SetPermitSubpixelAA(false);
    }

    Matrix mat;
    Matrix oldMat = aRunParams.dt->GetTransform();

    
    
    fontParams.passedInvMatrix = nullptr;

    fontParams.renderingOptions = GetGlyphRenderingOptions(&aRunParams);
    fontParams.drawOptions.mAntialiasMode = Get2DAAMode(mAntialiasOption);

    
    
    if (mScaledFont &&
        aRunParams.dt->GetBackendType() != BackendType::CAIRO) {
        cairo_matrix_t matrix;
        cairo_scaled_font_get_font_matrix(mScaledFont, &matrix);
        if (matrix.xy != 0) {
            
            
            
            
            
            
            mat = ToMatrix(*reinterpret_cast<gfxMatrix*>(&matrix));

            mat._11 = mat._22 = 1.0;
            mat._21 /= GetAdjustedSize();

            aRunParams.dt->SetTransform(mat * oldMat);

            fontParams.matInv = mat;
            fontParams.matInv.Invert();

            fontParams.passedInvMatrix = &fontParams.matInv;
        }
    }

    gfxFloat& baseline = fontParams.isVerticalFont ? aPt->x : aPt->y;
    gfxFloat origBaseline = baseline;
    if (mStyle.baselineOffset != 0.0) {
        baseline +=
            mStyle.baselineOffset * aTextRun->GetAppUnitsPerDevUnit();
    }

    bool emittedGlyphs =
        DrawGlyphs(aTextRun, aStart, aEnd - aStart, aPt,
                   aRunParams, fontParams);

    baseline = origBaseline;

    if (aRunParams.callbacks && emittedGlyphs) {
        aRunParams.callbacks->NotifyGlyphPathEmitted();
    }

    aRunParams.dt->SetTransform(oldMat);
    aRunParams.dt->SetPermitSubpixelAA(oldSubpixelAA);

    if (sideways) {
        aRunParams.context->Restore();
        *aPt = gfxPoint(origPt.x, origPt.y + (aPt->x - origPt.x));
    }
}

bool
gfxFont::RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint) const
{
    if (!GetFontEntry()->HasSVGGlyph(aGlyphId)) {
        return false;
    }

    const gfxFloat devUnitsPerSVGUnit =
        GetAdjustedSize() / GetFontEntry()->UnitsPerEm();
    gfxContextMatrixAutoSaveRestore matrixRestore(aContext);

    aContext->Save();
    aContext->SetMatrix(
      aContext->CurrentMatrix().Translate(aPoint.x, aPoint.y).
                                Scale(devUnitsPerSVGUnit, devUnitsPerSVGUnit));

    aContextPaint->InitStrokeGeometry(aContext, devUnitsPerSVGUnit);

    bool rv = GetFontEntry()->RenderSVGGlyph(aContext, aGlyphId,
                                             int(aDrawMode), aContextPaint);
    aContext->Restore();
    aContext->NewPath();
    return rv;
}

bool
gfxFont::RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint,
                        gfxTextRunDrawCallbacks *aCallbacks,
                        bool& aEmittedGlyphs) const
{
    if (aCallbacks && aEmittedGlyphs) {
        aCallbacks->NotifyGlyphPathEmitted();
        aEmittedGlyphs = false;
    }
    return RenderSVGGlyph(aContext, aPoint, aDrawMode, aGlyphId, aContextPaint);
}

bool
gfxFont::RenderColorGlyph(gfxContext* aContext,
                          mozilla::gfx::ScaledFont* scaledFont,
                          GlyphRenderingOptions* aRenderingOptions,
                          mozilla::gfx::DrawOptions aDrawOptions,
                          const mozilla::gfx::Point& aPoint,
                          uint32_t aGlyphId) const
{
    nsAutoTArray<uint16_t, 8> layerGlyphs;
    nsAutoTArray<mozilla::gfx::Color, 8> layerColors;

    if (!GetFontEntry()->GetColorLayersInfo(aGlyphId, layerGlyphs, layerColors)) {
        return false;
    }

    RefPtr<DrawTarget> dt = aContext->GetDrawTarget();
    for (uint32_t layerIndex = 0; layerIndex < layerGlyphs.Length();
         layerIndex++) {
        Glyph glyph;
        glyph.mIndex = layerGlyphs[layerIndex];
        glyph.mPosition = aPoint;

        mozilla::gfx::GlyphBuffer buffer;
        buffer.mGlyphs = &glyph;
        buffer.mNumGlyphs = 1;

        dt->FillGlyphs(scaledFont, buffer,
                       ColorPattern(layerColors[layerIndex]),
                       aDrawOptions, aRenderingOptions);
    }
    return true;
}

static void
UnionRange(gfxFloat aX, gfxFloat* aDestMin, gfxFloat* aDestMax)
{
    *aDestMin = std::min(*aDestMin, aX);
    *aDestMax = std::max(*aDestMax, aX);
}




static bool
NeedsGlyphExtents(gfxFont *aFont, gfxTextRun *aTextRun)
{
    return (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX) ||
        aFont->GetFontEntry()->IsUserFont();
}

bool
gfxFont::IsSpaceGlyphInvisible(gfxContext *aRefContext, gfxTextRun *aTextRun)
{
    if (!mFontEntry->mSpaceGlyphIsInvisibleInitialized &&
        GetAdjustedSize() >= 1.0) {
        gfxGlyphExtents *extents =
            GetOrCreateGlyphExtents(aTextRun->GetAppUnitsPerDevUnit());
        gfxRect glyphExtents;
        mFontEntry->mSpaceGlyphIsInvisible =
            extents->GetTightGlyphExtentsAppUnits(this, aRefContext,
                GetSpaceGlyph(), &glyphExtents) &&
            glyphExtents.IsEmpty();
        mFontEntry->mSpaceGlyphIsInvisibleInitialized = true;
    }
    return mFontEntry->mSpaceGlyphIsInvisible;
}

gfxFont::RunMetrics
gfxFont::Measure(gfxTextRun *aTextRun,
                 uint32_t aStart, uint32_t aEnd,
                 BoundingBoxType aBoundingBoxType,
                 gfxContext *aRefContext,
                 Spacing *aSpacing,
                 uint16_t aOrientation)
{
    
    
    
    
    if (aBoundingBoxType == TIGHT_HINTED_OUTLINE_EXTENTS &&
        mAntialiasOption != kAntialiasNone) {
        if (!mNonAAFont) {
            mNonAAFont = CopyWithAntialiasOption(kAntialiasNone);
        }
        
        
        if (mNonAAFont) {
            return mNonAAFont->Measure(aTextRun, aStart, aEnd,
                                       TIGHT_HINTED_OUTLINE_EXTENTS,
                                       aRefContext, aSpacing, aOrientation);
        }
    }

    const int32_t appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
    gfxFont::Orientation orientation =
        aOrientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT
        ? eVertical : eHorizontal;
    const gfxFont::Metrics& fontMetrics = GetMetrics(orientation);

    gfxFloat baselineOffset = 0;
    if (aTextRun->UseCenterBaseline() && orientation == eHorizontal) {
        
        
        
        
        
        
        
        
        baselineOffset = appUnitsPerDevUnit *
            (fontMetrics.emAscent - fontMetrics.emDescent) / 2;
    }

    RunMetrics metrics;
    metrics.mAscent = fontMetrics.maxAscent * appUnitsPerDevUnit;
    metrics.mDescent = fontMetrics.maxDescent * appUnitsPerDevUnit;

    if (aStart == aEnd) {
        
        metrics.mAscent -= baselineOffset;
        metrics.mDescent += baselineOffset;
        metrics.mBoundingBox = gfxRect(0, -metrics.mAscent,
                                       0, metrics.mAscent + metrics.mDescent);
        return metrics;
    }

    gfxFloat advanceMin = 0, advanceMax = 0;
    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    bool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    bool needsGlyphExtents = NeedsGlyphExtents(this, aTextRun);
    gfxGlyphExtents *extents =
        ((aBoundingBoxType == LOOSE_INK_EXTENTS &&
            !needsGlyphExtents &&
            !aTextRun->HasDetailedGlyphs()) ||
         (MOZ_UNLIKELY(GetStyle()->sizeAdjust == 0.0)) ||
         (MOZ_UNLIKELY(GetStyle()->size == 0))) ? nullptr
        : GetOrCreateGlyphExtents(aTextRun->GetAppUnitsPerDevUnit());
    double x = 0;
    if (aSpacing) {
        x += direction*aSpacing[0].mBefore;
    }
    uint32_t spaceGlyph = GetSpaceGlyph();
    bool allGlyphsInvisible = true;
    uint32_t i;
    for (i = aStart; i < aEnd; ++i) {
        const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            double advance = glyphData->GetSimpleAdvance();
            uint32_t glyphIndex = glyphData->GetSimpleGlyph();
            if (glyphIndex != spaceGlyph ||
                !IsSpaceGlyphInvisible(aRefContext, aTextRun)) {
                allGlyphsInvisible = false;
            }
            
            
            if ((aBoundingBoxType != LOOSE_INK_EXTENTS || needsGlyphExtents) &&
                extents){
                uint16_t extentsWidth = extents->GetContainedGlyphWidthAppUnits(glyphIndex);
                if (extentsWidth != gfxGlyphExtents::INVALID_WIDTH &&
                    aBoundingBoxType == LOOSE_INK_EXTENTS) {
                    UnionRange(x, &advanceMin, &advanceMax);
                    UnionRange(x + direction*extentsWidth, &advanceMin, &advanceMax);
                } else {
                    gfxRect glyphRect;
                    if (!extents->GetTightGlyphExtentsAppUnits(this,
                            aRefContext, glyphIndex, &glyphRect)) {
                        glyphRect = gfxRect(0, metrics.mBoundingBox.Y(),
                            advance, metrics.mBoundingBox.Height());
                    }
                    if (orientation == eVertical) {
                        Swap(glyphRect.x, glyphRect.y);
                        Swap(glyphRect.width, glyphRect.height);
                    }
                    if (isRTL) {
                        glyphRect -= gfxPoint(advance, 0);
                    }
                    glyphRect += gfxPoint(x, 0);
                    metrics.mBoundingBox = metrics.mBoundingBox.Union(glyphRect);
                }
            }
            x += direction*advance;
        } else {
            allGlyphsInvisible = false;
            uint32_t glyphCount = glyphData->GetGlyphCount();
            if (glyphCount > 0) {
                const gfxTextRun::DetailedGlyph *details =
                    aTextRun->GetDetailedGlyphs(i);
                NS_ASSERTION(details != nullptr,
                             "detaiedGlyph record should not be missing!");
                uint32_t j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    uint32_t glyphIndex = details->mGlyphID;
                    gfxPoint glyphPt(x + details->mXOffset, details->mYOffset);
                    double advance = details->mAdvance;
                    gfxRect glyphRect;
                    if (glyphData->IsMissing() || !extents ||
                        !extents->GetTightGlyphExtentsAppUnits(this,
                                aRefContext, glyphIndex, &glyphRect)) {
                        
                        
                        glyphRect = gfxRect(0, -metrics.mAscent,
                            advance, metrics.mAscent + metrics.mDescent);
                    }
                    if (orientation == eVertical) {
                        Swap(glyphRect.x, glyphRect.y);
                        Swap(glyphRect.width, glyphRect.height);
                    }
                    if (isRTL) {
                        glyphRect -= gfxPoint(advance, 0);
                    }
                    glyphRect += glyphPt;
                    metrics.mBoundingBox = metrics.mBoundingBox.Union(glyphRect);
                    x += direction*advance;
                }
            }
        }
        
        if (aSpacing) {
            double space = aSpacing[i - aStart].mAfter;
            if (i + 1 < aEnd) {
                space += aSpacing[i + 1 - aStart].mBefore;
            }
            x += direction*space;
        }
    }

    if (allGlyphsInvisible) {
        metrics.mBoundingBox.SetEmpty();
    } else {
        if (aBoundingBoxType == LOOSE_INK_EXTENTS) {
            UnionRange(x, &advanceMin, &advanceMax);
            gfxRect fontBox(advanceMin, -metrics.mAscent,
                            advanceMax - advanceMin, metrics.mAscent + metrics.mDescent);
            metrics.mBoundingBox = metrics.mBoundingBox.Union(fontBox);
        }
        if (isRTL) {
            metrics.mBoundingBox -= gfxPoint(x, 0);
        }
    }

    
    
    
    if (mStyle.style != NS_FONT_STYLE_NORMAL && !mFontEntry->IsItalic()) {
        gfxFloat extendLeftEdge =
            ceil(OBLIQUE_SKEW_FACTOR * metrics.mBoundingBox.YMost());
        gfxFloat extendRightEdge =
            ceil(OBLIQUE_SKEW_FACTOR * -metrics.mBoundingBox.y);
        metrics.mBoundingBox.width += extendLeftEdge + extendRightEdge;
        metrics.mBoundingBox.x -= extendLeftEdge;
    }

    if (baselineOffset != 0) {
        metrics.mAscent -= baselineOffset;
        metrics.mDescent += baselineOffset;
        metrics.mBoundingBox.y += baselineOffset;
    }

    metrics.mAdvanceWidth = x*direction;
    return metrics;
}

static PLDHashOperator
NotifyGlyphChangeObservers(nsPtrHashKey<gfxFont::GlyphChangeObserver>* aKey,
                           void* aClosure)
{
    aKey->GetKey()->NotifyGlyphsChanged();
    return PL_DHASH_NEXT;
}

void
gfxFont::NotifyGlyphsChanged()
{
    uint32_t i, count = mGlyphExtentsArray.Length();
    for (i = 0; i < count; ++i) {
        
        mGlyphExtentsArray[i]->NotifyGlyphsChanged();
    }

    if (mGlyphChangeObservers) {
        mGlyphChangeObservers->EnumerateEntries(NotifyGlyphChangeObservers, nullptr);
    }
}

static bool
IsBoundarySpace(char16_t aChar, char16_t aNextChar)
{
    return (aChar == ' ' || aChar == 0x00A0) && !IsClusterExtender(aNextChar);
}

#ifdef __GNUC__
#define GFX_MAYBE_UNUSED __attribute__((unused))
#else
#define GFX_MAYBE_UNUSED
#endif

template<typename T>
gfxShapedWord*
gfxFont::GetShapedWord(gfxContext *aContext,
                       const T    *aText,
                       uint32_t    aLength,
                       uint32_t    aHash,
                       int32_t     aRunScript,
                       bool        aVertical,
                       int32_t     aAppUnitsPerDevUnit,
                       uint32_t    aFlags,
                       gfxTextPerfMetrics *aTextPerf GFX_MAYBE_UNUSED)
{
    
    uint32_t wordCacheMaxEntries =
        gfxPlatform::GetPlatform()->WordCacheMaxEntries();
    if (mWordCache->Count() > wordCacheMaxEntries) {
        NS_WARNING("flushing shaped-word cache");
        ClearCachedWords();
    }

    
    CacheHashKey key(aText, aLength, aHash,
                     aRunScript,
                     aAppUnitsPerDevUnit,
                     aFlags);

    CacheHashEntry *entry = mWordCache->PutEntry(key);
    if (!entry) {
        NS_WARNING("failed to create word cache entry - expect missing text");
        return nullptr;
    }
    gfxShapedWord *sw = entry->mShapedWord;

    bool isContent = !mStyle.systemFont;

    if (sw) {
        sw->ResetAge();
        Telemetry::Accumulate((isContent ? Telemetry::WORD_CACHE_HITS_CONTENT :
                                   Telemetry::WORD_CACHE_HITS_CHROME),
                              aLength);
#ifndef RELEASE_BUILD
        if (aTextPerf) {
            aTextPerf->current.wordCacheHit++;
        }
#endif
        return sw;
    }

    Telemetry::Accumulate((isContent ? Telemetry::WORD_CACHE_MISSES_CONTENT :
                               Telemetry::WORD_CACHE_MISSES_CHROME),
                          aLength);
#ifndef RELEASE_BUILD
    if (aTextPerf) {
        aTextPerf->current.wordCacheMiss++;
    }
#endif

    sw = entry->mShapedWord = gfxShapedWord::Create(aText, aLength,
                                                    aRunScript,
                                                    aAppUnitsPerDevUnit,
                                                    aFlags);
    if (!sw) {
        NS_WARNING("failed to create gfxShapedWord - expect missing text");
        return nullptr;
    }

    DebugOnly<bool> ok =
        ShapeText(aContext, aText, 0, aLength, aRunScript, aVertical, sw);

    NS_WARN_IF_FALSE(ok, "failed to shape word - expect garbled text");

    return sw;
}

bool
gfxFont::CacheHashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    const gfxShapedWord *sw = mShapedWord;
    if (!sw) {
        return false;
    }
    if (sw->GetLength() != aKey->mLength ||
        sw->GetFlags() != aKey->mFlags ||
        sw->GetAppUnitsPerDevUnit() != aKey->mAppUnitsPerDevUnit ||
        sw->Script() != aKey->mScript) {
        return false;
    }
    if (sw->TextIs8Bit()) {
        if (aKey->mTextIs8Bit) {
            return (0 == memcmp(sw->Text8Bit(), aKey->mText.mSingle,
                                aKey->mLength * sizeof(uint8_t)));
        }
        
        
        
        const uint8_t   *s1 = sw->Text8Bit();
        const char16_t *s2 = aKey->mText.mDouble;
        const char16_t *s2end = s2 + aKey->mLength;
        while (s2 < s2end) {
            if (*s1++ != *s2++) {
                return false;
            }
        }
        return true;
    }
    NS_ASSERTION((aKey->mFlags & gfxTextRunFactory::TEXT_IS_8BIT) == 0 &&
                 !aKey->mTextIs8Bit, "didn't expect 8-bit text here");
    return (0 == memcmp(sw->TextUnicode(), aKey->mText.mDouble,
                        aKey->mLength * sizeof(char16_t)));
}

bool
gfxFont::ShapeText(gfxContext    *aContext,
                   const uint8_t *aText,
                   uint32_t       aOffset,
                   uint32_t       aLength,
                   int32_t        aScript,
                   bool           aVertical,
                   gfxShapedText *aShapedText)
{
    nsDependentCSubstring ascii((const char*)aText, aLength);
    nsAutoString utf16;
    AppendASCIItoUTF16(ascii, utf16);
    if (utf16.Length() != aLength) {
        return false;
    }
    return ShapeText(aContext, utf16.BeginReading(), aOffset, aLength,
                     aScript, aVertical, aShapedText);
}

bool
gfxFont::ShapeText(gfxContext      *aContext,
                   const char16_t *aText,
                   uint32_t         aOffset,
                   uint32_t         aLength,
                   int32_t          aScript,
                   bool             aVertical,
                   gfxShapedText   *aShapedText)
{
    bool ok = false;

    
    
    if (FontCanSupportGraphite() && !aVertical) {
        if (gfxPlatform::GetPlatform()->UseGraphiteShaping()) {
            if (!mGraphiteShaper) {
                mGraphiteShaper = new gfxGraphiteShaper(this);
            }
            ok = mGraphiteShaper->ShapeText(aContext, aText, aOffset, aLength,
                                            aScript, aVertical, aShapedText);
        }
    }

    if (!ok) {
        if (!mHarfBuzzShaper) {
            mHarfBuzzShaper = new gfxHarfBuzzShaper(this);
        }
        ok = mHarfBuzzShaper->ShapeText(aContext, aText, aOffset, aLength,
                                        aScript, aVertical, aShapedText);
    }

    NS_WARN_IF_FALSE(ok, "shaper failed, expect scrambled or missing text");

    PostShapingFixup(aContext, aText, aOffset, aLength, aVertical,
                     aShapedText);

    return ok;
}

void
gfxFont::PostShapingFixup(gfxContext      *aContext,
                          const char16_t *aText,
                          uint32_t         aOffset,
                          uint32_t         aLength,
                          bool             aVertical,
                          gfxShapedText   *aShapedText)
{
    if (IsSyntheticBold()) {
        const Metrics& metrics =
            GetMetrics(aVertical ? eVertical : eHorizontal);
        if (metrics.maxAdvance > metrics.aveCharWidth) {
            float synBoldOffset =
                    GetSyntheticBoldOffset() * CalcXScale(aContext);
            aShapedText->AdjustAdvancesForSyntheticBold(synBoldOffset,
                                                        aOffset, aLength);
        }
    }
}

#define MAX_SHAPING_LENGTH  32760 // slightly less than 32K, trying to avoid
                                  
#define BACKTRACK_LIMIT     16 // backtrack this far looking for a good place
                               

template<typename T>
bool
gfxFont::ShapeFragmentWithoutWordCache(gfxContext *aContext,
                                       const T    *aText,
                                       uint32_t    aOffset,
                                       uint32_t    aLength,
                                       int32_t     aScript,
                                       bool        aVertical,
                                       gfxTextRun *aTextRun)
{
    aTextRun->SetupClusterBoundaries(aOffset, aText, aLength);

    bool ok = true;

    while (ok && aLength > 0) {
        uint32_t fragLen = aLength;

        
        if (fragLen > MAX_SHAPING_LENGTH) {
            fragLen = MAX_SHAPING_LENGTH;

            
            
            if (sizeof(T) == sizeof(char16_t)) {
                uint32_t i;
                for (i = 0; i < BACKTRACK_LIMIT; ++i) {
                    if (aTextRun->IsClusterStart(aOffset + fragLen - i)) {
                        fragLen -= i;
                        break;
                    }
                }
                if (i == BACKTRACK_LIMIT) {
                    
                    
                    
                    if (NS_IS_LOW_SURROGATE(aText[fragLen]) &&
                        NS_IS_HIGH_SURROGATE(aText[fragLen - 1])) {
                        --fragLen;
                    }
                }
            }
        }

        ok = ShapeText(aContext, aText, aOffset, fragLen, aScript, aVertical,
                       aTextRun);

        aText += fragLen;
        aOffset += fragLen;
        aLength -= fragLen;
    }

    return ok;
}






static bool
IsInvalidControlChar(uint32_t aCh)
{
    return aCh != '\r' && ((aCh & 0x7f) < 0x20 || aCh == 0x7f);
}

template<typename T>
bool
gfxFont::ShapeTextWithoutWordCache(gfxContext *aContext,
                                   const T    *aText,
                                   uint32_t    aOffset,
                                   uint32_t    aLength,
                                   int32_t     aScript,
                                   bool        aVertical,
                                   gfxTextRun *aTextRun)
{
    uint32_t fragStart = 0;
    bool ok = true;

    for (uint32_t i = 0; i <= aLength && ok; ++i) {
        T ch = (i < aLength) ? aText[i] : '\n';
        bool invalid = gfxFontGroup::IsInvalidChar(ch);
        uint32_t length = i - fragStart;

        
        if (!invalid) {
            continue;
        }

        if (length > 0) {
            ok = ShapeFragmentWithoutWordCache(aContext, aText + fragStart,
                                               aOffset + fragStart, length,
                                               aScript, aVertical, aTextRun);
        }

        if (i == aLength) {
            break;
        }

        
        
        
        if (ch == '\t') {
            aTextRun->SetIsTab(aOffset + i);
        } else if (ch == '\n') {
            aTextRun->SetIsNewline(aOffset + i);
        } else if (IsInvalidControlChar(ch) &&
            !(aTextRun->GetFlags() & gfxTextRunFactory::TEXT_HIDE_CONTROL_CHARACTERS)) {
            if (GetFontEntry()->IsUserFont() && HasCharacter(ch)) {
                ShapeFragmentWithoutWordCache(aContext, aText + i,
                                              aOffset + i, 1,
                                              aScript, aVertical, aTextRun);
            } else {
                aTextRun->SetMissingGlyph(aOffset + i, ch, this);
            }
        }
        fragStart = i + 1;
    }

    NS_WARN_IF_FALSE(ok, "failed to shape text - expect garbled text");
    return ok;
}

#ifndef RELEASE_BUILD
#define TEXT_PERF_INCR(tp, m) (tp ? (tp)->current.m++ : 0)
#else
#define TEXT_PERF_INCR(tp, m)
#endif

inline static bool IsChar8Bit(uint8_t ) { return true; }
inline static bool IsChar8Bit(char16_t aCh) { return aCh < 0x100; }

inline static bool HasSpaces(const uint8_t *aString, uint32_t aLen)
{
    return memchr(aString, 0x20, aLen) != nullptr;
}

inline static bool HasSpaces(const char16_t *aString, uint32_t aLen)
{
    for (const char16_t *ch = aString; ch < aString + aLen; ch++) {
        if (*ch == 0x20) {
            return true;
        }
    }
    return false;
}

template<typename T>
bool
gfxFont::SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString, 
                             uint32_t aRunStart, 
                             uint32_t aRunLength,
                             int32_t aRunScript,
                             bool aVertical)
{
    if (aRunLength == 0) {
        return true;
    }

    gfxTextPerfMetrics *tp = nullptr;

#ifndef RELEASE_BUILD
    tp = aTextRun->GetFontGroup()->GetTextPerfMetrics();
    if (tp) {
        if (mStyle.systemFont) {
            tp->current.numChromeTextRuns++;
        } else {
            tp->current.numContentTextRuns++;
        }
        tp->current.numChars += aRunLength;
        if (aRunLength > tp->current.maxTextRunLen) {
            tp->current.maxTextRunLen = aRunLength;
        }
    }
#endif

    uint32_t wordCacheCharLimit =
        gfxPlatform::GetPlatform()->WordCacheCharLimit();

    
    
    
    
    if (SpaceMayParticipateInShaping(aRunScript)) {
        if (aRunLength > wordCacheCharLimit ||
            HasSpaces(aString, aRunLength)) {
            TEXT_PERF_INCR(tp, wordCacheSpaceRules);
            return ShapeTextWithoutWordCache(aContext, aString,
                                             aRunStart, aRunLength,
                                             aRunScript, aVertical,
                                             aTextRun);
        }
    }

    InitWordCache();

    
    uint32_t flags = aTextRun->GetFlags();
    flags &= (gfxTextRunFactory::TEXT_IS_RTL |
              gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES |
              gfxTextRunFactory::TEXT_USE_MATH_SCRIPT |
              gfxTextRunFactory::TEXT_ORIENT_MASK);
    if (sizeof(T) == sizeof(uint8_t)) {
        flags |= gfxTextRunFactory::TEXT_IS_8BIT;
    }

    uint32_t wordStart = 0;
    uint32_t hash = 0;
    bool wordIs8Bit = true;
    int32_t appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    T nextCh = aString[0];
    for (uint32_t i = 0; i <= aRunLength; ++i) {
        T ch = nextCh;
        nextCh = (i < aRunLength - 1) ? aString[i + 1] : '\n';
        bool boundary = IsBoundarySpace(ch, nextCh);
        bool invalid = !boundary && gfxFontGroup::IsInvalidChar(ch);
        uint32_t length = i - wordStart;

        
        
        
        if (!boundary && !invalid) {
            if (!IsChar8Bit(ch)) {
                wordIs8Bit = false;
            }
            
            hash = gfxShapedWord::HashMix(hash, ch);
            continue;
        }

        
        
        
        
        if (length > wordCacheCharLimit) {
            TEXT_PERF_INCR(tp, wordCacheLong);
            bool ok = ShapeFragmentWithoutWordCache(aContext,
                                                    aString + wordStart,
                                                    aRunStart + wordStart,
                                                    length,
                                                    aRunScript,
                                                    aVertical,
                                                    aTextRun);
            if (!ok) {
                return false;
            }
        } else if (length > 0) {
            uint32_t wordFlags = flags;
            
            
            
            if (sizeof(T) == sizeof(char16_t)) {
                if (wordIs8Bit) {
                    wordFlags |= gfxTextRunFactory::TEXT_IS_8BIT;
                }
            }
            gfxShapedWord *sw = GetShapedWord(aContext,
                                              aString + wordStart, length,
                                              hash, aRunScript, aVertical,
                                              appUnitsPerDevUnit,
                                              wordFlags, tp);
            if (sw) {
                aTextRun->CopyGlyphDataFrom(sw, aRunStart + wordStart);
            } else {
                return false; 
            }
        }

        if (boundary) {
            
            uint16_t orientation = flags & gfxTextRunFactory::TEXT_ORIENT_MASK;
            if (orientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_MIXED) {
                orientation = aVertical ?
                    gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT :
                    gfxTextRunFactory::TEXT_ORIENT_VERTICAL_SIDEWAYS_RIGHT;
            }
            if (!aTextRun->SetSpaceGlyphIfSimple(this, aContext,
                                                 aRunStart + i, ch,
                                                 orientation))
            {
                static const uint8_t space = ' ';
                gfxShapedWord *sw =
                    GetShapedWord(aContext,
                                  &space, 1,
                                  gfxShapedWord::HashMix(0, ' '), aRunScript, aVertical,
                                  appUnitsPerDevUnit,
                                  flags | gfxTextRunFactory::TEXT_IS_8BIT, tp);
                if (sw) {
                    aTextRun->CopyGlyphDataFrom(sw, aRunStart + i);
                } else {
                    return false;
                }
            }
            hash = 0;
            wordStart = i + 1;
            wordIs8Bit = true;
            continue;
        }

        if (i == aRunLength) {
            break;
        }

        NS_ASSERTION(invalid,
                     "how did we get here except via an invalid char?");

        
        
        
        if (ch == '\t') {
            aTextRun->SetIsTab(aRunStart + i);
        } else if (ch == '\n') {
            aTextRun->SetIsNewline(aRunStart + i);
        } else if (IsInvalidControlChar(ch) &&
            !(aTextRun->GetFlags() & gfxTextRunFactory::TEXT_HIDE_CONTROL_CHARACTERS)) {
            if (GetFontEntry()->IsUserFont() && HasCharacter(ch)) {
                ShapeFragmentWithoutWordCache(aContext, aString + i,
                                              aRunStart + i, 1,
                                              aRunScript, aVertical, aTextRun);
            } else {
                aTextRun->SetMissingGlyph(aRunStart + i, ch, this);
            }
        }

        hash = 0;
        wordStart = i + 1;
        wordIs8Bit = true;
    }

    return true;
}


template bool
gfxFont::SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const uint8_t *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript,
                             bool aVertical);
template bool
gfxFont::SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const char16_t *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript,
                             bool aVertical);

template<>
bool
gfxFont::InitFakeSmallCapsRun(gfxContext     *aContext,
                              gfxTextRun     *aTextRun,
                              const char16_t *aText,
                              uint32_t        aOffset,
                              uint32_t        aLength,
                              uint8_t         aMatchType,
                              uint16_t        aOrientation,
                              int32_t         aScript,
                              bool            aSyntheticLower,
                              bool            aSyntheticUpper)
{
    bool ok = true;

    nsRefPtr<gfxFont> smallCapsFont = GetSmallCapsFont();

    enum RunCaseAction {
        kNoChange,
        kUppercaseReduce,
        kUppercase
    };

    RunCaseAction runAction = kNoChange;
    uint32_t runStart = 0;
    bool vertical =
        aOrientation == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT;

    for (uint32_t i = 0; i <= aLength; ++i) {
        uint32_t extraCodeUnits = 0; 
                                     
                                     
        RunCaseAction chAction = kNoChange;
        
        
        if (i < aLength) {
            uint32_t ch = aText[i];
            if (NS_IS_HIGH_SURROGATE(ch) && i < aLength - 1 &&
                NS_IS_LOW_SURROGATE(aText[i + 1])) {
                ch = SURROGATE_TO_UCS4(ch, aText[i + 1]);
                extraCodeUnits = 1;
            }
            
            
            if (IsClusterExtender(ch)) {
                chAction = runAction;
            } else {
                if (ch != ToUpperCase(ch) || SpecialUpper(ch)) {
                    
                    chAction = (aSyntheticLower ? kUppercaseReduce : kNoChange);
                } else if (ch != ToLowerCase(ch)) {
                    
                    chAction = (aSyntheticUpper ? kUppercaseReduce : kNoChange);
                    if (mStyle.explicitLanguage &&
                        mStyle.language == nsGkAtoms::el) {
                        
                        
                        
                        
                        
                        mozilla::GreekCasing::State state;
                        uint32_t ch2 = mozilla::GreekCasing::UpperCase(ch, state);
                        if (ch != ch2 && !aSyntheticUpper) {
                            chAction = kUppercase;
                        }
                    }
                }
            }
        }

        
        
        
        
        
        if ((i == aLength || runAction != chAction) && runStart < i) {
            uint32_t runLength = i - runStart;
            gfxFont* f = this;
            switch (runAction) {
            case kNoChange:
                
                aTextRun->AddGlyphRun(f, aMatchType, aOffset + runStart, true,
                                      aOrientation);
                if (!f->SplitAndInitTextRun(aContext, aTextRun,
                                            aText + runStart,
                                            aOffset + runStart, runLength,
                                            aScript, vertical)) {
                    ok = false;
                }
                break;

            case kUppercaseReduce:
                
                f = smallCapsFont;

            case kUppercase:
                
                nsDependentSubstring origString(aText + runStart, runLength);
                nsAutoString convertedString;
                nsAutoTArray<bool,50> charsToMergeArray;
                nsAutoTArray<bool,50> deletedCharsArray;

                bool mergeNeeded = nsCaseTransformTextRunFactory::
                    TransformString(origString,
                                    convertedString,
                                    true,
                                    mStyle.explicitLanguage
                                      ? mStyle.language : nullptr,
                                    charsToMergeArray,
                                    deletedCharsArray);

                if (mergeNeeded) {
                    
                    
                    
                    
                    gfxTextRunFactory::Parameters params = {
                        aContext, nullptr, nullptr, nullptr, 0,
                        aTextRun->GetAppUnitsPerDevUnit()
                    };
                    nsAutoPtr<gfxTextRun> tempRun;
                    tempRun =
                        gfxTextRun::Create(&params, convertedString.Length(),
                                           aTextRun->GetFontGroup(), 0);
                    tempRun->AddGlyphRun(f, aMatchType, 0, true, aOrientation);
                    if (!f->SplitAndInitTextRun(aContext, tempRun,
                                                convertedString.BeginReading(),
                                                0, convertedString.Length(),
                                                aScript, vertical)) {
                        ok = false;
                    } else {
                        nsAutoPtr<gfxTextRun> mergedRun;
                        mergedRun =
                            gfxTextRun::Create(&params, runLength,
                                               aTextRun->GetFontGroup(), 0);
                        MergeCharactersInTextRun(mergedRun, tempRun,
                                                 charsToMergeArray.Elements(),
                                                 deletedCharsArray.Elements());
                        aTextRun->CopyGlyphDataFrom(mergedRun, 0, runLength,
                                                    aOffset + runStart);
                    }
                } else {
                    aTextRun->AddGlyphRun(f, aMatchType, aOffset + runStart,
                                          true, aOrientation);
                    if (!f->SplitAndInitTextRun(aContext, aTextRun,
                                                convertedString.BeginReading(),
                                                aOffset + runStart, runLength,
                                                aScript, vertical)) {
                        ok = false;
                    }
                }
                break;
            }

            runStart = i;
        }

        i += extraCodeUnits;
        if (i < aLength) {
            runAction = chAction;
        }
    }

    return ok;
}

template<>
bool
gfxFont::InitFakeSmallCapsRun(gfxContext     *aContext,
                              gfxTextRun     *aTextRun,
                              const uint8_t  *aText,
                              uint32_t        aOffset,
                              uint32_t        aLength,
                              uint8_t         aMatchType,
                              uint16_t        aOrientation,
                              int32_t         aScript,
                              bool            aSyntheticLower,
                              bool            aSyntheticUpper)
{
    NS_ConvertASCIItoUTF16 unicodeString(reinterpret_cast<const char*>(aText),
                                         aLength);
    return InitFakeSmallCapsRun(aContext, aTextRun, static_cast<const char16_t*>(unicodeString.get()),
                                aOffset, aLength, aMatchType, aOrientation,
                                aScript, aSyntheticLower, aSyntheticUpper);
}

already_AddRefed<gfxFont>
gfxFont::GetSmallCapsFont()
{
    gfxFontStyle style(*GetStyle());
    style.size *= SMALL_CAPS_SCALE_FACTOR;
    style.variantCaps = NS_FONT_VARIANT_CAPS_NORMAL;
    gfxFontEntry* fe = GetFontEntry();
    bool needsBold = style.weight >= 600 && !fe->IsBold();
    return fe->FindOrMakeFont(&style, needsBold, mUnicodeRangeMap);
}

already_AddRefed<gfxFont>
gfxFont::GetSubSuperscriptFont(int32_t aAppUnitsPerDevPixel)
{
    gfxFontStyle style(*GetStyle());
    style.AdjustForSubSuperscript(aAppUnitsPerDevPixel);
    gfxFontEntry* fe = GetFontEntry();
    bool needsBold = style.weight >= 600 && !fe->IsBold();
    return fe->FindOrMakeFont(&style, needsBold, mUnicodeRangeMap);
}

gfxGlyphExtents *
gfxFont::GetOrCreateGlyphExtents(int32_t aAppUnitsPerDevUnit) {
    uint32_t i, count = mGlyphExtentsArray.Length();
    for (i = 0; i < count; ++i) {
        if (mGlyphExtentsArray[i]->GetAppUnitsPerDevUnit() == aAppUnitsPerDevUnit)
            return mGlyphExtentsArray[i];
    }
    gfxGlyphExtents *glyphExtents = new gfxGlyphExtents(aAppUnitsPerDevUnit);
    if (glyphExtents) {
        mGlyphExtentsArray.AppendElement(glyphExtents);
        
        
        glyphExtents->SetContainedGlyphWidthAppUnits(GetSpaceGlyph(), 0);
    }
    return glyphExtents;
}

void
gfxFont::SetupGlyphExtents(gfxContext *aContext,
                           uint32_t aGlyphID, bool aNeedTight,
                           gfxGlyphExtents *aExtents)
{
    gfxContextMatrixAutoSaveRestore matrixRestore(aContext);
    aContext->SetMatrix(gfxMatrix());

    gfxRect svgBounds;
    if (mFontEntry->TryGetSVGData(this) && mFontEntry->HasSVGGlyph(aGlyphID) &&
        mFontEntry->GetSVGGlyphExtents(aContext, aGlyphID, &svgBounds)) {
        gfxFloat d2a = aExtents->GetAppUnitsPerDevUnit();
        aExtents->SetTightGlyphExtents(aGlyphID,
                                       gfxRect(svgBounds.x * d2a,
                                               svgBounds.y * d2a,
                                               svgBounds.width * d2a,
                                               svgBounds.height * d2a));
        return;
    }

    cairo_glyph_t glyph;
    glyph.index = aGlyphID;
    glyph.x = 0;
    glyph.y = 0;
    cairo_text_extents_t extents;
    cairo_glyph_extents(aContext->GetCairo(), &glyph, 1, &extents);

    const Metrics& fontMetrics = GetMetrics(eHorizontal);
    int32_t appUnitsPerDevUnit = aExtents->GetAppUnitsPerDevUnit();
    if (!aNeedTight && extents.x_bearing >= 0 &&
        extents.y_bearing >= -fontMetrics.maxAscent &&
        extents.height + extents.y_bearing <= fontMetrics.maxDescent) {
        uint32_t appUnitsWidth =
            uint32_t(ceil((extents.x_bearing + extents.width)*appUnitsPerDevUnit));
        if (appUnitsWidth < gfxGlyphExtents::INVALID_WIDTH) {
            aExtents->SetContainedGlyphWidthAppUnits(aGlyphID, uint16_t(appUnitsWidth));
            return;
        }
    }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    if (!aNeedTight) {
        ++gGlyphExtentsSetupFallBackToTight;
    }
#endif

    gfxFloat d2a = appUnitsPerDevUnit;
    gfxRect bounds(extents.x_bearing*d2a, extents.y_bearing*d2a,
                   extents.width*d2a, extents.height*d2a);
    aExtents->SetTightGlyphExtents(aGlyphID, bounds);
}








bool
gfxFont::InitMetricsFromSfntTables(Metrics& aMetrics)
{
    mIsValid = false; 

    const uint32_t kHheaTableTag = TRUETYPE_TAG('h','h','e','a');
    const uint32_t kPostTableTag = TRUETYPE_TAG('p','o','s','t');
    const uint32_t kOS_2TableTag = TRUETYPE_TAG('O','S','/','2');

    uint32_t len;

    if (mFUnitsConvFactor == 0.0) {
        
        
        uint16_t unitsPerEm = GetFontEntry()->UnitsPerEm();
        if (unitsPerEm == gfxFontEntry::kInvalidUPEM) {
            return false;
        }
        mFUnitsConvFactor = GetAdjustedSize() / unitsPerEm;
    }

    
    gfxFontEntry::AutoTable hheaTable(mFontEntry, kHheaTableTag);
    if (!hheaTable) {
        return false; 
    }
    const MetricsHeader* hhea =
        reinterpret_cast<const MetricsHeader*>
            (hb_blob_get_data(hheaTable, &len));
    if (len < sizeof(MetricsHeader)) {
        return false;
    }

#define SET_UNSIGNED(field,src) aMetrics.field = uint16_t(src) * mFUnitsConvFactor
#define SET_SIGNED(field,src)   aMetrics.field = int16_t(src) * mFUnitsConvFactor

    SET_UNSIGNED(maxAdvance, hhea->advanceWidthMax);
    SET_SIGNED(maxAscent, hhea->ascender);
    SET_SIGNED(maxDescent, -int16_t(hhea->descender));
    SET_SIGNED(externalLeading, hhea->lineGap);

    
    gfxFontEntry::AutoTable postTable(mFontEntry, kPostTableTag);
    if (!postTable) {
        return true; 
    }
    const PostTable *post =
        reinterpret_cast<const PostTable*>(hb_blob_get_data(postTable, &len));
    if (len < offsetof(PostTable, underlineThickness) + sizeof(uint16_t)) {
        return true; 
    }

    SET_SIGNED(underlineOffset, post->underlinePosition);
    SET_UNSIGNED(underlineSize, post->underlineThickness);

    
    
    gfxFontEntry::AutoTable os2Table(mFontEntry, kOS_2TableTag);
    if (os2Table) {
        const OS2Table *os2 =
            reinterpret_cast<const OS2Table*>(hb_blob_get_data(os2Table, &len));
        
        
        if (len >= offsetof(OS2Table, sxHeight) + sizeof(int16_t) &&
            uint16_t(os2->version) >= 2 && int16_t(os2->sxHeight) > 0) {
            
            SET_SIGNED(xHeight, os2->sxHeight);
        }
        
        if (len >= offsetof(OS2Table, sTypoLineGap) + sizeof(int16_t)) {
            SET_SIGNED(aveCharWidth, os2->xAvgCharWidth);
            SET_SIGNED(strikeoutSize, os2->yStrikeoutSize);
            SET_SIGNED(strikeoutOffset, os2->yStrikeoutPosition);

            
            
            
            
            const uint16_t kUseTypoMetricsMask = 1 << 7;
            if ((uint16_t(os2->fsSelection) & kUseTypoMetricsMask) ||
                mFontEntry->HasFontTable(TRUETYPE_TAG('M','A','T','H'))) {
                SET_SIGNED(maxAscent, os2->sTypoAscender);
                SET_SIGNED(maxDescent, - int16_t(os2->sTypoDescender));
                SET_SIGNED(externalLeading, os2->sTypoLineGap);
            }
        }
    }

#undef SET_SIGNED
#undef SET_UNSIGNED

    mIsValid = true;

    return true;
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
    return floor(aValue/aFraction + 0.5) * aFraction;
}

void gfxFont::CalculateDerivedMetrics(Metrics& aMetrics)
{
    aMetrics.maxAscent =
        ceil(RoundToNearestMultiple(aMetrics.maxAscent, 1/1024.0));
    aMetrics.maxDescent =
        ceil(RoundToNearestMultiple(aMetrics.maxDescent, 1/1024.0));

    if (aMetrics.xHeight <= 0) {
        
        
        
        aMetrics.xHeight = aMetrics.maxAscent * DEFAULT_XHEIGHT_FACTOR;
    }

    aMetrics.maxHeight = aMetrics.maxAscent + aMetrics.maxDescent;

    if (aMetrics.maxHeight - aMetrics.emHeight > 0.0) {
        aMetrics.internalLeading = aMetrics.maxHeight - aMetrics.emHeight;
    } else {
        aMetrics.internalLeading = 0.0;
    }

    aMetrics.emAscent = aMetrics.maxAscent * aMetrics.emHeight
                            / aMetrics.maxHeight;
    aMetrics.emDescent = aMetrics.emHeight - aMetrics.emAscent;

    if (GetFontEntry()->IsFixedPitch()) {
        
        
        
        aMetrics.maxAdvance = aMetrics.aveCharWidth;
    }

    if (!aMetrics.strikeoutOffset) {
        aMetrics.strikeoutOffset = aMetrics.xHeight * 0.5;
    }
    if (!aMetrics.strikeoutSize) {
        aMetrics.strikeoutSize = aMetrics.underlineSize;
    }
}

void
gfxFont::SanitizeMetrics(gfxFont::Metrics *aMetrics, bool aIsBadUnderlineFont)
{
    
    
    if (mStyle.size == 0.0 || mStyle.sizeAdjust == 0.0) {
        memset(aMetrics, 0, sizeof(gfxFont::Metrics));
        return;
    }

    aMetrics->underlineSize = std::max(1.0, aMetrics->underlineSize);
    aMetrics->strikeoutSize = std::max(1.0, aMetrics->strikeoutSize);

    aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -1.0);

    if (aMetrics->maxAscent < 1.0) {
        
        aMetrics->underlineSize = 0;
        aMetrics->underlineOffset = 0;
        aMetrics->strikeoutSize = 0;
        aMetrics->strikeoutOffset = 0;
        return;
    }

    







    if (!mStyle.systemFont && aIsBadUnderlineFont) {
        
        
        aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -2.0);

        
        if (aMetrics->internalLeading + aMetrics->externalLeading > aMetrics->underlineSize) {
            aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -aMetrics->emDescent);
        } else {
            aMetrics->underlineOffset = std::min(aMetrics->underlineOffset,
                                               aMetrics->underlineSize - aMetrics->emDescent);
        }
    }
    
    
    else if (aMetrics->underlineSize - aMetrics->underlineOffset > aMetrics->maxDescent) {
        if (aMetrics->underlineSize > aMetrics->maxDescent)
            aMetrics->underlineSize = std::max(aMetrics->maxDescent, 1.0);
        
        aMetrics->underlineOffset = aMetrics->underlineSize - aMetrics->maxDescent;
    }

    
    
    
    gfxFloat halfOfStrikeoutSize = floor(aMetrics->strikeoutSize / 2.0 + 0.5);
    if (halfOfStrikeoutSize + aMetrics->strikeoutOffset > aMetrics->maxAscent) {
        if (aMetrics->strikeoutSize > aMetrics->maxAscent) {
            aMetrics->strikeoutSize = std::max(aMetrics->maxAscent, 1.0);
            halfOfStrikeoutSize = floor(aMetrics->strikeoutSize / 2.0 + 0.5);
        }
        gfxFloat ascent = floor(aMetrics->maxAscent + 0.5);
        aMetrics->strikeoutOffset = std::max(halfOfStrikeoutSize, ascent / 2.0);
    }

    
    if (aMetrics->underlineSize > aMetrics->maxAscent) {
        aMetrics->underlineSize = aMetrics->maxAscent;
    }
}










const gfxFont::Metrics*
gfxFont::CreateVerticalMetrics()
{
    const uint32_t kHheaTableTag = TRUETYPE_TAG('h','h','e','a');
    const uint32_t kVheaTableTag = TRUETYPE_TAG('v','h','e','a');
    const uint32_t kPostTableTag = TRUETYPE_TAG('p','o','s','t');
    const uint32_t kOS_2TableTag = TRUETYPE_TAG('O','S','/','2');
    uint32_t len;

    Metrics* metrics = new Metrics;
    ::memset(metrics, 0, sizeof(Metrics));

    
    
    metrics->emHeight = GetAdjustedSize();
    metrics->emAscent = metrics->emHeight / 2;
    metrics->emDescent = metrics->emHeight - metrics->emAscent;

    metrics->maxAscent = metrics->emAscent;
    metrics->maxDescent = metrics->emDescent;

    const float UNINITIALIZED_LEADING = -10000.0f;
    metrics->externalLeading = UNINITIALIZED_LEADING;

    if (mFUnitsConvFactor == 0.0) {
        uint16_t upem = GetFontEntry()->UnitsPerEm();
        if (upem != gfxFontEntry::kInvalidUPEM) {
            mFUnitsConvFactor = GetAdjustedSize() / upem;
        }
    }

#define SET_UNSIGNED(field,src) metrics->field = uint16_t(src) * mFUnitsConvFactor
#define SET_SIGNED(field,src)   metrics->field = int16_t(src) * mFUnitsConvFactor

    gfxFontEntry::AutoTable os2Table(mFontEntry, kOS_2TableTag);
    if (os2Table && mFUnitsConvFactor > 0.0) {
        const OS2Table *os2 =
            reinterpret_cast<const OS2Table*>(hb_blob_get_data(os2Table, &len));
        
        if (len >= offsetof(OS2Table, sTypoLineGap) + sizeof(int16_t)) {
            SET_SIGNED(strikeoutSize, os2->yStrikeoutSize);
            
            
            gfxFloat ascentDescent = gfxFloat(mFUnitsConvFactor) *
                (int16_t(os2->sTypoAscender) - int16_t(os2->sTypoDescender));
            metrics->aveCharWidth =
                std::max(metrics->emHeight, ascentDescent);
            
            
            
            gfxFloat halfCharWidth =
                int16_t(os2->xAvgCharWidth) * gfxFloat(mFUnitsConvFactor) / 2;
            metrics->maxAscent = std::max(metrics->maxAscent, halfCharWidth);
            metrics->maxDescent = std::max(metrics->maxDescent, halfCharWidth);
        }
    }

    
    
    if (!metrics->aveCharWidth) {
        gfxFontEntry::AutoTable hheaTable(mFontEntry, kHheaTableTag);
        if (hheaTable && mFUnitsConvFactor > 0.0) {
            const MetricsHeader* hhea =
                reinterpret_cast<const MetricsHeader*>
                    (hb_blob_get_data(hheaTable, &len));
            if (len >= sizeof(MetricsHeader)) {
                SET_SIGNED(aveCharWidth, int16_t(hhea->ascender) -
                                         int16_t(hhea->descender));
                metrics->maxAscent = metrics->aveCharWidth / 2;
                metrics->maxDescent =
                    metrics->aveCharWidth - metrics->maxAscent;
            }
        }
    }

    
    gfxFontEntry::AutoTable vheaTable(mFontEntry, kVheaTableTag);
    if (vheaTable && mFUnitsConvFactor > 0.0) {
        const MetricsHeader* vhea =
            reinterpret_cast<const MetricsHeader*>
                (hb_blob_get_data(vheaTable, &len));
        if (len >= sizeof(MetricsHeader)) {
            SET_UNSIGNED(maxAdvance, vhea->advanceWidthMax);
            
            
            gfxFloat halfExtent = 0.5 * gfxFloat(mFUnitsConvFactor) *
                (int16_t(vhea->ascender) + std::abs(int16_t(vhea->descender)));
            
            
            
            if (halfExtent > 0) {
                metrics->maxAscent = halfExtent;
                metrics->maxDescent = halfExtent;
                SET_SIGNED(externalLeading, vhea->lineGap);
            }
        }
    }

    
    
    
    
    
    if (!metrics->aveCharWidth ||
        metrics->externalLeading == UNINITIALIZED_LEADING) {
        const Metrics& horizMetrics = GetHorizontalMetrics();
        if (!metrics->aveCharWidth) {
            metrics->aveCharWidth = horizMetrics.maxAscent + horizMetrics.maxDescent;
        }
        if (metrics->externalLeading == UNINITIALIZED_LEADING) {
            metrics->externalLeading = horizMetrics.externalLeading;
        }
    }

    
    gfxFontEntry::AutoTable postTable(mFontEntry, kPostTableTag);
    if (postTable) {
        const PostTable *post =
            reinterpret_cast<const PostTable*>(hb_blob_get_data(postTable,
                                                                &len));
        if (len >= offsetof(PostTable, underlineThickness) +
                       sizeof(uint16_t)) {
            SET_UNSIGNED(underlineSize, post->underlineThickness);
            
            if (!metrics->strikeoutSize) {
                metrics->strikeoutSize = metrics->underlineSize;
            }
        }
    }

#undef SET_UNSIGNED
#undef SET_SIGNED

    
    
    
    metrics->maxAdvance = std::max(metrics->maxAdvance, metrics->aveCharWidth);

    
    
    
    
    metrics->underlineSize = std::max(1.0, metrics->underlineSize);
    metrics->underlineOffset = - metrics->maxDescent - metrics->underlineSize;

    metrics->strikeoutSize = std::max(1.0, metrics->strikeoutSize);
    metrics->strikeoutOffset = - 0.5 * metrics->strikeoutSize;

    
    metrics->spaceWidth = metrics->aveCharWidth;
    metrics->zeroOrAveCharWidth = metrics->aveCharWidth;
    metrics->maxHeight = metrics->maxAscent + metrics->maxDescent;
    metrics->xHeight = metrics->emHeight / 2;

    return metrics;
}

gfxFloat
gfxFont::SynthesizeSpaceWidth(uint32_t aCh)
{
    
    
    
    switch (aCh) {
    case 0x2000:                                 
    case 0x2002: return GetAdjustedSize() / 2;   
    case 0x2001:                                 
    case 0x2003: return GetAdjustedSize();       
    case 0x2004: return GetAdjustedSize() / 3;   
    case 0x2005: return GetAdjustedSize() / 4;   
    case 0x2006: return GetAdjustedSize() / 6;   
    case 0x2007: return GetMetrics(eHorizontal).zeroOrAveCharWidth; 
    case 0x2008: return GetMetrics(eHorizontal).spaceWidth; 
    case 0x2009: return GetAdjustedSize() / 5;   
    case 0x200a: return GetAdjustedSize() / 10;  
    case 0x202f: return GetAdjustedSize() / 5;   
    default: return -1.0;
    }
}

void
gfxFont::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const
{
    for (uint32_t i = 0; i < mGlyphExtentsArray.Length(); ++i) {
        aSizes->mFontInstances +=
            mGlyphExtentsArray[i]->SizeOfIncludingThis(aMallocSizeOf);
    }
    if (mWordCache) {
        aSizes->mShapedWords += mWordCache->SizeOfIncludingThis(aMallocSizeOf);
    }
}

void
gfxFont::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}

void
gfxFont::AddGlyphChangeObserver(GlyphChangeObserver *aObserver)
{
    if (!mGlyphChangeObservers) {
        mGlyphChangeObservers = new nsTHashtable<nsPtrHashKey<GlyphChangeObserver> >;
    }
    mGlyphChangeObservers->PutEntry(aObserver);
}

void
gfxFont::RemoveGlyphChangeObserver(GlyphChangeObserver *aObserver)
{
    NS_ASSERTION(mGlyphChangeObservers, "No observers registered");
    NS_ASSERTION(mGlyphChangeObservers->Contains(aObserver), "Observer not registered");
    mGlyphChangeObservers->RemoveEntry(aObserver);
}


#define DEFAULT_PIXEL_FONT_SIZE 16.0f

 uint32_t
gfxFontStyle::ParseFontLanguageOverride(const nsString& aLangTag)
{
  if (!aLangTag.Length() || aLangTag.Length() > 4) {
    return NO_FONT_LANGUAGE_OVERRIDE;
  }
  uint32_t index, result = 0;
  for (index = 0; index < aLangTag.Length(); ++index) {
    char16_t ch = aLangTag[index];
    if (!nsCRT::IsAscii(ch)) { 
      return NO_FONT_LANGUAGE_OVERRIDE;
    }
    result = (result << 8) + ch;
  }
  while (index++ < 4) {
    result = (result << 8) + 0x20;
  }
  return result;
}

gfxFontStyle::gfxFontStyle() :
    language(nsGkAtoms::x_western),
    size(DEFAULT_PIXEL_FONT_SIZE), sizeAdjust(-1.0f), baselineOffset(0.0f),
    languageOverride(NO_FONT_LANGUAGE_OVERRIDE),
    weight(NS_FONT_WEIGHT_NORMAL), stretch(NS_FONT_STRETCH_NORMAL),
    systemFont(true), printerFont(false), useGrayscaleAntialiasing(false),
    style(NS_FONT_STYLE_NORMAL),
    allowSyntheticWeight(true), allowSyntheticStyle(true),
    noFallbackVariantFeatures(true),
    explicitLanguage(false),
    variantCaps(NS_FONT_VARIANT_CAPS_NORMAL),
    variantSubSuper(NS_FONT_VARIANT_POSITION_NORMAL)
{
}

gfxFontStyle::gfxFontStyle(uint8_t aStyle, uint16_t aWeight, int16_t aStretch,
                           gfxFloat aSize,
                           nsIAtom *aLanguage, bool aExplicitLanguage,
                           float aSizeAdjust, bool aSystemFont,
                           bool aPrinterFont,
                           bool aAllowWeightSynthesis,
                           bool aAllowStyleSynthesis,
                           const nsString& aLanguageOverride):
    language(aLanguage),
    size(aSize), sizeAdjust(aSizeAdjust), baselineOffset(0.0f),
    languageOverride(ParseFontLanguageOverride(aLanguageOverride)),
    weight(aWeight), stretch(aStretch),
    systemFont(aSystemFont), printerFont(aPrinterFont),
    useGrayscaleAntialiasing(false),
    style(aStyle),
    allowSyntheticWeight(aAllowWeightSynthesis),
    allowSyntheticStyle(aAllowStyleSynthesis),
    noFallbackVariantFeatures(true),
    explicitLanguage(aExplicitLanguage),
    variantCaps(NS_FONT_VARIANT_CAPS_NORMAL),
    variantSubSuper(NS_FONT_VARIANT_POSITION_NORMAL)
{
    MOZ_ASSERT(!mozilla::IsNaN(size));
    MOZ_ASSERT(!mozilla::IsNaN(sizeAdjust));

    if (weight > 900)
        weight = 900;
    if (weight < 100)
        weight = 100;

    if (size >= FONT_MAX_SIZE) {
        size = FONT_MAX_SIZE;
        sizeAdjust = -1.0f;
    } else if (size < 0.0) {
        NS_WARNING("negative font size");
        size = 0.0;
    }

    if (!language) {
        NS_WARNING("null language");
        language = nsGkAtoms::x_western;
    }
}

gfxFontStyle::gfxFontStyle(const gfxFontStyle& aStyle) :
    language(aStyle.language),
    featureValueLookup(aStyle.featureValueLookup),
    size(aStyle.size), sizeAdjust(aStyle.sizeAdjust),
    baselineOffset(aStyle.baselineOffset),
    languageOverride(aStyle.languageOverride),
    weight(aStyle.weight), stretch(aStyle.stretch),
    systemFont(aStyle.systemFont), printerFont(aStyle.printerFont),
    useGrayscaleAntialiasing(aStyle.useGrayscaleAntialiasing),
    style(aStyle.style),
    allowSyntheticWeight(aStyle.allowSyntheticWeight),
    allowSyntheticStyle(aStyle.allowSyntheticStyle),
    noFallbackVariantFeatures(aStyle.noFallbackVariantFeatures),
    explicitLanguage(aStyle.explicitLanguage),
    variantCaps(aStyle.variantCaps),
    variantSubSuper(aStyle.variantSubSuper)
{
    featureSettings.AppendElements(aStyle.featureSettings);
    alternateValues.AppendElements(aStyle.alternateValues);
}

int8_t
gfxFontStyle::ComputeWeight() const
{
    int8_t baseWeight = (weight + 50) / 100;

    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    return baseWeight;
}

void
gfxFontStyle::AdjustForSubSuperscript(int32_t aAppUnitsPerDevPixel)
{
    NS_PRECONDITION(variantSubSuper != NS_FONT_VARIANT_POSITION_NORMAL &&
                    baselineOffset == 0,
                    "can't adjust this style for sub/superscript");

    
    if (variantSubSuper == NS_FONT_VARIANT_POSITION_SUPER) {
        baselineOffset = size * -NS_FONT_SUPERSCRIPT_OFFSET_RATIO;
    } else {
        baselineOffset = size * NS_FONT_SUBSCRIPT_OFFSET_RATIO;
    }

    
    float cssSize = size * aAppUnitsPerDevPixel / AppUnitsPerCSSPixel();
    if (cssSize < NS_FONT_SUB_SUPER_SMALL_SIZE) {
        size *= NS_FONT_SUB_SUPER_SIZE_RATIO_SMALL;
    } else if (cssSize >= NS_FONT_SUB_SUPER_LARGE_SIZE) {
        size *= NS_FONT_SUB_SUPER_SIZE_RATIO_LARGE;
    } else {
        gfxFloat t = (cssSize - NS_FONT_SUB_SUPER_SMALL_SIZE) /
                         (NS_FONT_SUB_SUPER_LARGE_SIZE -
                          NS_FONT_SUB_SUPER_SMALL_SIZE);
        size *= (1.0 - t) * NS_FONT_SUB_SUPER_SIZE_RATIO_SMALL +
                    t * NS_FONT_SUB_SUPER_SIZE_RATIO_LARGE;
    }

    
    variantSubSuper = NS_FONT_VARIANT_POSITION_NORMAL;
}
