





































#include "nsSVGTransformList.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGEnum.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsIDOMSVGURIReference.h"
#include "nsIDOMSVGGradientElement.h"
#include "nsSVGAnimatedString.h"
#include "nsCOMPtr.h"
#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsSVGGradientElement.h"






NS_IMPL_ADDREF_INHERITED(nsSVGGradientElement,nsSVGGradientElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGradientElement,nsSVGGradientElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGradientElementBase)




nsSVGGradientElement::nsSVGGradientElement(nsINodeInfo* aNodeInfo)
  : nsSVGGradientElementBase(aNodeInfo)
{
}

nsresult
nsSVGGradientElement::Init()
{
  nsresult rv = nsSVGGradientElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping gUnitMap[] = {
        {&nsGkAtoms::objectBoundingBox, nsIDOMSVGGradientElement::SVG_GRUNITS_OBJECTBOUNDINGBOX},
        {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGGradientElement::SVG_GRUNITS_USERSPACEONUSE},
        {nsnull, 0}
  };

  static struct nsSVGEnumMapping gSpreadMap[] = {
        {&nsGkAtoms::pad, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD},
        {&nsGkAtoms::reflect, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REFLECT},
        {&nsGkAtoms::repeat, nsIDOMSVGGradientElement::SVG_SPREADMETHOD_REPEAT},
        {nsnull, 0}
  };

  

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGGradientElement::SVG_GRUNITS_OBJECTBOUNDINGBOX, gUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mGradientUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::gradientUnits, mGradientUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGTransformList> transformList;
    rv = nsSVGTransformList::Create(getter_AddRefs(transformList));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedTransformList(getter_AddRefs(mGradientTransform),
                                        transformList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::gradientTransform, mGradientTransform);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsISVGEnum> spread;
    rv = NS_NewSVGEnum(getter_AddRefs(spread), 
                       nsIDOMSVGGradientElement::SVG_SPREADMETHOD_PAD, gSpreadMap );
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mSpreadMethod), spread);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::spreadMethod, mSpreadMethod);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  

  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}





NS_IMETHODIMP nsSVGGradientElement::GetGradientUnits(nsIDOMSVGAnimatedEnumeration * *aGradientUnits)
{
  *aGradientUnits = mGradientUnits;
  NS_IF_ADDREF(*aGradientUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGGradientElement::GetGradientTransform(nsIDOMSVGAnimatedTransformList * *aGradientTransform)
{
  *aGradientTransform = mGradientTransform;
  NS_IF_ADDREF(*aGradientTransform);
  return NS_OK;
}


NS_IMETHODIMP nsSVGGradientElement::GetSpreadMethod(nsIDOMSVGAnimatedEnumeration * *aSpreadMethod)
{
  *aSpreadMethod = mSpreadMethod;
  NS_IF_ADDREF(*aSpreadMethod);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGGradientElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
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

NS_INTERFACE_MAP_BEGIN(nsSVGLinearGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGLinearGradientElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGLinearGradientElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGLinearGradientElementBase)




nsSVGLinearGradientElement::nsSVGLinearGradientElement(nsINodeInfo* aNodeInfo)
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

NS_INTERFACE_MAP_BEGIN(nsSVGRadialGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGGradientElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRadialGradientElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGRadialGradientElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGRadialGradientElementBase)




nsSVGRadialGradientElement::nsSVGRadialGradientElement(nsINodeInfo* aNodeInfo)
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

