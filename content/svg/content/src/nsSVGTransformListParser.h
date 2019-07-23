




































#ifndef __NS_SVGTRANSFORMLISTPARSER_H__
#define __NS_SVGTRANSFORMLISTPARSER_H__

#include "nsSVGDataParser.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIDOMSVGTransformList.h"
#include "nsIAtom.h"








class nsSVGTransformListParser : public nsSVGDataParser
{
public:
  nsSVGTransformListParser(nsCOMArray<nsIDOMSVGTransform>* aTransforms);

private:
  nsCOMArray<nsIDOMSVGTransform> *mTransform;

  
  nsresult Match();

  nsresult MatchNumberArguments(float *aResult,
                                PRUint32 aMaxNum,
                                PRUint32 *aParsedNum);
  nsIDOMSVGTransform *AppendTransform();

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
