



































#ifndef __NS_SVGMASKELEMENT_H__
#define __NS_SVGMASKELEMENT_H__

#include "DOMSVGTests.h"
#include "nsIDOMSVGMaskElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGEnum.h"
#include "nsSVGLength2.h"
#include "nsSVGStylableElement.h"



typedef nsSVGStylableElement nsSVGMaskElementBase;

class nsSVGMaskElement : public nsSVGMaskElementBase,
                         public nsIDOMSVGMaskElement,
                         public DOMSVGTests,
                         public nsIDOMSVGUnitTypes
{
  friend class nsSVGMaskFrame;

protected:
  friend nsresult NS_NewSVGMaskElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGMaskElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMSVGMASKELEMENT

  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual bool HasValidDimensions() const;
protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual EnumAttributesInfo GetEnumInfo();

  
  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { MASKUNITS, MASKCONTENTUNITS };
  nsSVGEnum mEnumAttributes[2];
  static EnumInfo sEnumInfo[2];
};

#endif
