



































#ifndef __NS_SVGFILTERELEMENT_H__
#define __NS_SVGFILTERELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGFilterElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGLength2.h"

typedef nsSVGGraphicElement nsSVGFilterElementBase;

class nsSVGFilterElement : public nsSVGFilterElementBase,
                           public nsIDOMSVGFilterElement,
                           public nsIDOMSVGURIReference
{
  friend class nsSVGFilterFrame;

protected:
  friend nsresult NS_NewSVGFilterElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGFilterElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

  
  NS_IMETHOD SetValueString(const nsAString &aValue) { return NS_OK; }
  NS_IMETHOD GetValueString(nsAString& aValue) { return NS_ERROR_NOT_IMPLEMENTED; }

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE(nsSVGFilterElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGFilterElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFilterElementBase::)

  
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();
  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mFilterUnits;
  nsCOMPtr<nsIDOMSVGAnimatedEnumeration> mPrimitiveUnits;
  nsCOMPtr<nsIDOMSVGAnimatedInteger> mFilterResX;
  nsCOMPtr<nsIDOMSVGAnimatedInteger> mFilterResY;
  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
};

#endif
