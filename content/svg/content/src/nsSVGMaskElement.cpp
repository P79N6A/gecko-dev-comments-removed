



































#include "nsSVGEnum.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsSVGMaskElement.h"



nsSVGElement::LengthInfo nsSVGMaskElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Mask)




NS_IMPL_ADDREF_INHERITED(nsSVGMaskElement,nsSVGMaskElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMaskElement,nsSVGMaskElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGMaskElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGMaskElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGMaskElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMaskElementBase)




nsSVGMaskElement::nsSVGMaskElement(nsINodeInfo* aNodeInfo)
  : nsSVGMaskElementBase(aNodeInfo)
{
}

nsresult
nsSVGMaskElement::Init()
{
  nsresult rv = nsSVGMaskElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping pUnitMap[] = {
        {&nsGkAtoms::objectBoundingBox, nsIDOMSVGMaskElement::SVG_MUNITS_OBJECTBOUNDINGBOX},
        {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGMaskElement::SVG_MUNITS_USERSPACEONUSE},
        {nsnull, 0}
  };

  

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGMaskElement::SVG_MUNITS_OBJECTBOUNDINGBOX, pUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mMaskUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::maskUnits, mMaskUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGMaskElement::SVG_MUNITS_USERSPACEONUSE, pUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mMaskContentUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::maskContentUnits, mMaskContentUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMaskElement)





NS_IMETHODIMP nsSVGMaskElement::GetMaskUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  *aMaskUnits = mMaskUnits;
  NS_IF_ADDREF(*aMaskUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGMaskElement::GetMaskContentUnits(nsIDOMSVGAnimatedEnumeration * *aMaskUnits)
{
  *aMaskUnits = mMaskContentUnits;
  NS_IF_ADDREF(*aMaskUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGMaskElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGMaskElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}




nsSVGElement::LengthAttributesInfo
nsSVGMaskElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}




NS_IMETHODIMP_(PRBool)
nsSVGMaskElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGMaskElementBase::IsAttributeMapped(name);
}
