





#ifndef MOZILLA_SVGTRANSFORMLISTPARSER_H__
#define MOZILLA_SVGTRANSFORMLISTPARSER_H__

#include "nsSVGDataParser.h"
#include "nsTArray.h"








class nsIAtom;

namespace mozilla {

class nsSVGTransform;

class SVGTransformListParser : public nsSVGDataParser
{
public:
  const nsTArray<nsSVGTransform>& GetTransformList() const {
    return mTransforms;
  }

private:
  nsTArray<nsSVGTransform> mTransforms;

  
  virtual nsresult Match();

  nsresult MatchNumberArguments(float *aResult,
                                uint32_t aMaxNum,
                                uint32_t *aParsedNum);

  nsresult MatchTransformList();

  nsresult GetTransformToken(nsIAtom** aKeyatom, bool aAdvancePos);
  nsresult MatchTransforms();

  nsresult MatchTransform();

  bool IsTokenTransformStarter();

  nsresult MatchTranslate();

  nsresult MatchScale();
  nsresult MatchRotate();
  nsresult MatchSkewX();
  nsresult MatchSkewY();
  nsresult MatchMatrix();
};

} 

#endif 
