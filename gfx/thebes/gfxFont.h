







































#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "prtypes.h"
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
class gfxShapedWord;

class nsILanguageAtomService;

typedef struct _hb_blob_t hb_blob_t;


#define FONT_STYLE_NORMAL              NS_FONT_STYLE_NORMAL
#define FONT_STYLE_ITALIC              NS_FONT_STYLE_ITALIC
#define FONT_STYLE_OBLIQUE             NS_FONT_STYLE_OBLIQUE


#define FONT_WEIGHT_NORMAL             NS_FONT_WEIGHT_NORMAL
#define FONT_WEIGHT_BOLD               NS_FONT_WEIGHT_BOLD

#define FONT_MAX_SIZE                  2000.0

#define NO_FONT_LANGUAGE_OVERRIDE      0


struct THEBES_API gfxFontFeature {
    PRUint32 mTag; 
    PRUint32 mValue; 
                     
};

inline bool
operator<(const gfxFontFeature& a, const gfxFontFeature& b)
{
    return (a.mTag < b.mTag) || ((a.mTag == b.mTag) && (a.mValue < b.mValue));
}

inline bool
operator==(const gfxFontFeature& a, const gfxFontFeature& b)
{
    return (a.mTag == b.mTag) && (a.mValue == b.mValue);
}


struct THEBES_API gfxFontStyle {
    gfxFontStyle();
    gfxFontStyle(PRUint8 aStyle, PRUint16 aWeight, PRInt16 aStretch,
                 gfxFloat aSize, nsIAtom *aLanguage,
                 float aSizeAdjust, bool aSystemFont,
                 bool aPrinterFont,
                 const nsString& aFeatureSettings,
                 const nsString& aLanguageOverride);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    PRUint8 style : 7;

    
    
    
    bool systemFont : 1;

    
    bool printerFont : 1;

    
    PRUint16 weight;

    
    
    PRInt16 stretch;

    
    gfxFloat size;

    
    
    
    
    float sizeAdjust;

    
    
    
    nsRefPtr<nsIAtom> language;

    
    
    
    
    
    
    
    
    
    
    PRUint32 languageOverride;

    
    nsTArray<gfxFontFeature> featureSettings;

    
    
    gfxFloat GetAdjustedSize(gfxFloat aspect) const {
        NS_ASSERTION(sizeAdjust != 0.0, "Not meant to be called when sizeAdjust = 0");
        gfxFloat adjustedSize = NS_MAX(NS_round(size*(sizeAdjust/aspect)), 1.0);
        return NS_MIN(adjustedSize, FONT_MAX_SIZE);
    }

    PLDHashNumber Hash() const {
        return ((style + (systemFont << 7) +
            (weight << 8)) + PRUint32(size*1000) + PRUint32(sizeAdjust*1000)) ^
            nsISupportsHashKey::HashKey(language);
    }

    PRInt8 ComputeWeight() const;

    bool Equals(const gfxFontStyle& other) const {
        return (size == other.size) &&
            (style == other.style) &&
            (systemFont == other.systemFont) &&
            (printerFont == other.printerFont) &&
            (weight == other.weight) &&
            (stretch == other.stretch) &&
            (language == other.language) &&
            (sizeAdjust == other.sizeAdjust) &&
            (featureSettings == other.featureSettings) &&
            (languageOverride == other.languageOverride);
    }

    static void ParseFontFeatureSettings(const nsString& aFeatureString,
                                         nsTArray<gfxFontFeature>& aFeatures);

    static PRUint32 ParseFontLanguageOverride(const nsString& aLangTag);
};

class gfxFontEntry {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFontEntry)

    gfxFontEntry(const nsAString& aName, gfxFontFamily *aFamily = nsnull,
                 bool aIsStandardFace = false) : 
        mName(aName), mItalic(false), mFixedPitch(false),
        mIsProxy(false), mIsValid(true), 
        mIsBadUnderlineFont(false), mIsUserFont(false),
        mIsLocalUserFont(false), mStandardFace(aIsStandardFace),
        mSymbolFont(false),
        mIgnoreGDEF(false),
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
#ifdef MOZ_GRAPHITE
        mCheckedForGraphiteTables(false),
#endif
        mHasCmapTable(false),
        mCmapInitialized(false),
        mUVSOffset(0), mUVSData(nsnull),
        mUserFontData(nsnull),
        mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE),
        mFamily(aFamily)
    { }

    virtual ~gfxFontEntry();

    
    
    const nsString& Name() const { return mName; }

    
    
    virtual nsString RealFaceName();

    gfxFontFamily* Family() const { return mFamily; }

    PRUint16 Weight() const { return mWeight; }
    PRInt16 Stretch() const { return mStretch; }

    bool IsUserFont() const { return mIsUserFont; }
    bool IsLocalUserFont() const { return mIsLocalUserFont; }
    bool IsFixedPitch() const { return mFixedPitch; }
    bool IsItalic() const { return mItalic; }
    bool IsBold() const { return mWeight >= 600; } 
    bool IgnoreGDEF() const { return mIgnoreGDEF; }

    virtual bool IsSymbolFont();

#ifdef MOZ_GRAPHITE
    inline bool HasGraphiteTables() {
        if (!mCheckedForGraphiteTables) {
            CheckForGraphiteTables();
            mCheckedForGraphiteTables = true;
        }
        return mHasGraphiteTables;
    }
#endif

    inline bool HasCmapTable() {
        if (!mCmapInitialized) {
            ReadCMAP();
        }
        return mHasCmapTable;
    }

    inline bool HasCharacter(PRUint32 ch) {
        if (mCharacterMap.test(ch))
            return true;

        return TestCharacterMap(ch);
    }

    virtual bool SkipDuringSystemFallback() { return false; }
    virtual bool TestCharacterMap(PRUint32 aCh);
    nsresult InitializeUVSMap();
    PRUint16 GetUVSGlyph(PRUint32 aCh, PRUint32 aVS);
    virtual nsresult ReadCMAP();

    virtual bool MatchesGenericFamily(const nsACString& aGeneric) const {
        return true;
    }
    virtual bool SupportsLangGroup(nsIAtom *aLangGroup) const {
        return true;
    }

    virtual nsresult GetFontTable(PRUint32 aTableTag, FallibleTArray<PRUint8>& aBuffer) {
        return NS_ERROR_FAILURE; 
    }

    void SetFamily(gfxFontFamily* aFamily) {
        mFamily = aFamily;
    }

    virtual nsString FamilyName() const;

    already_AddRefed<gfxFont> FindOrMakeFont(const gfxFontStyle *aStyle,
                                             bool aNeedsBold);

    
    
    
    
    
    
    bool GetExistingFontTable(PRUint32 aTag, hb_blob_t** aBlob);

    
    
    
    
    
    
    
    hb_blob_t *ShareFontTableAndGetBlob(PRUint32 aTag,
                                        FallibleTArray<PRUint8>* aTable);

    nsString         mName;

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

    PRUint16         mWeight;
    PRInt16          mStretch;

#ifdef MOZ_GRAPHITE
    bool             mHasGraphiteTables;
    bool             mCheckedForGraphiteTables;
#endif
    bool             mHasCmapTable;
    bool             mCmapInitialized;
    gfxSparseBitSet  mCharacterMap;
    PRUint32         mUVSOffset;
    nsAutoArrayPtr<PRUint8> mUVSData;
    gfxUserFontData* mUserFontData;

    nsTArray<gfxFontFeature> mFeatureSettings;
    PRUint32         mLanguageOverride;

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
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
#ifdef MOZ_GRAPHITE
        mCheckedForGraphiteTables(false),
#endif
        mHasCmapTable(false),
        mCmapInitialized(false),
        mUVSOffset(0), mUVSData(nsnull),
        mUserFontData(nsnull),
        mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE),
        mFamily(nsnull)
    { }

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold) {
        NS_NOTREACHED("oops, somebody didn't override CreateFontInstance");
        return nsnull;
    }

#ifdef MOZ_GRAPHITE
    virtual void CheckForGraphiteTables();
#endif

    gfxFontFamily *mFamily;

private:

    























    class FontTableBlobData;

    














    class FontTableHashEntry : public nsUint32HashKey
    {
    public:
        

        typedef nsUint32HashKey KeyClass;
        typedef KeyClass::KeyType KeyType;
        typedef KeyClass::KeyTypePointer KeyTypePointer;

        FontTableHashEntry(KeyTypePointer aTag)
            : KeyClass(aTag), mBlob() { };
        
        FontTableHashEntry(FontTableHashEntry& toCopy)
            : KeyClass(toCopy), mBlob(toCopy.mBlob)
        {
            toCopy.mBlob = nsnull;
        }

        ~FontTableHashEntry() { Clear(); }

        

        
        
        
        
        hb_blob_t *
        ShareTableAndGetBlob(FallibleTArray<PRUint8>& aTable,
                             nsTHashtable<FontTableHashEntry> *aHashtable);

        
        
        void SaveTable(FallibleTArray<PRUint8>& aTable);

        
        
        hb_blob_t *GetBlob() const;

        void Clear();

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



struct FontSearch {
    FontSearch(const PRUint32 aCharacter, gfxFont *aFont) :
        mCh(aCharacter), mFontToMatch(aFont), mMatchRank(0), mCount(0) {
    }
    const PRUint32         mCh;
    gfxFont*               mFontToMatch;
    PRInt32                mMatchRank;
    nsRefPtr<gfxFontEntry> mBestMatch;
    PRUint32               mCount;
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
        mIsBadUnderlineFamily(false)
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
        mAvailableFonts.AppendElement(aFontEntry);
        aFontEntry->SetFamily(this);
    }

    
    void SetHasStyles(bool aHasStyles) { mHasStyles = aHasStyles; }

    
    
    
    
    
    gfxFontEntry *FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                   bool& aNeedsSyntheticBold);

    
    
    void FindFontForChar(FontSearch *aMatchData);

    
    virtual void ReadOtherFamilyNames(gfxPlatformFontList *aPlatformFontList);

    
    void SetOtherFamilyNamesInitialized() {
        mOtherFamilyNamesInitialized = true;
    }

    
    
    virtual void ReadFaceNames(gfxPlatformFontList *aPlatformFontList,
                               bool aNeedFullnamePostscriptNames);

    
    
    virtual void FindStyleVariations() { }

    
    gfxFontEntry* FindFont(const nsAString& aPostscriptName);

    
    void ReadCMAP() {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        
        
        for (i = 0; i < numFonts; i++)
            mAvailableFonts[i]->ReadCMAP();
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

protected:
    
    
    virtual bool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                       bool anItalic, PRInt16 aStretch);

    bool ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                       FallibleTArray<PRUint8>& aNameTable,
                                       bool useFullName = false);

    
    void SetBadUnderlineFonts() {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++) {
            if (mAvailableFonts[i]) {
                mAvailableFonts[i]->mIsBadUnderlineFont = true;
            }
        }
    }

    nsString mName;
    nsTArray<nsRefPtr<gfxFontEntry> >  mAvailableFonts;
    bool mOtherFamilyNamesInitialized;
    bool mHasOtherFamilyNames;
    bool mFaceNamesInitialized;
    bool mHasStyles;
    bool mIsSimpleFamily;
    bool mIsBadUnderlineFamily;

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
    gfxTextRange(PRUint32 aStart, PRUint32 aEnd,
                 gfxFont* aFont, PRUint8 aMatchType)
        : start(aStart),
          end(aEnd),
          font(aFont),
          matchType(aMatchType)
    { }
    PRUint32 Length() const { return end - start; }
    PRUint32 start, end;
    nsRefPtr<gfxFont> font;
    PRUint8 matchType;
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
        mFonts.EnumerateEntries(ClearCachedWordsForFont, nsnull);
    }

protected:
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

        
        
        HashEntry(KeyTypePointer aStr) : mFont(nsnull) { }
        HashEntry(const HashEntry& toCopy) : mFont(toCopy.mFont) { }
        ~HashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return NS_PTR_TO_INT32(aKey->mFontEntry) ^ aKey->mStyle->Hash();
        }
        enum { ALLOW_MEMMOVE = true };

        gfxFont* mFont;
    };

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
        
        
        PRUint32     *mInitialBreaks;
        PRUint32      mInitialBreakCount;
        
        PRUint32      mAppUnitsPerDevUnit;
    };

    virtual ~gfxTextRunFactory() {}
};














class THEBES_API gfxGlyphExtents {
public:
    gfxGlyphExtents(PRUint32 aAppUnitsPerDevUnit) :
        mAppUnitsPerDevUnit(aAppUnitsPerDevUnit) {
        MOZ_COUNT_CTOR(gfxGlyphExtents);
        mTightGlyphExtents.Init();
    }
    ~gfxGlyphExtents();

    enum { INVALID_WIDTH = 0xFFFF };

    
    
    
    
    PRUint16 GetContainedGlyphWidthAppUnits(PRUint32 aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID);
    }

    bool IsGlyphKnown(PRUint32 aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID) != INVALID_WIDTH ||
            mTightGlyphExtents.GetEntry(aGlyphID) != nsnull;
    }

    bool IsGlyphKnownWithTightExtents(PRUint32 aGlyphID) const {
        return mTightGlyphExtents.GetEntry(aGlyphID) != nsnull;
    }

    
    
    
    bool GetTightGlyphExtentsAppUnits(gfxFont *aFont, gfxContext *aContext,
            PRUint32 aGlyphID, gfxRect *aExtents);

    void SetContainedGlyphWidthAppUnits(PRUint32 aGlyphID, PRUint16 aWidth) {
        mContainedGlyphWidths.Set(aGlyphID, aWidth);
    }
    void SetTightGlyphExtents(PRUint32 aGlyphID, const gfxRect& aExtentsAppUnits);

    PRUint32 GetAppUnitsPerDevUnit() { return mAppUnitsPerDevUnit; }

private:
    class HashEntry : public nsUint32HashKey {
    public:
        
        
        HashEntry(KeyTypePointer aPtr) : nsUint32HashKey(aPtr) {}
        HashEntry(const HashEntry& toCopy) : nsUint32HashKey(toCopy) {
          x = toCopy.x; y = toCopy.y; width = toCopy.width; height = toCopy.height;
        }

        float x, y, width, height;
    };

    typedef PRUptrdiff PtrBits;
    enum { BLOCK_SIZE_BITS = 7, BLOCK_SIZE = 1 << BLOCK_SIZE_BITS }; 

    class GlyphWidths {
    public:
        void Set(PRUint32 aIndex, PRUint16 aValue);
        PRUint16 Get(PRUint32 aIndex) const {
            PRUint32 block = aIndex >> BLOCK_SIZE_BITS;
            if (block >= mBlocks.Length())
                return INVALID_WIDTH;
            PtrBits bits = mBlocks[block];
            if (!bits)
                return INVALID_WIDTH;
            PRUint32 indexInBlock = aIndex & (BLOCK_SIZE - 1);
            if (bits & 0x1) {
                if (GetGlyphOffset(bits) != indexInBlock)
                    return INVALID_WIDTH;
                return GetWidth(bits);
            }
            PRUint16 *widths = reinterpret_cast<PRUint16 *>(bits);
            return widths[indexInBlock];
        }

#ifdef DEBUG
        PRUint32 ComputeSize();
#endif
        
        ~GlyphWidths();

    private:
        static PRUint32 GetGlyphOffset(PtrBits aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return (aBits >> 1) & ((1 << BLOCK_SIZE_BITS) - 1);
        }
        static PRUint32 GetWidth(PtrBits aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return aBits >> (1 + BLOCK_SIZE_BITS);
        }
        static PtrBits MakeSingle(PRUint32 aGlyphOffset, PRUint16 aWidth) {
            return (aWidth << (1 + BLOCK_SIZE_BITS)) + (aGlyphOffset << 1) + 1;
        }

        nsTArray<PtrBits> mBlocks;
    };
    
    GlyphWidths             mContainedGlyphWidths;
    nsTHashtable<HashEntry> mTightGlyphExtents;
    PRUint32                mAppUnitsPerDevUnit;
};



















class gfxFontShaper {
public:
    gfxFontShaper(gfxFont *aFont)
        : mFont(aFont)
    {
        NS_ASSERTION(aFont, "shaper requires a valid font!");
    }

    virtual ~gfxFontShaper() { }

    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText) = 0;

    gfxFont *GetFont() const { return mFont; }

protected:
    
    gfxFont * mFont;
};


class THEBES_API gfxFont {
public:
    nsrefcnt AddRef(void) {
        NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
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

    PRInt32 GetRefCount() { return mRefCnt; }

    
    typedef enum {
        kAntialiasDefault,
        kAntialiasNone,
        kAntialiasGrayscale,
        kAntialiasSubpixel
    } AntialiasOption;

    
    typedef enum {
        
        
        GLYPH_FILL = 1,
        
        GLYPH_STROKE = 2,
        
        
        GLYPH_PATH = 4
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
            cairo_scaled_font_t *aScaledFont = nsnull);

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

    cairo_scaled_font_t* GetCairoScaledFont() { return mScaledFont; }

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption) {
        
        return nsnull;
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

#ifdef MOZ_GRAPHITE
    
    bool FontCanSupportGraphite() {
        return mFontEntry->HasGraphiteTables();
    }
#endif

    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual hb_blob_t *GetFontTable(PRUint32 aTag);

    
    
    
    virtual bool ProvidesGetGlyph() const {
        return false;
    }
    
    
    virtual PRUint32 GetGlyph(PRUint32 unicode, PRUint32 variation_selector) {
        return 0;
    }

    
    
    
    virtual bool ProvidesGlyphWidths() {
        return false;
    }

    
    
    virtual PRInt32 GetGlyphWidth(gfxContext *aCtx, PRUint16 aGID) {
        return -1;
    }

    gfxFloat SynthesizeSpaceWidth(PRUint32 aCh);

    
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

    






















    virtual void Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                      gfxContext *aContext, DrawMode aDrawMode, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing);
    




















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);
    




    bool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   PRUint32 aStart, PRUint32 aLength)
    { return false; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    virtual PRUint32 GetSpaceGlyph() = 0;

    gfxGlyphExtents *GetOrCreateGlyphExtents(PRUint32 aAppUnitsPerDevUnit);

    
    virtual void SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
                                   bool aNeedTight, gfxGlyphExtents *aExtents);

    
    virtual bool SetupCairoFont(gfxContext *aContext) = 0;

    bool IsSyntheticBold() { return mApplySyntheticBold; }

    
    gfxFloat GetSyntheticBoldOffset() {
        return GetAdjustedSize() * (1.0 / 16.0);
    }

    gfxFontEntry *GetFontEntry() { return mFontEntry.get(); }
    bool HasCharacter(PRUint32 ch) {
        if (!mIsValid)
            return false;
        return mFontEntry->HasCharacter(ch); 
    }

    PRUint16 GetUVSGlyph(PRUint32 aCh, PRUint32 aVS) {
        if (!mIsValid) {
            return 0;
        }
        return mFontEntry->GetUVSGlyph(aCh, aVS); 
    }

    
    
    
    template<typename T>
    bool SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength,
                             PRInt32 aRunScript);

    
    
    template<typename T>
    gfxShapedWord* GetShapedWord(gfxContext *aContext,
                                 const T *aText,
                                 PRUint32 aLength,
                                 PRUint32 aHash,
                                 PRInt32 aRunScript,
                                 PRInt32 aAppUnitsPerDevUnit,
                                 PRUint32 aFlags);

    
    
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

protected:
    
    
    
    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText,
                           bool aPreferPlatformShaping = false);

    nsRefPtr<gfxFontEntry> mFontEntry;

    struct CacheHashKey {
        union {
            const PRUint8   *mSingle;
            const PRUnichar *mDouble;
        }                mText;
        PRUint32         mLength;
        PRUint32         mFlags;
        PRInt32          mScript;
        PRInt32          mAppUnitsPerDevUnit;
        PLDHashNumber    mHashKey;
        bool             mTextIs8Bit;

        CacheHashKey(const PRUint8 *aText, PRUint32 aLength,
                     PRUint32 aStringHash,
                     PRInt32 aScriptCode, PRInt32 aAppUnitsPerDevUnit,
                     PRUint32 aFlags)
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

        CacheHashKey(const PRUnichar *aText, PRUint32 aLength,
                     PRUint32 aStringHash,
                     PRInt32 aScriptCode, PRInt32 aAppUnitsPerDevUnit,
                     PRUint32 aFlags)
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

    nsTHashtable<CacheHashEntry> mWordCache;

    static PLDHashOperator AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData);
    static const PRUint32  kShapedWordCacheMaxAge = 3;

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
#ifdef MOZ_GRAPHITE
    nsAutoPtr<gfxFontShaper>   mGraphiteShaper;
#endif

    
    
    
    virtual void CreatePlatformShaper() { }

    
    
    
    
    
    
    
    
    bool InitMetricsFromSfntTables(Metrics& aMetrics);

    
    
    void CalculateDerivedMetrics(Metrics& aMetrics);

    
    
    void SanitizeMetrics(gfxFont::Metrics *aMetrics, bool aIsBadUnderlineFont);

    
    
    
    
    
    
    
    static double CalcXScale(gfxContext *aContext);
};


#define DEFAULT_XHEIGHT_FACTOR 0.56f






















class gfxShapedWord
{
public:
    static const PRUint32 kMaxLength = 0x7fff;

    
    
    
    
    
    
    
    
    
    static gfxShapedWord* Create(const PRUint8 *aText, PRUint32 aLength,
                                 PRInt32 aRunScript,
                                 PRInt32 aAppUnitsPerDevUnit,
                                 PRUint32 aFlags) {
        NS_ASSERTION(aLength <= kMaxLength, "excessive length for gfxShapedWord!");

        
        
        PRUint32 size =
            offsetof(gfxShapedWord, mCharacterGlyphs) +
            aLength * (sizeof(CompressedGlyph) + sizeof(PRUint8));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nsnull;
        }

        
        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    static gfxShapedWord* Create(const PRUnichar *aText, PRUint32 aLength,
                                 PRInt32 aRunScript,
                                 PRInt32 aAppUnitsPerDevUnit,
                                 PRUint32 aFlags) {
        NS_ASSERTION(aLength <= kMaxLength, "excessive length for gfxShapedWord!");

        
        
        
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            nsCAutoString narrowText;
            LossyAppendUTF16toASCII(nsDependentSubstring(aText, aLength),
                                    narrowText);
            return Create((const PRUint8*)(narrowText.BeginReading()),
                          aLength, aRunScript, aAppUnitsPerDevUnit, aFlags);
        }

        PRUint32 size =
            offsetof(gfxShapedWord, mCharacterGlyphs) +
            aLength * (sizeof(CompressedGlyph) + sizeof(PRUnichar));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nsnull;
        }

        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    














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
            
            GLYPH_COUNT_MASK = 0x00FFFF00U,
            GLYPH_COUNT_SHIFT = 8
        };

        
        
        
        

        
        static bool IsSimpleGlyphID(PRUint32 aGlyph) {
            return (aGlyph & GLYPH_MASK) == aGlyph;
        }
        
        
        static bool IsSimpleAdvance(PRUint32 aAdvance) {
            return (aAdvance & (ADVANCE_MASK >> ADVANCE_SHIFT)) == aAdvance;
        }

        bool IsSimpleGlyph() const { return (mValue & FLAG_IS_SIMPLE_GLYPH) != 0; }
        PRUint32 GetSimpleAdvance() const { return (mValue & ADVANCE_MASK) >> ADVANCE_SHIFT; }
        PRUint32 GetSimpleGlyph() const { return mValue & GLYPH_MASK; }

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

        void SetClusterStart(bool aIsClusterStart) {
            NS_ASSERTION(!IsSimpleGlyph(),
                         "can't call SetClusterStart on simple glyphs");
            if (aIsClusterStart) {
                mValue &= ~FLAG_NOT_CLUSTER_START;
            } else {
                mValue |= FLAG_NOT_CLUSTER_START;
            }
        }

        PRUint8 CanBreakBefore() const {
            return (mValue & FLAGS_CAN_BREAK_BEFORE) >> FLAGS_CAN_BREAK_SHIFT;
        }
        
        PRUint32 SetCanBreakBefore(PRUint8 aCanBreakBefore) {
            NS_ASSERTION(aCanBreakBefore <= 2,
                         "Bogus break-before value!");
            PRUint32 breakMask = (PRUint32(aCanBreakBefore) << FLAGS_CAN_BREAK_SHIFT);
            PRUint32 toggle = breakMask ^ (mValue & FLAGS_CAN_BREAK_BEFORE);
            mValue ^= toggle;
            return toggle;
        }

        CompressedGlyph& SetSimpleGlyph(PRUint32 aAdvanceAppUnits, PRUint32 aGlyph) {
            NS_ASSERTION(IsSimpleAdvance(aAdvanceAppUnits), "Advance overflow");
            NS_ASSERTION(IsSimpleGlyphID(aGlyph), "Glyph overflow");
            mValue = (mValue & FLAGS_CAN_BREAK_BEFORE) |
                FLAG_IS_SIMPLE_GLYPH |
                (aAdvanceAppUnits << ADVANCE_SHIFT) | aGlyph;
            return *this;
        }
        CompressedGlyph& SetComplex(bool aClusterStart, bool aLigatureStart,
                PRUint32 aGlyphCount) {
            mValue = (mValue & FLAGS_CAN_BREAK_BEFORE) |
                FLAG_NOT_MISSING |
                (aClusterStart ? 0 : FLAG_NOT_CLUSTER_START) |
                (aLigatureStart ? 0 : FLAG_NOT_LIGATURE_GROUP_START) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        



        CompressedGlyph& SetMissing(PRUint32 aGlyphCount) {
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_NOT_CLUSTER_START)) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        PRUint32 GetGlyphCount() const {
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
        PRUint32 mValue;
    };

    



    struct DetailedGlyph {
        

        PRUint32 mGlyphID;
        


   
        PRInt32  mAdvance;
        float    mXOffset, mYOffset;
    };

    bool IsClusterStart(PRUint32 aPos) {
        NS_ASSERTION(aPos < Length(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }

    bool IsLigatureGroupStart(PRUint32 aPos) {
        NS_ASSERTION(aPos < Length(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }

    PRUint32 Length() const {
        return mLength;
    }

    const PRUint8* Text8Bit() const {
        NS_ASSERTION(TextIs8Bit(), "invalid use of Text8Bit()");
        return reinterpret_cast<const PRUint8*>(&mCharacterGlyphs[Length()]);
    }

    const PRUnichar* TextUnicode() const {
        NS_ASSERTION(!TextIs8Bit(), "invalid use of TextUnicode()");
        return reinterpret_cast<const PRUnichar*>(&mCharacterGlyphs[Length()]);
    }

    PRUnichar GetCharAt(PRUint32 aOffset) const {
        NS_ASSERTION(aOffset < Length(), "aOffset out of range");
        return TextIs8Bit() ?
            PRUnichar(Text8Bit()[aOffset]) : TextUnicode()[aOffset];
    }

    PRUint32 Flags() const {
        return mFlags;
    }

    bool IsRightToLeft() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_RTL) != 0;
    }

    float GetDirection() const {
        return IsRightToLeft() ? -1.0 : 1.0;
    }

    bool DisableLigatures() const {
        return (Flags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;
    }

    bool TextIs8Bit() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_8BIT) != 0;
    }

    PRInt32 Script() const {
        return mScript;
    }

    PRInt32 AppUnitsPerDevUnit() const {
        return mAppUnitsPerDevUnit;
    }

    void ResetAge() {
        mAgeCounter = 0;
    }
    PRUint32 IncrementAge() {
        return ++mAgeCounter;
    }

    void SetSimpleGlyph(PRUint32 aCharIndex, CompressedGlyph aGlyph) {
        NS_ASSERTION(aGlyph.IsSimpleGlyph(), "Should be a simple glyph here");
        NS_ASSERTION(mCharacterGlyphs, "mCharacterGlyphs pointer is null!");
        mCharacterGlyphs[aCharIndex] = aGlyph;
    }

    void SetGlyphs(PRUint32 aCharIndex, CompressedGlyph aGlyph,
                   const DetailedGlyph *aGlyphs);

    void SetMissingGlyph(PRUint32 aIndex, PRUint32 aChar, gfxFont *aFont);

    void SetIsSpace(PRUint32 aIndex) {
        mCharacterGlyphs[aIndex].SetIsSpace();
    }

    void SetIsLowSurrogate(PRUint32 aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nsnull);
        mCharacterGlyphs[aIndex].SetIsLowSurrogate();
    }

    bool FilterIfIgnorable(PRUint32 aIndex);

    const CompressedGlyph *GetCharacterGlyphs() const {
        return &mCharacterGlyphs[0];
    }

    bool HasDetailedGlyphs() const {
        return mDetailedGlyphs != nsnull;
    }

    
    
    
    DetailedGlyph *GetDetailedGlyphs(PRUint32 aCharIndex) const {
        NS_ASSERTION(HasDetailedGlyphs() &&
                     !mCharacterGlyphs[aCharIndex].IsSimpleGlyph() &&
                     mCharacterGlyphs[aCharIndex].GetGlyphCount() > 0,
                     "invalid use of GetDetailedGlyphs; check the caller!");
        return mDetailedGlyphs->Get(aCharIndex);
    }

    void AdjustAdvancesForSyntheticBold(float aSynBoldOffset);

    
    
    
    static void
    SetupClusterBoundaries(CompressedGlyph *aGlyphs,
                           const PRUnichar *aString, PRUint32 aLength);

private:
    
    friend class gfxTextRun;

    
    gfxShapedWord(const PRUint8 *aText, PRUint32 aLength,
                  PRInt32 aRunScript, PRInt32 aAppUnitsPerDevUnit,
                  PRUint32 aFlags)
        : mLength(aLength)
        , mFlags(aFlags | gfxTextRunFactory::TEXT_IS_8BIT)
        , mAppUnitsPerDevUnit(aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharacterGlyphs, 0, aLength * sizeof(CompressedGlyph));
        PRUint8 *text = reinterpret_cast<PRUint8*>(&mCharacterGlyphs[aLength]);
        memcpy(text, aText, aLength * sizeof(PRUint8));
    }

    gfxShapedWord(const PRUnichar *aText, PRUint32 aLength,
                  PRInt32 aRunScript, PRInt32 aAppUnitsPerDevUnit,
                  PRUint32 aFlags)
        : mLength(aLength)
        , mFlags(aFlags)
        , mAppUnitsPerDevUnit(aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharacterGlyphs, 0, aLength * sizeof(CompressedGlyph));
        PRUnichar *text = reinterpret_cast<PRUnichar*>(&mCharacterGlyphs[aLength]);
        memcpy(text, aText, aLength * sizeof(PRUnichar));
        SetupClusterBoundaries(&mCharacterGlyphs[0], aText, aLength);
    }

    
    DetailedGlyph *AllocateDetailedGlyphs(PRUint32 aCharIndex,
                                          PRUint32 aCount);

    
    
    
    
    
    
    class DetailedGlyphStore {
    public:
        DetailedGlyphStore()
            : mLastUsed(0)
        { }

        
        
        
        
        
        
        
        
        
        
        
        
        DetailedGlyph* Get(PRUint32 aOffset) {
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

        DetailedGlyph* Allocate(PRUint32 aOffset, PRUint32 aCount) {
            PRUint32 detailIndex = mDetails.Length();
            DetailedGlyph *details = mDetails.AppendElements(aCount);
            if (!details) {
                return nsnull;
            }
            
            
            
            
            if (mOffsetToIndex.Length() == 0 ||
                aOffset > mOffsetToIndex[mOffsetToIndex.Length() - 1].mOffset) {
                if (!mOffsetToIndex.AppendElement(DGRec(aOffset, detailIndex))) {
                    return nsnull;
                }
            } else {
                if (!mOffsetToIndex.InsertElementSorted(DGRec(aOffset, detailIndex),
                                                        CompareRecordOffsets())) {
                    return nsnull;
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
            DGRec(const PRUint32& aOffset, const PRUint32& aIndex)
                : mOffset(aOffset), mIndex(aIndex) { }
            PRUint32 mOffset; 
            PRUint32 mIndex;  
        };

        struct CompareToOffset {
            bool Equals(const DGRec& a, const PRUint32& b) const {
                return a.mOffset == b;
            }
            bool LessThan(const DGRec& a, const PRUint32& b) const {
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

    
    
    
    
    PRUint32                        mLength;

    PRUint32                        mFlags;

    PRInt32                         mAppUnitsPerDevUnit;
    PRInt32                         mScript;

    PRUint32                        mAgeCounter;

    
    
    
    
    
    CompressedGlyph                 mCharacterGlyphs[1];
};




















class THEBES_API gfxTextRun {
public:
    
    
    typedef gfxShapedWord::CompressedGlyph    CompressedGlyph;
    typedef gfxShapedWord::DetailedGlyph      DetailedGlyph;
    typedef gfxShapedWord::DetailedGlyphStore DetailedGlyphStore;

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    virtual ~gfxTextRun();

    typedef gfxFont::RunMetrics Metrics;

    

    bool IsClusterStart(PRUint32 aPos) {
        NS_ASSERTION(aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    bool IsLigatureGroupStart(PRUint32 aPos) {
        NS_ASSERTION(aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }
    bool CanBreakLineBefore(PRUint32 aPos) {
        NS_ASSERTION(aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_NORMAL;
    }
    bool CanHyphenateBefore(PRUint32 aPos) {
        NS_ASSERTION(aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_HYPHEN;
    }

    bool CharIsSpace(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsSpace();
    }
    bool CharIsTab(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsTab();
    }
    bool CharIsNewline(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsNewline();
    }
    bool CharIsLowSurrogate(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsLowSurrogate();
    }

    PRUint32 GetLength() { return mCharacterCount; }

    
    
    
    
    

    











    virtual bool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                          PRUint8 *aBreakBefore,
                                          gfxContext *aRefContext);

    









    class PropertyProvider {
    public:
        
        
        virtual void GetHyphenationBreaks(PRUint32 aStart, PRUint32 aLength,
                                          bool *aBreakBefore) = 0;

        
        
        
        virtual PRInt8 GetHyphensOption() = 0;

        
        
        virtual gfxFloat GetHyphenWidth() = 0;

        typedef gfxFont::Spacing Spacing;

        





        virtual void GetSpacing(PRUint32 aStart, PRUint32 aLength,
                                Spacing *aSpacing) = 0;
    };

    class ClusterIterator {
    public:
        ClusterIterator(gfxTextRun *aTextRun);

        void Reset();

        bool NextCluster();

        PRUint32 Position() const {
            return mCurrentChar;
        }

        PRUint32 ClusterLength() const;

        gfxFloat ClusterAdvance(PropertyProvider *aProvider) const;

    private:
        gfxTextRun *mTextRun;
        PRUint32    mCurrentChar;
    };

    




















    void Draw(gfxContext *aContext, gfxPoint aPt,
              gfxFont::DrawMode aDrawMode,
              PRUint32 aStart, PRUint32 aLength,
              PropertyProvider *aProvider,
              gfxFloat *aAdvanceWidth);

    




    Metrics MeasureText(PRUint32 aStart, PRUint32 aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContextForTightBoundingBox,
                        PropertyProvider *aProvider);

    



    gfxFloat GetAdvanceWidth(PRUint32 aStart, PRUint32 aLength,
                             PropertyProvider *aProvider);

    


























    virtual bool SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                 bool aLineBreakBefore, bool aLineBreakAfter,
                                 gfxFloat *aAdvanceWidthDelta,
                                 gfxContext *aRefContext);

    

























































    PRUint32 BreakAndMeasureText(PRUint32 aStart, PRUint32 aMaxLength,
                                 bool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 bool aSuppressInitialBreak,
                                 gfxFloat *aTrimWhitespace,
                                 Metrics *aMetrics,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContextForTightBoundingBox,
                                 bool *aUsedHyphenation,
                                 PRUint32 *aLastBreak,
                                 bool aCanWordWrap,
                                 gfxBreakPriority *aBreakPriority);

    




    void SetContext(gfxContext *aContext) {}

    

    bool IsRightToLeft() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0; }
    gfxFloat GetDirection() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) ? -1.0 : 1.0; }
    void *GetUserData() const { return mUserData; }
    void SetUserData(void *aUserData) { mUserData = aUserData; }
    PRUint32 GetFlags() const { return mFlags; }
    void SetFlagBits(PRUint32 aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags |= aFlags;
    }
    void ClearFlagBits(PRUint32 aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags &= ~aFlags;
    }
    const gfxSkipChars& GetSkipChars() const { return mSkipChars; }
    PRUint32 GetAppUnitsPerDevUnit() const { return mAppUnitsPerDevUnit; }
    gfxFontGroup *GetFontGroup() const { return mFontGroup; }


    
    
    static gfxTextRun *Create(const gfxTextRunFactory::Parameters *aParams,
        const void *aText, PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags);

    
    struct GlyphRun {
        nsRefPtr<gfxFont> mFont;   
        PRUint32          mCharacterOffset; 
        PRUint8           mMatchType;
    };

    class THEBES_API GlyphRunIterator {
    public:
        GlyphRunIterator(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aLength)
          : mTextRun(aTextRun), mStartOffset(aStart), mEndOffset(aStart + aLength) {
            mNextIndex = mTextRun->FindFirstGlyphRunContaining(aStart);
        }
        bool NextRun();
        GlyphRun *GetGlyphRun() { return mGlyphRun; }
        PRUint32 GetStringStart() { return mStringStart; }
        PRUint32 GetStringEnd() { return mStringEnd; }
    private:
        gfxTextRun *mTextRun;
        GlyphRun   *mGlyphRun;
        PRUint32    mStringStart;
        PRUint32    mStringEnd;
        PRUint32    mNextIndex;
        PRUint32    mStartOffset;
        PRUint32    mEndOffset;
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

    
    
    












    nsresult AddGlyphRun(gfxFont *aFont, PRUint8 aMatchType,
                         PRUint32 aStartCharIndex, bool aForceNewRun);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    void SortGlyphRuns();
    void SanitizeGlyphRuns();

    
    
    void SetSimpleGlyph(PRUint32 aCharIndex, CompressedGlyph aGlyph) {
        NS_ASSERTION(aGlyph.IsSimpleGlyph(), "Should be a simple glyph here");
        mCharacterGlyphs[aCharIndex] = aGlyph;
    }
    




    void SetGlyphs(PRUint32 aCharIndex, CompressedGlyph aGlyph,
                   const DetailedGlyph *aGlyphs);
    void SetMissingGlyph(PRUint32 aCharIndex, PRUint32 aUnicodeChar);
    void SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, PRUint32 aCharIndex);

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                               PRUint32 aCharIndex, PRUnichar aSpaceChar);

    
    
    
    
    
    
    void SetIsTab(PRUint32 aIndex) {
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
    void SetIsNewline(PRUint32 aIndex) {
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
    void SetIsLowSurrogate(PRUint32 aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nsnull);
        mCharacterGlyphs[aIndex].SetIsLowSurrogate();
    }

    





    void FetchGlyphExtents(gfxContext *aRefContext);

    
    
    CompressedGlyph *GetCharacterGlyphs() { return mCharacterGlyphs; }

    
    
    
    DetailedGlyph *GetDetailedGlyphs(PRUint32 aCharIndex) {
        NS_ASSERTION(mDetailedGlyphs != nsnull &&
                     !mCharacterGlyphs[aCharIndex].IsSimpleGlyph() &&
                     mCharacterGlyphs[aCharIndex].GetGlyphCount() > 0,
                     "invalid use of GetDetailedGlyphs; check the caller!");
        return mDetailedGlyphs->Get(aCharIndex);
    }

    bool HasDetailedGlyphs() { return mDetailedGlyphs != nsnull; }
    PRUint32 CountMissingGlyphs();
    const GlyphRun *GetGlyphRuns(PRUint32 *aNumGlyphRuns) {
        *aNumGlyphRuns = mGlyphRuns.Length();
        return mGlyphRuns.Elements();
    }
    
    
    PRUint32 FindFirstGlyphRunContaining(PRUint32 aOffset);

    
    void CopyGlyphDataFrom(const gfxShapedWord *aSource, PRUint32 aStart);

    
    
    void CopyGlyphDataFrom(gfxTextRun *aSource, PRUint32 aStart,
                           PRUint32 aLength, PRUint32 aDest);

    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    struct LigatureData {
        
        PRUint32 mLigatureStart;
        PRUint32 mLigatureEnd;
        
        
        gfxFloat mPartAdvance;
        
        
        
        gfxFloat mPartWidth;
        
        bool mClipBeforePart;
        bool mClipAfterPart;
    };
    
    
    
    virtual NS_MUST_OVERRIDE size_t
        SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf);
    virtual NS_MUST_OVERRIDE size_t
        SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf);

    
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
    





    gfxTextRun(const gfxTextRunFactory::Parameters *aParams, const void *aText,
               PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags);

    




    static void* AllocateStorageForTextRun(size_t aSize, PRUint32 aLength);

    
    
    
    
    CompressedGlyph  *mCharacterGlyphs;

private:
    

    
    DetailedGlyph *AllocateDetailedGlyphs(PRUint32 aCharIndex, PRUint32 aCount);

    
    PRInt32 GetAdvanceForGlyphs(PRUint32 aStart, PRUint32 aEnd);

    
    
    
    
    bool GetAdjustedSpacingArray(PRUint32 aStart, PRUint32 aEnd,
                                   PropertyProvider *aProvider,
                                   PRUint32 aSpacingStart, PRUint32 aSpacingEnd,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);

    
    
    

    
    LigatureData ComputeLigatureData(PRUint32 aPartStart, PRUint32 aPartEnd,
                                     PropertyProvider *aProvider);
    gfxFloat ComputePartialLigatureWidth(PRUint32 aPartStart, PRUint32 aPartEnd,
                                         PropertyProvider *aProvider);
    void DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx,
                             PRUint32 aStart, PRUint32 aEnd, gfxPoint *aPt,
                             PropertyProvider *aProvider);
    
    
    void ShrinkToLigatureBoundaries(PRUint32 *aStart, PRUint32 *aEnd);
    
    gfxFloat GetPartialLigatureWidth(PRUint32 aStart, PRUint32 aEnd, PropertyProvider *aProvider);
    void AccumulatePartialLigatureMetrics(gfxFont *aFont,
                                          PRUint32 aStart, PRUint32 aEnd,
                                          gfxFont::BoundingBoxType aBoundingBoxType,
                                          gfxContext *aRefContext,
                                          PropertyProvider *aProvider,
                                          Metrics *aMetrics);

    
    void AccumulateMetricsForRun(gfxFont *aFont, PRUint32 aStart, PRUint32 aEnd,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContext,
                                 PropertyProvider *aProvider,
                                 PRUint32 aSpacingStart, PRUint32 aSpacingEnd,
                                 Metrics *aMetrics);

    
    void DrawGlyphs(gfxFont *aFont, gfxContext *aContext,
                    gfxFont::DrawMode aDrawMode, gfxPoint *aPt,
                    PRUint32 aStart, PRUint32 aEnd,
                    PropertyProvider *aProvider,
                    PRUint32 aSpacingStart, PRUint32 aSpacingEnd);

    nsAutoPtr<DetailedGlyphStore>   mDetailedGlyphs;

    
    
    nsAutoTArray<GlyphRun,1>        mGlyphRuns;

    void             *mUserData;
    gfxFontGroup     *mFontGroup; 
    gfxSkipChars      mSkipChars;
    nsExpirationState mExpirationState;
    PRUint32          mAppUnitsPerDevUnit;
    PRUint32          mFlags;
    PRUint32          mCharacterCount;

    bool              mSkipDrawing; 
                                    
                                    
};

class THEBES_API gfxFontGroup : public gfxTextRunFactory {
public:
    static void Shutdown(); 

    gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle, gfxUserFontSet *aUserFontSet = nsnull);

    virtual ~gfxFontGroup();

    virtual gfxFont *GetFontAt(PRInt32 i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");
        NS_ASSERTION(mFonts.Length() > PRUint32(i), 
                     "Requesting a font index that doesn't exist");

        return static_cast<gfxFont*>(mFonts[i]);
    }
    virtual PRUint32 FontListLength() const {
        return mFonts.Length();
    }

    bool Equals(const gfxFontGroup& other) const {
        return mFamilies.Equals(other.mFamilies) &&
            mStyle.Equals(other.mStyle);
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    



    static bool IsInvalidChar(PRUint8 ch);
    static bool IsInvalidChar(PRUnichar ch);

    





    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);
    





    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags);

    



    template<typename T>
    gfxTextRun *MakeTextRun(const T *aString, PRUint32 aLength,
                            gfxContext *aRefContext,
                            PRUint32 aAppUnitsPerDevUnit,
                            PRUint32 aFlags)
    {
        gfxTextRunFactory::Parameters params = {
            aRefContext, nsnull, nsnull, nsnull, 0, aAppUnitsPerDevUnit
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

    
    
    
    
    
    enum { UNDERLINE_OFFSET_NOT_SET = PR_INT16_MAX };
    virtual gfxFloat GetUnderlineOffset() {
        if (mUnderlineOffset == UNDERLINE_OFFSET_NOT_SET)
            mUnderlineOffset = GetFontAt(0)->GetMetrics().underlineOffset;
        return mUnderlineOffset;
    }

    virtual already_AddRefed<gfxFont>
        FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRInt32 aRunScript,
                        gfxFont *aPrevMatchedFont,
                        PRUint8 *aMatchType);

    
    virtual already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh);

    virtual already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh);

    template<typename T>
    void ComputeRanges(nsTArray<gfxTextRange>& mRanges,
                       const T *aString, PRUint32 aLength,
                       PRInt32 aRunScript);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    PRUint64 GetGeneration();

    
    
    virtual void UpdateFontList();

    bool ShouldSkipDrawing() const {
        return mSkipDrawing;
    }

protected:
    nsString mFamilies;
    gfxFontStyle mStyle;
    nsTArray< nsRefPtr<gfxFont> > mFonts;
    gfxFloat mUnderlineOffset;

    gfxUserFontSet* mUserFontSet;
    PRUint64 mCurrGeneration;  

    
    nsRefPtr<gfxFontFamily> mLastPrefFamily;
    nsRefPtr<gfxFont>       mLastPrefFont;
    eFontPrefLang           mLastPrefLang;       
    eFontPrefLang           mPageLang;
    bool                    mLastPrefFirstFont;  

    bool                    mSkipDrawing; 
                                          
                                          

    



    gfxTextRun *MakeEmptyTextRun(const Parameters *aParams, PRUint32 aFlags);
    gfxTextRun *MakeSpaceTextRun(const Parameters *aParams, PRUint32 aFlags);
    gfxTextRun *MakeBlankTextRun(const void* aText, PRUint32 aLength,
                                 const Parameters *aParams, PRUint32 aFlags);

    
    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    void BuildFontList();

    
    
    
    void InitMetricsForBadFont(gfxFont* aBadFont);

    
    
    template<typename T>
    void InitTextRun(gfxContext *aContext,
                     gfxTextRun *aTextRun,
                     const T *aString,
                     PRUint32 aLength);

    
    
    template<typename T>
    void InitScriptRun(gfxContext *aContext,
                       gfxTextRun *aTextRun,
                       const T *aString,
                       PRUint32 aScriptRunStart,
                       PRUint32 aScriptRunEnd,
                       PRInt32 aRunScript);

    









    bool ForEachFontInternal(const nsAString& aFamilies,
                               nsIAtom *aLanguage,
                               bool aResolveGeneric,
                               bool aResolveFontName,
                               bool aUseFontSet,
                               FontCreationCallback fc,
                               void *closure);

    static bool FontResolverProc(const nsAString& aName, void *aClosure);

    static bool FindPlatformFont(const nsAString& aName,
                                   const nsACString& aGenericName,
                                   bool aUseFontSet,
                                   void *closure);

    static NS_HIDDEN_(nsILanguageAtomService*) gLangService;
};
#endif
