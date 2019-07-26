




#include "mozilla/Util.h"

#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/SVGFilterElement.h"
#include "nsSVGEffects.h"

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Filter)

DOMCI_NODE_DATA(SVGFilterElement, mozilla::dom::SVGFilterElement)

namespace mozilla {
namespace dom {

nsSVGElement::LengthInfo SVGFilterElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::y, -10, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
  { &nsGkAtoms::width, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::X },
  { &nsGkAtoms::height, 120, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, SVGContentUtils::Y },
};

nsSVGElement::IntegerPairInfo SVGFilterElement::sIntegerPairInfo[1] =
{
  { &nsGkAtoms::filterRes, 0 }
};

nsSVGElement::EnumInfo SVGFilterElement::sEnumInfo[2] =
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

nsSVGElement::StringInfo SVGFilterElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_ADDREF_INHERITED(SVGFilterElement,SVGFilterElementBase)
NS_IMPL_RELEASE_INHERITED(SVGFilterElement,SVGFilterElementBase)

NS_INTERFACE_TABLE_HEAD(SVGFilterElement)
  NS_NODE_INTERFACE_TABLE5(SVGFilterElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGFilterElement,
                           nsIDOMSVGURIReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGFilterElement)
NS_INTERFACE_MAP_END_INHERITING(SVGFilterElementBase)




SVGFilterElement::SVGFilterElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGFilterElementBase(aNodeInfo)
{
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(SVGFilterElement)






NS_IMETHODIMP SVGFilterElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP SVGFilterElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP SVGFilterElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP SVGFilterElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}


NS_IMETHODIMP SVGFilterElement::GetFilterUnits(nsIDOMSVGAnimatedEnumeration * *aUnits)
{
  return mEnumAttributes[FILTERUNITS].ToDOMAnimatedEnum(aUnits, this);
}


NS_IMETHODIMP SVGFilterElement::GetPrimitiveUnits(nsIDOMSVGAnimatedEnumeration * *aUnits)
{
  return mEnumAttributes[PRIMITIVEUNITS].ToDOMAnimatedEnum(aUnits, this);
}


NS_IMETHODIMP SVGFilterElement::GetFilterResX(nsIDOMSVGAnimatedInteger * *aFilterResX)
{
  return mIntegerPairAttributes[FILTERRES].ToDOMAnimatedInteger(aFilterResX,
                                                                nsSVGIntegerPair::eFirst,
                                                                this);
}


NS_IMETHODIMP SVGFilterElement::GetFilterResY(nsIDOMSVGAnimatedInteger * *aFilterResY)
{
  return mIntegerPairAttributes[FILTERRES].ToDOMAnimatedInteger(aFilterResY,
                                                                nsSVGIntegerPair::eSecond,
                                                                this);
}



NS_IMETHODIMP
SVGFilterElement::SetFilterRes(uint32_t filterResX, uint32_t filterResY)
{
  mIntegerPairAttributes[FILTERRES].SetBaseValues(filterResX, filterResY, this);
  return NS_OK;
}





NS_IMETHODIMP 
SVGFilterElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




NS_IMETHODIMP_(bool)
SVGFilterElement::IsAttributeMapped(const nsIAtom* name) const
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
  return FindAttributeDependence(name, map) ||
    SVGFilterElementBase::IsAttributeMapped(name);
}

void
SVGFilterElement::Invalidate()
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




 bool
SVGFilterElement::HasValidDimensions() const
{
  return (!mLengthAttributes[WIDTH].IsExplicitlySet() ||
           mLengthAttributes[WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[HEIGHT].IsExplicitlySet() || 
           mLengthAttributes[HEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
SVGFilterElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::IntegerPairAttributesInfo
SVGFilterElement::GetIntegerPairInfo()
{
  return IntegerPairAttributesInfo(mIntegerPairAttributes, sIntegerPairInfo,
                                   ArrayLength(sIntegerPairInfo));
}

nsSVGElement::EnumAttributesInfo
SVGFilterElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            ArrayLength(sEnumInfo));
}

nsSVGElement::StringAttributesInfo
SVGFilterElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}

} 
} 
