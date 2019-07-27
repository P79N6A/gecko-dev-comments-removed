




#ifndef MATHMLTEXTRUNFACTORY_H_
#define MATHMLTEXTRUNFACTORY_H_

#include "nsTextRunTransformations.h"




class MathMLTextRunFactory : public nsTransformingTextRunFactory {
public:
  MathMLTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                       uint32_t aFlags, uint8_t aSSTYScriptLevel)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mFlags(aFlags),
      mSSTYScriptLevel(aSSTYScriptLevel) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext) MOZ_OVERRIDE;
  enum {
    
    MATH_FONT_STYLING_NORMAL   = 0x1, 
    MATH_FONT_WEIGHT_BOLD      = 0x2, 
  };

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  uint32_t mFlags;
  uint8_t mSSTYScriptLevel;
};

#endif 
