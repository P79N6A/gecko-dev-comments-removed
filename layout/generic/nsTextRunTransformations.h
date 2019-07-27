




#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "gfxTextRun.h"
#include "nsStyleContext.h"

class nsTransformedTextRun;

struct nsTransformedCharStyle MOZ_FINAL {
  NS_INLINE_DECL_REFCOUNTING(nsTransformedCharStyle)

  explicit nsTransformedCharStyle(nsStyleContext* aContext)
    : mFont(aContext->StyleFont()->mFont)
    , mLanguage(aContext->StyleFont()->mLanguage)
    , mPresContext(aContext->PresContext())
    , mScriptSizeMultiplier(aContext->StyleFont()->mScriptSizeMultiplier)
    , mTextTransform(aContext->StyleText()->mTextTransform)
    , mMathVariant(aContext->StyleFont()->mMathVariant)
    , mExplicitLanguage(aContext->StyleFont()->mExplicitLanguage) {}

  nsFont                  mFont;
  nsCOMPtr<nsIAtom>       mLanguage;
  nsRefPtr<nsPresContext> mPresContext;
  float                   mScriptSizeMultiplier;
  uint8_t                 mTextTransform;
  uint8_t                 mMathVariant;
  bool                    mExplicitLanguage;

private:
  ~nsTransformedCharStyle() {}
  nsTransformedCharStyle(const nsTransformedCharStyle& aOther) = delete;
  nsTransformedCharStyle& operator=(const nsTransformedCharStyle& aOther) = delete;
};

class nsTransformingTextRunFactory {
public:
  virtual ~nsTransformingTextRunFactory() {}

  
  nsTransformedTextRun* MakeTextRun(const uint8_t* aString, uint32_t aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, uint32_t aFlags,
                                    nsTArray<nsRefPtr<nsTransformedCharStyle>>&& aStyles,
                                    bool aOwnsFactory);
  nsTransformedTextRun* MakeTextRun(const char16_t* aString, uint32_t aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, uint32_t aFlags,
                                    nsTArray<nsRefPtr<nsTransformedCharStyle>>&& aStyles,
                                    bool aOwnsFactory);

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext,
                              gfxMissingFontRecorder* aMFR) = 0;
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  
  
  
  
  
  
  
  explicit nsCaseTransformTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                         bool aAllUppercase = false)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext,
                              gfxMissingFontRecorder* aMFR) MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  
  static bool TransformString(const nsAString& aString,
                              nsString& aConvertedString,
                              bool aAllUppercase,
                              const nsIAtom* aLanguage,
                              nsTArray<bool>& aCharsToMergeArray,
                              nsTArray<bool>& aDeletedCharsArray,
                              nsTransformedTextRun* aTextRun = nullptr,
                              nsTArray<uint8_t>* aCanBreakBeforeArray = nullptr,
                              nsTArray<nsRefPtr<nsTransformedCharStyle>>* aStyleArray = nullptr);

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  bool                                    mAllUppercase;
};





class nsTransformedTextRun MOZ_FINAL : public gfxTextRun {
public:

  static nsTransformedTextRun *Create(const gfxTextRunFactory::Parameters* aParams,
                                      nsTransformingTextRunFactory* aFactory,
                                      gfxFontGroup* aFontGroup,
                                      const char16_t* aString, uint32_t aLength,
                                      const uint32_t aFlags,
                                      nsTArray<nsRefPtr<nsTransformedCharStyle>>&& aStyles,
                                      bool aOwnsFactory);

  ~nsTransformedTextRun() {
    if (mOwnsFactory) {
      delete mFactory;
    }
  }
  
  void SetCapitalization(uint32_t aStart, uint32_t aLength,
                         bool* aCapitalization,
                         gfxContext* aRefContext);
  virtual bool SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                        uint8_t* aBreakBefore,
                                        gfxContext* aRefContext);
  




  void FinishSettingProperties(gfxContext* aRefContext,
                               gfxMissingFontRecorder* aMFR)
  {
    if (mNeedsRebuild) {
      mNeedsRebuild = false;
      mFactory->RebuildTextRun(this, aRefContext, aMFR);
    }
  }

  
  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) MOZ_MUST_OVERRIDE;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) MOZ_MUST_OVERRIDE;

  nsTransformingTextRunFactory       *mFactory;
  nsTArray<nsRefPtr<nsTransformedCharStyle>> mStyles;
  nsTArray<bool>                      mCapitalize;
  nsString                            mString;
  bool                                mOwnsFactory;
  bool                                mNeedsRebuild;

private:
  nsTransformedTextRun(const gfxTextRunFactory::Parameters* aParams,
                       nsTransformingTextRunFactory* aFactory,
                       gfxFontGroup* aFontGroup,
                       const char16_t* aString, uint32_t aLength,
                       const uint32_t aFlags,
                       nsTArray<nsRefPtr<nsTransformedCharStyle>>&& aStyles,
                       bool aOwnsFactory)
    : gfxTextRun(aParams, aLength, aFontGroup, aFlags),
      mFactory(aFactory), mStyles(aStyles), mString(aString, aLength),
      mOwnsFactory(aOwnsFactory), mNeedsRebuild(true)
  {
    mCharacterGlyphs = reinterpret_cast<CompressedGlyph*>(this + 1);
  }
};































void
MergeCharactersInTextRun(gfxTextRun* aDest, gfxTextRun* aSrc,
                         const bool* aCharsToMerge, const bool* aDeletedChars);

gfxTextRunFactory::Parameters
GetParametersForInner(nsTransformedTextRun* aTextRun, uint32_t* aFlags,
                      gfxContext* aRefContext);


#endif 
