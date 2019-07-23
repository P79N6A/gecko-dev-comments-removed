





































#include "nsSVGTransformList.h"
#include "nsSVGAnimatedTransformList.h"
#include "nsSVGEnum.h"
#include "nsSVGAnimatedEnumeration.h"
#include "nsIDOMSVGAnimatedEnum.h"
#include "nsSVGAnimatedString.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsSVGAnimatedRect.h"
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGPreserveAspectRatio.h"
#include "nsSVGPatternElement.h"
#include "nsIFrame.h"



nsSVGElement::LengthInfo nsSVGPatternElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
  { &nsGkAtoms::width, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::X },
  { &nsGkAtoms::height, 100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Pattern)




NS_IMPL_ADDREF_INHERITED(nsSVGPatternElement,nsSVGPatternElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPatternElement,nsSVGPatternElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPatternElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGFitToViewBox)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPatternElement)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPatternElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPatternElementBase)




nsSVGPatternElement::nsSVGPatternElement(nsINodeInfo* aNodeInfo)
  : nsSVGPatternElementBase(aNodeInfo)
{
  AddMutationObserver(this);
}

nsresult
nsSVGPatternElement::Init()
{
  nsresult rv = nsSVGPatternElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  static struct nsSVGEnumMapping pUnitMap[] = {
        {&nsGkAtoms::objectBoundingBox, nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX},
        {&nsGkAtoms::userSpaceOnUse, nsIDOMSVGPatternElement::SVG_PUNITS_USERSPACEONUSE},
        {nsnull, 0}
  };

  

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGPatternElement::SVG_PUNITS_OBJECTBOUNDINGBOX, pUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mPatternUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::patternUnits, mPatternUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsISVGEnum> units;
    rv = NS_NewSVGEnum(getter_AddRefs(units),
                       nsIDOMSVGPatternElement::SVG_PUNITS_USERSPACEONUSE, pUnitMap);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedEnumeration(getter_AddRefs(mPatternContentUnits), units);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::patternContentUnits, mPatternContentUnits);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGTransformList> transformList;
    rv = nsSVGTransformList::Create(getter_AddRefs(transformList));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedTransformList(getter_AddRefs(mPatternTransform),
                                        transformList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::patternTransform, mPatternTransform);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  

  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  

  
  {
    nsCOMPtr<nsIDOMSVGRect> viewbox;
    rv = NS_NewSVGRect(getter_AddRefs(viewbox));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedRect(getter_AddRefs(mViewBox), viewbox);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::viewBox, mViewBox);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGPreserveAspectRatio> preserveAspectRatio;
    rv = NS_NewSVGPreserveAspectRatio(getter_AddRefs(preserveAspectRatio));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedPreserveAspectRatio(
                                          getter_AddRefs(mPreserveAspectRatio),
                                          preserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::preserveAspectRatio,
                           mPreserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
  }


  return NS_OK;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGPatternElement)





NS_IMETHODIMP nsSVGPatternElement::GetViewBox(nsIDOMSVGAnimatedRect * *aViewBox)
{
  *aViewBox = mViewBox;
  NS_ADDREF(*aViewBox);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGPatternElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio * *aPreserveAspectRatio)
{
  *aPreserveAspectRatio = mPreserveAspectRatio;
  NS_ADDREF(*aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP nsSVGPatternElement::GetPatternUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  *aPatternUnits = mPatternUnits;
  NS_IF_ADDREF(*aPatternUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGPatternElement::GetPatternContentUnits(nsIDOMSVGAnimatedEnumeration * *aPatternUnits)
{
  *aPatternUnits = mPatternContentUnits;
  NS_IF_ADDREF(*aPatternUnits);
  return NS_OK;
}


NS_IMETHODIMP nsSVGPatternElement::GetPatternTransform(nsIDOMSVGAnimatedTransformList * *aPatternTransform)
{
  *aPatternTransform = mPatternTransform;
  NS_IF_ADDREF(*aPatternTransform);
  return NS_OK;
}


NS_IMETHODIMP nsSVGPatternElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGPatternElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}






NS_IMETHODIMP
nsSVGPatternElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




NS_IMETHODIMP_(PRBool)
nsSVGPatternElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGPatternElementBase::IsAttributeMapped(name);
}




nsSVGElement::LengthAttributesInfo
nsSVGPatternElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}




void
nsSVGPatternElement::PushUpdate()
{
  nsIFrame *frame = GetPrimaryFrame();

  if (frame) {
    nsISVGValue *value = nsnull;
    CallQueryInterface(frame, &value);
    if (value) {
      value->BeginBatchUpdate();
      value->EndBatchUpdate();
    }
  }
}

void
nsSVGPatternElement::CharacterDataChanged(nsIDocument *aDocument,
                                          nsIContent *aContent,
                                          CharacterDataChangeInfo *aInfo)
{
  PushUpdate();
}

void
nsSVGPatternElement::AttributeChanged(nsIDocument *aDocument,
                                      nsIContent *aContent,
                                      PRInt32 aNameSpaceID,
                                      nsIAtom *aAttribute,
                                      PRInt32 aModType)
{
  PushUpdate();
}

void
nsSVGPatternElement::ContentAppended(nsIDocument *aDocument,
                                     nsIContent *aContainer,
                                     PRInt32 aNewIndexInContainer)
{
  PushUpdate();
}

void
nsSVGPatternElement::ContentInserted(nsIDocument *aDocument,
                                     nsIContent *aContainer,
                                     nsIContent *aChild,
                                     PRInt32 aIndexInContainer)
{
  PushUpdate();
}

void
nsSVGPatternElement::ContentRemoved(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    nsIContent *aChild,
                                    PRInt32 aIndexInContainer)
{
  PushUpdate();
}
