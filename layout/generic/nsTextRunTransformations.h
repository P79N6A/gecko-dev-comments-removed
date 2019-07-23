




































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
                                    nsStyleContext** aStyles, PRBool aOwnsFactory = PR_TRUE);
  nsTransformedTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                    const gfxFontGroup::Parameters* aParams,
                                    gfxFontGroup* aFontGroup, PRUint32 aFlags,
                                    nsStyleContext** aStyles, PRBool aOwnsFactory = PR_TRUE);

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext) = 0;
};





class nsFontVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext);
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  
  
  
  
  
  
  
  nsCaseTransformTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                PRBool aAllUppercase = PR_FALSE)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun, gfxContext* aRefContext);

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  PRPackedBool                            mAllUppercase;
};





class nsTransformedTextRun : public gfxTextRun {
public:
  static nsTransformedTextRun *Create(const gfxTextRunFactory::Parameters* aParams,
                                      nsTransformingTextRunFactory* aFactory,
                                      gfxFontGroup* aFontGroup,
                                      const PRUnichar* aString, PRUint32 aLength,
                                      const PRUint32 aFlags, nsStyleContext** aStyles,
                                      PRBool aOwnsFactory);

  ~nsTransformedTextRun() {
    if (mOwnsFactory) {
      delete mFactory;
    }
  }
  
  void SetCapitalization(PRUint32 aStart, PRUint32 aLength,
                         PRPackedBool* aCapitalization,
                         gfxContext* aRefContext);
  virtual PRBool SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                        PRPackedBool* aBreakBefore,
                                        gfxContext* aRefContext);
  




  void FinishSettingProperties(gfxContext* aRefContext)
  {
    if (mNeedsRebuild) {
      mNeedsRebuild = PR_FALSE;
      mFactory->RebuildTextRun(this, aRefContext);
    }
  }

  nsTransformingTextRunFactory       *mFactory;
  nsTArray<nsRefPtr<nsStyleContext> > mStyles;
  nsTArray<PRPackedBool>              mCapitalize;
  PRPackedBool                        mOwnsFactory;
  PRPackedBool                        mNeedsRebuild;

private:
  nsTransformedTextRun(const gfxTextRunFactory::Parameters* aParams,
                       nsTransformingTextRunFactory* aFactory,
                       gfxFontGroup* aFontGroup,
                       const PRUnichar* aString, PRUint32 aLength,
                       const PRUint32 aFlags, nsStyleContext** aStyles,
                       PRBool aOwnsFactory)
    : gfxTextRun(aParams, aString, aLength, aFontGroup, aFlags, sizeof(nsTransformedTextRun)),
      mFactory(aFactory), mOwnsFactory(aOwnsFactory), mNeedsRebuild(PR_TRUE)
  {
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
      mStyles.AppendElement(aStyles[i]);
    }
  }  
};

#endif 
