




#ifndef GFX_TEXTRUN_H
#define GFX_TEXTRUN_H

#include "gfxTypes.h"
#include "nsString.h"
#include "gfxPoint.h"
#include "gfxFont.h"
#include "nsTArray.h"
#include "gfxSkipChars.h"
#include "gfxPlatform.h"
#include "mozilla/MemoryReporting.h"
#include "DrawMode.h"
#include "harfbuzz/hb.h"
#include "nsUnicodeScriptCodes.h"

#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxFontGroup;
class gfxUserFontEntry;
class gfxUserFontSet;
class gfxTextContextPaint;
class nsIAtom;
class nsILanguageAtomService;
class gfxMissingFontRecorder;





struct gfxTextRunDrawCallbacks {

    






    explicit gfxTextRunDrawCallbacks(bool aShouldPaintSVGGlyphs = false)
      : mShouldPaintSVGGlyphs(aShouldPaintSVGGlyphs)
    {
    }

    




    virtual void NotifyGlyphPathEmitted() = 0;

    bool mShouldPaintSVGGlyphs;
};




















class gfxTextRun : public gfxShapedText {
public:

    
    
    void operator delete(void* p) {
        free(p);
    }

    virtual ~gfxTextRun();

    typedef gfxFont::RunMetrics Metrics;

    

    bool IsClusterStart(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    bool IsLigatureGroupStart(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }
    bool CanBreakLineBefore(uint32_t aPos) const {
        return CanBreakBefore(aPos) == CompressedGlyph::FLAG_BREAK_TYPE_NORMAL;
    }
    bool CanHyphenateBefore(uint32_t aPos) const {
        return CanBreakBefore(aPos) == CompressedGlyph::FLAG_BREAK_TYPE_HYPHEN;
    }

    
    
    uint8_t CanBreakBefore(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore();
    }

    bool CharIsSpace(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsSpace();
    }
    bool CharIsTab(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsTab();
    }
    bool CharIsNewline(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsNewline();
    }
    bool CharIsLowSurrogate(uint32_t aPos) const {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsLowSurrogate();
    }

    
    
    
    
    

    











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

        
        
        virtual already_AddRefed<gfxContext> GetContext() = 0;

        
        
        virtual uint32_t GetAppUnitsPerDevUnit() = 0;
    };

    class ClusterIterator {
    public:
        explicit ClusterIterator(gfxTextRun *aTextRun);

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
              DrawMode aDrawMode,
              uint32_t aStart, uint32_t aLength,
              PropertyProvider *aProvider,
              gfxFloat *aAdvanceWidth, gfxTextContextPaint *aContextPaint,
              gfxTextRunDrawCallbacks *aCallbacks = nullptr);

    




    Metrics MeasureText(uint32_t aStart, uint32_t aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContextForTightBoundingBox,
                        PropertyProvider *aProvider);

    






    gfxFloat GetAdvanceWidth(uint32_t aStart, uint32_t aLength,
                             PropertyProvider *aProvider,
                             PropertyProvider::Spacing* aSpacing = nullptr);

    


























    virtual bool SetLineBreaks(uint32_t aStart, uint32_t aLength,
                                 bool aLineBreakBefore, bool aLineBreakAfter,
                                 gfxFloat *aAdvanceWidthDelta,
                                 gfxContext *aRefContext);

    enum SuppressBreak {
      eNoSuppressBreak,
      
      eSuppressInitialBreak,
      
      eSuppressAllBreaks
    };

    






















































    uint32_t BreakAndMeasureText(uint32_t aStart, uint32_t aMaxLength,
                                 bool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 SuppressBreak aSuppressBreak,
                                 gfxFloat *aTrimWhitespace,
                                 Metrics *aMetrics,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContextForTightBoundingBox,
                                 bool *aUsedHyphenation,
                                 uint32_t *aLastBreak,
                                 bool aCanWordWrap,
                                 gfxBreakPriority *aBreakPriority);

    




    void SetContext(gfxContext *aContext) {}

    

    void *GetUserData() const { return mUserData; }
    void SetUserData(void *aUserData) { mUserData = aUserData; }

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
        uint16_t          mOrientation; 
    };

    class GlyphRunIterator {
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
                         uint32_t aStartCharIndex, bool aForceNewRun,
                         uint16_t aOrientation);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    void SortGlyphRuns();
    void SanitizeGlyphRuns();

    CompressedGlyph* GetCharacterGlyphs() {
        NS_ASSERTION(mCharacterGlyphs, "failed to initialize mCharacterGlyphs");
        return mCharacterGlyphs;
    }

    
    void ClearGlyphsAndCharacters();

    void SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, uint32_t aCharIndex,
                       uint16_t aOrientation);

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                               uint32_t aCharIndex, char16_t aSpaceChar,
                               uint16_t aOrientation);

    
    
    
    
    
    
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
    
    
    
    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
      MOZ_MUST_OVERRIDE;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
      MOZ_MUST_OVERRIDE;

    
    size_t MaybeSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)  {
        if (mFlags & gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED) {
            return 0;
        }
        mFlags |= gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
        return SizeOfIncludingThis(aMallocSizeOf);
    }
    void ResetSizeOfAccountingFlags() {
        mFlags &= ~gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
    }

    
    
    

    enum ShapingState {
        eShapingState_Normal,                 
        eShapingState_ShapingWithFeature,     
        eShapingState_ShapingWithFallback,    
        eShapingState_Aborted,                
        eShapingState_ForceFallbackFeature    
    };

    ShapingState GetShapingState() const { return mShapingState; }
    void SetShapingState(ShapingState aShapingState) {
        mShapingState = aShapingState;
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
    

    
    int32_t GetAdvanceForGlyphs(uint32_t aStart, uint32_t aEnd);

    
    
    
    
    bool GetAdjustedSpacingArray(uint32_t aStart, uint32_t aEnd,
                                   PropertyProvider *aProvider,
                                   uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);

    
    
    

    
    LigatureData ComputeLigatureData(uint32_t aPartStart, uint32_t aPartEnd,
                                     PropertyProvider *aProvider);
    gfxFloat ComputePartialLigatureWidth(uint32_t aPartStart, uint32_t aPartEnd,
                                         PropertyProvider *aProvider);
    void DrawPartialLigature(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                             gfxPoint *aPt, PropertyProvider *aProvider,
                             TextRunDrawParams& aParams, uint16_t aOrientation);
    
    
    void ShrinkToLigatureBoundaries(uint32_t *aStart, uint32_t *aEnd);
    
    gfxFloat GetPartialLigatureWidth(uint32_t aStart, uint32_t aEnd, PropertyProvider *aProvider);
    void AccumulatePartialLigatureMetrics(gfxFont *aFont,
                                          uint32_t aStart, uint32_t aEnd,
                                          gfxFont::BoundingBoxType aBoundingBoxType,
                                          gfxContext *aRefContext,
                                          PropertyProvider *aProvider,
                                          uint16_t aOrientation,
                                          Metrics *aMetrics);

    
    void AccumulateMetricsForRun(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContext,
                                 PropertyProvider *aProvider,
                                 uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                 uint16_t aOrientation,
                                 Metrics *aMetrics);

    
    void DrawGlyphs(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                    gfxPoint *aPt, PropertyProvider *aProvider,
                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                    TextRunDrawParams& aParams, uint16_t aOrientation);

    
    
    nsAutoTArray<GlyphRun,1>        mGlyphRuns;

    void             *mUserData;
    gfxFontGroup     *mFontGroup; 
                                  
    gfxSkipChars      mSkipChars;
    nsExpirationState mExpirationState;

    bool              mSkipDrawing; 
                                    
                                    
    bool              mReleasedFontGroup; 
                                          

    
    
    ShapingState      mShapingState;
};

class gfxFontGroup : public gfxTextRunFactory {
public:
    static void Shutdown(); 

    gfxFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                 const gfxFontStyle *aStyle,
                 gfxUserFontSet *aUserFontSet = nullptr);

    virtual ~gfxFontGroup();

    
    
    virtual gfxFont* GetFirstValidFont(uint32_t aCh = 0x20);

    
    
    
    gfxFont *GetFirstMathFont();

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    



    static bool IsInvalidChar(uint8_t ch);
    static bool IsInvalidChar(char16_t ch);

    





    virtual gfxTextRun *MakeTextRun(const char16_t *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags,
                                    gfxMissingFontRecorder *aMFR);
    





    virtual gfxTextRun *MakeTextRun(const uint8_t *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags,
                                    gfxMissingFontRecorder *aMFR);

    



    template<typename T>
    gfxTextRun *MakeTextRun(const T *aString, uint32_t aLength,
                            gfxContext *aRefContext,
                            int32_t aAppUnitsPerDevUnit,
                            uint32_t aFlags,
                            gfxMissingFontRecorder *aMFR)
    {
        gfxTextRunFactory::Parameters params = {
            aRefContext, nullptr, nullptr, nullptr, 0, aAppUnitsPerDevUnit
        };
        return MakeTextRun(aString, aLength, &params, aFlags, aMFR);
    }

    





    gfxFloat GetHyphenWidth(gfxTextRun::PropertyProvider* aProvider);

    






    gfxTextRun *MakeHyphenTextRun(gfxContext *aCtx,
                                  uint32_t aAppUnitsPerDevUnit);

    



    bool HasFont(const gfxFontEntry *aFontEntry);

    
    
    
    
    
    
    enum { UNDERLINE_OFFSET_NOT_SET = INT16_MAX };
    virtual gfxFloat GetUnderlineOffset();

    virtual already_AddRefed<gfxFont>
        FindFontForChar(uint32_t ch, uint32_t prevCh, uint32_t aNextCh,
                        int32_t aRunScript, gfxFont *aPrevMatchedFont,
                        uint8_t *aMatchType);

    
    virtual already_AddRefed<gfxFont> WhichPrefFontSupportsChar(uint32_t aCh);

    already_AddRefed<gfxFont>
        WhichSystemFontSupportsChar(uint32_t aCh, uint32_t aNextCh,
                                    int32_t aRunScript);

    template<typename T>
    void ComputeRanges(nsTArray<gfxTextRange>& mRanges,
                       const T *aString, uint32_t aLength,
                       int32_t aRunScript, uint16_t aOrientation);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    uint64_t GetGeneration();

    
    uint64_t GetRebuildGeneration();

    
    gfxTextPerfMetrics *GetTextPerfMetrics() { return mTextPerf; }
    void SetTextPerfMetrics(gfxTextPerfMetrics *aTextPerf) { mTextPerf = aTextPerf; }

    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    
    virtual void UpdateUserFonts();

    
    bool ContainsUserFont(const gfxUserFontEntry* aUserFont);

    bool ShouldSkipDrawing() const {
        return mSkipDrawing;
    }

    class LazyReferenceContextGetter {
    public:
      virtual already_AddRefed<gfxContext> GetRefContext() = 0;
    };
    
    
    
    
    
    gfxTextRun* GetEllipsisTextRun(int32_t aAppUnitsPerDevPixel, uint32_t aFlags,
                                   LazyReferenceContextGetter& aRefContextGetter);

    
    static void
    ResolveGenericFontNames(mozilla::FontFamilyType aGenericType,
                            nsIAtom *aLanguage,
                            nsTArray<nsString>& aGenericFamilies);

protected:
    class FamilyFace {
    public:
        FamilyFace() : mFamily(nullptr), mFontEntry(nullptr),
                       mNeedsBold(false), mFontCreated(false),
                       mLoading(false), mInvalid(false)
        { }

        FamilyFace(gfxFontFamily* aFamily, gfxFont* aFont)
            : mFamily(aFamily), mNeedsBold(false), mFontCreated(true),
              mLoading(false), mInvalid(false)
        {
            NS_ASSERTION(aFont, "font pointer must not be null");
            NS_ASSERTION(!aFamily ||
                         aFamily->ContainsFace(aFont->GetFontEntry()),
                         "font is not a member of the given family");
            mFont = aFont;
            NS_ADDREF(aFont);
        }

        FamilyFace(gfxFontFamily* aFamily, gfxFontEntry* aFontEntry,
                   bool aNeedsBold)
            : mFamily(aFamily), mNeedsBold(aNeedsBold), mFontCreated(false),
              mLoading(false), mInvalid(false)
        {
            NS_ASSERTION(aFontEntry, "font entry pointer must not be null");
            NS_ASSERTION(!aFamily ||
                         aFamily->ContainsFace(aFontEntry),
                         "font is not a member of the given family");
            mFontEntry = aFontEntry;
            NS_ADDREF(aFontEntry);
        }

        FamilyFace(const FamilyFace& aOtherFamilyFace)
            : mFamily(aOtherFamilyFace.mFamily),
              mNeedsBold(aOtherFamilyFace.mNeedsBold),
              mFontCreated(aOtherFamilyFace.mFontCreated),
              mLoading(aOtherFamilyFace.mLoading),
              mInvalid(aOtherFamilyFace.mInvalid)
        {
            if (mFontCreated) {
                mFont = aOtherFamilyFace.mFont;
                NS_ADDREF(mFont);
            } else {
                mFontEntry = aOtherFamilyFace.mFontEntry;
                NS_IF_ADDREF(mFontEntry);
            }
        }

        ~FamilyFace()
        {
            if (mFontCreated) {
                NS_RELEASE(mFont);
            } else {
                NS_IF_RELEASE(mFontEntry);
            }
        }

        FamilyFace& operator=(const FamilyFace& aOther)
        {
            if (mFontCreated) {
                NS_RELEASE(mFont);
            } else {
                NS_IF_RELEASE(mFontEntry);
            }

            mFamily = aOther.mFamily;
            mNeedsBold = aOther.mNeedsBold;
            mFontCreated = aOther.mFontCreated;
            mLoading = aOther.mLoading;
            mInvalid = aOther.mInvalid;

            if (mFontCreated) {
                mFont = aOther.mFont;
                NS_ADDREF(mFont);
            } else {
                mFontEntry = aOther.mFontEntry;
                NS_IF_ADDREF(mFontEntry);
            }

            return *this;
        }

        gfxFontFamily* Family() const { return mFamily.get(); }
        gfxFont* Font() const {
            return mFontCreated ? mFont : nullptr;
        }

        gfxFontEntry* FontEntry() const {
            return mFontCreated ? mFont->GetFontEntry() : mFontEntry;
        }

        bool NeedsBold() const { return mNeedsBold; }
        bool IsUserFontContainer() const {
            return FontEntry()->mIsUserFontContainer;
        }
        bool IsLoading() const { return mLoading; }
        bool IsInvalid() const { return mInvalid; }
        void CheckState(bool& aSkipDrawing);
        void SetLoading(bool aIsLoading) { mLoading = aIsLoading; }
        void SetInvalid() { mInvalid = true; }

        void SetFont(gfxFont* aFont)
        {
            NS_ASSERTION(aFont, "font pointer must not be null");
            NS_ADDREF(aFont);
            if (mFontCreated) {
                NS_RELEASE(mFont);
            } else {
                NS_IF_RELEASE(mFontEntry);
            }
            mFont = aFont;
            mFontCreated = true;
            mLoading = false;
        }

        bool EqualsUserFont(const gfxUserFontEntry* aUserFont) const;

    private:
        nsRefPtr<gfxFontFamily> mFamily;
        
        union {
            gfxFont*            mFont;
            gfxFontEntry*       mFontEntry;
        };
        bool                    mNeedsBold   : 1;
        bool                    mFontCreated : 1;
        bool                    mLoading     : 1;
        bool                    mInvalid     : 1;
    };

    
    
    mozilla::FontFamilyList mFamilyList;

    
    
    
    nsTArray<FamilyFace> mFonts;

    nsRefPtr<gfxFont> mDefaultFont;
    gfxFontStyle mStyle;

    gfxFloat mUnderlineOffset;
    gfxFloat mHyphenWidth;

    nsRefPtr<gfxUserFontSet> mUserFontSet;
    uint64_t mCurrGeneration;  

    gfxTextPerfMetrics *mTextPerf;

    
    
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

    
    void BuildFontList();

    
    
    
    virtual gfxFont* GetFontAt(int32_t i, uint32_t aCh = 0x20);

    
    
    bool FontLoadingForFamily(gfxFontFamily* aFamily, uint32_t aCh) const;

    
    gfxFont* GetDefaultFont();

    
    
    
    void InitMetricsForBadFont(gfxFont* aBadFont);

    
    
    template<typename T>
    void InitTextRun(gfxContext *aContext,
                     gfxTextRun *aTextRun,
                     const T *aString,
                     uint32_t aLength,
                     gfxMissingFontRecorder *aMFR);

    
    
    template<typename T>
    void InitScriptRun(gfxContext *aContext,
                       gfxTextRun *aTextRun,
                       const T *aString,
                       uint32_t aScriptRunStart,
                       uint32_t aScriptRunEnd,
                       int32_t aRunScript,
                       gfxMissingFontRecorder *aMFR);

    
    
    
    
    already_AddRefed<gfxFont>
    FindNonItalicFaceForChar(gfxFontFamily* aFamily, uint32_t aCh);

    

    
    void EnumerateFontList(nsIAtom *aLanguage, void *aClosure = nullptr);

    
    void FindGenericFonts(mozilla::FontFamilyType aGenericType,
                          nsIAtom *aLanguage,
                          void *aClosure);

    
    virtual void FindPlatformFont(const nsAString& aName,
                                  bool aUseFontSet,
                                  void *aClosure);

    static nsILanguageAtomService* gLangService;
};






#define GFX_MISSING_FONTS_NOTIFY_PREF "gfx.missing_fonts.notify"

class gfxMissingFontRecorder {
public:
    gfxMissingFontRecorder()
    {
        MOZ_COUNT_CTOR(gfxMissingFontRecorder);
        memset(&mMissingFonts, 0, sizeof(mMissingFonts));
    }

    ~gfxMissingFontRecorder()
    {
#ifdef DEBUG
        for (uint32_t i = 0; i < kNumScriptBitsWords; i++) {
            NS_ASSERTION(mMissingFonts[i] == 0,
                         "failed to flush the missing-font recorder");
        }
#endif
        MOZ_COUNT_DTOR(gfxMissingFontRecorder);
    }

    
    void RecordScript(int32_t aScriptCode)
    {
        mMissingFonts[uint32_t(aScriptCode) >> 5] |=
            (1 << (uint32_t(aScriptCode) & 0x1f));
    }

    
    
    void Flush();

    
    
    void Clear()
    {
        memset(&mMissingFonts, 0, sizeof(mMissingFonts));
    }

private:
    
    static const uint32_t kNumScriptBitsWords =
        ((MOZ_NUM_SCRIPT_CODES + 31) / 32);
    uint32_t mMissingFonts[kNumScriptBitsWords];
};

#endif
