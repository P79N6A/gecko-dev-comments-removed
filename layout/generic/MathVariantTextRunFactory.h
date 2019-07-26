




#ifndef MATHVARIANTTEXTRUNFACTORY_H_
#define MATHVARIANTTEXTRUNFACTORY_H_

#include "nsTextRunTransformations.h"




class nsMathVariantTextRunFactory : public nsTransformingTextRunFactory {
public:
  nsMathVariantTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext) MOZ_OVERRIDE;
protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
};

#endif 
