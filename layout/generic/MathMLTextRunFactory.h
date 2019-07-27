




#ifndef MATHMLTEXTRUNFACTORY_H_
#define MATHMLTEXTRUNFACTORY_H_

#include "nsTextRunTransformations.h"




class MathMLTextRunFactory : public nsTransformingTextRunFactory {
public:
  MathMLTextRunFactory(nsTransformingTextRunFactory* aInnerTransformingTextRunFactory,
                       uint32_t aFlags, uint8_t aSSTYScriptLevel,
                       float aFontInflation)
    : mInnerTransformingTextRunFactory(aInnerTransformingTextRunFactory),
      mFlags(aFlags),
      mFontInflation(aFontInflation),
      mSSTYScriptLevel(aSSTYScriptLevel) {}

  virtual void RebuildTextRun(nsTransformedTextRun* aTextRun,
                              gfxContext* aRefContext,
                              gfxMissingFontRecorder* aMFR) MOZ_OVERRIDE;
  enum {
    
    MATH_FONT_STYLING_NORMAL   = 0x1, 
    MATH_FONT_WEIGHT_BOLD      = 0x2, 
    MATH_FONT_FEATURE_DTLS     = 0x4, 
  };

protected:
  nsAutoPtr<nsTransformingTextRunFactory> mInnerTransformingTextRunFactory;
  uint32_t mFlags;
  float mFontInflation;
  uint8_t mSSTYScriptLevel;
};

#endif 
