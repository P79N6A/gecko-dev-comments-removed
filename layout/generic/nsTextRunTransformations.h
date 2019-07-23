




































#ifndef NSTEXTRUNTRANSFORMATIONS_H_
#define NSTEXTRUNTRANSFORMATIONS_H_

#include "gfxFont.h"

class nsCaseTransformingTextRun;
class nsStyleContext;

class nsTransformingTextRunFactory : public gfxTextRunFactory {
public:
  nsTransformingTextRunFactory(gfxTextRunFactory* aInnerTextRunFactory,
                               nsTransformingTextRunFactory* aInnerTransformingTextRunFactory)
    : mInnerTextRunFactory(aInnerTextRunFactory),
      mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mStyles(nsnull) {}

  
  
  
  virtual void SetStyles(nsStyleContext** aStyles) { mStyles = aStyles; }
  
  virtual gfxTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                  Parameters* aParams);
  
  virtual gfxTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                  Parameters* aParams) = 0;

protected:
  nsRefPtr<gfxTextRunFactory>            mInnerTextRunFactory;
  nsRefPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  nsStyleContext**                       mStyles;
};





class nsFontVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  nsFontVariantTextRunFactory(gfxFontGroup* aFontGroup)
    : nsTransformingTextRunFactory(aFontGroup, nsnull), mFontGroup(aFontGroup) {}
    
  
  virtual gfxTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                  Parameters* aParams) {
    return nsTransformingTextRunFactory::MakeTextRun(aString, aLength, aParams);
  }
  virtual gfxTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                  Parameters* aParams);

private:
  nsRefPtr<gfxFontGroup> mFontGroup;
};





class nsCaseTransformTextRunFactory : public nsTransformingTextRunFactory {
public:
  nsCaseTransformTextRunFactory(gfxTextRunFactory* aInnerTextRunFactory,
                                nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                                PRBool aAllUppercase = PR_FALSE)
    : nsTransformingTextRunFactory(aInnerTextRunFactory, aInnerTransformingTextRunFactory),
      mAllUppercase(aAllUppercase) {}

  
  virtual gfxTextRun* MakeTextRun(const PRUint8* aString, PRUint32 aLength,
                                  Parameters* aParams) {
    return nsTransformingTextRunFactory::MakeTextRun(aString, aLength, aParams);
  }
  virtual gfxTextRun* MakeTextRun(const PRUnichar* aString, PRUint32 aLength,
                                  Parameters* aParams);

  
  gfxTextRunFactory* GetInnerTextRunFactory() { return mInnerTextRunFactory; }
  nsTransformingTextRunFactory* GetInnerTransformingTextRunFactory() {
    return mInnerTransformingTextRunFactory;
  }
  nsStyleContext** GetStyles() { return mStyles; }
  PRBool IsAllUppercase() { return mAllUppercase; }

private:
  PRPackedBool mAllUppercase;
};

#endif 
