



































#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsSVGFilterElement.h"

nsSVGElement::LengthInfo nsSVGFilterElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGElement::IntegerInfo nsSVGFilterElement::sIntegerInfo[2] =
{
  { &nsGkAtoms::filterRes, 0 },
  { &nsGkAtoms::filterRes, 0 }
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

nsSVGElement::StringInfo nsSVGFilterElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Filter)




NS_IMPL_ADDREF_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGFilterElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFilterElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFilterElement,
                           nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGFilterElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFilterElementBase)




nsSVGFilterElement::nsSVGFilterElement(nsINodeInfo *aNodeInfo)
  : nsSVGFilterElementBase(aNodeInfo)
{
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
  return mIntegerAttributes[FILTERRES_X].ToDOMAnimatedInteger(aFilterResX, this);
}


NS_IMETHODIMP nsSVGFilterElement::GetFilterResY(nsIDOMSVGAnimatedInteger * *aFilterResY)
{
  return mIntegerAttributes[FILTERRES_Y].ToDOMAnimatedInteger(aFilterResY, this);
}



NS_IMETHODIMP
nsSVGFilterElement::SetFilterRes(PRUint32 filterResX, PRUint32 filterResY)
{
  mIntegerAttributes[FILTERRES_X].SetBaseValue(filterResX, this, PR_FALSE);
  mIntegerAttributes[FILTERRES_Y].SetBaseValue(filterResY, this, PR_FALSE);
  return NS_OK;
}





NS_IMETHODIMP 
nsSVGFilterElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
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

void
nsSVGFilterElement::Invalidate()
{
  nsTObserverArray<nsIMutationObserver*> *observers = GetMutationObservers();

  if (observers && !observers->IsEmpty()) {
    nsTObserverArray<nsIMutationObserver*>::ForwardIterator iter(*observers);
    while (iter.HasMore()) {
      nsCOMPtr<nsIMutationObserver> obs(iter.GetNext());
      nsCOMPtr<nsISVGFilterProperty> filter = do_QueryInterface(obs);
      if (filter)
        filter->Invalidate();
    }
  }
}




nsSVGElement::LengthAttributesInfo
nsSVGFilterElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

nsSVGElement::IntegerAttributesInfo
nsSVGFilterElement::GetIntegerInfo()
{
  return IntegerAttributesInfo(mIntegerAttributes, sIntegerInfo,
                               NS_ARRAY_LENGTH(sIntegerInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGFilterElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGFilterElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}
