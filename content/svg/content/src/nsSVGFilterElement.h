



































#ifndef __NS_SVGFILTERELEMENT_H__
#define __NS_SVGFILTERELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGFilterElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGUnitTypes.h"
#include "nsSVGLength2.h"
#include "nsSVGInteger.h"
#include "nsSVGEnum.h"
#include "nsSVGString.h"

typedef nsSVGGraphicElement nsSVGFilterElementBase;

class nsSVGFilterElement : public nsSVGFilterElementBase,
                           public nsIDOMSVGFilterElement,
                           public nsIDOMSVGURIReference,
                           public nsIDOMSVGUnitTypes
{
  friend class nsSVGFilterFrame;
  friend class nsAutoFilterInstance;

protected:
  friend nsresult NS_NewSVGFilterElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGFilterElement(nsINodeInfo* aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGFILTERELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE(nsSVGFilterElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGFilterElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGFilterElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  
  void Invalidate();

protected:

  virtual LengthAttributesInfo GetLengthInfo();
  virtual IntegerAttributesInfo GetIntegerInfo();
  virtual EnumAttributesInfo GetEnumInfo();
  virtual StringAttributesInfo GetStringInfo();

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];

  enum { FILTERRES_X, FILTERRES_Y };
  nsSVGInteger mIntegerAttributes[2];
  static IntegerInfo sIntegerInfo[2];

  enum { FILTERUNITS, PRIMITIVEUNITS };
  nsSVGEnum mEnumAttributes[2];
  static EnumInfo sEnumInfo[2];

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

#endif
