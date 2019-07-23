



































#include "nsGkAtoms.h"
#include "nsSVGLength.h"
#include "nsCOMPtr.h"
#include "nsSVGAnimatedInteger.h"
#include "nsSVGAnimatedString.h"
#include "nsSVGFilterElement.h"

nsSVGElement::LengthInfo nsSVGFilterElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGElement::EnumInfo nsSVGFilterElement::sEnumInfo[2] =
{
  { &nsGkAtoms::filterUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_OBJECTBOUNDINGBOX
  },
  { &nsGkAtoms::primitiveUnits,
    sSVGUnitTypesMap,
    nsIDOMSVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE
  }
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
  return mEnumAttributes[FILTERUNITS].ToDOMAnimatedEnum(aUnits, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetPrimitiveUnits(nsIDOMSVGAnimatedEnumeration * *aUnits)
{
  return mEnumAttributes[PRIMITIVEUNITS].ToDOMAnimatedEnum(aUnits, this);
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

nsresult
nsSVGFilterElement::UnsetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                              PRBool aNotify)
{
  if (aName == nsGkAtoms::filterRes && aNamespaceID == kNameSpaceID_None) {
    mFilterResX->SetBaseVal(0);
    mFilterResY->SetBaseVal(0);

    return nsGenericElement::UnsetAttr(aNamespaceID, aName, aNotify);
  }

  return nsSVGFilterElementBase::UnsetAttr(aNamespaceID, aName, aNotify);
}

NS_IMETHODIMP_(PRBool)
nsSVGFilterElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
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

nsSVGElement::EnumAttributesInfo
nsSVGFilterElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}
