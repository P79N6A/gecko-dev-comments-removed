




#ifndef MATHMLTEXTRUNFACTORY_H_
#define MATHMLTEXTRUNFACTORY_H_

#include "nsTextRunTransformations.h"




class MathMLTextRunFactory : public nsTransformingTextRunFactory {
public:
  MathMLTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                       uint8_t aSSTYScriptLevel)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mSSTYScriptLevel(aSSTYScriptLevel) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext) MOZ_OVERRIDE;
protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  uint8_t mSSTYScriptLevel;
};

#endif 
