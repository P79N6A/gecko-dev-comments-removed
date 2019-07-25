




































#ifndef __NS_SVGTRANSFORMLISTPARSER_H__
#define __NS_SVGTRANSFORMLISTPARSER_H__

#include "nsSVGDataParser.h"
#include "nsTArray.h"








class nsIAtom;
namespace mozilla { class SVGTransform; }

class nsSVGTransformListParser : public nsSVGDataParser
{
public:
  const nsTArray<mozilla::SVGTransform>& GetTransformList() const {
    return mTransforms;
  }

private:
  nsTArray<mozilla::SVGTransform> mTransforms;

  
  virtual nsresult Match();

  nsresult MatchNumberArguments(float *aResult,
                                PRUint32 aMaxNum,
                                PRUint32 *aParsedNum);

  nsresult MatchTransformList();

  nsresult GetTransformToken(nsIAtom** aKeyatom, PRBool aAdvancePos);
  nsresult MatchTransforms();

  nsresult MatchTransform();

  PRBool IsTokenTransformStarter();

  nsresult MatchTranslate();

  nsresult MatchScale();
  nsresult MatchRotate();
  nsresult MatchSkewX();
  nsresult MatchSkewY();
  nsresult MatchMatrix();
};

#endif 
