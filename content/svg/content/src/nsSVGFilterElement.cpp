



































#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsSVGFilterElement.h"
#include "nsSVGEffects.h"

nsSVGElement::LengthInfo nsSVGFilterElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

nsSVGElement::IntegerPairInfo nsSVGFilterElement::sIntegerPairInfo[1] =
{
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
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_TRUE }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Filter)




NS_IMPL_ADDREF_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGFilterElement,nsSVGFilterElementBase)

DOMCI_NODE_DATA(SVGFilterElement, nsSVGFilterElement)

NS_INTERFACE_TABLE_HEAD(nsSVGFilterElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGFilterElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGFilterElement,
                           nsIDOMSVGURIReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFilterElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGFilterElementBase)




nsSVGFilterElement::nsSVGFilterElement(already_AddRefed<nsINodeInfo> aNodeInfo)
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
  return mIntegerPairAttributes[FILTERRES].ToDOMAnimatedInteger(aFilterResX,
                                                                nsSVGIntegerPair::eFirst,
                                                                this);
}


NS_IMETHODIMP nsSVGFilterElement::GetFilterResY(nsIDOMSVGAnimatedInteger * *aFilterResY)
{
  return mIntegerPairAttributes[FILTERRES].ToDOMAnimatedInteger(aFilterResY,
                                                                nsSVGIntegerPair::eSecond,
                                                                this);
}



NS_IMETHODIMP
nsSVGFilterElement::SetFilterRes(PRUint32 filterResX, PRUint32 filterResY)
{
  mIntegerPairAttributes[FILTERRES].SetBaseValues(filterResX, filterResY, this, PR_FALSE);
  return NS_OK;
}





NS_IMETHODIMP 
nsSVGFilterElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




NS_IMETHODIMP_(bool)
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

nsSVGElement::IntegerPairAttributesInfo
nsSVGFilterElement::GetIntegerPairInfo()
{
  return IntegerPairAttributesInfo(mIntegerPairAttributes, sIntegerPairInfo,
                                   NS_ARRAY_LENGTH(sIntegerPairInfo));
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

inline static void DidAnimateAttr(nsSVGFilterElement *aFilterElement)
{
  
  nsIFrame* frame = aFilterElement->GetPrimaryFrame();
  if (frame) {
    nsSVGEffects::InvalidateRenderingObservers(frame);
  }
}

void
nsSVGFilterElement::DidAnimateLength(PRUint8 aAttrEnum)
{
  DidAnimateAttr(this);
}

void
nsSVGFilterElement::DidAnimateIntegerPair(PRUint8 aAttrEnum)
{
  DidAnimateAttr(this);
}

void
nsSVGFilterElement::DidAnimateEnum(PRUint8 aAttrEnum)
{
  DidAnimateAttr(this);
}

void
nsSVGFilterElement::DidAnimateString(PRUint8 aAttrEnum)
{
  DidAnimateAttr(this);
}
