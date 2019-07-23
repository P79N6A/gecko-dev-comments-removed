







































#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "prtypes.h"
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

#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxTextRun;
class nsIAtom;
class gfxFont;
class gfxFontFamily;
class gfxFontGroup;
class gfxUserFontSet;
class gfxUserFontData;


#define FONT_STYLE_NORMAL              NS_FONT_STYLE_NORMAL
#define FONT_STYLE_ITALIC              NS_FONT_STYLE_ITALIC
#define FONT_STYLE_OBLIQUE             NS_FONT_STYLE_OBLIQUE


#define FONT_WEIGHT_NORMAL             NS_FONT_WEIGHT_NORMAL
#define FONT_WEIGHT_BOLD               NS_FONT_WEIGHT_BOLD

#define FONT_MAX_SIZE                  2000.0

struct THEBES_API gfxFontStyle {
    gfxFontStyle();
    gfxFontStyle(PRUint8 aStyle, PRUint16 aWeight, PRInt16 aStretch,
                 gfxFloat aSize, const nsACString& aLangGroup,
                 float aSizeAdjust, PRPackedBool aSystemFont,
                 PRPackedBool aFamilyNameQuirks,
                 PRPackedBool aPrinterFont);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    PRUint8 style : 7;

    
    
    
    PRPackedBool systemFont : 1;

    
    PRPackedBool printerFont : 1;

    
    
    PRPackedBool familyNameQuirks : 1;

    
    
    
    
    
    PRUint16 weight;

    
    
    PRInt16 stretch;

    
    gfxFloat size;

    
    nsCString langGroup;

    
    
    
    
    float sizeAdjust;

    
    
    gfxFloat GetAdjustedSize(gfxFloat aspect) const {
        NS_ASSERTION(sizeAdjust != 0.0, "Not meant to be called when sizeAdjust = 0");
        gfxFloat adjustedSize = PR_MAX(NS_round(size*(sizeAdjust/aspect)), 1.0);
        return PR_MIN(adjustedSize, FONT_MAX_SIZE);
    }

    PLDHashNumber Hash() const {
        return ((style + (systemFont << 7) + (familyNameQuirks << 8) +
            (weight << 9)) + PRUint32(size*1000) + PRUint32(sizeAdjust*1000)) ^
            HashString(langGroup);
    }

    void ComputeWeightAndOffset(PRInt8 *outBaseWeight,
                                PRInt8 *outOffset) const;

    PRBool Equals(const gfxFontStyle& other) const {
        return (size == other.size) &&
            (style == other.style) &&
            (systemFont == other.systemFont) &&
            (printerFont == other.printerFont) &&
            (familyNameQuirks == other.familyNameQuirks) &&
            (weight == other.weight) &&
            (stretch == other.stretch) &&
            (langGroup.Equals(other.langGroup)) &&
            (sizeAdjust == other.sizeAdjust);
    }
};

class gfxFontEntry {
public:
    THEBES_INLINE_DECL_REFCOUNTING(gfxFontEntry)

    gfxFontEntry(const nsAString& aName, gfxFontFamily *aFamily = nsnull,
                 PRBool aIsStandardFace = PR_FALSE) : 
        mName(aName), mItalic(PR_FALSE), mFixedPitch(PR_FALSE),
        mIsProxy(PR_FALSE), mIsValid(PR_TRUE), 
        mIsBadUnderlineFont(PR_FALSE), mIsUserFont(PR_FALSE),
        mStandardFace(aIsStandardFace),
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
        mCmapInitialized(PR_FALSE), mUserFontData(nsnull),
        mFamily(aFamily)
    { }

    gfxFontEntry(const gfxFontEntry& aEntry) : 
        mName(aEntry.mName), mItalic(aEntry.mItalic), 
        mFixedPitch(aEntry.mFixedPitch), mIsProxy(aEntry.mIsProxy), 
        mIsValid(aEntry.mIsValid), mIsBadUnderlineFont(aEntry.mIsBadUnderlineFont),
        mIsUserFont(aEntry.mIsUserFont),
        mStandardFace(aEntry.mStandardFace),
        mWeight(aEntry.mWeight), mCmapInitialized(aEntry.mCmapInitialized),
        mCharacterMap(aEntry.mCharacterMap), mUserFontData(aEntry.mUserFontData),
        mFamily(aEntry.mFamily)
    { }

    virtual ~gfxFontEntry();

    
    const nsString& Name() const { return mName; }

    PRUint16 Weight() { return mWeight; }
    PRInt16 Stretch() { return mStretch; }

    PRBool IsUserFont() { return mIsUserFont; }
    PRBool IsFixedPitch() { return mFixedPitch; }
    PRBool IsItalic() { return mItalic; }
    PRBool IsBold() { return mWeight >= 600; } 

    inline PRBool HasCharacter(PRUint32 ch) {
        if (mCharacterMap.test(ch))
            return PR_TRUE;
            
        return TestCharacterMap(ch);
    }

    virtual PRBool TestCharacterMap(PRUint32 aCh);
    virtual nsresult ReadCMAP();

    const nsString& FamilyName();

    nsString         mName;

    PRPackedBool     mItalic      : 1;
    PRPackedBool     mFixedPitch  : 1;
    PRPackedBool     mIsProxy     : 1;
    PRPackedBool     mIsValid     : 1;
    PRPackedBool     mIsBadUnderlineFont : 1;
    PRPackedBool     mIsUserFont  : 1;
    PRPackedBool     mStandardFace : 1;

    PRUint16         mWeight;
    PRInt16          mStretch;

    PRPackedBool     mCmapInitialized;
    gfxSparseBitSet  mCharacterMap;
    gfxUserFontData* mUserFontData;

protected:
    friend class gfxPlatformFontList;
    friend class gfxMacPlatformFontList;
    friend class gfxFcFontEntry;
    friend class gfxFontFamily;

    gfxFontEntry() :
        mItalic(PR_FALSE), mFixedPitch(PR_FALSE),
        mIsProxy(PR_FALSE), mIsValid(PR_TRUE), 
        mIsBadUnderlineFont(PR_FALSE),
        mIsUserFont(PR_FALSE),
        mStandardFace(PR_FALSE),
        mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
        mCmapInitialized(PR_FALSE),
        mUserFontData(nsnull),
        mFamily(nsnull)
    { }

    virtual nsresult GetFontTable(PRUint32 aTableTag, nsTArray<PRUint8>& aBuffer) {
        return NS_ERROR_FAILURE; 
    }

    gfxFontFamily *mFamily;
};



struct FontSearch {
    FontSearch(const PRUint32 aCharacter, gfxFont *aFont) :
        mCh(aCharacter), mFontToMatch(aFont), mMatchRank(0) {
    }
    const PRUint32         mCh;
    gfxFont*               mFontToMatch;
    PRInt32                mMatchRank;
    nsRefPtr<gfxFontEntry> mBestMatch;
};


class AddOtherFamilyNameFunctor;

class gfxFontFamily {
public:
    THEBES_INLINE_DECL_REFCOUNTING(gfxFontFamily)

    gfxFontFamily(const nsAString& aName) :
        mName(aName), mOtherFamilyNamesInitialized(PR_FALSE), mHasOtherFamilyNames(PR_FALSE),
        mIsSimpleFamily(PR_FALSE),
        mHasStyles(PR_FALSE)
        { }

    virtual ~gfxFontFamily() { }

    const nsString& Name() { return mName; }

    virtual void LocalizedName(nsAString& aLocalizedName);
    virtual PRBool HasOtherFamilyNames();
    
    nsTArray<nsRefPtr<gfxFontEntry> >& GetFontList() { return mAvailableFonts; }
    
    void AddFontEntry(nsRefPtr<gfxFontEntry> aFontEntry) {
        mAvailableFonts.AppendElement(aFontEntry);
    }

    
    void SetHasStyles(PRBool aHasStyles) { mHasStyles = aHasStyles; }

    
    
    
    
    gfxFontEntry *FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                   PRBool& aNeedsBold);

    
    
    void FindFontForChar(FontSearch *aMatchData);

    
    virtual void ReadOtherFamilyNames(AddOtherFamilyNameFunctor& aOtherFamilyFunctor);

    
    virtual void FindStyleVariations() { }

    
    gfxFontEntry* FindFont(const nsAString& aPostscriptName);

    
    void ReadCMAP() {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        
        
        for (i = 0; i < numFonts; i++)
            mAvailableFonts[i]->ReadCMAP();
    }

    
    void SetBadUnderlineFont(PRBool aIsBadUnderlineFont) {
        PRUint32 i, numFonts = mAvailableFonts.Length();
        
        
        
        for (i = 0; i < numFonts; i++)
            mAvailableFonts[i]->mIsBadUnderlineFont = aIsBadUnderlineFont;
    }

    
    void SortAvailableFonts();

    
    
    
    void CheckForSimpleFamily();

protected:
    
    
    virtual PRBool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                       PRBool anItalic, PRInt16 aStretch);

    PRBool ReadOtherFamilyNamesForFace(AddOtherFamilyNameFunctor& aOtherFamilyFunctor,
                                       gfxFontEntry *aFontEntry,
                                       PRBool useFullName);

    nsString mName;
    nsTArray<nsRefPtr<gfxFontEntry> >  mAvailableFonts;
    PRPackedBool mOtherFamilyNamesInitialized;
    PRPackedBool mHasOtherFamilyNames;
    PRPackedBool mIsSimpleFamily;
    PRPackedBool mHasStyles;

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
    gfxTextRange(PRUint32 aStart,  PRUint32 aEnd) : start(aStart), end(aEnd) { }
    PRUint32 Length() const { return end - start; }
    nsRefPtr<gfxFont> font;
    PRUint32 start, end;
};
















class THEBES_API gfxFontCache : public nsExpirationTracker<gfxFont,3> {
public:
    enum { TIMEOUT_SECONDS = 10 };
    gfxFontCache()
        : nsExpirationTracker<gfxFont,3>(TIMEOUT_SECONDS*1000) { mFonts.Init(); }
    ~gfxFontCache() {
        
        AgeAllGenerations();
        
        NS_WARN_IF_FALSE(mFonts.Count() == 0,
                         "Fonts still alive while shutting down gfxFontCache");
        
        
        
    }

    



    static gfxFontCache* GetCache() {
        return gGlobalCache;
    }

    static nsresult Init();
    
    static void Shutdown();

    
    
    already_AddRefed<gfxFont> Lookup(const nsAString &aName,
                                     const gfxFontStyle *aFontGroup);
    
    
    
    
    void AddNew(gfxFont *aFont);

    
    
    
    void NotifyReleased(gfxFont *aFont);

    
    
    virtual void NotifyExpired(gfxFont *aFont);

protected:
    void DestroyFont(gfxFont *aFont);

    static gfxFontCache *gGlobalCache;

    struct Key {
        const nsAString&    mString;
        const gfxFontStyle* mStyle;
        Key(const nsAString& aString, const gfxFontStyle* aStyle)
            : mString(aString), mStyle(aStyle) {}
    };

    class HashEntry : public PLDHashEntryHdr {
    public:
        typedef const Key& KeyType;
        typedef const Key* KeyTypePointer;

        
        
        HashEntry(KeyTypePointer aStr) : mFont(nsnull) { }
        HashEntry(const HashEntry& toCopy) : mFont(toCopy.mFont) { }
        ~HashEntry() { }

        PRBool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return HashString(aKey->mString) ^ aKey->mStyle->Hash();
        }
        enum { ALLOW_MEMMOVE = PR_TRUE };

        gfxFont* mFont;
    };

    nsTHashtable<HashEntry> mFonts;
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

    PRBool IsGlyphKnown(PRUint32 aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID) != INVALID_WIDTH ||
            mTightGlyphExtents.GetEntry(aGlyphID) != nsnull;
    }

    PRBool IsGlyphKnownWithTightExtents(PRUint32 aGlyphID) const {
        return mTightGlyphExtents.GetEntry(aGlyphID) != nsnull;
    }

    
    
    
    PRBool GetTightGlyphExtentsAppUnits(gfxFont *aFont, gfxContext *aContext,
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

    typedef unsigned long PtrBits;
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

protected:
    nsAutoRefCnt mRefCnt;

    void NotifyReleased() {
        gfxFontCache *cache = gfxFontCache::GetCache();
        if (cache) {
            
            
            cache->NotifyReleased(this);
        } else {
            
            delete this;
        }
    }

    gfxFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle);

public:
    virtual ~gfxFont();

    
    typedef enum {
        LOOSE_INK_EXTENTS,
            
            
            
        TIGHT_INK_EXTENTS,
            
            
            
            
        TIGHT_HINTED_OUTLINE_EXTENTS
            
            
            
            
            
            
            
            
            
            
            
            
            
    } BoundingBoxType;

    const nsString& GetName() const { return mFontEntry->Name(); }
    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual nsString GetUniqueName() = 0;

    
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

        void CombineWith(const RunMetrics& aOther, PRBool aOtherIsOnLeft);

        
        
        
        
        gfxFloat mAdvanceWidth;
        
        
        gfxFloat mAscent;  
        gfxFloat mDescent; 
        
        
        
        
        
        
        
        gfxRect  mBoundingBox;
    };

    






















    virtual void Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                      gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing);
    




















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);
    




    PRBool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   PRUint32 aStart, PRUint32 aLength)
    { return PR_FALSE; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    virtual PRUint32 GetSpaceGlyph() = 0;

    gfxGlyphExtents *GetOrCreateGlyphExtents(PRUint32 aAppUnitsPerDevUnit);

    
    virtual void SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID,
                                   PRBool aNeedTight, gfxGlyphExtents *aExtents);

    
    virtual PRBool SetupCairoFont(gfxContext *aContext) = 0;

    PRBool IsSyntheticBold() { return mSyntheticBoldOffset != 0; }
    PRUint32 GetSyntheticBoldOffset() { return mSyntheticBoldOffset; }

    gfxFontEntry *GetFontEntry() { return mFontEntry.get(); }
    PRBool HasCharacter(PRUint32 ch) {
        if (!mIsValid)
            return PR_FALSE;
        return mFontEntry->HasCharacter(ch); 
    }

protected:
    nsRefPtr<gfxFontEntry> mFontEntry;

    PRPackedBool               mIsValid;
    nsExpirationState          mExpirationState;
    gfxFontStyle               mStyle;
    nsAutoTArray<gfxGlyphExtents*,1> mGlyphExtentsArray;

    
    PRUint32                   mSyntheticBoldOffset;  

    
    
    void SanitizeMetrics(gfxFont::Metrics *aMetrics, PRBool aIsBadUnderlineFont);
};

class THEBES_API gfxTextRunFactory {
    THEBES_INLINE_DECL_REFCOUNTING(gfxTextRunFactory)

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
        




        TEXT_OPTIMIZE_SPEED          = 0x0100
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





































class THEBES_API gfxTextRun {
public:
    
    void operator delete(void* aPtr);
    virtual ~gfxTextRun();

    typedef gfxFont::RunMetrics Metrics;

    

    PRBool IsClusterStart(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    PRBool IsLigatureGroupStart(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }
    PRBool CanBreakLineBefore(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore();
    }

    PRUint32 GetLength() { return mCharacterCount; }

    
    
    
    
    

    











    virtual PRBool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                          PRPackedBool *aBreakBefore,
                                          gfxContext *aRefContext);

    









    class PropertyProvider {
    public:
        
        
        virtual void GetHyphenationBreaks(PRUint32 aStart, PRUint32 aLength,
                                          PRPackedBool *aBreakBefore) = 0;

        
        
        virtual gfxFloat GetHyphenWidth() = 0;

        typedef gfxFont::Spacing Spacing;

        





        virtual void GetSpacing(PRUint32 aStart, PRUint32 aLength,
                                Spacing *aSpacing) = 0;
    };

    
























    void Draw(gfxContext *aContext, gfxPoint aPt,
              PRUint32 aStart, PRUint32 aLength,
              const gfxRect *aDirtyRect,
              PropertyProvider *aProvider,
              gfxFloat *aAdvanceWidth);

    













    void DrawToPath(gfxContext *aContext, gfxPoint aPt,
                    PRUint32 aStart, PRUint32 aLength,
                    PropertyProvider *aBreakProvider,
                    gfxFloat *aAdvanceWidth);

    




    Metrics MeasureText(PRUint32 aStart, PRUint32 aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContextForTightBoundingBox,
                        PropertyProvider *aProvider);

    



    gfxFloat GetAdvanceWidth(PRUint32 aStart, PRUint32 aLength,
                             PropertyProvider *aProvider);

    


























    virtual PRBool SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                 PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                                 gfxFloat *aAdvanceWidthDelta,
                                 gfxContext *aRefContext);

    

























































    PRUint32 BreakAndMeasureText(PRUint32 aStart, PRUint32 aMaxLength,
                                 PRBool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 PRBool aSuppressInitialBreak,
                                 gfxFloat *aTrimWhitespace,
                                 Metrics *aMetrics,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContextForTightBoundingBox,
                                 PRBool *aUsedHyphenation,
                                 PRUint32 *aLastBreak,
                                 PRBool aCanWordWrap,
                                 gfxBreakPriority *aBreakPriority);

    




    void SetContext(gfxContext *aContext) {}

    

    PRBool IsRightToLeft() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0; }
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
    const PRUint8 *GetText8Bit() const
    { return (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) ? mText.mSingle : nsnull; }
    const PRUnichar *GetTextUnicode() const
    { return (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) ? nsnull : mText.mDouble; }
    const void *GetTextAt(PRUint32 aIndex) {
        return (mFlags & gfxTextRunFactory::TEXT_IS_8BIT)
            ? static_cast<const void *>(mText.mSingle + aIndex)
            : static_cast<const void *>(mText.mDouble + aIndex);
    }
    const PRUnichar GetChar(PRUint32 i) const
    { return (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) ? mText.mSingle[i] : mText.mDouble[i]; }
    PRUint32 GetHashCode() const { return mHashCode; }
    void SetHashCode(PRUint32 aHash) { mHashCode = aHash; }

    
    
    static gfxTextRun *Create(const gfxTextRunFactory::Parameters *aParams,
        const void *aText, PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags);

    
    
    
    
    
    
    virtual gfxTextRun *Clone(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                              PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags);

    














    class CompressedGlyph {
    public:
        CompressedGlyph() { mValue = 0; }

        enum {
            
            
            
            
            
            FLAG_IS_SIMPLE_GLYPH  = 0x80000000U,
            
            FLAG_CAN_BREAK_BEFORE = 0x40000000U,

            
            ADVANCE_MASK  = 0x3FFF0000U,
            ADVANCE_SHIFT = 16,

            GLYPH_MASK = 0x0000FFFFU,

            
            
            

            
            
            
            
            
            FLAG_NOT_MISSING              = 0x01,
            FLAG_NOT_CLUSTER_START        = 0x02,
            FLAG_NOT_LIGATURE_GROUP_START = 0x04,
            
            GLYPH_COUNT_MASK = 0x00FFFF00U,
            GLYPH_COUNT_SHIFT = 8
        };

        
        
        
        

        
        static PRBool IsSimpleGlyphID(PRUint32 aGlyph) {
            return (aGlyph & GLYPH_MASK) == aGlyph;
        }
        
        
        static PRBool IsSimpleAdvance(PRUint32 aAdvance) {
            return (aAdvance & (ADVANCE_MASK >> ADVANCE_SHIFT)) == aAdvance;
        }

        PRBool IsSimpleGlyph() const { return (mValue & FLAG_IS_SIMPLE_GLYPH) != 0; }
        PRUint32 GetSimpleAdvance() const { return (mValue & ADVANCE_MASK) >> ADVANCE_SHIFT; }
        PRUint32 GetSimpleGlyph() const { return mValue & GLYPH_MASK; }

        PRBool IsMissing() const { return (mValue & (FLAG_NOT_MISSING|FLAG_IS_SIMPLE_GLYPH)) == 0; }
        PRBool IsClusterStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_CLUSTER_START);
        }
        PRBool IsLigatureGroupStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_LIGATURE_GROUP_START);
        }
        PRBool IsLigatureContinuation() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) == 0 &&
                (mValue & (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING)) ==
                    (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING);
        }

        PRBool CanBreakBefore() const { return (mValue & FLAG_CAN_BREAK_BEFORE) != 0; }
        
        PRUint32 SetCanBreakBefore(PRBool aCanBreakBefore) {
            NS_ASSERTION(aCanBreakBefore == PR_FALSE || aCanBreakBefore == PR_TRUE,
                         "Bogus break-before value!");
            PRUint32 breakMask = aCanBreakBefore*FLAG_CAN_BREAK_BEFORE;
            PRUint32 toggle = breakMask ^ (mValue & FLAG_CAN_BREAK_BEFORE);
            mValue ^= toggle;
            return toggle;
        }

        CompressedGlyph& SetSimpleGlyph(PRUint32 aAdvanceAppUnits, PRUint32 aGlyph) {
            NS_ASSERTION(IsSimpleAdvance(aAdvanceAppUnits), "Advance overflow");
            NS_ASSERTION(IsSimpleGlyphID(aGlyph), "Glyph overflow");
            mValue = (mValue & FLAG_CAN_BREAK_BEFORE) | FLAG_IS_SIMPLE_GLYPH |
                (aAdvanceAppUnits << ADVANCE_SHIFT) | aGlyph;
            return *this;
        }
        CompressedGlyph& SetComplex(PRBool aClusterStart, PRBool aLigatureStart,
                PRUint32 aGlyphCount) {
            mValue = (mValue & FLAG_CAN_BREAK_BEFORE) | FLAG_NOT_MISSING |
                (aClusterStart ? 0 : FLAG_NOT_CLUSTER_START) |
                (aLigatureStart ? 0 : FLAG_NOT_LIGATURE_GROUP_START) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        


        CompressedGlyph& SetMissing(PRUint32 aGlyphCount) {
            mValue = (mValue & FLAG_CAN_BREAK_BEFORE) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        PRUint32 GetGlyphCount() const {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            return (mValue & GLYPH_COUNT_MASK) >> GLYPH_COUNT_SHIFT;
        }

    private:
        PRUint32 mValue;
    };

    



    struct DetailedGlyph {
        

        PRUint32 mGlyphID;
        


   
        PRInt32  mAdvance;
        float    mXOffset, mYOffset;
    };

    
    struct GlyphRun {
        nsRefPtr<gfxFont> mFont;   
        PRUint32          mCharacterOffset; 
    };

    class THEBES_API GlyphRunIterator {
    public:
        GlyphRunIterator(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aLength)
          : mTextRun(aTextRun), mStartOffset(aStart), mEndOffset(aStart + aLength) {
            mNextIndex = mTextRun->FindFirstGlyphRunContaining(aStart);
        }
        PRBool NextRun();
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
        PRBool Equals(const GlyphRun& a,
                      const GlyphRun& b) const
        {
            return a.mCharacterOffset == b.mCharacterOffset;
        }

        PRBool LessThan(const GlyphRun& a,
                        const GlyphRun& b) const
        {
            return a.mCharacterOffset < b.mCharacterOffset;
        }
    };

    friend class GlyphRunIterator;
    friend class FontSelector;

    
    
    












    nsresult AddGlyphRun(gfxFont *aFont, PRUint32 aStartCharIndex, PRBool aForceNewRun = PR_FALSE);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    void SortGlyphRuns();
    void SanitizeGlyphRuns();

    
    
    




    void SetSimpleGlyph(PRUint32 aCharIndex, CompressedGlyph aGlyph) {
        NS_ASSERTION(aGlyph.IsSimpleGlyph(), "Should be a simple glyph here");
        if (mCharacterGlyphs) {
            mCharacterGlyphs[aCharIndex] = aGlyph;
        }
        if (mDetailedGlyphs) {
            mDetailedGlyphs[aCharIndex] = nsnull;
        }
    }
    void SetGlyphs(PRUint32 aCharIndex, CompressedGlyph aGlyph,
                   const DetailedGlyph *aGlyphs);
    void SetMissingGlyph(PRUint32 aCharIndex, PRUint32 aUnicodeChar);
    void SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, PRUint32 aCharIndex);
    
    





    void FetchGlyphExtents(gfxContext *aRefContext);

    
    
    const CompressedGlyph *GetCharacterGlyphs() { return mCharacterGlyphs; }
    const DetailedGlyph *GetDetailedGlyphs(PRUint32 aCharIndex) {
        return mDetailedGlyphs ? mDetailedGlyphs[aCharIndex].get() : nsnull;
    }
    PRBool HasDetailedGlyphs() { return mDetailedGlyphs.get() != nsnull; }
    PRUint32 CountMissingGlyphs();
    const GlyphRun *GetGlyphRuns(PRUint32 *aNumGlyphRuns) {
        *aNumGlyphRuns = mGlyphRuns.Length();
        return mGlyphRuns.Elements();
    }
    
    
    PRUint32 FindFirstGlyphRunContaining(PRUint32 aOffset);
    
    
    
    
    virtual void CopyGlyphDataFrom(gfxTextRun *aSource, PRUint32 aStart,
                                   PRUint32 aLength, PRUint32 aDest,
                                   PRBool aStealData);

    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    struct LigatureData {
        
        PRUint32 mLigatureStart;
        PRUint32 mLigatureEnd;
        
        
        gfxFloat mPartAdvance;
        
        
        
        gfxFloat mPartWidth;
        
        PRPackedBool mClipBeforePart;
        PRPackedBool mClipAfterPart;
    };
    
    
    PRUint64 GetUserFontSetGeneration() { return mUserFontSetGeneration; }

#ifdef DEBUG
    
    PRUint32 mCachedWords;
    
    
    PRUint32 mCacheGeneration;
    
    void Dump(FILE* aOutput);
#endif

    
    void AdjustAdvancesForSyntheticBold(PRUint32 aStart, PRUint32 aLength);

protected:
    
    
    void *operator new(size_t aSize, PRUint32 aLength, PRUint32 aFlags);

    




    gfxTextRun(const gfxTextRunFactory::Parameters *aParams, const void *aText,
               PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags,
               PRUint32 aObjectSize);

private:
    

    
    DetailedGlyph *AllocateDetailedGlyphs(PRUint32 aCharIndex, PRUint32 aCount);

    
    
    
    
    PRBool GetAdjustedSpacingArray(PRUint32 aStart, PRUint32 aEnd,
                                   PropertyProvider *aProvider,
                                   PRUint32 aSpacingStart, PRUint32 aSpacingEnd,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);

    
    
    

    
    LigatureData ComputeLigatureData(PRUint32 aPartStart, PRUint32 aPartEnd,
                                     PropertyProvider *aProvider);
    gfxFloat ComputePartialLigatureWidth(PRUint32 aPartStart, PRUint32 aPartEnd,
                                         PropertyProvider *aProvider);
    void DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx, PRUint32 aStart,
                             PRUint32 aEnd, const gfxRect *aDirtyRect, gfxPoint *aPt,
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

    
    void DrawGlyphs(gfxFont *aFont, gfxContext *aContext, PRBool aDrawToPath,
                    gfxPoint *aPt, PRUint32 aStart, PRUint32 aEnd,
                    PropertyProvider *aProvider,
                    PRUint32 aSpacingStart, PRUint32 aSpacingEnd);

    
    
    
    
    CompressedGlyph*                               mCharacterGlyphs;
    nsAutoArrayPtr<nsAutoArrayPtr<DetailedGlyph> > mDetailedGlyphs; 
    
    
    nsAutoTArray<GlyphRun,1>                       mGlyphRuns;
    
    
    
    
    
    union {
        const PRUint8   *mSingle;
        const PRUnichar *mDouble;
    } mText;
    void             *mUserData;
    gfxFontGroup     *mFontGroup; 
    gfxSkipChars      mSkipChars;
    nsExpirationState mExpirationState;
    PRUint32          mAppUnitsPerDevUnit;
    PRUint32          mFlags;
    PRUint32          mCharacterCount;
    PRUint32          mHashCode;
    PRUint64          mUserFontSetGeneration; 
};

class THEBES_API gfxFontGroup : public gfxTextRunFactory {
protected:
    gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle, gfxUserFontSet *aUserFontSet = nsnull);

public:
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

    PRBool Equals(const gfxFontGroup& other) const {
        return mFamilies.Equals(other.mFamilies) &&
            mStyle.Equals(other.mStyle);
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle) = 0;

    



    static PRBool IsInvalidChar(PRUnichar ch);
    
    



    gfxTextRun *MakeEmptyTextRun(const Parameters *aParams, PRUint32 aFlags);
    



    gfxTextRun *MakeSpaceTextRun(const Parameters *aParams, PRUint32 aFlags);

    





    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags) = 0;
    





    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    const Parameters *aParams, PRUint32 aFlags) = 0;

    


    typedef PRBool (*FontCreationCallback) (const nsAString& aName,
                                            const nsACString& aGenericName,
                                            void *closure);
     PRBool ForEachFont(const nsAString& aFamilies,
                              const nsACString& aLangGroup,
                              FontCreationCallback fc,
                              void *closure);
    PRBool ForEachFont(FontCreationCallback fc, void *closure);

    const nsString& GetFamilies() { return mFamilies; }

    
    
    
    
    
    enum { UNDERLINE_OFFSET_NOT_SET = PR_INT16_MAX };
    virtual gfxFloat GetUnderlineOffset() {
        if (mUnderlineOffset == UNDERLINE_OFFSET_NOT_SET)
            mUnderlineOffset = GetFontAt(0)->GetMetrics().underlineOffset;
        return mUnderlineOffset;
    }

    already_AddRefed<gfxFont> FindFontForChar(PRUint32 ch, PRUint32 prevCh, PRUint32 nextCh, gfxFont *aPrevMatchedFont);

    virtual already_AddRefed<gfxFont> WhichPrefFontSupportsChar(PRUint32 aCh) { return nsnull; }

    virtual already_AddRefed<gfxFont> WhichSystemFontSupportsChar(PRUint32 aCh) { return nsnull; }

    void ComputeRanges(nsTArray<gfxTextRange>& mRanges, const PRUnichar *aString, PRUint32 begin, PRUint32 end);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    PRUint64 GetGeneration();

    
    
    virtual void UpdateFontList() { }

protected:
    nsString mFamilies;
    gfxFontStyle mStyle;
    nsTArray< nsRefPtr<gfxFont> > mFonts;
    gfxFloat mUnderlineOffset;

    gfxUserFontSet* mUserFontSet;
    PRUint64 mCurrGeneration;  

    
    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    
    
    void InitMetricsForBadFont(gfxFont* aBadFont);

    







     PRBool ForEachFontInternal(const nsAString& aFamilies,
                                      const nsACString& aLangGroup,
                                      PRBool aResolveGeneric,
                                      PRBool aResolveFontName,
                                      FontCreationCallback fc,
                                      void *closure);

    static PRBool FontResolverProc(const nsAString& aName, void *aClosure);

    inline gfxFont* WhichFontSupportsChar(nsTArray< nsRefPtr<gfxFont> >& aFontList, PRUint32 aCh) {
        PRUint32 len = aFontList.Length();
        for (PRUint32 i = 0; i < len; i++) {
            gfxFont* font = aFontList.ElementAt(i).get();
            if (font && font->HasCharacter(aCh))
                return font;
        }
        return nsnull;
    }

};
#endif
