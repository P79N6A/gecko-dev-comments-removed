



































#include "nsGkAtoms.h"
#include "nsSVGLength.h"
#include "nsCOMPtr.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsSVGAnimatedInteger.h"
#include "nsSVGAnimatedString.h"
#include "nsSVGEnum.h"
#include "nsSVGFilterElement.h"

nsSVGElement::LengthInfo nsSVGFilterElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Filter)




NS_IMPL_ADDREF_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGFilterElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFilterElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGFilterElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFilterElementBase)




nsSVGFilterElement::nsSVGFilterElement(nsINodeInfo *aNodeInfo)
  : nsSVGFilterElementBase(aNodeInfo)
{
}

nsresult
nsSVGFilterElement::Init()
{
  nsresult rv = nsSVGFilterElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping gUnitMap[] = {
        {&nsGkAtoms::objectBoundingBox, nsIDOMSVGFilterElement::SVG_FUNITS_OBJECTBOUNDINGBOX},
        {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGFilterElement::SVG_FUNITS_USERSPACEONUSE},
        {nsnull, 0}
  };

  

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGFilterElement::SVG_FUNITS_OBJECTBOUNDINGBOX, gUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mFilterUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::filterUnits, mFilterUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGFilterElement::SVG_FUNITS_USERSPACEONUSE, gUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mPrimitiveUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::primitiveUnits, mPrimitiveUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    rv = NS_NewSVGAnimatedInteger(getter_AddRefs(mFilterResX), 0);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    rv = NS_NewSVGAnimatedInteger(getter_AddRefs(mFilterResY), 0);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGFilterElement)






NS_IMETHODIMP nsSVGFilterElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetFilterUnits(nsIDOMSVGAnimatedEnumeration * *aUnits)
{
  *aUnits = mFilterUnits;
  NS_IF_ADDREF(*aUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGFilterElement::GetPrimitiveUnits(nsIDOMSVGAnimatedEnumeration * *aUnits)
{
  *aUnits = mPrimitiveUnits;
  NS_IF_ADDREF(*aUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGFilterElement::GetFilterResX(nsIDOMSVGAnimatedInteger * *aFilterResX)
{
  *aFilterResX = mFilterResX;
  NS_IF_ADDREF(*aFilterResX);
  return NS_OK;
}


NS_IMETHODIMP nsSVGFilterElement::GetFilterResY(nsIDOMSVGAnimatedInteger * *aFilterResY)
{
  *aFilterResY = mFilterResY;
  NS_IF_ADDREF(*aFilterResY);
  return NS_OK;
}



NS_IMETHODIMP
nsSVGFilterElement::SetFilterRes(PRUint32 filterResX, PRUint32 filterResY)
{
  mFilterResX->SetBaseVal(filterResX);
  mFilterResY->SetBaseVal(filterResY);
  return NS_OK;
}





NS_IMETHODIMP 
nsSVGFilterElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




nsresult
nsSVGFilterElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                            nsIAtom* aPrefix, const nsAString& aValue,
                            PRBool aNotify)
{
  nsresult rv = nsSVGFilterElementBase::SetAttr(aNameSpaceID, aName, aPrefix,
                                                aValue, aNotify);

  if (aName == nsGkAtoms::filterRes && aNameSpaceID == kNameSpaceID_None) {
    PRUint32 resX, resY;
    char *str;
    str = ToNewCString(aValue);
    int num = sscanf(str, "%d %d\n", &resX, &resY);
    switch (num) {
    case 2:
      mFilterResX->SetBaseVal(resX);
      mFilterResY->SetBaseVal(resY);
      break;
    case 1:
      mFilterResX->SetBaseVal(resX);
      mFilterResY->SetBaseVal(resX);
      break;
    default:
      break;
    }
    nsMemory::Free(str);
  }

  return rv;
}

NS_IMETHODIMP_(PRBool)
nsSVGFilterElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGGraphicElementBase::IsAttributeMapped(name);
}




nsSVGElement::LengthAttributesInfo
nsSVGFilterElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}
