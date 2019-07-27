




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

#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxFontGroup;
class gfxUserFontSet;
class gfxTextContextPaint;
class nsIAtom;
class nsILanguageAtomService;





struct gfxTextRunDrawCallbacks {

    







    explicit gfxTextRunDrawCallbacks(bool aShouldPaintSVGGlyphs = false)
      : mShouldPaintSVGGlyphs(aShouldPaintSVGGlyphs)
    {
    }

    




    virtual void NotifyGlyphPathEmitted() = 0;

    


    virtual void NotifyBeforeSVGGlyphPainted() { }

    


    virtual void NotifyAfterSVGGlyphPainted() { }

    bool mShouldPaintSVGGlyphs;
};




















class gfxTextRun : public gfxShapedText {
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
                             TextRunDrawParams& aParams);
    
    
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

    
    void DrawGlyphs(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                    gfxPoint *aPt, PropertyProvider *aProvider,
                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                    TextRunDrawParams& aParams);

    
    
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

    gfxFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                 const gfxFontStyle *aStyle,
                 gfxUserFontSet *aUserFontSet = nullptr);

    virtual ~gfxFontGroup();

    virtual gfxFont *GetFontAt(int32_t i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");
        NS_ASSERTION(mFonts.Length() > uint32_t(i) && mFonts[i].Font(), 
                     "Requesting a font index that doesn't exist");

        return mFonts[i].Font();
    }

    
    
    
    gfxFont *GetFirstMathFont();

    uint32_t FontListLength() const {
        return mFonts.Length();
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    



    static bool IsInvalidChar(uint8_t ch);
    static bool IsInvalidChar(char16_t ch);

    





    virtual gfxTextRun *MakeTextRun(const char16_t *aString, uint32_t aLength,
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

    





    gfxFloat GetHyphenWidth(gfxTextRun::PropertyProvider* aProvider);

    






    gfxTextRun *MakeHyphenTextRun(gfxContext *aCtx,
                                  uint32_t aAppUnitsPerDevUnit);

    



    bool HasFont(const gfxFontEntry *aFontEntry);

    
    
    
    
    
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
                       int32_t aRunScript, uint16_t aOrientation);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    uint64_t GetGeneration();

    
    gfxTextPerfMetrics *GetTextPerfMetrics() { return mTextPerf; }
    void SetTextPerfMetrics(gfxTextPerfMetrics *aTextPerf) { mTextPerf = aTextPerf; }

    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    
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

    
    static void
    ResolveGenericFontNames(mozilla::FontFamilyType aGenericType,
                            nsIAtom *aLanguage,
                            nsTArray<nsString>& aGenericFamilies);

protected:
    mozilla::FontFamilyList mFamilyList;
    gfxFontStyle mStyle;
    nsTArray<FamilyFace> mFonts;
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

    
    
    
    already_AddRefed<gfxFont> TryAllFamilyMembers(gfxFontFamily* aFamily,
                                                  uint32_t aCh);

    

    
    void EnumerateFontList(nsIAtom *aLanguage, void *aClosure = nullptr);

    
    void FindGenericFonts(mozilla::FontFamilyType aGenericType,
                          nsIAtom *aLanguage,
                          void *aClosure);

    
    virtual void FindPlatformFont(const nsAString& aName,
                                  bool aUseFontSet,
                                  void *aClosure);

    static nsILanguageAtomService* gLangService;
};
#endif
