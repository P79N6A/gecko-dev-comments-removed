



































#ifndef __NS_SVGFILTERSELEMENT_H__
#define __NS_SVGFILTERSELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsSVGLength2.h"

typedef nsSVGStylableElement nsSVGFEBase;

class nsSVGFE : public nsSVGFEBase

{
  friend class nsSVGFilterInstance;

protected:
  nsSVGFE(nsINodeInfo *aNodeInfo);
  nsresult Init();

  PRBool ScanDualValueAttribute(const nsAString& aValue, nsIAtom* aAttribute,
                                nsSVGNumber2* aNum1, nsSVGNumber2* aNum2,
                                NumberInfo* aInfo1, NumberInfo* aInfo2,
                                nsAttrValue& aResult);

  nsSVGFilterInstance::ColorModel
  GetColorModel(nsSVGFilterInstance::ColorModel::AlphaChannel aAlphaChannel);

public:
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERPRIMITIVESTANDARDATTRIBUTES

protected:

  
  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsCOMPtr<nsIDOMSVGAnimatedString> mResult;
};

#endif
