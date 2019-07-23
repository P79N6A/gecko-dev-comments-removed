



































#ifndef __NS_SVGMASKELEMENT_H__
#define __NS_SVGMASKELEMENT_H__

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGMaskElement.h"
#include "nsSVGLength2.h"



typedef nsSVGStylableElement nsSVGMaskElementBase;

class nsSVGMaskElement : public nsSVGMaskElementBase,
                         public nsIDOMSVGMaskElement
{
  friend class nsSVGMaskFrame;

protected:
  friend nsresult NS_NewSVGMaskElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGMaskElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGMASKELEMENT

  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mMaskUnits;
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mMaskContentUnits;
};

#endif
