




































#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "gfxFont.h"

class nsTransformedTextRun;
class nsStyleContext;

class nsTransformingTextRunFactory {
public:
  virtual ~nsTransformingTextRunFactory() {}

  
  gfxTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                          gfxFontGroup::Parameters* aParams, nsStyleContext** aStyles);
  gfxTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                          gfxFontGroup::Parameters* aParams, nsStyleContext** aStyles);

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun) = 0;
};





class nsFontVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  nsFontVariantTextRunFactory(gfxFontGroup* aFontGroup)
    : mFontGroup(aFontGroup) {}
    
  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun);

protected:
  nsRefPtr<gfxFontGroup> mFontGroup;
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  
  
  
  
  
  
  nsCaseTransformTextRunFactory(gfxFontGroup* aFontGroup,
                                nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                PRBool aAllUppercase = PR_FALSE)
    : mFontGroup(aFontGroup),
      mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun);

protected:
  nsRefPtr<gfxFontGroup>                  mFontGroup;
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  PRPackedBool                            mAllUppercase;
};

#endif 
