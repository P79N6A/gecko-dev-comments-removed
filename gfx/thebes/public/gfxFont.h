





































#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "prtypes.h"
#include "gfxTypes.h"
#include "nsString.h"
#include "gfxPoint.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "gfxSkipChars.h"
#include "gfxRect.h"
#include "nsExpirationTracker.h"

class gfxContext;
class gfxTextRun;
class nsIAtom;
class gfxFont;
class gfxFontGroup;
typedef struct _cairo cairo_t;

#define FONT_STYLE_NORMAL              0
#define FONT_STYLE_ITALIC              1
#define FONT_STYLE_OBLIQUE             2

#define FONT_VARIANT_NORMAL            0
#define FONT_VARIANT_SMALL_CAPS        1

#define FONT_DECORATION_NONE           0x0
#define FONT_DECORATION_UNDERLINE      0x1
#define FONT_DECORATION_OVERLINE       0x2
#define FONT_DECORATION_STRIKEOUT      0x4

#define FONT_WEIGHT_NORMAL             400
#define FONT_WEIGHT_BOLD               700

struct THEBES_API gfxFontStyle {
    gfxFontStyle(PRUint8 aStyle, PRUint8 aVariant,
                 PRUint16 aWeight, PRUint8 aDecoration, gfxFloat aSize,
                 const nsACString& aLangGroup,
                 float aSizeAdjust, PRPackedBool aSystemFont,
                 PRPackedBool aFamilyNameQuirks);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    PRUint8 style : 7;

    
    
    
    PRPackedBool systemFont : 1;

    
    PRUint8 variant : 7;

    
    
    PRPackedBool familyNameQuirks : 1;
    
    
    
    
    
    
    PRUint16 weight;

    
    
    PRUint8 decorations;

    
    gfxFloat size;

    
    nsCString langGroup;

    
    
    
    
    float sizeAdjust;

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
            (familyNameQuirks == other.familyNameQuirks) &&
            (weight == other.weight) &&
            (langGroup.Equals(other.langGroup)) &&
            (sizeAdjust == other.sizeAdjust);
    }
};















class THEBES_API gfxFontCache : public nsExpirationTracker<gfxFont,3> {
public:
    enum { TIMEOUT_SECONDS = 1 }; 
    gfxFontCache()
        : nsExpirationTracker<gfxFont,3>(TIMEOUT_SECONDS*1000) { mFonts.Init(); }
    ~gfxFontCache() {
        
        AgeAllGenerations();
        
        
        NS_ASSERTION(mFonts.Count() == 0,
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


class THEBES_API gfxFont {
public:
    nsrefcnt AddRef(void) {
        NS_PRECONDITION(PRInt32(mRefCnt) >= 0, "illegal refcnt");
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "gfxFont", sizeof(*this));
        return mRefCnt;
    }
    nsrefcnt Release(void) {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "gfxFont");
        if (mRefCnt == 0) {
            
            
            gfxFontCache::GetCache()->NotifyReleased(this);
            return 0;
        }
        return mRefCnt;
    }

    PRInt32 GetRefCount() { return mRefCnt; }

protected:
    nsAutoRefCnt mRefCnt;

public:
    gfxFont(const nsAString &aName, const gfxFontStyle *aFontGroup);
    virtual ~gfxFont() {}

    const nsString& GetName() const { return mName; }
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
        gfxFloat height;

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
    };
    virtual const gfxFont::Metrics& GetMetrics() = 0;

    







    struct Spacing {
        gfxFloat mBefore;
        gfxFloat mAfter;
    };
    


    struct RunMetrics {
        RunMetrics() {
            mAdvanceWidth = mAscent = mDescent = 0.0;
            mBoundingBox = gfxRect(0,0,0,0);
            mClusterCount = 0;
        }

        void CombineWith(const RunMetrics& aOtherOnRight) {
            mAscent = PR_MAX(mAscent, aOtherOnRight.mAscent);
            mDescent = PR_MAX(mDescent, aOtherOnRight.mDescent);
            mBoundingBox =
                mBoundingBox.Union(aOtherOnRight.mBoundingBox + gfxPoint(mAdvanceWidth, 0));
            mAdvanceWidth += aOtherOnRight.mAdvanceWidth;
            mClusterCount += aOtherOnRight.mClusterCount;
        }

        
        
        
        
        gfxFloat mAdvanceWidth;
        
        
        gfxFloat mAscent;  
        gfxFloat mDescent; 
        
        
        
        
        
        
        
        gfxRect  mBoundingBox;
        
        
        
        PRInt32  mClusterCount;
    };

    






















    virtual void Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                      gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aBaselineOrigin,
                      Spacing *aSpacing);
    


















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               PRUint32 aStart, PRUint32 aEnd,
                               PRBool aTightBoundingBox,
                               Spacing *aSpacing);
    




    PRBool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   PRUint32 aStart, PRUint32 aLength)
    { return PR_FALSE; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

protected:
    
    nsString          mName;
    nsExpirationState mExpirationState;
    gfxFontStyle      mStyle;

    
    virtual void SetupCairoFont(cairo_t *aCR) = 0;
};

class THEBES_API gfxTextRunFactory {
    THEBES_INLINE_DECL_REFCOUNTING(gfxTextRunFactory)

public:
    
    
    
    enum {
        



        TEXT_IS_PERSISTENT           = 0x0001,
        


        TEXT_IS_ASCII                = 0x0002,
        


        TEXT_IS_RTL                  = 0x0004,
        



        TEXT_ENABLE_SPACING          = 0x0008,
        


        TEXT_ENABLE_NEGATIVE_SPACING = 0x0010,
        



        TEXT_ABSOLUTE_SPACING        = 0x0020,
        



        TEXT_ENABLE_HYPHEN_BREAKS    = 0x0040,
        


        TEXT_IS_8BIT                 = 0x0080,
        



        TEXT_HAS_SURROGATES          = 0x0100,
        






        TEXT_NEED_BOUNDING_BOX       = 0x0200
    };

    


    struct Parameters {
        
        gfxContext   *mContext;
        
        void         *mUserData;
        
        nsIAtom      *mLangGroup;
        
        
        
        gfxSkipChars *mSkipChars;
        
        
        PRUint32     *mInitialBreaks;
        PRUint32      mInitialBreakCount;
        
        PRUint32      mAppUnitsPerDevUnit;
        
        PRUint32      mFlags;
    };

    virtual ~gfxTextRunFactory() {}

    



    virtual gfxTextRun *MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                                    Parameters *aParams) = 0;
    



    virtual gfxTextRun *MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                                    Parameters *aParams) = 0;
};












































class THEBES_API gfxTextRun {
public:
    ~gfxTextRun() {}

    typedef gfxFont::RunMetrics Metrics;

    

    PRBool IsClusterStart(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    PRBool IsLigatureContinuation(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureContinuation();
    }
    PRBool CanBreakLineBefore(PRUint32 aPos) {
        NS_ASSERTION(0 <= aPos && aPos < mCharacterCount, "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore();
    }    

    PRUint32 GetLength() { return mCharacterCount; }

    
    
    
    
    

    





    void RememberText(const PRUnichar *aText, PRUint32 aLength) {}
    void RememberText(const PRUint8 *aText, PRUint32 aLength) {}

    











    virtual PRBool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                          PRPackedBool *aBreakBefore);

    









    class PropertyProvider {
    public:
        







        virtual void ForceRememberText() = 0;

        
        
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
                        PRBool aTightBoundingBox,
                        PropertyProvider *aProvider);

    



    gfxFloat GetAdvanceWidth(PRUint32 aStart, PRUint32 aLength,
                             PropertyProvider *aProvider);

    


























    virtual PRBool SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                 PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                                 PropertyProvider *aProvider,
                                 gfxFloat *aAdvanceWidthDelta);

    










































    PRUint32 BreakAndMeasureText(PRUint32 aStart, PRUint32 aMaxLength,
                                 PRBool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 PRBool aSuppressInitialBreak,
                                 Metrics *aMetrics, PRBool aTightBoundingBox,
                                 PRBool *aUsedHyphenation,
                                 PRUint32 *aLastBreak);

    




    void SetContext(gfxContext *aContext) {}

    

    PRBool IsRightToLeft() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0; }
    gfxFloat GetDirection() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) ? -1.0 : 1.0; }
    void *GetUserData() const { return mUserData; }
    PRUint32 GetFlags() const { return mFlags; }
    const gfxSkipChars& GetSkipChars() const { return mSkipChars; }
    PRUint32 GetAppUnitsPerDevUnit() const { return mAppUnitsPerDevUnit; }

    
    
    gfxTextRun(gfxTextRunFactory::Parameters *aParams, PRUint32 aLength);

    




    class CompressedGlyph {
    public:
        CompressedGlyph() { mValue = 0; }

        enum {
            
            
            
            
            
            
            FLAG_IS_SIMPLE_GLYPH  = 0x80000000U,
            
            FLAG_CAN_BREAK_BEFORE = 0x40000000U,

            
            ADVANCE_MASK  = 0x3FFF0000U,
            ADVANCE_SHIFT = 16,

            GLYPH_MASK = 0x0000FFFFU,

            
            
            TAG_MASK                  = 0x000000FFU,
            
            
            
            TAG_MISSING               = 0x00U,
            
            
            
            TAG_COMPLEX_CLUSTER       = 0x01U,
            
            
            
            
            TAG_LIGATURE_CONTINUATION = 0x21U,
            
            
            
            
            
            
            TAG_LOW_SURROGATE         = 0x80U,
            
            
            
            TAG_CLUSTER_CONTINUATION  = 0x81U
        };

        
        
        
        

        
        static PRBool IsSimpleGlyphID(PRUint32 aGlyph) {
            return (aGlyph & GLYPH_MASK) == aGlyph;
        }
        
        
        static PRBool IsSimpleAdvance(PRUint32 aAdvance) {
            return (aAdvance & (ADVANCE_MASK >> ADVANCE_SHIFT)) == aAdvance;
        }

        PRBool IsSimpleGlyph() const { return (mValue & FLAG_IS_SIMPLE_GLYPH) != 0; }
        PRBool IsComplex(PRUint32 aTag) const { return (mValue & (FLAG_IS_SIMPLE_GLYPH|TAG_MASK))  == aTag; }
        PRBool IsMissing() const { return IsComplex(TAG_MISSING); }
        PRBool IsComplexCluster() const { return IsComplex(TAG_COMPLEX_CLUSTER); }
        PRBool IsComplexOrMissing() const {
            return IsComplex(TAG_COMPLEX_CLUSTER) || IsComplex(TAG_MISSING);
        }
        PRBool IsLigatureContinuation() const { return IsComplex(TAG_LIGATURE_CONTINUATION); }
        PRBool IsClusterContinuation() const { return IsComplex(TAG_CLUSTER_CONTINUATION); }
        PRBool IsLowSurrogate() const { return IsComplex(TAG_LOW_SURROGATE); }
        PRBool IsClusterStart() const { return (mValue & (FLAG_IS_SIMPLE_GLYPH|0x80U)) != 0x80U; }

        PRUint32 GetSimpleAdvance() const { return (mValue & ADVANCE_MASK) >> ADVANCE_SHIFT; }
        PRUint32 GetSimpleGlyph() const { return mValue & GLYPH_MASK; }

        PRUint32 GetComplexTag() const { return mValue & TAG_MASK; }

        PRBool CanBreakBefore() const { return (mValue & FLAG_CAN_BREAK_BEFORE) != 0; }
        
        PRUint32 SetCanBreakBefore(PRBool aCanBreakBefore) {
            PRUint32 breakMask = aCanBreakBefore*FLAG_CAN_BREAK_BEFORE;
            PRUint32 toggle = breakMask ^ (mValue & FLAG_CAN_BREAK_BEFORE);
            mValue ^= toggle;
            return toggle;
        }

        CompressedGlyph& SetSimpleGlyph(PRUint32 aAdvancePixels, PRUint32 aGlyph) {
            NS_ASSERTION(IsSimpleAdvance(aAdvancePixels), "Advance overflow");
            NS_ASSERTION(IsSimpleGlyphID(aGlyph), "Glyph overflow");
            mValue = (mValue & FLAG_CAN_BREAK_BEFORE) | FLAG_IS_SIMPLE_GLYPH |
                (aAdvancePixels << ADVANCE_SHIFT) | aGlyph;
            return *this;
        }
        CompressedGlyph& SetComplex(PRUint32 aTag) {
            mValue = (mValue & FLAG_CAN_BREAK_BEFORE) | aTag;
            return *this;
        }
        CompressedGlyph& SetMissing() { return SetComplex(TAG_MISSING); }
        CompressedGlyph& SetComplexCluster() { return SetComplex(TAG_COMPLEX_CLUSTER); }
        CompressedGlyph& SetLowSurrogate() { return SetComplex(TAG_LOW_SURROGATE); }
        CompressedGlyph& SetLigatureContinuation() { return SetComplex(TAG_LIGATURE_CONTINUATION); }
        CompressedGlyph& SetClusterContinuation() { return SetComplex(TAG_CLUSTER_CONTINUATION); }
    private:
        PRUint32 mValue;
    };

    



    struct DetailedGlyph {
        

        PRUint32 mIsLastGlyph:1;
        

        PRUint32 mGlyphID:31;
        


   
        PRInt32  mAdvance;
        float    mXOffset, mYOffset;
    };

    
    struct GlyphRun {
        nsRefPtr<gfxFont> mFont;   
        PRUint32          mCharacterOffset; 
    };

    class GlyphRunIterator {
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

    friend class GlyphRunIterator;
    friend class FontSelector;

    
    
    



    void RecordSurrogates(const PRUnichar *aString);
    





    nsresult AddGlyphRun(gfxFont *aFont, PRUint32 aStartCharIndex);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    
    
    



    void SetCharacterGlyph(PRUint32 aCharIndex, CompressedGlyph aGlyph) {
        NS_ASSERTION(aCharIndex > 0 ||
                     (aGlyph.IsClusterStart() && !aGlyph.IsLigatureContinuation()),
                     "First character must be the start of a cluster and can't be a ligature continuation!");
        if (mCharacterGlyphs) {
            mCharacterGlyphs[aCharIndex] = aGlyph;
        }
        if (mDetailedGlyphs) {
            mDetailedGlyphs[aCharIndex] = nsnull;
        }
    }
    



    void SetDetailedGlyphs(PRUint32 aCharIndex, const DetailedGlyph *aGlyphs,
                           PRUint32 aNumGlyphs);
    void SetMissingGlyph(PRUint32 aCharIndex, PRUnichar aChar);

    
    
    const CompressedGlyph *GetCharacterGlyphs() { return mCharacterGlyphs; }
    const DetailedGlyph *GetDetailedGlyphs(PRUint32 aCharIndex) {
        NS_ASSERTION(mDetailedGlyphs && mDetailedGlyphs[aCharIndex],
                     "Requested detailed glyphs when there aren't any, "
                     "I think I'll go and have a lie down...");
        return mDetailedGlyphs[aCharIndex];
    }
    PRUint32 CountMissingGlyphs();
    const GlyphRun *GetGlyphRuns(PRUint32 *aNumGlyphRuns) {
        *aNumGlyphRuns = mGlyphRuns.Length();
        return mGlyphRuns.Elements();
    }

private:
    

    
    DetailedGlyph *AllocateDetailedGlyphs(PRUint32 aCharIndex, PRUint32 aCount);
    
    
    PRUint32 FindFirstGlyphRunContaining(PRUint32 aOffset);
    
    
    PRInt32 ComputeClusterAdvance(PRUint32 aClusterOffset);

    
    
    

    struct LigatureData {
        PRUint32 mStartOffset;
        PRUint32 mEndOffset;
        PRUint32 mClusterCount;
        PRUint32 mPartClusterIndex;
        PRInt32  mLigatureWidth;  
        gfxFloat mBeforeSpacing;  
        gfxFloat mAfterSpacing;   
    };
    
    LigatureData ComputeLigatureData(PRUint32 aPartOffset, PropertyProvider *aProvider);
    void GetAdjustedSpacing(PRUint32 aStart, PRUint32 aEnd,
                            PropertyProvider *aProvider, PropertyProvider::Spacing *aSpacing);
    PRBool GetAdjustedSpacingArray(PRUint32 aStart, PRUint32 aEnd,
                                   PropertyProvider *aProvider,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);
    void DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx, PRUint32 aOffset,
                             const gfxRect *aDirtyRect, gfxPoint *aPt,
                             PropertyProvider *aProvider);
    void ShrinkToLigatureBoundaries(PRUint32 *aStart, PRUint32 *aEnd);
    
    gfxFloat GetPartialLigatureWidth(PRUint32 aStart, PRUint32 aEnd, PropertyProvider *aProvider);
    void AccumulatePartialLigatureMetrics(gfxFont *aFont,
                                          PRUint32 aOffset, PRBool aTight,
                                          PropertyProvider *aProvider,
                                          Metrics *aMetrics);

    
    void AccumulateMetricsForRun(gfxFont *aFont, PRUint32 aStart,
                                 PRUint32 aEnd, PRBool aTight,
                                 PropertyProvider *aProvider,
                                 Metrics *aMetrics);

    
    void DrawGlyphs(gfxFont *aFont, gfxContext *aContext, PRBool aDrawToPath,
                    gfxPoint *aPt, PRUint32 aStart, PRUint32 aEnd,
                    PropertyProvider *aProvider);

    
    nsAutoArrayPtr<CompressedGlyph>                mCharacterGlyphs;
    nsAutoArrayPtr<nsAutoArrayPtr<DetailedGlyph> > mDetailedGlyphs; 
    
    
    nsAutoTArray<GlyphRun,1>                       mGlyphRuns;

    void        *mUserData;
    gfxSkipChars mSkipChars;
    
    
    PRUint32     mAppUnitsPerDevUnit;
    PRUint32     mFlags;
    PRUint32     mCharacterCount;
};

class THEBES_API gfxFontGroup : public gfxTextRunFactory {
public:
    gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle);

    virtual ~gfxFontGroup() {
        mFonts.Clear();
    }

    gfxFont *GetFontAt(PRInt32 i) {
        return NS_STATIC_CAST(gfxFont*, mFonts[i]);
    }
    PRUint32 FontListLength() const {
        return mFonts.Length();
    }

    PRBool Equals(const gfxFontGroup& other) const {
        return mFamilies.Equals(other.mFamilies) &&
            mStyle.Equals(other.mStyle);
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle) = 0;

    
    
    virtual gfxTextRun *MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    Parameters* aParams) = 0;
    
    virtual gfxTextRun *MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    Parameters* aParams) = 0;

    


    typedef PRBool (*FontCreationCallback) (const nsAString& aName,
                                            const nsACString& aGenericName,
                                            void *closure);
    static PRBool ForEachFont(const nsAString& aFamilies,
                              const nsACString& aLangGroup,
                              FontCreationCallback fc,
                              void *closure);
    PRBool ForEachFont(FontCreationCallback fc, void *closure);

    
    void FindGenericFontFromStyle(FontCreationCallback fc, void *closure);

    const nsString& GetFamilies() { return mFamilies; }

    




    enum SpecialString {
        STRING_ELLIPSIS,
        STRING_HYPHEN,
        STRING_SPACE,
        STRING_MAX = STRING_SPACE
    };

    
    
    
    
    
    gfxTextRun *GetSpecialStringTextRun(SpecialString aString,
                                        gfxTextRun *aTemplate);

protected:
    nsString mFamilies;
    gfxFontStyle mStyle;
    nsTArray< nsRefPtr<gfxFont> > mFonts;
    nsAutoPtr<gfxTextRun> mSpecialStrings[STRING_MAX + 1];

    static PRBool ForEachFontInternal(const nsAString& aFamilies,
                                      const nsACString& aLangGroup,
                                      PRBool aResolveGeneric,
                                      FontCreationCallback fc,
                                      void *closure);

    static PRBool FontResolverProc(const nsAString& aName, void *aClosure);
};
#endif
