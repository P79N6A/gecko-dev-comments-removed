




#ifndef __NS_SVGFILTERELEMENT_H__
#define __NS_SVGFILTERELEMENT_H__

#include "nsIDOMSVGFilterElement.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsIDOMSVGURIReference.h"
#include "nsSVGEnum.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGIntegerPair.h"
#include "nsSVGLength2.h"
#include "nsSVGString.h"

typedef nsSVGElement nsSVGFilterElementBase;

class nsSVGFilterElement : public nsSVGFilterElementBase,
                           public nsIDOMSVGFilterElement,
                           public nsIDOMSVGURIReference,
                           public nsIDOMSVGUnitTypes
{
  friend class nsSVGFilterFrame;
  friend class nsAutoFilterInstance;

protected:
  friend nsresult NS_NewSVGFilterElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGFilterElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFilterElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  
  void Invalidate();

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual bool HasValidDimensions() const;
protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual IntegerPairAttributesInfo GetIntegerPairInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { FILTERRES };
  nsSVGIntegerPair mIntegerPairAttributes[1];
  static IntegerPairInfo sIntegerPairInfo[1];

  enum { FILTERUNITS, PRIMITIVEUNITS };
  nsSVGEnum mEnumAttributes[2];
  static EnumInfo sEnumInfo[2];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

#endif
