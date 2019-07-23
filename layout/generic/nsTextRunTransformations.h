




































#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "gfxFont.h"

class nsTransformedTextRun;
class nsStyleContext;

class nsTransformingTextRunFactory {
public:
  virtual ~nsTransformingTextRunFactory() {}

  
  gfxTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                          const gfxFontGroup::Parameters* aParams,
                          gfxFontGroup* aFontGroup, PRUint32 aFlags,
                          nsStyleContext** aStyles, PRBool aOwnsFactory = PR_TRUE);
  gfxTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
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

#endif 
