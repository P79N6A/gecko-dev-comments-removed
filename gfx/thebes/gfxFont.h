




#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "nsAlgorithm.h"
#include "gfxTypes.h"
#include "nsString.h"
#include "gfxPoint.h"
#include "gfxFontUtils.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "gfxSkipChars.h"
#include "gfxRect.h"
#include "nsExpirationTracker.h"
#include "gfxFontConstants.h"
#include "gfxPlatform.h"
#include "nsIAtom.h"
#include "nsISupportsImpl.h"
#include "gfxPattern.h"
#include "mozilla/HashFunctions.h"
#include "nsIMemoryReporter.h"
#include "gfxFontFeatures.h"
#include "mozilla/gfx/Types.h"
#include "mozilla/Attributes.h"
#include <algorithm>

typedef struct _cairo_scaled_font cairo_scaled_font_t;

#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxTextRun;
class gfxFont;
class gfxFontFamily;
class gfxFontGroup;
class gfxUserFontSet;
class gfxUserFontData;
class gfxShapedText;
class gfxShapedWord;
class gfxSVGGlyphs;
class gfxTextObjectPaint;

class nsILanguageAtomService;

typedef struct hb_blob_t hb_blob_t;

#define FONT_MAX_SIZE                  2000.0

#define NO_FONT_LANGUAGE_OVERRIDE      0

struct FontListSizes;
struct gfxTextRunDrawCallbacks;

struct THEBES_API gfxFontStyle {
    gfxFontStyle();
    gfxFontStyle(uint8_t aStyle, uint16_t aWeight, int16_t aStretch,
                 gfxFloat aSize, nsIAtom *aLanguage,
                 float aSizeAdjust, bool aSystemFont,
                 bool aPrinterFont,
                 const nsString& aLanguageOverride);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    
    
    nsRefPtr<nsIAtom> language;

    
    nsTArray<gfxFontFeature> featureSettings;

    
    gfxFloat size;

    
    
    
    
    float sizeAdjust;

    
    
    
    
    
    
    
    
    
    
    uint32_t languageOverride;

    
    uint16_t weight;

    
    
    int8_t stretch;

    
    
    
    bool systemFont : 1;

    
    bool printerFont : 1;

    
    uint8_t style : 2;

    
    
    gfxFloat GetAdjustedSize(gfxFloat aspect) const {
        NS_ASSERTION(sizeAdjust != 0.0, "Not meant to be called when sizeAdjust = 0");
        gfxFloat adjustedSize = std::max(NS_round(size*(sizeAdjust/aspect)), 1.0);
        return std::min(adjustedSize, FONT_MAX_SIZE);
    }

    PLDHashNumber Hash() const {
        return ((style + (systemFont << 7) +
            (weight << 8)) + uint32_t(size*1000) + uint32_t(sizeAdjust*1000)) ^
            nsISupportsHashKey::HashKey(language);
    }

    int8_t ComputeWeight() const;

    bool Equals(const gfxFontStyle& other) const {
        return
            (*reinterpret_cast<const uint64_t*>(&size) ==
             *reinterpret_cast<const uint64_t*>(&other.size)) &&
            (style == other.style) &&
            (systemFont == other.systemFont) &&
            (printerFont == other.printerFont) &&
            (weight == other.weight) &&
            (stretch == other.stretch) &&
            (language == other.language) &&
            (*reinterpret_cast<const uint32_t*>(&sizeAdjust) ==
             *reinterpret_cast<const uint32_t*>(&other.sizeAdjust)) &&
            (featureSettings == other.featureSettings) &&
            (languageOverride == other.languageOverride);
    }

    static void ParseFontFeatureSettings(const nsString& aFeatureString,
                                         nsTArray<gfxFontFeature>& aFeatures);

    static uint32_t ParseFontLanguageOverride(const nsString& aLangTag);
};

class gfxCharacterMap : public gfxSparseBitSet {
public:
    nsrefcnt AddRef() {
        NS_PRECONDITION(int32_t(mRefCnt) >= 0, "illegal refcnt");
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "gfxCharacterMap", sizeof(*this));
        return mRefCnt;
    }

    nsrefcnt Release() {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "gfxCharacterMap");
        if (mRefCnt == 0) {
            NotifyReleased();
            
            return 0;
        }
        return mRefCnt;
    }

    gfxCharacterMap() :
        mHash(0), mBuildOnTheFly(false), mShared(false)
    { }

    void CalcHash() { mHash = GetChecksum(); }

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        return gfxSparseBitSet::SizeOfExcludingThis(aMallocSizeOf);
    }

    
    uint32_t mHash;

    
    bool mBuildOnTheFly;

    
    bool mShared;

protected:
    void NotifyReleased();

    nsAutoRefCnt mRefCnt;

private:
    gfxCharacterMap(const gfxCharacterMap&);
    gfxCharacterMap& operator=(const gfxCharacterMap&);
};

class gfxFontEntry {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFontEntry)

    gfxFontEntry(const nsAString& aName, bool aIsStandardFace = false) :
        mName(aName), mItalic(false), mFixedPitch(false),
        mIsProxy(false), mIsValid(true), 
        mIsBadUnderlineFont(false), mIsUserFont(false),
        mIsLocalUserFont(false), mStandardFace(aIsStandardFace),
        mSymbolFont(false),
        mIgnoreGDEF(false),
        mIgnoreGSUB(false),
        mSVGInitialized(false),
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
        mCheckedForGraphiteTables(false),
        mHasCmapTable(false),
        mUVSOffset(0), mUVSData(nullptr),
        mUserFontData(nullptr),
        mSVGGlyphs(nullptr),
        mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE)
    { }

    virtual ~gfxFontEntry();

    
    
    const nsString& Name() const { return mName; }

    
    const nsString& FamilyName() const { return mFamilyName; }

    
    
    
    

    
    
    virtual nsString RealFaceName();

    uint16_t Weight() const { return mWeight; }
    int16_t Stretch() const { return mStretch; }

    bool IsUserFont() const { return mIsUserFont; }
    bool IsLocalUserFont() const { return mIsLocalUserFont; }
    bool IsFixedPitch() const { return mFixedPitch; }
    bool IsItalic() const { return mItalic; }
    bool IsBold() const { return mWeight >= 600; } 
    bool IgnoreGDEF() const { return mIgnoreGDEF; }
    bool IgnoreGSUB() const { return mIgnoreGSUB; }

    virtual bool IsSymbolFont();

    inline bool HasGraphiteTables() {
        if (!mCheckedForGraphiteTables) {
            CheckForGraphiteTables();
            mCheckedForGraphiteTables = true;
        }
        return mHasGraphiteTables;
    }

    inline bool HasCmapTable() {
        if (!mCharacterMap) {
            ReadCMAP();
            NS_ASSERTION(mCharacterMap, "failed to initialize character map");
        }
        return mHasCmapTable;
    }

    inline bool HasCharacter(uint32_t ch) {
        if (mCharacterMap && mCharacterMap->test(ch)) {
            return true;
        }
        return TestCharacterMap(ch);
    }

    virtual bool SkipDuringSystemFallback() { return false; }
    virtual bool TestCharacterMap(uint32_t aCh);
    nsresult InitializeUVSMap();
    uint16_t GetUVSGlyph(uint32_t aCh, uint32_t aVS);
    virtual nsresult ReadCMAP();

    bool TryGetSVGData();
    bool HasSVGGlyph(uint32_t aGlyphId);
    bool GetSVGGlyphExtents(gfxContext *aContext, uint32_t aGlyphId,
                            gfxRect *aResult);
    bool RenderSVGGlyph(gfxContext *aContext, uint32_t aGlyphId, int aDrawMode,
                        gfxTextObjectPaint *aObjectPaint);

    virtual bool MatchesGenericFamily(const nsACString& aGeneric) const {
        return true;
    }
    virtual bool SupportsLangGroup(nsIAtom *aLangGroup) const {
        return true;
    }

    virtual nsresult GetFontTable(uint32_t aTableTag, FallibleTArray<uint8_t>& aBuffer) {
        return NS_ERROR_FAILURE; 
    }

    already_AddRefed<gfxFont> FindOrMakeFont(const gfxFontStyle *aStyle,
                                             bool aNeedsBold);

    
    
    
    
    
    
    bool GetExistingFontTable(uint32_t aTag, hb_blob_t** aBlob);

    
    
    
    
    
    
    
    hb_blob_t *ShareFontTableAndGetBlob(uint32_t aTag,
                                        FallibleTArray<uint8_t>* aTable);

    
    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

    nsString         mName;
    nsString         mFamilyName;

    bool             mItalic      : 1;
    bool             mFixedPitch  : 1;
    bool             mIsProxy     : 1;
    bool             mIsValid     : 1;
    bool             mIsBadUnderlineFont : 1;
    bool             mIsUserFont  : 1;
    bool             mIsLocalUserFont  : 1;
    bool             mStandardFace : 1;
    bool             mSymbolFont  : 1;
    bool             mIgnoreGDEF  : 1;
    bool             mIgnoreGSUB  : 1;
    bool             mSVGInitialized : 1;

    uint16_t         mWeight;
    int16_t          mStretch;

    bool             mHasGraphiteTables;
    bool             mCheckedForGraphiteTables;
    bool             mHasCmapTable;
    nsRefPtr<gfxCharacterMap> mCharacterMap;
    uint32_t         mUVSOffset;
    nsAutoArrayPtr<uint8_t> mUVSData;
    gfxUserFontData* mUserFontData;
    gfxSVGGlyphs    *mSVGGlyphs;

    nsTArray<gfxFontFeature> mFeatureSettings;
    uint32_t         mLanguageOverride;

protected:
    friend class gfxPlatformFontList;
    friend class gfxMacPlatformFontList;
    friend class gfxUserFcFontEntry;
    friend class gfxFontFamily;
    friend class gfxSingleFaceMacFontFamily;

    gfxFontEntry() :
        mItalic(false), mFixedPitch(false),
        mIsProxy(false), mIsValid(true), 
        mIsBadUnderlineFont(false),
        mIsUserFont(false),
        mIsLocalUserFont(false),
        mStandardFace(false),
        mSymbolFont(false),
        mIgnoreGDEF(false),
        mIgnoreGSUB(false),
        mSVGInitialized(false),
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
        mCheckedForGraphiteTables(false),
        mHasCmapTable(false),
        mUVSOffset(0), mUVSData(nullptr),
        mUserFontData(nullptr),
        mSVGGlyphs(nullptr),
        mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE)
    { }

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold) {
        NS_NOTREACHED("oops, somebody didn't override CreateFontInstance");
        return nullptr;
    }

    virtual void CheckForGraphiteTables();

private:

    























    class FontTableBlobData;

    










    class FontTableHashEntry : public nsUint32HashKey
    {
    public:
        

        typedef nsUint32HashKey KeyClass;
        typedef KeyClass::KeyType KeyType;
        typedef KeyClass::KeyTypePointer KeyTypePointer;

        FontTableHashEntry(KeyTypePointer aTag)
            : KeyClass(aTag), mBlob() { }
        
        FontTableHashEntry(FontTableHashEntry& toCopy)
            : KeyClass(toCopy), mBlob(toCopy.mBlob)
        {
            toCopy.mBlob = nullptr;
        }

        ~FontTableHashEntry() { Clear(); }

        

        
        
        
        
        hb_blob_t *
        ShareTableAndGetBlob(FallibleTArray<uint8_t>& aTable,
                             nsTHashtable<FontTableHashEntry> *aHashtable);

        
        
        hb_blob_t *GetBlob() const;

        void Clear();

        static size_t
        SizeOfEntryExcludingThis(FontTableHashEntry *aEntry,
                                 nsMallocSizeOfFun   aMallocSizeOf,
                                 void*               aUserArg);

    private:
        static void DeleteFontTableBlobData(void *aBlobData);
        
        FontTableHashEntry& operator=(FontTableHashEntry& toCopy);

        FontTableBlobData *mSharedBlobData;
        hb_blob_t *mBlob;
    };

    nsTHashtable<FontTableHashEntry> mFontTableCache;

    gfxFontEntry(const gfxFontEntry&);
    gfxFontEntry& operator=(const gfxFontEntry&);
};



struct GlobalFontMatch {
    GlobalFontMatch(const uint32_t aCharacter,
                    int32_t aRunScript,
                    const gfxFontStyle *aStyle) :
        mCh(aCharacter), mRunScript(aRunScript), mStyle(aStyle),
        mMatchRank(0), mCount(0), mCmapsTested(0)
        {

        }

    const uint32_t         mCh;          
    int32_t                mRunScript;   
    const gfxFontStyle*    mStyle;       
    int32_t                mMatchRank;   
    nsRefPtr<gfxFontEntry> mBestMatch;   
    nsRefPtr<gfxFontFamily> mMatchedFamily; 
    uint32_t               mCount;       
    uint32_t               mCmapsTested; 
};

class gfxFontFamily {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFontFamily)

    gfxFontFamily(const nsAString& aName) :
        mName(aName),
        mOtherFamilyNamesInitialized(false),
        mHasOtherFamilyNames(false),
        mFaceNamesInitialized(false),
        mHasStyles(false),
        mIsSimpleFamily(false),
        mIsBadUnderlineFamily(false),
        mFamilyCharacterMapInitialized(false)
        { }

    virtual ~gfxFontFamily() { }

    const nsString& Name() { return mName; }

    virtual void LocalizedName(nsAString& aLocalizedName);
    virtual bool HasOtherFamilyNames();
    
    nsTArray<nsRefPtr<gfxFontEntry> >& GetFontList() { return mAvailableFonts; }
    
    void AddFontEntry(nsRefPtr<gfxFontEntry> aFontEntry) {
        
        
        if (aFontEntry->IsItalic() && !aFontEntry->IsUserFont() &&
            Name().EqualsLiteral("Times New Roman"))
        {
            aFontEntry->mIgnoreGDEF = true;
        }
        aFontEntry->mFamilyName = Name();
        mAvailableFonts.AppendElement(aFontEntry);
    }

    
    void SetHasStyles(bool aHasStyles) { mHasStyles = aHasStyles; }

    
    
    
    
    
    gfxFontEntry *FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                   bool& aNeedsSyntheticBold);

    
    
    void FindFontForChar(GlobalFontMatch *aMatchData);

    
    void SearchAllFontsForChar(GlobalFontMatch *aMatchData);

    
    virtual void ReadOtherFamilyNames(gfxPlatformFontList *aPlatformFontList);

    
    void SetOtherFamilyNamesInitialized() {
        mOtherFamilyNamesInitialized = true;
    }

    
    
    virtual void ReadFaceNames(gfxPlatformFontList *aPlatformFontList,
                               bool aNeedFullnamePostscriptNames);

    
    
    virtual void FindStyleVariations() { }

    
    gfxFontEntry* FindFont(const nsAString& aPostscriptName);

    
    void ReadAllCMAPs() {
        uint32_t i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++) {
            gfxFontEntry *fe = mAvailableFonts[i];
            
            if (!fe || fe->mIsProxy) {
                continue;
            }
            fe->ReadCMAP();
            mFamilyCharacterMap.Union(*(fe->mCharacterMap));
        }
        mFamilyCharacterMap.Compact();
        mFamilyCharacterMapInitialized = true;
    }

    bool TestCharacterMap(uint32_t aCh) {
        if (!mFamilyCharacterMapInitialized) {
            ReadAllCMAPs();
        }
        return mFamilyCharacterMap.test(aCh);
    }

    void ResetCharacterMap() {
        mFamilyCharacterMap.reset();
        mFamilyCharacterMapInitialized = false;
    }

    
    void SetBadUnderlineFamily() {
        mIsBadUnderlineFamily = true;
        if (mHasStyles) {
            SetBadUnderlineFonts();
        }
    }

    bool IsBadUnderlineFamily() const { return mIsBadUnderlineFamily; }

    
    void SortAvailableFonts();

    
    
    
    void CheckForSimpleFamily();

    
    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontListSizes*    aSizes) const;

    
    bool ContainsFace(gfxFontEntry* aFontEntry) {
        uint32_t i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++) {
            if (mAvailableFonts[i] == aFontEntry) {
                return true;
            }
        }
        return false;
    }

protected:
    
    
    virtual bool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                       bool anItalic, int16_t aStretch);

    bool ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                       FallibleTArray<uint8_t>& aNameTable,
                                       bool useFullName = false);

    
    void SetBadUnderlineFonts() {
        uint32_t i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++) {
            if (mAvailableFonts[i]) {
                mAvailableFonts[i]->mIsBadUnderlineFont = true;
            }
        }
    }

    nsString mName;
    nsTArray<nsRefPtr<gfxFontEntry> >  mAvailableFonts;
    gfxSparseBitSet mFamilyCharacterMap;
    bool mOtherFamilyNamesInitialized : 1;
    bool mHasOtherFamilyNames : 1;
    bool mFaceNamesInitialized : 1;
    bool mHasStyles : 1;
    bool mIsSimpleFamily : 1;
    bool mIsBadUnderlineFamily : 1;
    bool mFamilyCharacterMapInitialized : 1;

    enum {
        
        
        kRegularFaceIndex    = 0,
        kBoldFaceIndex       = 1,
        kItalicFaceIndex     = 2,
        kBoldItalicFaceIndex = 3,
        
        kBoldMask   = 0x01,
        kItalicMask = 0x02
    };
};

struct gfxTextRange {
    enum {
        
        kFontGroup      = 0x0001,
        kPrefsFallback  = 0x0002,
        kSystemFallback = 0x0004
    };
    gfxTextRange(uint32_t aStart, uint32_t aEnd,
                 gfxFont* aFont, uint8_t aMatchType)
        : start(aStart),
          end(aEnd),
          font(aFont),
          matchType(aMatchType)
    { }
    uint32_t Length() const { return end - start; }
    uint32_t start, end;
    nsRefPtr<gfxFont> font;
    uint8_t matchType;
};



























struct FontCacheSizes {
    FontCacheSizes()
        : mFontInstances(0), mShapedWords(0)
    { }

    size_t mFontInstances; 
    size_t mShapedWords; 
};

class THEBES_API gfxFontCache MOZ_FINAL : public nsExpirationTracker<gfxFont,3> {
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

    
    
    already_AddRefed<gfxFont> Lookup(const gfxFontEntry *aFontEntry,
                                     const gfxFontStyle *aStyle);
    
    
    
    
    void AddNew(gfxFont *aFont);

    
    
    
    void NotifyReleased(gfxFont *aFont);

    
    
    virtual void NotifyExpired(gfxFont *aFont);

    
    
    
    void Flush() {
        mFonts.Clear();
        AgeAllGenerations();
    }

    void FlushShapedWordCaches() {
        mFonts.EnumerateEntries(ClearCachedWordsForFont, nullptr);
    }

    void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             FontCacheSizes*   aSizes) const;
    void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             FontCacheSizes*   aSizes) const;

protected:
    class MemoryReporter MOZ_FINAL
        : public nsIMemoryMultiReporter
    {
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIMEMORYMULTIREPORTER
    };

    void DestroyFont(gfxFont *aFont);

    static gfxFontCache *gGlobalCache;

    struct Key {
        const gfxFontEntry* mFontEntry;
        const gfxFontStyle* mStyle;
        Key(const gfxFontEntry* aFontEntry, const gfxFontStyle* aStyle)
            : mFontEntry(aFontEntry), mStyle(aStyle) {}
    };

    class HashEntry : public PLDHashEntryHdr {
    public:
        typedef const Key& KeyType;
        typedef const Key* KeyTypePointer;

        
        
        HashEntry(KeyTypePointer aStr) : mFont(nullptr) { }
        HashEntry(const HashEntry& toCopy) : mFont(toCopy.mFont) { }
        ~HashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return mozilla::HashGeneric(aKey->mStyle->Hash(), aKey->mFontEntry);
        }
        enum { ALLOW_MEMMOVE = true };

        gfxFont* mFont;
    };

    static size_t SizeOfFontEntryExcludingThis(HashEntry*        aHashEntry,
                                               nsMallocSizeOfFun aMallocSizeOf,
                                               void*             aUserArg);

    nsTHashtable<HashEntry> mFonts;

    static PLDHashOperator ClearCachedWordsForFont(HashEntry* aHashEntry, void*);
    static PLDHashOperator AgeCachedWordsForFont(HashEntry* aHashEntry, void*);
    static void WordCacheExpirationTimerCallback(nsITimer* aTimer, void* aCache);
    nsCOMPtr<nsITimer>      mWordCacheExpirationTimer;
};

class THEBES_API gfxTextRunFactory {
    NS_INLINE_DECL_REFCOUNTING(gfxTextRunFactory)

public:
    
    
    
    enum {
        CACHE_TEXT_FLAGS    = 0xF0000000,
        USER_TEXT_FLAGS     = 0x0FFF0000,
        PLATFORM_TEXT_FLAGS = 0x0000F000,
        TEXTRUN_TEXT_FLAGS  = 0x00000FFF,
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

        




        TEXT_TRAILING_ARABICCHAR = 0x20000000,
        




        TEXT_INCOMING_ARABICCHAR = 0x40000000,

        TEXT_UNUSED_FLAGS = 0x90000000
    };

    


    struct Parameters {
        
        gfxContext   *mContext;
        
        void         *mUserData;
        
        
        
        gfxSkipChars *mSkipChars;
        
        
        uint32_t     *mInitialBreaks;
        uint32_t      mInitialBreakCount;
        
        int32_t       mAppUnitsPerDevUnit;
    };

    virtual ~gfxTextRunFactory() {}
};














class THEBES_API gfxGlyphExtents {
public:
    gfxGlyphExtents(int32_t aAppUnitsPerDevUnit) :
        mAppUnitsPerDevUnit(aAppUnitsPerDevUnit) {
        MOZ_COUNT_CTOR(gfxGlyphExtents);
        mTightGlyphExtents.Init();
    }
    ~gfxGlyphExtents();

    enum { INVALID_WIDTH = 0xFFFF };

    
    
    
    
    uint16_t GetContainedGlyphWidthAppUnits(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID);
    }

    bool IsGlyphKnown(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID) != INVALID_WIDTH ||
            mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    bool IsGlyphKnownWithTightExtents(uint32_t aGlyphID) const {
        return mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    
    
    
    bool GetTightGlyphExtentsAppUnits(gfxFont *aFont, gfxContext *aContext,
            uint32_t aGlyphID, gfxRect *aExtents);

    void SetContainedGlyphWidthAppUnits(uint32_t aGlyphID, uint16_t aWidth) {
        mContainedGlyphWidths.Set(aGlyphID, aWidth);
    }
    void SetTightGlyphExtents(uint32_t aGlyphID, const gfxRect& aExtentsAppUnits);

    int32_t GetAppUnitsPerDevUnit() { return mAppUnitsPerDevUnit; }

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

private:
    class HashEntry : public nsUint32HashKey {
    public:
        
        
        HashEntry(KeyTypePointer aPtr) : nsUint32HashKey(aPtr) {}
        HashEntry(const HashEntry& toCopy) : nsUint32HashKey(toCopy) {
          x = toCopy.x; y = toCopy.y; width = toCopy.width; height = toCopy.height;
        }

        float x, y, width, height;
    };

    enum { BLOCK_SIZE_BITS = 7, BLOCK_SIZE = 1 << BLOCK_SIZE_BITS }; 

    class GlyphWidths {
    public:
        void Set(uint32_t aIndex, uint16_t aValue);
        uint16_t Get(uint32_t aIndex) const {
            uint32_t block = aIndex >> BLOCK_SIZE_BITS;
            if (block >= mBlocks.Length())
                return INVALID_WIDTH;
            uintptr_t bits = mBlocks[block];
            if (!bits)
                return INVALID_WIDTH;
            uint32_t indexInBlock = aIndex & (BLOCK_SIZE - 1);
            if (bits & 0x1) {
                if (GetGlyphOffset(bits) != indexInBlock)
                    return INVALID_WIDTH;
                return GetWidth(bits);
            }
            uint16_t *widths = reinterpret_cast<uint16_t *>(bits);
            return widths[indexInBlock];
        }

        uint32_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
        
        ~GlyphWidths();

    private:
        static uint32_t GetGlyphOffset(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return (aBits >> 1) & ((1 << BLOCK_SIZE_BITS) - 1);
        }
        static uint32_t GetWidth(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return aBits >> (1 + BLOCK_SIZE_BITS);
        }
        static uintptr_t MakeSingle(uint32_t aGlyphOffset, uint16_t aWidth) {
            return (aWidth << (1 + BLOCK_SIZE_BITS)) + (aGlyphOffset << 1) + 1;
        }

        nsTArray<uintptr_t> mBlocks;
    };

    GlyphWidths             mContainedGlyphWidths;
    nsTHashtable<HashEntry> mTightGlyphExtents;
    int32_t                 mAppUnitsPerDevUnit;
};



















class gfxFontShaper {
public:
    gfxFontShaper(gfxFont *aFont)
        : mFont(aFont)
    {
        NS_ASSERTION(aFont, "shaper requires a valid font!");
    }

    virtual ~gfxFontShaper() { }

    
    
    
    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText) = 0;

    gfxFont *GetFont() const { return mFont; }

    
    static bool
    MergeFontFeatures(const nsTArray<gfxFontFeature>& aStyleRuleFeatures,
                      const nsTArray<gfxFontFeature>& aFontFeatures,
                      bool aDisableLigatures,
                      nsDataHashtable<nsUint32HashKey,uint32_t>& aMergedFeatures);

protected:
    
    gfxFont * mFont;
};


class THEBES_API gfxFont {
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

    
    typedef enum {
        
        
        GLYPH_FILL = 1,
        
        GLYPH_STROKE = 2,
        
        
        GLYPH_PATH = 4,
        
        
        GLYPH_STROKE_UNDERNEATH = 8
    } DrawMode;

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

    virtual gfxFloat GetAdjustedSize() {
        return mAdjustedSize > 0.0 ? mAdjustedSize : mStyle.size;
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

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual hb_blob_t *GetFontTable(uint32_t aTag);

    
    
    
    virtual bool ProvidesGetGlyph() const {
        return false;
    }
    
    
    virtual uint32_t GetGlyph(uint32_t unicode, uint32_t variation_selector) {
        return 0;
    }

    
    
    
    virtual bool ProvidesGlyphWidths() {
        return false;
    }

    
    
    virtual int32_t GetGlyphWidth(gfxContext *aCtx, uint16_t aGID) {
        return -1;
    }

    
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
      GetGlyphRenderingOptions() { return nullptr; }

    gfxFloat SynthesizeSpaceWidth(uint32_t aCh);

    
    struct Metrics {
        gfxFloat xHeight;
        gfxFloat superscriptOffset;
        gfxFloat subscriptOffset;
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
    virtual const gfxFont::Metrics& GetMetrics() = 0;

    







    struct Spacing {
        gfxFloat mBefore;
        gfxFloat mAfter;
    };
    


    struct THEBES_API RunMetrics {
        RunMetrics() {
            mAdvanceWidth = mAscent = mDescent = 0.0;
            mBoundingBox = gfxRect(0,0,0,0);
        }

        void CombineWith(const RunMetrics& aOther, bool aOtherIsOnLeft);

        
        
        
        
        gfxFloat mAdvanceWidth;
        
        
        gfxFloat mAscent;  
        gfxFloat mDescent; 
        
        
        
        
        
        
        
        gfxRect  mBoundingBox;
    };

    

























    virtual void Draw(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
                      gfxContext *aContext, DrawMode aDrawMode, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing, gfxTextObjectPaint *aObjectPaint,
                      gfxTextRunDrawCallbacks *aCallbacks);

    




















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);
    




    bool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   uint32_t aStart, uint32_t aLength)
    { return false; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    virtual uint32_t GetSpaceGlyph() = 0;

    gfxGlyphExtents *GetOrCreateGlyphExtents(int32_t aAppUnitsPerDevUnit);

    
    virtual void SetupGlyphExtents(gfxContext *aContext, uint32_t aGlyphID,
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

    gfxFontEntry *GetFontEntry() { return mFontEntry.get(); }
    bool HasCharacter(uint32_t ch) {
        if (!mIsValid)
            return false;
        return mFontEntry->HasCharacter(ch); 
    }

    uint16_t GetUVSGlyph(uint32_t aCh, uint32_t aVS) {
        if (!mIsValid) {
            return 0;
        }
        return mFontEntry->GetUVSGlyph(aCh, aVS); 
    }

    
    
    
    template<typename T>
    bool SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript);

    
    
    template<typename T>
    gfxShapedWord* GetShapedWord(gfxContext *aContext,
                                 const T *aText,
                                 uint32_t aLength,
                                 uint32_t aHash,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags);

    
    
    void InitWordCache() {
        if (!mWordCache.IsInitialized()) {
            mWordCache.Init();
        }
    }

    
    
    void AgeCachedWords() {
        if (mWordCache.IsInitialized()) {
            (void)mWordCache.EnumerateEntries(AgeCacheEntry, this);
        }
    }

    
    void ClearCachedWords() {
        if (mWordCache.IsInitialized()) {
            mWordCache.Clear();
        }
    }

    virtual void SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;
    virtual void SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                     FontCacheSizes*   aSizes) const;

    typedef enum {
        FONT_TYPE_DWRITE,
        FONT_TYPE_GDI,
        FONT_TYPE_FT2,
        FONT_TYPE_MAC,
        FONT_TYPE_OS2,
        FONT_TYPE_CAIRO
    } FontType;

    virtual FontType GetType() const = 0;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont> GetScaledFont(mozilla::gfx::DrawTarget *aTarget)
    { return gfxPlatform::GetPlatform()->GetScaledFontForFont(aTarget, this); }

protected:
    
    bool ShapeText(gfxContext    *aContext,
                   const uint8_t *aText,
                   uint32_t       aOffset, 
                   uint32_t       aLength,
                   int32_t        aScript,
                   gfxShapedText *aShapedText, 
                   bool           aPreferPlatformShaping = false);

    
    
    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText,
                           bool             aPreferPlatformShaping = false);

    
    
    
    void PostShapingFixup(gfxContext      *aContext,
                          const PRUnichar *aText,
                          uint32_t         aOffset, 
                          uint32_t         aLength,
                          gfxShapedText   *aShapedText);

    
    
    
    
    
    
    
    template<typename T>
    bool ShapeTextWithoutWordCache(gfxContext *aContext,
                                   const T    *aText,
                                   uint32_t    aOffset,
                                   uint32_t    aLength,
                                   int32_t     aScript,
                                   gfxTextRun *aTextRun);

    
    
    
    
    
    template<typename T>
    bool ShapeFragmentWithoutWordCache(gfxContext *aContext,
                                       const T    *aText,
                                       uint32_t    aOffset,
                                       uint32_t    aLength,
                                       int32_t     aScript,
                                       gfxTextRun *aTextRun);

    nsRefPtr<gfxFontEntry> mFontEntry;

    struct CacheHashKey {
        union {
            const uint8_t   *mSingle;
            const PRUnichar *mDouble;
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

        CacheHashKey(const PRUnichar *aText, uint32_t aLength,
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

        
        
        CacheHashEntry(KeyTypePointer aKey) { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;

        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return aKey->mHashKey;
        }

        enum { ALLOW_MEMMOVE = true };

        nsAutoPtr<gfxShapedWord> mShapedWord;
    };

    static size_t
    WordCacheEntrySizeOfExcludingThis(CacheHashEntry*   aHashEntry,
                                      nsMallocSizeOfFun aMallocSizeOf,
                                      void*             aUserArg);

    nsTHashtable<CacheHashEntry> mWordCache;

    static PLDHashOperator AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData);
    static const uint32_t  kShapedWordCacheMaxAge = 3;

    bool                       mIsValid;

    
    
    bool                       mApplySyntheticBold;

    nsExpirationState          mExpirationState;
    gfxFontStyle               mStyle;
    nsAutoTArray<gfxGlyphExtents*,1> mGlyphExtentsArray;

    gfxFloat                   mAdjustedSize;

    float                      mFUnitsConvFactor; 

    
    AntialiasOption            mAntialiasOption;

    
    
    nsAutoPtr<gfxFont>         mNonAAFont;

    
    
    nsAutoPtr<gfxFontShaper>   mPlatformShaper;
    nsAutoPtr<gfxFontShaper>   mHarfBuzzShaper;
    nsAutoPtr<gfxFontShaper>   mGraphiteShaper;

    mozilla::RefPtr<mozilla::gfx::ScaledFont> mAzureScaledFont;

    
    
    
    virtual void CreatePlatformShaper() { }

    
    
    
    
    
    
    
    
    bool InitMetricsFromSfntTables(Metrics& aMetrics);

    
    
    void CalculateDerivedMetrics(Metrics& aMetrics);

    
    
    void SanitizeMetrics(gfxFont::Metrics *aMetrics, bool aIsBadUnderlineFont);

    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextObjectPaint *aObjectPaint);
    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextObjectPaint *aObjectPaint,
                        gfxTextRunDrawCallbacks *aCallbacks,
                        bool& aEmittedGlyphs);

    
    
    
    
    
    
    
    static double CalcXScale(gfxContext *aContext);
};


#define DEFAULT_XHEIGHT_FACTOR 0.56f




















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

    bool IsClusterStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return GetCharacterGlyphs()[aPos].IsClusterStart();
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
                                const PRUnichar *aString,
                                uint32_t         aLength);
    
    
    void SetupClusterBoundaries(uint32_t       aOffset,
                                const uint8_t *aString,
                                uint32_t       aLength);

    uint32_t Flags() const {
        return mFlags;
    }

    bool IsRightToLeft() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_RTL) != 0;
    }

    float GetDirection() const {
        return IsRightToLeft() ? -1.0f : 1.0f;
    }

    bool DisableLigatures() const {
        return (Flags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;
    }

    bool TextIs8Bit() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_8BIT) != 0;
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
            if (!details) {
                return nullptr;
            }
            
            
            
            
            if (mOffsetToIndex.Length() == 0 ||
                aOffset > mOffsetToIndex[mOffsetToIndex.Length() - 1].mOffset) {
                if (!mOffsetToIndex.AppendElement(DGRec(aOffset, detailIndex))) {
                    return nullptr;
                }
            } else {
                if (!mOffsetToIndex.InsertElementSorted(DGRec(aOffset, detailIndex),
                                                        CompareRecordOffsets())) {
                    return nullptr;
                }
            }
            return details;
        }

        size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) {
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
    static const uint32_t kMaxLength = 32;

    
    
    
    
    
    
    
    
    
    static gfxShapedWord* Create(const uint8_t *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= kMaxLength, "excessive length for gfxShapedWord!");

        
        
        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(uint8_t));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nullptr;
        }

        
        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    static gfxShapedWord* Create(const PRUnichar *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= kMaxLength, "excessive length for gfxShapedWord!");

        
        
        
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            nsAutoCString narrowText;
            LossyAppendUTF16toASCII(nsDependentSubstring(aText, aLength),
                                    narrowText);
            return Create((const uint8_t*)(narrowText.BeginReading()),
                          aLength, aRunScript, aAppUnitsPerDevUnit, aFlags);
        }

        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(PRUnichar));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nullptr;
        }

        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    CompressedGlyph *GetCharacterGlyphs() {
        return &mCharGlyphsStorage[0];
    }

    const uint8_t* Text8Bit() const {
        NS_ASSERTION(TextIs8Bit(), "invalid use of Text8Bit()");
        return reinterpret_cast<const uint8_t*>(mCharGlyphsStorage + GetLength());
    }

    const PRUnichar* TextUnicode() const {
        NS_ASSERTION(!TextIs8Bit(), "invalid use of TextUnicode()");
        return reinterpret_cast<const PRUnichar*>(mCharGlyphsStorage + GetLength());
    }

    PRUnichar GetCharAt(uint32_t aOffset) const {
        NS_ASSERTION(aOffset < GetLength(), "aOffset out of range");
        return TextIs8Bit() ?
            PRUnichar(Text8Bit()[aOffset]) : TextUnicode()[aOffset];
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

    gfxShapedWord(const PRUnichar *aText, uint32_t aLength,
                  int32_t aRunScript, int32_t aAppUnitsPerDevUnit,
                  uint32_t aFlags)
        : gfxShapedText(aLength, aFlags, aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharGlyphsStorage, 0, aLength * sizeof(CompressedGlyph));
        PRUnichar *text = reinterpret_cast<PRUnichar*>(&mCharGlyphsStorage[aLength]);
        memcpy(text, aText, aLength * sizeof(PRUnichar));
        SetupClusterBoundaries(0, aText, aLength);
    }

    int32_t          mScript;

    uint32_t         mAgeCounter;

    
    
    
    
    
    CompressedGlyph  mCharGlyphsStorage[1];
};





struct gfxTextRunDrawCallbacks {

    







    gfxTextRunDrawCallbacks(bool aShouldPaintSVGGlyphs = false)
      : mShouldPaintSVGGlyphs(aShouldPaintSVGGlyphs)
    {
    }

    




    virtual void NotifyGlyphPathEmitted() = 0;

    


    virtual void NotifyBeforeSVGGlyphPainted() { }

    


    virtual void NotifyAfterSVGGlyphPainted() { }

    bool mShouldPaintSVGGlyphs;
};




















class THEBES_API gfxTextRun : public gfxShapedText {
public:

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    virtual ~gfxTextRun();

    typedef gfxFont::RunMetrics Metrics;

    

    bool IsClusterStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    bool IsLigatureGroupStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }
    bool CanBreakLineBefore(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_NORMAL;
    }
    bool CanHyphenateBefore(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_HYPHEN;
    }

    bool CharIsSpace(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsSpace();
    }
    bool CharIsTab(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsTab();
    }
    bool CharIsNewline(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsNewline();
    }
    bool CharIsLowSurrogate(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsLowSurrogate();
    }

    uint32_t GetLength() { return mLength; }

    
    
    
    
    

    











    virtual bool SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                          uint8_t *aBreakBefore,
                                          gfxContext *aRefContext);

    









    class PropertyProvider {
    public:
        
        
        virtual void GetHyphenationBreaks(uint32_t aStart, uint32_t aLength,
                                          bool *aBreakBefore) = 0;

        
        
        
        virtual int8_t GetHyphensOption() = 0;

        
        
        virtual gfxFloat GetHyphenWidth() = 0;

        typedef gfxFont::Spacing Spacing;

        





        virtual void GetSpacing(uint32_t aStart, uint32_t aLength,
                                Spacing *aSpacing) = 0;
    };

    class ClusterIterator {
    public:
        ClusterIterator(gfxTextRun *aTextRun);

        void Reset();

        bool NextCluster();

        uint32_t Position() const {
            return mCurrentChar;
        }

        uint32_t ClusterLength() const;

        gfxFloat ClusterAdvance(PropertyProvider *aProvider) const;

    private:
        gfxTextRun *mTextRun;
        uint32_t    mCurrentChar;
    };

    




















    void Draw(gfxContext *aContext, gfxPoint aPt,
              gfxFont::DrawMode aDrawMode,
              uint32_t aStart, uint32_t aLength,
              PropertyProvider *aProvider,
              gfxFloat *aAdvanceWidth, gfxTextObjectPaint *aObjectPaint,
              gfxTextRunDrawCallbacks *aCallbacks = nullptr);

    




    Metrics MeasureText(uint32_t aStart, uint32_t aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContextForTightBoundingBox,
                        PropertyProvider *aProvider);

    



    gfxFloat GetAdvanceWidth(uint32_t aStart, uint32_t aLength,
                             PropertyProvider *aProvider);

    


























    virtual bool SetLineBreaks(uint32_t aStart, uint32_t aLength,
                                 bool aLineBreakBefore, bool aLineBreakAfter,
                                 gfxFloat *aAdvanceWidthDelta,
                                 gfxContext *aRefContext);

    

























































    uint32_t BreakAndMeasureText(uint32_t aStart, uint32_t aMaxLength,
                                 bool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 bool aSuppressInitialBreak,
                                 gfxFloat *aTrimWhitespace,
                                 Metrics *aMetrics,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContextForTightBoundingBox,
                                 bool *aUsedHyphenation,
                                 uint32_t *aLastBreak,
                                 bool aCanWordWrap,
                                 gfxBreakPriority *aBreakPriority);

    




    void SetContext(gfxContext *aContext) {}

    

    gfxFloat GetDirection() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) ? -1.0 : 1.0; }
    void *GetUserData() const { return mUserData; }
    void SetUserData(void *aUserData) { mUserData = aUserData; }
    uint32_t GetFlags() const { return mFlags; }
    void SetFlagBits(uint32_t aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags |= aFlags;
    }
    void ClearFlagBits(uint32_t aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags &= ~aFlags;
    }
    const gfxSkipChars& GetSkipChars() const { return mSkipChars; }
    gfxFontGroup *GetFontGroup() const { return mFontGroup; }


    
    
    static gfxTextRun *Create(const gfxTextRunFactory::Parameters *aParams,
                              uint32_t aLength, gfxFontGroup *aFontGroup,
                              uint32_t aFlags);

    
    struct GlyphRun {
        nsRefPtr<gfxFont> mFont;   
        uint32_t          mCharacterOffset; 
        uint8_t           mMatchType;
    };

    class THEBES_API GlyphRunIterator {
    public:
        GlyphRunIterator(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aLength)
          : mTextRun(aTextRun), mStartOffset(aStart), mEndOffset(aStart + aLength) {
            mNextIndex = mTextRun->FindFirstGlyphRunContaining(aStart);
        }
        bool NextRun();
        GlyphRun *GetGlyphRun() { return mGlyphRun; }
        uint32_t GetStringStart() { return mStringStart; }
        uint32_t GetStringEnd() { return mStringEnd; }
    private:
        gfxTextRun *mTextRun;
        GlyphRun   *mGlyphRun;
        uint32_t    mStringStart;
        uint32_t    mStringEnd;
        uint32_t    mNextIndex;
        uint32_t    mStartOffset;
        uint32_t    mEndOffset;
    };

    class GlyphRunOffsetComparator {
    public:
        bool Equals(const GlyphRun& a,
                      const GlyphRun& b) const
        {
            return a.mCharacterOffset == b.mCharacterOffset;
        }

        bool LessThan(const GlyphRun& a,
                        const GlyphRun& b) const
        {
            return a.mCharacterOffset < b.mCharacterOffset;
        }
    };

    friend class GlyphRunIterator;
    friend class FontSelector;

    
    
    












    nsresult AddGlyphRun(gfxFont *aFont, uint8_t aMatchType,
                         uint32_t aStartCharIndex, bool aForceNewRun);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    void SortGlyphRuns();
    void SanitizeGlyphRuns();

    CompressedGlyph* GetCharacterGlyphs() {
        NS_ASSERTION(mCharacterGlyphs, "failed to initialize mCharacterGlyphs");
        return mCharacterGlyphs;
    }

    void SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, uint32_t aCharIndex);

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                               uint32_t aCharIndex, PRUnichar aSpaceChar);

    
    
    
    
    
    
    void SetIsTab(uint32_t aIndex) {
        CompressedGlyph *g = &mCharacterGlyphs[aIndex];
        if (g->IsSimpleGlyph()) {
            DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
            details->mGlyphID = g->GetSimpleGlyph();
            details->mAdvance = g->GetSimpleAdvance();
            details->mXOffset = details->mYOffset = 0;
            SetGlyphs(aIndex, CompressedGlyph().SetComplex(true, true, 1), details);
        }
        g->SetIsTab();
    }
    void SetIsNewline(uint32_t aIndex) {
        CompressedGlyph *g = &mCharacterGlyphs[aIndex];
        if (g->IsSimpleGlyph()) {
            DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
            details->mGlyphID = g->GetSimpleGlyph();
            details->mAdvance = g->GetSimpleAdvance();
            details->mXOffset = details->mYOffset = 0;
            SetGlyphs(aIndex, CompressedGlyph().SetComplex(true, true, 1), details);
        }
        g->SetIsNewline();
    }
    void SetIsLowSurrogate(uint32_t aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nullptr);
        mCharacterGlyphs[aIndex].SetIsLowSurrogate();
    }

    





    void FetchGlyphExtents(gfxContext *aRefContext);

    uint32_t CountMissingGlyphs();
    const GlyphRun *GetGlyphRuns(uint32_t *aNumGlyphRuns) {
        *aNumGlyphRuns = mGlyphRuns.Length();
        return mGlyphRuns.Elements();
    }
    
    
    uint32_t FindFirstGlyphRunContaining(uint32_t aOffset);

    
    void CopyGlyphDataFrom(gfxShapedWord *aSource, uint32_t aStart);

    
    
    void CopyGlyphDataFrom(gfxTextRun *aSource, uint32_t aStart,
                           uint32_t aLength, uint32_t aDest);

    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    
    
    
    
    
    void ReleaseFontGroup();

    struct LigatureData {
        
        uint32_t mLigatureStart;
        uint32_t mLigatureEnd;
        
        
        gfxFloat mPartAdvance;
        
        
        
        gfxFloat mPartWidth;
        
        bool mClipBeforePart;
        bool mClipAfterPart;
    };
    
    
    
    virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
      MOZ_MUST_OVERRIDE;
    virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
      MOZ_MUST_OVERRIDE;

    
    size_t MaybeSizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)  {
        if (mFlags & gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED) {
            return 0;
        }
        mFlags |= gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
        return SizeOfIncludingThis(aMallocSizeOf);
    }
    void ResetSizeOfAccountingFlags() {
        mFlags &= ~gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
    }

#ifdef DEBUG
    void Dump(FILE* aOutput);
#endif

protected:
    





    gfxTextRun(const gfxTextRunFactory::Parameters *aParams,
               uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags);

    




    static void* AllocateStorageForTextRun(size_t aSize, uint32_t aLength);

    
    
    CompressedGlyph *mCharacterGlyphs;

private:
    

    
    DetailedGlyph *AllocateDetailedGlyphs(uint32_t aCharIndex, uint32_t aCount);

    
    int32_t GetAdvanceForGlyphs(uint32_t aStart, uint32_t aEnd);

    
    
    
    
    bool GetAdjustedSpacingArray(uint32_t aStart, uint32_t aEnd,
                                   PropertyProvider *aProvider,
                                   uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);

    
    
    

    
    LigatureData ComputeLigatureData(uint32_t aPartStart, uint32_t aPartEnd,
                                     PropertyProvider *aProvider);
    gfxFloat ComputePartialLigatureWidth(uint32_t aPartStart, uint32_t aPartEnd,
                                         PropertyProvider *aProvider);
    void DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx,
                             uint32_t aStart, uint32_t aEnd, gfxPoint *aPt,
                             PropertyProvider *aProvider,
                             gfxTextRunDrawCallbacks *aCallbacks);
    
    
    void ShrinkToLigatureBoundaries(uint32_t *aStart, uint32_t *aEnd);
    
    gfxFloat GetPartialLigatureWidth(uint32_t aStart, uint32_t aEnd, PropertyProvider *aProvider);
    void AccumulatePartialLigatureMetrics(gfxFont *aFont,
                                          uint32_t aStart, uint32_t aEnd,
                                          gfxFont::BoundingBoxType aBoundingBoxType,
                                          gfxContext *aRefContext,
                                          PropertyProvider *aProvider,
                                          Metrics *aMetrics);

    
    void AccumulateMetricsForRun(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContext,
                                 PropertyProvider *aProvider,
                                 uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                 Metrics *aMetrics);

    
    void DrawGlyphs(gfxFont *aFont, gfxContext *aContext,
                    gfxFont::DrawMode aDrawMode, gfxPoint *aPt,
                    gfxTextObjectPaint *aObjectPaint, uint32_t aStart,
                    uint32_t aEnd, PropertyProvider *aProvider,
                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                    gfxTextRunDrawCallbacks *aCallbacks);

    
    
    nsAutoTArray<GlyphRun,1>        mGlyphRuns;

    void             *mUserData;
    gfxFontGroup     *mFontGroup; 
                                  
    gfxSkipChars      mSkipChars;
    nsExpirationState mExpirationState;

    bool              mSkipDrawing; 
                                    
                                    
    bool              mReleasedFontGroup; 
                                          
};

class THEBES_API gfxFontGroup : public gfxTextRunFactory {
public:
    class FamilyFace {
    public:
        FamilyFace() { }

        FamilyFace(gfxFontFamily* aFamily, gfxFont* aFont)
            : mFamily(aFamily), mFont(aFont)
        {
            NS_ASSERTION(aFont, "font pointer must not be null");
            NS_ASSERTION(!aFamily ||
                         aFamily->ContainsFace(aFont->GetFontEntry()),
                         "font is not a member of the given family");
        }

        gfxFontFamily* Family() const { return mFamily.get(); }
        gfxFont* Font() const { return mFont.get(); }

    private:
        nsRefPtr<gfxFontFamily> mFamily;
        nsRefPtr<gfxFont>       mFont;
    };

    static void Shutdown(); 

    gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle, gfxUserFontSet *aUserFontSet = nullptr);

    virtual ~gfxFontGroup();

    virtual gfxFont *GetFontAt(int32_t i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");
        NS_ASSERTION(mFonts.Length() > uint32_t(i) && mFonts[i].Font(), 
                     "Requesting a font index that doesn't exist");

        return mFonts[i].Font();
    }

    uint32_t FontListLength() const {
        return mFonts.Length();
    }

    bool Equals(const gfxFontGroup& other) const {
        return mFamilies.Equals(other.mFamilies) &&
            mStyle.Equals(other.mStyle);
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    



    static bool IsInvalidChar(uint8_t ch);
    static bool IsInvalidChar(PRUnichar ch);

    





    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags);
    





    virtual gfxTextRun *MakeTextRun(const uint8_t *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags);

    



    template<typename T>
    gfxTextRun *MakeTextRun(const T *aString, uint32_t aLength,
                            gfxContext *aRefContext,
                            int32_t aAppUnitsPerDevUnit,
                            uint32_t aFlags)
    {
        gfxTextRunFactory::Parameters params = {
            aRefContext, nullptr, nullptr, nullptr, 0, aAppUnitsPerDevUnit
        };
        return MakeTextRun(aString, aLength, &params, aFlags);
    }

    


    typedef bool (*FontCreationCallback) (const nsAString& aName,
                                            const nsACString& aGenericName,
                                            bool aUseFontSet,
                                            void *closure);
    bool ForEachFont(const nsAString& aFamilies,
                       nsIAtom *aLanguage,
                       FontCreationCallback fc,
                       void *closure);
    bool ForEachFont(FontCreationCallback fc, void *closure);

    



    bool HasFont(const gfxFontEntry *aFontEntry);

    const nsString& GetFamilies() { return mFamilies; }

    
    
    
    
    
    enum { UNDERLINE_OFFSET_NOT_SET = INT16_MAX };
    virtual gfxFloat GetUnderlineOffset() {
        if (mUnderlineOffset == UNDERLINE_OFFSET_NOT_SET)
            mUnderlineOffset = GetFontAt(0)->GetMetrics().underlineOffset;
        return mUnderlineOffset;
    }

    virtual already_AddRefed<gfxFont>
        FindFontForChar(uint32_t ch, uint32_t prevCh, int32_t aRunScript,
                        gfxFont *aPrevMatchedFont,
                        uint8_t *aMatchType);

    
    virtual already_AddRefed<gfxFont> WhichPrefFontSupportsChar(uint32_t aCh);

    virtual already_AddRefed<gfxFont>
        WhichSystemFontSupportsChar(uint32_t aCh, int32_t aRunScript);

    template<typename T>
    void ComputeRanges(nsTArray<gfxTextRange>& mRanges,
                       const T *aString, uint32_t aLength,
                       int32_t aRunScript);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    uint64_t GetGeneration();

    
    
    virtual void UpdateFontList();

    bool ShouldSkipDrawing() const {
        return mSkipDrawing;
    }

    class LazyReferenceContextGetter {
    public:
      virtual already_AddRefed<gfxContext> GetRefContext() = 0;
    };
    
    
    
    
    
    gfxTextRun* GetEllipsisTextRun(int32_t aAppUnitsPerDevPixel,
                                   LazyReferenceContextGetter& aRefContextGetter);

protected:
    nsString mFamilies;
    gfxFontStyle mStyle;
    nsTArray<FamilyFace> mFonts;
    gfxFloat mUnderlineOffset;

    gfxUserFontSet* mUserFontSet;
    uint64_t mCurrGeneration;  

    
    
    nsAutoPtr<gfxTextRun>   mCachedEllipsisTextRun;

    
    nsRefPtr<gfxFontFamily> mLastPrefFamily;
    nsRefPtr<gfxFont>       mLastPrefFont;
    eFontPrefLang           mLastPrefLang;       
    eFontPrefLang           mPageLang;
    bool                    mLastPrefFirstFont;  

    bool                    mSkipDrawing; 
                                          
                                          

    



    gfxTextRun *MakeEmptyTextRun(const Parameters *aParams, uint32_t aFlags);
    gfxTextRun *MakeSpaceTextRun(const Parameters *aParams, uint32_t aFlags);
    gfxTextRun *MakeBlankTextRun(uint32_t aLength,
                                 const Parameters *aParams, uint32_t aFlags);

    
    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    void BuildFontList();

    
    
    
    void InitMetricsForBadFont(gfxFont* aBadFont);

    
    
    template<typename T>
    void InitTextRun(gfxContext *aContext,
                     gfxTextRun *aTextRun,
                     const T *aString,
                     uint32_t aLength);

    
    
    template<typename T>
    void InitScriptRun(gfxContext *aContext,
                       gfxTextRun *aTextRun,
                       const T *aString,
                       uint32_t aScriptRunStart,
                       uint32_t aScriptRunEnd,
                       int32_t aRunScript);

    









    bool ForEachFontInternal(const nsAString& aFamilies,
                               nsIAtom *aLanguage,
                               bool aResolveGeneric,
                               bool aResolveFontName,
                               bool aUseFontSet,
                               FontCreationCallback fc,
                               void *closure);

    
    
    
    already_AddRefed<gfxFont> TryAllFamilyMembers(gfxFontFamily* aFamily,
                                                  uint32_t aCh);

    static bool FontResolverProc(const nsAString& aName, void *aClosure);

    static bool FindPlatformFont(const nsAString& aName,
                                   const nsACString& aGenericName,
                                   bool aUseFontSet,
                                   void *closure);

    static NS_HIDDEN_(nsILanguageAtomService*) gLangService;
};
#endif
