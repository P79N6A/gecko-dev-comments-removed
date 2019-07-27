




#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "gfxTypes.h"
#include "gfxFontEntry.h"
#include "nsString.h"
#include "gfxPoint.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "gfxRect.h"
#include "nsExpirationTracker.h"
#include "gfxPlatform.h"
#include "nsIAtom.h"
#include "mozilla/HashFunctions.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Attributes.h"
#include <algorithm>
#include "DrawMode.h"
#include "nsDataHashtable.h"
#include "harfbuzz/hb.h"
#include "mozilla/gfx/2D.h"

typedef struct _cairo_scaled_font cairo_scaled_font_t;


#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxTextRun;
class gfxFont;
class gfxGlyphExtents;
class gfxShapedText;
class gfxShapedWord;
class gfxSkipChars;
class gfxTextContextPaint;

#define FONT_MAX_SIZE                  2000.0

#define NO_FONT_LANGUAGE_OVERRIDE      0

#define SMALL_CAPS_SCALE_FACTOR        0.8



#ifdef XP_WIN
#define OBLIQUE_SKEW_FACTOR  0.3
#else
#define OBLIQUE_SKEW_FACTOR  0.25
#endif

struct gfxTextRunDrawCallbacks;

namespace mozilla {
namespace gfx {
class GlyphRenderingOptions;
}
}

struct gfxFontStyle {
    gfxFontStyle();
    gfxFontStyle(uint8_t aStyle, uint16_t aWeight, int16_t aStretch,
                 gfxFloat aSize, nsIAtom *aLanguage, bool aExplicitLanguage,
                 float aSizeAdjust, bool aSystemFont,
                 bool aPrinterFont,
                 bool aWeightSynthesis, bool aStyleSynthesis,
                 const nsString& aLanguageOverride);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    
    
    nsRefPtr<nsIAtom> language;

    
    
    

    
    nsTArray<gfxFontFeature> featureSettings;

    
    
    

    
    nsTArray<gfxAlternateValue> alternateValues;

    
    nsRefPtr<gfxFontFeatureValueSet> featureValueLookup;

    
    gfxFloat size;

    
    
    
    
    float sizeAdjust;

    
    float baselineOffset;

    
    
    
    
    
    
    
    
    
    
    uint32_t languageOverride;

    
    uint16_t weight;

    
    
    int8_t stretch;

    
    
    
    bool systemFont : 1;

    
    bool printerFont : 1;

    
    bool useGrayscaleAntialiasing : 1;

    
    uint8_t style : 2;

    
    bool allowSyntheticWeight : 1;
    bool allowSyntheticStyle : 1;

    
    
    bool noFallbackVariantFeatures : 1;

    
    
    bool explicitLanguage : 1;

    
    uint8_t variantCaps;

    
    uint8_t variantSubSuper;

    
    
    gfxFloat GetAdjustedSize(gfxFloat aspect) const {
        NS_ASSERTION(sizeAdjust >= 0.0, "Not meant to be called when sizeAdjust = -1.0");
        gfxFloat adjustedSize = std::max(NS_round(size*(sizeAdjust/aspect)), 1.0);
        return std::min(adjustedSize, FONT_MAX_SIZE);
    }

    PLDHashNumber Hash() const {
        return ((style + (systemFont << 7) +
            (weight << 8)) + uint32_t(size*1000) + uint32_t(sizeAdjust*1000)) ^
            nsISupportsHashKey::HashKey(language);
    }

    int8_t ComputeWeight() const;

    
    
    void AdjustForSubSuperscript(int32_t aAppUnitsPerDevPixel);

    bool Equals(const gfxFontStyle& other) const {
        return
            (*reinterpret_cast<const uint64_t*>(&size) ==
             *reinterpret_cast<const uint64_t*>(&other.size)) &&
            (style == other.style) &&
            (variantCaps == other.variantCaps) &&
            (variantSubSuper == other.variantSubSuper) &&
            (allowSyntheticWeight == other.allowSyntheticWeight) &&
            (allowSyntheticStyle == other.allowSyntheticStyle) &&
            (systemFont == other.systemFont) &&
            (printerFont == other.printerFont) &&
            (useGrayscaleAntialiasing == other.useGrayscaleAntialiasing) &&
            (explicitLanguage == other.explicitLanguage) &&
            (weight == other.weight) &&
            (stretch == other.stretch) &&
            (language == other.language) &&
            (baselineOffset == other.baselineOffset) &&
            (*reinterpret_cast<const uint32_t*>(&sizeAdjust) ==
             *reinterpret_cast<const uint32_t*>(&other.sizeAdjust)) &&
            (featureSettings == other.featureSettings) &&
            (languageOverride == other.languageOverride) &&
            (alternateValues == other.alternateValues) &&
            (featureValueLookup == other.featureValueLookup);
    }

    static void ParseFontFeatureSettings(const nsString& aFeatureString,
                                         nsTArray<gfxFontFeature>& aFeatures);

    static uint32_t ParseFontLanguageOverride(const nsString& aLangTag);
};

struct gfxTextRange {
    enum {
        
        kFontGroup      = 0x0001,
        kPrefsFallback  = 0x0002,
        kSystemFallback = 0x0004
    };
    gfxTextRange(uint32_t aStart, uint32_t aEnd,
                 gfxFont* aFont, uint8_t aMatchType,
                 uint16_t aOrientation)
        : start(aStart),
          end(aEnd),
          font(aFont),
          matchType(aMatchType),
          orientation(aOrientation)
    { }
    uint32_t Length() const { return end - start; }
    uint32_t start, end;
    nsRefPtr<gfxFont> font;
    uint8_t matchType;
    uint16_t orientation;
};



























struct FontCacheSizes {
    FontCacheSizes()
        : mFontInstances(0), mShapedWords(0)
    { }

    size_t mFontInstances; 
    size_t mShapedWords; 
};

class gfxFontCache final : public nsExpirationTracker<gfxFont,3> {
public:
    enum {
        FONT_TIMEOUT_SECONDS = 10,
        SHAPED_WORD_TIMEOUT_SECONDS = 60
    };

    gfxFontCache();
    ~gfxFontCache();

    



    static gfxFontCache* GetCache() {
        return gGlobalCache;
    }

    static nsresult Init();
    
    static void Shutdown();

    
    
    already_AddRefed<gfxFont>
    Lookup(const gfxFontEntry* aFontEntry,
           const gfxFontStyle* aStyle,
           const gfxCharacterMap* aUnicodeRangeMap = nullptr);

    
    
    
    
    void AddNew(gfxFont *aFont);

    
    
    
    void NotifyReleased(gfxFont *aFont);

    
    
    virtual void NotifyExpired(gfxFont *aFont) override;

    
    
    
    void Flush() {
        mFonts.Clear();
        AgeAllGenerations();
    }

    void FlushShapedWordCaches() {
        mFonts.EnumerateEntries(ClearCachedWordsForFont, nullptr);
    }

    void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const;
    void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const;

protected:
    class MemoryReporter final : public nsIMemoryReporter
    {
        ~MemoryReporter() {}
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIMEMORYREPORTER
    };

    
    class Observer final
        : public nsIObserver
    {
        ~Observer() {}
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER
    };

    void DestroyFont(gfxFont *aFont);

    static gfxFontCache *gGlobalCache;

    struct Key {
        const gfxFontEntry* mFontEntry;
        const gfxFontStyle* mStyle;
        const gfxCharacterMap* mUnicodeRangeMap;
        Key(const gfxFontEntry* aFontEntry, const gfxFontStyle* aStyle,
            const gfxCharacterMap* aUnicodeRangeMap)
            : mFontEntry(aFontEntry), mStyle(aStyle),
              mUnicodeRangeMap(aUnicodeRangeMap)
        {}
    };

    class HashEntry : public PLDHashEntryHdr {
    public:
        typedef const Key& KeyType;
        typedef const Key* KeyTypePointer;

        
        
        explicit HashEntry(KeyTypePointer aStr) : mFont(nullptr) { }
        HashEntry(const HashEntry& toCopy) : mFont(toCopy.mFont) { }
        ~HashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return mozilla::HashGeneric(aKey->mStyle->Hash(), aKey->mFontEntry,
                                        aKey->mUnicodeRangeMap);
        }
        enum { ALLOW_MEMMOVE = true };

        gfxFont* mFont;
    };

    static size_t AddSizeOfFontEntryExcludingThis(HashEntry* aHashEntry,
                                                  mozilla::MallocSizeOf aMallocSizeOf,
                                                  void* aUserArg);

    nsTHashtable<HashEntry> mFonts;

    static PLDHashOperator ClearCachedWordsForFont(HashEntry* aHashEntry, void*);
    static PLDHashOperator AgeCachedWordsForFont(HashEntry* aHashEntry, void*);
    static void WordCacheExpirationTimerCallback(nsITimer* aTimer, void* aCache);
    nsCOMPtr<nsITimer>      mWordCacheExpirationTimer;
};

class gfxTextPerfMetrics {
public:

    struct TextCounts {
        uint32_t    numContentTextRuns;
        uint32_t    numChromeTextRuns;
        uint32_t    numChars;
        uint32_t    maxTextRunLen;
        uint32_t    wordCacheSpaceRules;
        uint32_t    wordCacheLong;
        uint32_t    wordCacheHit;
        uint32_t    wordCacheMiss;
        uint32_t    fallbackPrefs;
        uint32_t    fallbackSystem;
        uint32_t    textrunConst;
        uint32_t    textrunDestr;
    };

    uint32_t reflowCount;

    
    TextCounts current;

    
    TextCounts cumulative;

    gfxTextPerfMetrics() {
        memset(this, 0, sizeof(gfxTextPerfMetrics));
    }

    
    void Accumulate() {
        if (current.numChars == 0) {
            return;
        }
        cumulative.numContentTextRuns += current.numContentTextRuns;
        cumulative.numChromeTextRuns += current.numChromeTextRuns;
        cumulative.numChars += current.numChars;
        if (current.maxTextRunLen > cumulative.maxTextRunLen) {
            cumulative.maxTextRunLen = current.maxTextRunLen;
        }
        cumulative.wordCacheSpaceRules += current.wordCacheSpaceRules;
        cumulative.wordCacheLong += current.wordCacheLong;
        cumulative.wordCacheHit += current.wordCacheHit;
        cumulative.wordCacheMiss += current.wordCacheMiss;
        cumulative.fallbackPrefs += current.fallbackPrefs;
        cumulative.fallbackSystem += current.fallbackSystem;
        cumulative.textrunConst += current.textrunConst;
        cumulative.textrunDestr += current.textrunDestr;
        memset(&current, 0, sizeof(current));
    }
};

class gfxTextRunFactory {
    NS_INLINE_DECL_REFCOUNTING(gfxTextRunFactory)

public:
    
    
    
    enum {
        CACHE_TEXT_FLAGS    = 0xF0000000,
        USER_TEXT_FLAGS     = 0x0FFF0000,
        TEXTRUN_TEXT_FLAGS  = 0x0000FFFF,
        SETTABLE_FLAGS      = CACHE_TEXT_FLAGS | USER_TEXT_FLAGS,

        



        TEXT_IS_PERSISTENT           = 0x0001,
        


        TEXT_IS_ASCII                = 0x0002,
        


        TEXT_IS_RTL                  = 0x0004,
        



        TEXT_ENABLE_SPACING          = 0x0008,
        



        TEXT_ENABLE_HYPHEN_BREAKS    = 0x0010,
        



        TEXT_IS_8BIT                 = 0x0020,
        






        TEXT_NEED_BOUNDING_BOX       = 0x0040,
        



        TEXT_DISABLE_OPTIONAL_LIGATURES = 0x0080,
        




        TEXT_OPTIMIZE_SPEED          = 0x0100,
        






        TEXT_RUN_SIZE_ACCOUNTED      = 0x0200,
        



        TEXT_HIDE_CONTROL_CHARACTERS = 0x0400,

        

















        TEXT_ORIENT_MASK                    = 0xF000,
        TEXT_ORIENT_HORIZONTAL              = 0x0000,
        TEXT_ORIENT_VERTICAL_UPRIGHT        = 0x1000,
        TEXT_ORIENT_VERTICAL_SIDEWAYS_RIGHT = 0x2000,
        TEXT_ORIENT_VERTICAL_SIDEWAYS_LEFT  = 0x4000,
        TEXT_ORIENT_VERTICAL_MIXED          = 0x8000,

        




        TEXT_TRAILING_ARABICCHAR = 0x20000000,
        




        TEXT_INCOMING_ARABICCHAR = 0x40000000,

        
        TEXT_USE_MATH_SCRIPT = 0x80000000,

        TEXT_UNUSED_FLAGS = 0x10000000
    };

    


    struct Parameters {
        
        gfxContext   *mContext;
        
        void         *mUserData;
        
        
        
        gfxSkipChars *mSkipChars;
        
        
        uint32_t     *mInitialBreaks;
        uint32_t      mInitialBreakCount;
        
        int32_t       mAppUnitsPerDevUnit;
    };

protected:
    
    virtual ~gfxTextRunFactory() {}
};



















class gfxFontShaper {
public:
    explicit gfxFontShaper(gfxFont *aFont)
        : mFont(aFont)
    {
        NS_ASSERTION(aFont, "shaper requires a valid font!");
    }

    virtual ~gfxFontShaper() { }

    
    
    
    virtual bool ShapeText(gfxContext     *aContext,
                           const char16_t *aText,
                           uint32_t        aOffset,
                           uint32_t        aLength,
                           int32_t         aScript,
                           bool            aVertical,
                           gfxShapedText  *aShapedText) = 0;

    gfxFont *GetFont() const { return mFont; }

    static void
    MergeFontFeatures(const gfxFontStyle *aStyle,
                      const nsTArray<gfxFontFeature>& aFontFeatures,
                      bool aDisableLigatures,
                      const nsAString& aFamilyName,
                      bool aAddSmallCaps,
                      PLDHashOperator (*aHandleFeature)(const uint32_t&,
                                                        uint32_t&, void*),
                      void* aHandleFeatureData);

protected:
    
    gfxFont * mFont;
};





















class gfxShapedText
{
public:
    gfxShapedText(uint32_t aLength, uint32_t aFlags,
                  int32_t aAppUnitsPerDevUnit)
        : mLength(aLength)
        , mFlags(aFlags)
        , mAppUnitsPerDevUnit(aAppUnitsPerDevUnit)
    { }

    virtual ~gfxShapedText() { }

    














    class CompressedGlyph {
    public:
        CompressedGlyph() { mValue = 0; }

        enum {
            
            
            
            
            
            FLAG_IS_SIMPLE_GLYPH  = 0x80000000U,

            
            
            
            FLAGS_CAN_BREAK_BEFORE = 0x60000000U,

            FLAGS_CAN_BREAK_SHIFT = 29,
            FLAG_BREAK_TYPE_NONE   = 0,
            FLAG_BREAK_TYPE_NORMAL = 1,
            FLAG_BREAK_TYPE_HYPHEN = 2,

            FLAG_CHAR_IS_SPACE     = 0x10000000U,

            
            ADVANCE_MASK  = 0x0FFF0000U,
            ADVANCE_SHIFT = 16,

            GLYPH_MASK = 0x0000FFFFU,

            
            
            

            
            
            
            
            
            FLAG_NOT_MISSING              = 0x01,
            FLAG_NOT_CLUSTER_START        = 0x02,
            FLAG_NOT_LIGATURE_GROUP_START = 0x04,

            FLAG_CHAR_IS_TAB              = 0x08,
            FLAG_CHAR_IS_NEWLINE          = 0x10,
            FLAG_CHAR_IS_LOW_SURROGATE    = 0x20,
            CHAR_IDENTITY_FLAGS_MASK      = 0x38,

            GLYPH_COUNT_MASK = 0x00FFFF00U,
            GLYPH_COUNT_SHIFT = 8
        };

        
        
        
        

        
        static bool IsSimpleGlyphID(uint32_t aGlyph) {
            return (aGlyph & GLYPH_MASK) == aGlyph;
        }
        
        
        static bool IsSimpleAdvance(uint32_t aAdvance) {
            return (aAdvance & (ADVANCE_MASK >> ADVANCE_SHIFT)) == aAdvance;
        }

        bool IsSimpleGlyph() const { return (mValue & FLAG_IS_SIMPLE_GLYPH) != 0; }
        uint32_t GetSimpleAdvance() const { return (mValue & ADVANCE_MASK) >> ADVANCE_SHIFT; }
        uint32_t GetSimpleGlyph() const { return mValue & GLYPH_MASK; }

        bool IsMissing() const { return (mValue & (FLAG_NOT_MISSING|FLAG_IS_SIMPLE_GLYPH)) == 0; }
        bool IsClusterStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_CLUSTER_START);
        }
        bool IsLigatureGroupStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_LIGATURE_GROUP_START);
        }
        bool IsLigatureContinuation() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) == 0 &&
                (mValue & (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING)) ==
                    (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING);
        }

        
        
        
        bool CharIsSpace() const {
            return (mValue & FLAG_CHAR_IS_SPACE) != 0;
        }

        bool CharIsTab() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_TAB) != 0;
        }
        bool CharIsNewline() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_NEWLINE) != 0;
        }
        bool CharIsLowSurrogate() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_LOW_SURROGATE) != 0;
        }

        uint32_t CharIdentityFlags() const {
            return IsSimpleGlyph() ? 0 : (mValue & CHAR_IDENTITY_FLAGS_MASK);
        }

        void SetClusterStart(bool aIsClusterStart) {
            NS_ASSERTION(!IsSimpleGlyph(),
                         "can't call SetClusterStart on simple glyphs");
            if (aIsClusterStart) {
                mValue &= ~FLAG_NOT_CLUSTER_START;
            } else {
                mValue |= FLAG_NOT_CLUSTER_START;
            }
        }

        uint8_t CanBreakBefore() const {
            return (mValue & FLAGS_CAN_BREAK_BEFORE) >> FLAGS_CAN_BREAK_SHIFT;
        }
        
        uint32_t SetCanBreakBefore(uint8_t aCanBreakBefore) {
            NS_ASSERTION(aCanBreakBefore <= 2,
                         "Bogus break-before value!");
            uint32_t breakMask = (uint32_t(aCanBreakBefore) << FLAGS_CAN_BREAK_SHIFT);
            uint32_t toggle = breakMask ^ (mValue & FLAGS_CAN_BREAK_BEFORE);
            mValue ^= toggle;
            return toggle;
        }

        CompressedGlyph& SetSimpleGlyph(uint32_t aAdvanceAppUnits, uint32_t aGlyph) {
            NS_ASSERTION(IsSimpleAdvance(aAdvanceAppUnits), "Advance overflow");
            NS_ASSERTION(IsSimpleGlyphID(aGlyph), "Glyph overflow");
            NS_ASSERTION(!CharIdentityFlags(), "Char identity flags lost");
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_CHAR_IS_SPACE)) |
                FLAG_IS_SIMPLE_GLYPH |
                (aAdvanceAppUnits << ADVANCE_SHIFT) | aGlyph;
            return *this;
        }
        CompressedGlyph& SetComplex(bool aClusterStart, bool aLigatureStart,
                uint32_t aGlyphCount) {
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_CHAR_IS_SPACE)) |
                FLAG_NOT_MISSING |
                CharIdentityFlags() |
                (aClusterStart ? 0 : FLAG_NOT_CLUSTER_START) |
                (aLigatureStart ? 0 : FLAG_NOT_LIGATURE_GROUP_START) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        



        CompressedGlyph& SetMissing(uint32_t aGlyphCount) {
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_NOT_CLUSTER_START |
                                FLAG_CHAR_IS_SPACE)) |
                CharIdentityFlags() |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        uint32_t GetGlyphCount() const {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            return (mValue & GLYPH_COUNT_MASK) >> GLYPH_COUNT_SHIFT;
        }

        void SetIsSpace() {
            mValue |= FLAG_CHAR_IS_SPACE;
        }
        void SetIsTab() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_TAB;
        }
        void SetIsNewline() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_NEWLINE;
        }
        void SetIsLowSurrogate() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_LOW_SURROGATE;
        }

    private:
        uint32_t mValue;
    };

    
    
    virtual CompressedGlyph *GetCharacterGlyphs() = 0;

    



    struct DetailedGlyph {
        

        uint32_t mGlyphID;
        


   
        int32_t  mAdvance;
        float    mXOffset, mYOffset;
    };

    void SetGlyphs(uint32_t aCharIndex, CompressedGlyph aGlyph,
                   const DetailedGlyph *aGlyphs);

    void SetMissingGlyph(uint32_t aIndex, uint32_t aChar, gfxFont *aFont);

    void SetIsSpace(uint32_t aIndex) {
        GetCharacterGlyphs()[aIndex].SetIsSpace();
    }

    void SetIsLowSurrogate(uint32_t aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nullptr);
        GetCharacterGlyphs()[aIndex].SetIsLowSurrogate();
    }

    bool HasDetailedGlyphs() const {
        return mDetailedGlyphs != nullptr;
    }

    bool IsLigatureGroupStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return GetCharacterGlyphs()[aPos].IsLigatureGroupStart();
    }

    
    
    
    DetailedGlyph *GetDetailedGlyphs(uint32_t aCharIndex) {
        NS_ASSERTION(GetCharacterGlyphs() && HasDetailedGlyphs() &&
                     !GetCharacterGlyphs()[aCharIndex].IsSimpleGlyph() &&
                     GetCharacterGlyphs()[aCharIndex].GetGlyphCount() > 0,
                     "invalid use of GetDetailedGlyphs; check the caller!");
        return mDetailedGlyphs->Get(aCharIndex);
    }

    void AdjustAdvancesForSyntheticBold(float aSynBoldOffset,
                                        uint32_t aOffset, uint32_t aLength);

    
    
    
    void SetupClusterBoundaries(uint32_t         aOffset,
                                const char16_t *aString,
                                uint32_t         aLength);
    
    
    void SetupClusterBoundaries(uint32_t       aOffset,
                                const uint8_t *aString,
                                uint32_t       aLength);

    uint32_t GetFlags() const {
        return mFlags;
    }

    bool IsVertical() const {
        return (GetFlags() & gfxTextRunFactory::TEXT_ORIENT_MASK) !=
                gfxTextRunFactory::TEXT_ORIENT_HORIZONTAL;
    }

    bool UseCenterBaseline() const {
        uint32_t orient = GetFlags() & gfxTextRunFactory::TEXT_ORIENT_MASK;
        return orient == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_MIXED ||
               orient == gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT;
    }

    bool IsRightToLeft() const {
        return (GetFlags() & gfxTextRunFactory::TEXT_IS_RTL) != 0;
    }

    gfxFloat GetDirection() const {
        return IsRightToLeft() ? -1.0f : 1.0f;
    }

    bool DisableLigatures() const {
        return (GetFlags() &
                gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;
    }

    bool TextIs8Bit() const {
        return (GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) != 0;
    }

    int32_t GetAppUnitsPerDevUnit() const {
        return mAppUnitsPerDevUnit;
    }

    uint32_t GetLength() const {
        return mLength;
    }

    bool FilterIfIgnorable(uint32_t aIndex, uint32_t aCh);

protected:
    
    DetailedGlyph *AllocateDetailedGlyphs(uint32_t aCharIndex,
                                          uint32_t aCount);

    
    
    
    
    
    
    class DetailedGlyphStore {
    public:
        DetailedGlyphStore()
            : mLastUsed(0)
        { }

        
        
        
        
        
        
        
        
        
        
        
        
        DetailedGlyph* Get(uint32_t aOffset) {
            NS_ASSERTION(mOffsetToIndex.Length() > 0,
                         "no detailed glyph records!");
            DetailedGlyph* details = mDetails.Elements();
            
            if (mLastUsed < mOffsetToIndex.Length() - 1 &&
                aOffset == mOffsetToIndex[mLastUsed + 1].mOffset) {
                ++mLastUsed;
            } else if (aOffset == mOffsetToIndex[0].mOffset) {
                mLastUsed = 0;
            } else if (aOffset == mOffsetToIndex[mLastUsed].mOffset) {
                
            } else if (mLastUsed > 0 &&
                       aOffset == mOffsetToIndex[mLastUsed - 1].mOffset) {
                --mLastUsed;
            } else {
                mLastUsed =
                    mOffsetToIndex.BinaryIndexOf(aOffset, CompareToOffset());
            }
            NS_ASSERTION(mLastUsed != nsTArray<DGRec>::NoIndex,
                         "detailed glyph record missing!");
            return details + mOffsetToIndex[mLastUsed].mIndex;
        }

        DetailedGlyph* Allocate(uint32_t aOffset, uint32_t aCount) {
            uint32_t detailIndex = mDetails.Length();
            DetailedGlyph *details = mDetails.AppendElements(aCount);
            
            
            
            
            if (mOffsetToIndex.Length() == 0 ||
                aOffset > mOffsetToIndex[mOffsetToIndex.Length() - 1].mOffset) {
                mOffsetToIndex.AppendElement(DGRec(aOffset, detailIndex));
            } else {
                mOffsetToIndex.InsertElementSorted(DGRec(aOffset, detailIndex),
                                                   CompareRecordOffsets());
            }
            return details;
        }

        size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) {
            return aMallocSizeOf(this) +
                mDetails.SizeOfExcludingThis(aMallocSizeOf) +
                mOffsetToIndex.SizeOfExcludingThis(aMallocSizeOf);
        }

    private:
        struct DGRec {
            DGRec(const uint32_t& aOffset, const uint32_t& aIndex)
                : mOffset(aOffset), mIndex(aIndex) { }
            uint32_t mOffset; 
            uint32_t mIndex;  
        };

        struct CompareToOffset {
            bool Equals(const DGRec& a, const uint32_t& b) const {
                return a.mOffset == b;
            }
            bool LessThan(const DGRec& a, const uint32_t& b) const {
                return a.mOffset < b;
            }
        };

        struct CompareRecordOffsets {
            bool Equals(const DGRec& a, const DGRec& b) const {
                return a.mOffset == b.mOffset;
            }
            bool LessThan(const DGRec& a, const DGRec& b) const {
                return a.mOffset < b.mOffset;
            }
        };

        
        
        
        nsTArray<DetailedGlyph>     mDetails;

        
        
        
        nsTArray<DGRec>             mOffsetToIndex;

        
        
        
        nsTArray<DGRec>::index_type mLastUsed;
    };

    nsAutoPtr<DetailedGlyphStore>   mDetailedGlyphs;

    
    uint32_t                        mLength;

    
    uint32_t                        mFlags;

    int32_t                         mAppUnitsPerDevUnit;
};








class gfxShapedWord : public gfxShapedText
{
public:
    
    
    
    
    
    
    
    
    
    static gfxShapedWord* Create(const uint8_t *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= gfxPlatform::GetPlatform()->WordCacheCharLimit(),
                     "excessive length for gfxShapedWord!");

        
        
        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(uint8_t));
        void *storage = malloc(size);
        if (!storage) {
            return nullptr;
        }

        
        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    static gfxShapedWord* Create(const char16_t *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= gfxPlatform::GetPlatform()->WordCacheCharLimit(),
                     "excessive length for gfxShapedWord!");

        
        
        
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            nsAutoCString narrowText;
            LossyAppendUTF16toASCII(nsDependentSubstring(aText, aLength),
                                    narrowText);
            return Create((const uint8_t*)(narrowText.BeginReading()),
                          aLength, aRunScript, aAppUnitsPerDevUnit, aFlags);
        }

        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(char16_t));
        void *storage = malloc(size);
        if (!storage) {
            return nullptr;
        }

        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    
    
    void operator delete(void* p) {
        free(p);
    }

    virtual CompressedGlyph *GetCharacterGlyphs() override {
        return &mCharGlyphsStorage[0];
    }

    const uint8_t* Text8Bit() const {
        NS_ASSERTION(TextIs8Bit(), "invalid use of Text8Bit()");
        return reinterpret_cast<const uint8_t*>(mCharGlyphsStorage + GetLength());
    }

    const char16_t* TextUnicode() const {
        NS_ASSERTION(!TextIs8Bit(), "invalid use of TextUnicode()");
        return reinterpret_cast<const char16_t*>(mCharGlyphsStorage + GetLength());
    }

    char16_t GetCharAt(uint32_t aOffset) const {
        NS_ASSERTION(aOffset < GetLength(), "aOffset out of range");
        return TextIs8Bit() ?
            char16_t(Text8Bit()[aOffset]) : TextUnicode()[aOffset];
    }

    int32_t Script() const {
        return mScript;
    }

    void ResetAge() {
        mAgeCounter = 0;
    }
    uint32_t IncrementAge() {
        return ++mAgeCounter;
    }

    
    static uint32_t HashMix(uint32_t aHash, char16_t aCh)
    {
        return (aHash >> 28) ^ (aHash << 4) ^ aCh;
    }

private:
    
    friend class gfxTextRun;

    
    gfxShapedWord(const uint8_t *aText, uint32_t aLength,
                  int32_t aRunScript, int32_t aAppUnitsPerDevUnit,
                  uint32_t aFlags)
        : gfxShapedText(aLength, aFlags | gfxTextRunFactory::TEXT_IS_8BIT,
                        aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharGlyphsStorage, 0, aLength * sizeof(CompressedGlyph));
        uint8_t *text = reinterpret_cast<uint8_t*>(&mCharGlyphsStorage[aLength]);
        memcpy(text, aText, aLength * sizeof(uint8_t));
    }

    gfxShapedWord(const char16_t *aText, uint32_t aLength,
                  int32_t aRunScript, int32_t aAppUnitsPerDevUnit,
                  uint32_t aFlags)
        : gfxShapedText(aLength, aFlags, aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharGlyphsStorage, 0, aLength * sizeof(CompressedGlyph));
        char16_t *text = reinterpret_cast<char16_t*>(&mCharGlyphsStorage[aLength]);
        memcpy(text, aText, aLength * sizeof(char16_t));
        SetupClusterBoundaries(0, aText, aLength);
    }

    int32_t          mScript;

    uint32_t         mAgeCounter;

    
    
    
    
    
    CompressedGlyph  mCharGlyphsStorage[1];
};

class GlyphBufferAzure;
struct TextRunDrawParams;
struct FontDrawParams;

class gfxFont {

    friend class gfxHarfBuzzShaper;
    friend class gfxGraphiteShaper;

protected:
    typedef mozilla::gfx::DrawTarget DrawTarget;

public:
    nsrefcnt AddRef(void) {
        NS_PRECONDITION(int32_t(mRefCnt) >= 0, "illegal refcnt");
        if (mExpirationState.IsTracked()) {
            gfxFontCache::GetCache()->RemoveObject(this);
        }
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "gfxFont", sizeof(*this));
        return mRefCnt;
    }
    nsrefcnt Release(void) {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "gfxFont");
        if (mRefCnt == 0) {
            NotifyReleased();
            
            return 0;
        }
        return mRefCnt;
    }

    int32_t GetRefCount() { return mRefCnt; }

    
    typedef enum {
        kAntialiasDefault,
        kAntialiasNone,
        kAntialiasGrayscale,
        kAntialiasSubpixel
    } AntialiasOption;

protected:
    nsAutoRefCnt mRefCnt;
    cairo_scaled_font_t *mScaledFont;

    void NotifyReleased() {
        gfxFontCache *cache = gfxFontCache::GetCache();
        if (cache) {
            
            
            cache->NotifyReleased(this);
        } else {
            
            delete this;
        }
    }

    gfxFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
            AntialiasOption anAAOption = kAntialiasDefault,
            cairo_scaled_font_t *aScaledFont = nullptr);

public:
    virtual ~gfxFont();

    bool Valid() const {
        return mIsValid;
    }

    
    typedef enum {
        LOOSE_INK_EXTENTS,
            
            
            
        TIGHT_INK_EXTENTS,
            
            
            
            
        TIGHT_HINTED_OUTLINE_EXTENTS
            
            
            
            
            
            
            
            
            
            
            
            
            
    } BoundingBoxType;

    const nsString& GetName() const { return mFontEntry->Name(); }
    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual cairo_scaled_font_t* GetCairoScaledFont() { return mScaledFont; }

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption) {
        
        return nullptr;
    }

    virtual gfxFloat GetAdjustedSize() const {
        return mAdjustedSize > 0.0
                 ? mAdjustedSize
                 : (mStyle.sizeAdjust == 0.0 ? 0.0 : mStyle.size);
    }

    float FUnitsToDevUnitsFactor() const {
        
        NS_ASSERTION(mFUnitsConvFactor > 0.0f, "mFUnitsConvFactor not valid");
        return mFUnitsConvFactor;
    }

    
    bool FontCanSupportHarfBuzz() {
        return mFontEntry->HasCmapTable();
    }

    
    bool FontCanSupportGraphite() {
        return mFontEntry->HasGraphiteTables();
    }

    
    
    
    bool AlwaysNeedsMaskForShadow() {
        return mFontEntry->TryGetColorGlyphs() ||
               mFontEntry->TryGetSVGData(this) ||
               mFontEntry->HasFontTable(TRUETYPE_TAG('C','B','D','T')) ||
               mFontEntry->HasFontTable(TRUETYPE_TAG('s','b','i','x'));
    }

    
    
    bool SupportsFeature(int32_t aScript, uint32_t aFeatureTag);

    
    
    bool SupportsVariantCaps(int32_t aScript, uint32_t aVariantCaps,
                             bool& aFallbackToSmallCaps,
                             bool& aSyntheticLowerToSmallCaps,
                             bool& aSyntheticUpperToSmallCaps);

    
    
    
    bool SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const uint8_t *aString,
                                uint32_t aLength, int32_t aRunScript);

    bool SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const char16_t *aString,
                                uint32_t aLength, int32_t aRunScript);

    
    
    
    virtual bool ProvidesGetGlyph() const {
        return false;
    }
    
    
    virtual uint32_t GetGlyph(uint32_t unicode, uint32_t variation_selector) {
        return 0;
    }
    
    gfxFloat GetGlyphHAdvance(gfxContext *aCtx, uint16_t aGID);

    
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
      GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams = nullptr)
    { return nullptr; }

    gfxFloat SynthesizeSpaceWidth(uint32_t aCh);

    
    struct Metrics {
        gfxFloat xHeight;
        gfxFloat strikeoutSize;
        gfxFloat strikeoutOffset;
        gfxFloat underlineSize;
        gfxFloat underlineOffset;

        gfxFloat internalLeading;
        gfxFloat externalLeading;

        gfxFloat emHeight;
        gfxFloat emAscent;
        gfxFloat emDescent;
        gfxFloat maxHeight;
        gfxFloat maxAscent;
        gfxFloat maxDescent;
        gfxFloat maxAdvance;

        gfxFloat aveCharWidth;
        gfxFloat spaceWidth;
        gfxFloat zeroOrAveCharWidth;  
                                      
                                      
    };

    enum Orientation {
        eHorizontal,
        eVertical
    };

    const Metrics& GetMetrics(Orientation aOrientation)
    {
        if (aOrientation == eHorizontal) {
            return GetHorizontalMetrics();
        }
        if (!mVerticalMetrics) {
            mVerticalMetrics = CreateVerticalMetrics();
        }
        return *mVerticalMetrics;
    }

    







    struct Spacing {
        gfxFloat mBefore;
        gfxFloat mAfter;
    };
    


    struct RunMetrics {
        RunMetrics() {
            mAdvanceWidth = mAscent = mDescent = 0.0;
        }

        void CombineWith(const RunMetrics& aOther, bool aOtherIsOnLeft);

        
        
        
        
        gfxFloat mAdvanceWidth;
        
        
        gfxFloat mAscent;  
        gfxFloat mDescent; 
        
        
        
        
        
        
        
        gfxRect  mBoundingBox;
    };

    


























    void Draw(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
              gfxPoint *aPt, const TextRunDrawParams& aRunParams,
              uint16_t aOrientation);

    




















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing, uint16_t aOrientation);
    




    bool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   uint32_t aStart, uint32_t aLength)
    { return false; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    virtual uint32_t GetSpaceGlyph() = 0;

    gfxGlyphExtents *GetOrCreateGlyphExtents(int32_t aAppUnitsPerDevUnit);

    
    virtual void SetupGlyphExtents(gfxContext *aContext,
                                   Orientation aOrientation, uint32_t aGlyphID,
                                   bool aNeedTight, gfxGlyphExtents *aExtents);

    
    virtual bool SetupCairoFont(gfxContext *aContext) = 0;

    virtual bool AllowSubpixelAA() { return true; }

    bool IsSyntheticBold() { return mApplySyntheticBold; }

    
    
    
    
    gfxFloat GetSyntheticBoldOffset() {
        gfxFloat size = GetAdjustedSize();
        const gfxFloat threshold = 48.0;
        return size < threshold ? (0.25 + 0.75 * size / threshold) :
                                  (size / threshold);
    }

    gfxFontEntry *GetFontEntry() const { return mFontEntry.get(); }
    bool HasCharacter(uint32_t ch) {
        if (!mIsValid ||
            (mUnicodeRangeMap && !mUnicodeRangeMap->test(ch))) {
            return false;
        }
        return mFontEntry->HasCharacter(ch); 
    }

    const gfxCharacterMap* GetUnicodeRangeMap() const {
        return mUnicodeRangeMap.get();
    }

    void SetUnicodeRangeMap(gfxCharacterMap* aUnicodeRangeMap) {
        mUnicodeRangeMap = aUnicodeRangeMap;
    }

    uint16_t GetUVSGlyph(uint32_t aCh, uint32_t aVS) {
        if (!mIsValid) {
            return 0;
        }
        return mFontEntry->GetUVSGlyph(aCh, aVS); 
    }

    template<typename T>
    bool InitFakeSmallCapsRun(gfxContext *aContext,
                              gfxTextRun *aTextRun,
                              const T    *aText,
                              uint32_t    aOffset,
                              uint32_t    aLength,
                              uint8_t     aMatchType,
                              uint16_t    aOrientation,
                              int32_t     aScript,
                              bool        aSyntheticLower,
                              bool        aSyntheticUpper);

    
    
    
    template<typename T>
    bool SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript,
                             bool aVertical);

    
    
    template<typename T>
    gfxShapedWord* GetShapedWord(gfxContext *aContext,
                                 const T *aText,
                                 uint32_t aLength,
                                 uint32_t aHash,
                                 int32_t aRunScript,
                                 bool aVertical,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags,
                                 gfxTextPerfMetrics *aTextPerf);

    
    
    void InitWordCache() {
        if (!mWordCache) {
            mWordCache = new nsTHashtable<CacheHashEntry>;
        }
    }

    
    
    void AgeCachedWords() {
        if (mWordCache) {
            (void)mWordCache->EnumerateEntries(AgeCacheEntry, this);
        }
    }

    
    void ClearCachedWords() {
        if (mWordCache) {
            mWordCache->Clear();
        }
    }

    
    void NotifyGlyphsChanged();

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

    typedef enum {
        FONT_TYPE_DWRITE,
        FONT_TYPE_GDI,
        FONT_TYPE_FT2,
        FONT_TYPE_MAC,
        FONT_TYPE_OS2,
        FONT_TYPE_CAIRO
    } FontType;

    virtual FontType GetType() const = 0;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont> GetScaledFont(DrawTarget* aTarget)
    { return gfxPlatform::GetPlatform()->GetScaledFontForFont(aTarget, this); }

    bool KerningDisabled() {
        return mKerningSet && !mKerningEnabled;
    }

    



    class GlyphChangeObserver {
    public:
        virtual ~GlyphChangeObserver()
        {
            if (mFont) {
                mFont->RemoveGlyphChangeObserver(this);
            }
        }
        
        void ForgetFont() { mFont = nullptr; }
        virtual void NotifyGlyphsChanged() = 0;
    protected:
        explicit GlyphChangeObserver(gfxFont *aFont) : mFont(aFont)
        {
            mFont->AddGlyphChangeObserver(this);
        }
        gfxFont* mFont;
    };
    friend class GlyphChangeObserver;

    bool GlyphsMayChange()
    {
        
        return mFontEntry->TryGetSVGData(this);
    }

    static void DestroySingletons() {
        delete sScriptTagToCode;
        delete sDefaultFeatures;
    }

    
    
    
    nscoord GetMathConstant(gfxFontEntry::MathConstant aConstant,
                            uint32_t aAppUnitsPerDevPixel)
    {
        return NSToCoordRound(mFontEntry->GetMathConstant(aConstant) *
                              GetAdjustedSize() * aAppUnitsPerDevPixel);
    }

    
    
    
    float GetMathConstant(gfxFontEntry::MathConstant aConstant)
    {
        return mFontEntry->GetMathConstant(aConstant);
    }

    
    virtual already_AddRefed<gfxFont>
    GetSubSuperscriptFont(int32_t aAppUnitsPerDevPixel);

protected:
    virtual const Metrics& GetHorizontalMetrics() = 0;

    const Metrics* CreateVerticalMetrics();

    
    
    
    
    void DrawOneGlyph(uint32_t           aGlyphID,
                      double             aAdvance,
                      gfxPoint          *aPt,
                      GlyphBufferAzure&  aBuffer,
                      bool              *aEmittedGlyphs) const;

    
    
    
    bool DrawGlyphs(gfxShapedText            *aShapedText,
                    uint32_t                  aOffset, 
                    uint32_t                  aCount, 
                    gfxPoint                 *aPt,
                    const TextRunDrawParams&  aRunParams,
                    const FontDrawParams&     aFontParams);

    
    
    void CalculateSubSuperSizeAndOffset(int32_t aAppUnitsPerDevPixel,
                                        gfxFloat& aSubSuperSizeRatio,
                                        float& aBaselineOffset);

    
    
    
    
    
    virtual already_AddRefed<gfxFont> GetSmallCapsFont();

    
    
    
    virtual bool ProvidesGlyphWidths() const {
        return false;
    }

    
    
    virtual int32_t GetGlyphWidth(DrawTarget& aDrawTarget, uint16_t aGID) {
        return -1;
    }

    bool IsSpaceGlyphInvisible(gfxContext *aRefContext, gfxTextRun *aTextRun);

    void AddGlyphChangeObserver(GlyphChangeObserver *aObserver);
    void RemoveGlyphChangeObserver(GlyphChangeObserver *aObserver);

    
    bool HasSubstitutionRulesWithSpaceLookups(int32_t aRunScript);

    
    bool SpaceMayParticipateInShaping(int32_t aRunScript);

    
    bool ShapeText(gfxContext    *aContext,
                   const uint8_t *aText,
                   uint32_t       aOffset, 
                   uint32_t       aLength,
                   int32_t        aScript,
                   bool           aVertical,
                   gfxShapedText *aShapedText); 

    
    
    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           bool             aVertical,
                           gfxShapedText   *aShapedText);

    
    
    
    void PostShapingFixup(gfxContext      *aContext,
                          const char16_t *aText,
                          uint32_t         aOffset, 
                          uint32_t         aLength,
                          bool             aVertical,
                          gfxShapedText   *aShapedText);

    
    
    
    
    
    
    
    template<typename T>
    bool ShapeTextWithoutWordCache(gfxContext *aContext,
                                   const T    *aText,
                                   uint32_t    aOffset,
                                   uint32_t    aLength,
                                   int32_t     aScript,
                                   bool        aVertical,
                                   gfxTextRun *aTextRun);

    
    
    
    
    
    template<typename T>
    bool ShapeFragmentWithoutWordCache(gfxContext *aContext,
                                       const T    *aText,
                                       uint32_t    aOffset,
                                       uint32_t    aLength,
                                       int32_t     aScript,
                                       bool        aVertical,
                                       gfxTextRun *aTextRun);

    void CheckForFeaturesInvolvingSpace();

    
    
    bool HasFeatureSet(uint32_t aFeature, bool& aFeatureOn);

    
    static nsDataHashtable<nsUint32HashKey, int32_t> *sScriptTagToCode;
    static nsTHashtable<nsUint32HashKey>             *sDefaultFeatures;

    nsRefPtr<gfxFontEntry> mFontEntry;

    struct CacheHashKey {
        union {
            const uint8_t   *mSingle;
            const char16_t *mDouble;
        }                mText;
        uint32_t         mLength;
        uint32_t         mFlags;
        int32_t          mScript;
        int32_t          mAppUnitsPerDevUnit;
        PLDHashNumber    mHashKey;
        bool             mTextIs8Bit;

        CacheHashKey(const uint8_t *aText, uint32_t aLength,
                     uint32_t aStringHash,
                     int32_t aScriptCode, int32_t aAppUnitsPerDevUnit,
                     uint32_t aFlags)
            : mLength(aLength),
              mFlags(aFlags),
              mScript(aScriptCode),
              mAppUnitsPerDevUnit(aAppUnitsPerDevUnit),
              mHashKey(aStringHash + aScriptCode +
                  aAppUnitsPerDevUnit * 0x100 + aFlags * 0x10000),
              mTextIs8Bit(true)
        {
            NS_ASSERTION(aFlags & gfxTextRunFactory::TEXT_IS_8BIT,
                         "8-bit flag should have been set");
            mText.mSingle = aText;
        }

        CacheHashKey(const char16_t *aText, uint32_t aLength,
                     uint32_t aStringHash,
                     int32_t aScriptCode, int32_t aAppUnitsPerDevUnit,
                     uint32_t aFlags)
            : mLength(aLength),
              mFlags(aFlags),
              mScript(aScriptCode),
              mAppUnitsPerDevUnit(aAppUnitsPerDevUnit),
              mHashKey(aStringHash + aScriptCode +
                  aAppUnitsPerDevUnit * 0x100 + aFlags * 0x10000),
              mTextIs8Bit(false)
        {
            
            
            
            
            mText.mDouble = aText;
        }
    };

    class CacheHashEntry : public PLDHashEntryHdr {
    public:
        typedef const CacheHashKey &KeyType;
        typedef const CacheHashKey *KeyTypePointer;

        
        
        explicit CacheHashEntry(KeyTypePointer aKey) { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;

        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return aKey->mHashKey;
        }

        size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        {
            return aMallocSizeOf(mShapedWord.get());
        }

        enum { ALLOW_MEMMOVE = true };

        nsAutoPtr<gfxShapedWord> mShapedWord;
    };

    nsAutoPtr<nsTHashtable<CacheHashEntry> > mWordCache;

    static PLDHashOperator AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData);
    static const uint32_t  kShapedWordCacheMaxAge = 3;

    bool                       mIsValid;

    
    
    bool                       mApplySyntheticBold;

    bool                       mKerningSet;     
    bool                       mKerningEnabled; 

    nsExpirationState          mExpirationState;
    gfxFontStyle               mStyle;
    nsAutoTArray<gfxGlyphExtents*,1> mGlyphExtentsArray;
    nsAutoPtr<nsTHashtable<nsPtrHashKey<GlyphChangeObserver> > > mGlyphChangeObservers;

    gfxFloat                   mAdjustedSize;

    float                      mFUnitsConvFactor; 

    
    AntialiasOption            mAntialiasOption;

    
    
    nsAutoPtr<gfxFont>         mNonAAFont;

    
    
    
    nsAutoPtr<gfxFontShaper>   mHarfBuzzShaper;
    nsAutoPtr<gfxFontShaper>   mGraphiteShaper;

    
    
    nsRefPtr<gfxCharacterMap> mUnicodeRangeMap;

    mozilla::RefPtr<mozilla::gfx::ScaledFont> mAzureScaledFont;

    
    nsAutoPtr<const Metrics> mVerticalMetrics;

    
    
    
    
    
    
    
    
    bool InitMetricsFromSfntTables(Metrics& aMetrics);

    
    
    void CalculateDerivedMetrics(Metrics& aMetrics);

    
    
    void SanitizeMetrics(Metrics *aMetrics, bool aIsBadUnderlineFont);

    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint) const;
    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint,
                        gfxTextRunDrawCallbacks *aCallbacks,
                        bool& aEmittedGlyphs) const;

    bool RenderColorGlyph(gfxContext* aContext,
                          mozilla::gfx::ScaledFont* scaledFont,
                          mozilla::gfx::GlyphRenderingOptions* renderingOptions,
                          mozilla::gfx::DrawOptions drawOptions,
                          const mozilla::gfx::Point& aPoint,
                          uint32_t aGlyphId) const;

    
    
    
    
    
    
    
    static double CalcXScale(gfxContext *aContext);
};


#define DEFAULT_XHEIGHT_FACTOR 0.56f





struct TextRunDrawParams {
    mozilla::RefPtr<mozilla::gfx::DrawTarget> dt;
    gfxContext              *context;
    gfxFont::Spacing        *spacing;
    gfxTextRunDrawCallbacks *callbacks;
    gfxTextContextPaint     *runContextPaint;
    mozilla::gfx::Color      fontSmoothingBGColor;
    gfxFloat                 direction;
    double                   devPerApp;
    DrawMode                 drawMode;
    bool                     isVerticalRun;
    bool                     isRTL;
    bool                     paintSVGGlyphs;
};

struct FontDrawParams {
    mozilla::RefPtr<mozilla::gfx::ScaledFont>            scaledFont;
    mozilla::RefPtr<mozilla::gfx::GlyphRenderingOptions> renderingOptions;
    gfxTextContextPaint      *contextPaint;
    mozilla::gfx::Matrix     *passedInvMatrix;
    mozilla::gfx::Matrix      matInv;
    double                    synBoldOnePixelOffset;
    int32_t                   extraStrikes;
    mozilla::gfx::DrawOptions drawOptions;
    bool                      isVerticalFont;
    bool                      haveSVGGlyphs;
    bool                      haveColorGlyphs;
};

#endif
