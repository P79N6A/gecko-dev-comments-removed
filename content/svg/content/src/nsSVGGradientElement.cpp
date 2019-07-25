





































#include "DOMSVGAnimatedTransformList.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsIDOMMutationEvent.h"
#include "nsCOMPtr.h"
#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsSVGGradientElement.h"
#include "nsIFrame.h"

using namespace mozilla;



nsSVGEnumMapping nsSVGGradientElement::sSpreadMethodMap[] = {
  {&nsGkAtoms::pad, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD},
  {&nsGkAtoms::reflect, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REFLECT},
  {&nsGkAtoms::repeat, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REPEAT},
  {nsnull, 0}
};

nsSVGElement::EnumInfo nsSVGGradientElement::sEnumInfo[2] =
{
  { &nsGkAtoms::gradientUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::spreadMethod,
    sSpreadMethodMap,
    nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD
  }
};

nsSVGElement::StringInfo nsSVGGradientElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_TRUE }
};




NS_IMPL_ADDREF_INHERITED(nsSVGGradientElement,nsSVGGradientElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGradientElement,nsSVGGradientElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGUnitTypes)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGradientElementBase)




nsSVGGradientElement::nsSVGGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGGradientElementBase(aNodeInfo)
{
}




nsSVGElement::EnumAttributesInfo
nsSVGGradientElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGGradientElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}





NS_IMETHODIMP nsSVGGradientElement::GetGradientUnits(nsIDOMSVGAnimatedEnumeration * *aGradientUnits)
{
  return mEnumAttributes[GRADIENTUNITS].ToDOMAnimatedEnum(aGradientUnits, this);
}


NS_IMETHODIMP nsSVGGradientElement::GetGradientTransform(nsIDOMSVGAnimatedTransformList * *aGradientTransform)
{
  *aGradientTransform =
    DOMSVGAnimatedTransformList::GetDOMWrapper(GetAnimatedTransformList(), this)
    .get();
  return NS_OK;
}


NS_IMETHODIMP nsSVGGradientElement::GetSpreadMethod(nsIDOMSVGAnimatedEnumeration * *aSpreadMethod)
{
  return mEnumAttributes[SPREADMETHOD].ToDOMAnimatedEnum(aSpreadMethod, this);
}





NS_IMETHODIMP
nsSVGGradientElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




NS_IMETHODIMP_(bool)
nsSVGGradientElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sGradientStopMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGGradientElementBase::IsAttributeMapped(name);
}



nsSVGElement::LengthInfo nsSVGLinearGradientElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y1, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::x2, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y2, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(LinearGradient)




NS_IMPL_ADDREF_INHERITED(nsSVGLinearGradientElement,nsSVGLinearGradientElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGLinearGradientElement,nsSVGLinearGradientElementBase)

DOMCI_NODE_DATA(SVGLinearGradientElement, nsSVGLinearGradientElement)

NS_INTERFACE_TABLE_HEAD(nsSVGLinearGradientElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGLinearGradientElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGGradientElement,
                           nsIDOMSVGLinearGradientElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGLinearGradientElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGLinearGradientElementBase)




nsSVGLinearGradientElement::nsSVGLinearGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGLinearGradientElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGLinearGradientElement)





NS_IMETHODIMP nsSVGLinearGradientElement::GetX1(nsIDOMSVGAnimatedLength * *aX1)
{
  return mLengthAttributes[X1].ToDOMAnimatedLength(aX1, this);
}


NS_IMETHODIMP nsSVGLinearGradientElement::GetY1(nsIDOMSVGAnimatedLength * *aY1)
{
  return mLengthAttributes[Y1].ToDOMAnimatedLength(aY1, this);
}


NS_IMETHODIMP nsSVGLinearGradientElement::GetX2(nsIDOMSVGAnimatedLength * *aX2)
{
  return mLengthAttributes[X2].ToDOMAnimatedLength(aX2, this);
}


NS_IMETHODIMP nsSVGLinearGradientElement::GetY2(nsIDOMSVGAnimatedLength * *aY2)
{
  return mLengthAttributes[Y2].ToDOMAnimatedLength(aY2, this);
}





SVGAnimatedTransformList*
nsSVGGradientElement::GetAnimatedTransformList()
{
  if (!mGradientTransform) {
    mGradientTransform = new SVGAnimatedTransformList();
  }
  return mGradientTransform;
}

nsSVGElement::LengthAttributesInfo
nsSVGLinearGradientElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}



nsSVGElement::LengthInfo nsSVGRadialGradientElement::sLengthInfo[5] =
{
  { &nsGkAtoms::cx, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::cy, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::r, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::XY },
  { &nsGkAtoms::fx, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::fy, 50, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(RadialGradient)




NS_IMPL_ADDREF_INHERITED(nsSVGRadialGradientElement,nsSVGRadialGradientElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGRadialGradientElement,nsSVGRadialGradientElementBase)

DOMCI_NODE_DATA(SVGRadialGradientElement, nsSVGRadialGradientElement)

NS_INTERFACE_TABLE_HEAD(nsSVGRadialGradientElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGRadialGradientElement, nsIDOMNode,
                           nsIDOMElement, nsIDOMSVGElement,
                           nsIDOMSVGGradientElement,
                           nsIDOMSVGRadialGradientElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGRadialGradientElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGRadialGradientElementBase)




nsSVGRadialGradientElement::nsSVGRadialGradientElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGRadialGradientElementBase(aNodeInfo)
{
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGRadialGradientElement)





NS_IMETHODIMP nsSVGRadialGradientElement::GetCx(nsIDOMSVGAnimatedLength * *aCx)
{
  return mLengthAttributes[CX].ToDOMAnimatedLength(aCx, this);
}


NS_IMETHODIMP nsSVGRadialGradientElement::GetCy(nsIDOMSVGAnimatedLength * *aCy)
{
  return mLengthAttributes[CY].ToDOMAnimatedLength(aCy, this);
}


NS_IMETHODIMP nsSVGRadialGradientElement::GetR(nsIDOMSVGAnimatedLength * *aR)
{
  return mLengthAttributes[R].ToDOMAnimatedLength(aR, this);
}


NS_IMETHODIMP nsSVGRadialGradientElement::GetFx(nsIDOMSVGAnimatedLength * *aFx)
{
  return mLengthAttributes[FX].ToDOMAnimatedLength(aFx, this);
}


NS_IMETHODIMP nsSVGRadialGradientElement::GetFy(nsIDOMSVGAnimatedLength * *aFy)
{
  return mLengthAttributes[FY].ToDOMAnimatedLength(aFy, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGRadialGradientElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}
