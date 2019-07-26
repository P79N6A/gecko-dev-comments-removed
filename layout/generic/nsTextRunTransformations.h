




#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "mozilla/Attributes.h"
#include "gfxFont.h"

class nsTransformedTextRun;
class nsStyleContext;

class nsTransformingTextRunFactory {
public:
  virtual ~nsTransformingTextRunFactory() {}

  
  nsTransformedTextRun* MakeTextRun(const uint8_t* aString, uint32_t aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, uint32_t aFlags,
                                    nsStyleContext** aStyles, bool aOwnsFactory = true);
  nsTransformedTextRun* MakeTextRun(const PRUnichar* aString, uint32_t aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, uint32_t aFlags,
                                    nsStyleContext** aStyles, bool aOwnsFactory = true);

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext) = 0;
};





class nsFontVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext) MOZ_OVERRIDE;
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  
  
  
  
  
  
  
  nsCaseTransformTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                bool aAllUppercase = false)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext) MOZ_OVERRIDE;

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  bool                                    mAllUppercase;
};





class nsTransformedTextRun : public gfxTextRun {
public:
  static nsTransformedTextRun *Create(const gfxTextRunFactory::Parameters* aParams,
                                      nsTransformingTextRunFactory* aFactory,
                                      gfxFontGroup* aFontGroup,
                                      const PRUnichar* aString, uint32_t aLength,
                                      const uint32_t aFlags, nsStyleContext** aStyles,
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
  




  void FinishSettingProperties(gfxContext* aRefContext)
  {
    if (mNeedsRebuild) {
      mNeedsRebuild = false;
      mFactory->RebuildTextRun(this, aRefContext);
    }
  }

  
  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) MOZ_MUST_OVERRIDE;
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) MOZ_MUST_OVERRIDE;

  nsTransformingTextRunFactory       *mFactory;
  nsTArray<nsRefPtr<nsStyleContext> > mStyles;
  nsTArray<bool>                      mCapitalize;
  nsString                            mString;
  bool                                mOwnsFactory;
  bool                                mNeedsRebuild;

private:
  nsTransformedTextRun(const gfxTextRunFactory::Parameters* aParams,
                       nsTransformingTextRunFactory* aFactory,
                       gfxFontGroup* aFontGroup,
                       const PRUnichar* aString, uint32_t aLength,
                       const uint32_t aFlags, nsStyleContext** aStyles,
                       bool aOwnsFactory)
    : gfxTextRun(aParams, aLength, aFontGroup, aFlags),
      mFactory(aFactory), mString(aString, aLength),
      mOwnsFactory(aOwnsFactory), mNeedsRebuild(true)
  {
    mCharacterGlyphs = reinterpret_cast<CompressedGlyph*>(this + 1);

    uint32_t i;
    for (i = 0; i < aLength; ++i) {
      mStyles.AppendElement(aStyles[i]);
    }
  }
};

#endif 
