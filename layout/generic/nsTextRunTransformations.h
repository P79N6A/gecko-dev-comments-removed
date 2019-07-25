




#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "gfxFont.h"

class nsTransformedTextRun;
class nsStyleContext;

class nsTransformingTextRunFactory {
public:
  virtual ~nsTransformingTextRunFactory() {}

  
  nsTransformedTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                    nsStyleContext** aStyles, bool aOwnsFactory = true);
  nsTransformedTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                    nsStyleContext** aStyles, bool aOwnsFactory = true);

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext) = 0;
};





class nsFontVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext);
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  
  
  
  
  
  
  
  nsCaseTransformTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                bool aAllUppercase = false)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext);

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  bool                                    mAllUppercase;
};





class nsTransformedTextRun : public gfxTextRun {
public:
  static nsTransformedTextRun *Create(const gfxTextRunFactory::Parameters* aParams,
                                      nsTransformingTextRunFactory* aFactory,
                                      gfxFontGroup* aFontGroup,
                                      const PRUnichar* aString, PRUint32 aLength,
                                      const PRUint32 aFlags, nsStyleContext** aStyles,
                                      bool aOwnsFactory);

  ~nsTransformedTextRun() {
    if (mOwnsFactory) {
      delete mFactory;
    }
  }
  
  void SetCapitalization(PRUint32 aStart, PRUint32 aLength,
                         bool* aCapitalization,
                         gfxContext* aRefContext);
  virtual bool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                        PRUint8* aBreakBefore,
                                        gfxContext* aRefContext);
  




  void FinishSettingProperties(gfxContext* aRefContext)
  {
    if (mNeedsRebuild) {
      mNeedsRebuild = false;
      mFactory->RebuildTextRun(this, aRefContext);
    }
  }

  
  virtual NS_MUST_OVERRIDE size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf);
  virtual NS_MUST_OVERRIDE size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf);

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
                       const PRUnichar* aString, PRUint32 aLength,
                       const PRUint32 aFlags, nsStyleContext** aStyles,
                       bool aOwnsFactory)
    : gfxTextRun(aParams, aLength, aFontGroup, aFlags),
      mFactory(aFactory), mString(aString, aLength),
      mOwnsFactory(aOwnsFactory), mNeedsRebuild(true)
  {
    mCharacterGlyphs = reinterpret_cast<CompressedGlyph*>(this + 1);

    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      mStyles.AppendElement(aStyles[i]);
    }
  }
};

#endif 
