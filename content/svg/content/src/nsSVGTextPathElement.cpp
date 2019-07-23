




































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTextPathElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsISVGTextContentMetrics.h"
#include "nsIFrame.h"
#include "nsSVGTextPathElement.h"
#include "nsSVGAnimatedString.h"
#include "nsDOMError.h"

nsSVGElement::LengthInfo nsSVGTextPathElement::sLengthInfo[1] =
{
  { &nsGkAtoms::startOffset, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
};

nsSVGEnumMapping nsSVGTextPathElement::sMethodMap[] = {
  {&nsGkAtoms::align, nsIDOMSVGTextPathElement::TEXTPATH_METHODTYPE_ALIGN},
  {&nsGkAtoms::stretch, nsIDOMSVGTextPathElement::TEXTPATH_METHODTYPE_STRETCH},
  {nsnull, 0}
};

nsSVGEnumMapping nsSVGTextPathElement::sSpacingMap[] = {
  {&nsGkAtoms::_auto, nsIDOMSVGTextPathElement::TEXTPATH_SPACINGTYPE_AUTO},
  {&nsGkAtoms::exact, nsIDOMSVGTextPathElement::TEXTPATH_SPACINGTYPE_EXACT},
  {nsnull, 0}
};

nsSVGElement::EnumInfo nsSVGTextPathElement::sEnumInfo[2] =
{
  { &nsGkAtoms::method,
    sMethodMap,
    nsIDOMSVGTextPathElement::TEXTPATH_METHODTYPE_ALIGN
  },
  { &nsGkAtoms::spacing,
    sSpacingMap,
    nsIDOMSVGTextPathElement::TEXTPATH_SPACINGTYPE_EXACT
  }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(TextPath)




NS_IMPL_ADDREF_INHERITED(nsSVGTextPathElement,nsSVGTextPathElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTextPathElement,nsSVGTextPathElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGTextPathElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTextPathElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTextContentElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTextPathElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTextPathElementBase)




nsSVGTextPathElement::nsSVGTextPathElement(nsINodeInfo *aNodeInfo)
  : nsSVGTextPathElementBase(aNodeInfo)
{
}

nsresult
nsSVGTextPathElement::Init()
{
  nsresult rv = nsSVGTextPathElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  return rv;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTextPathElement)





NS_IMETHODIMP nsSVGTextPathElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




NS_IMETHODIMP nsSVGTextPathElement::GetStartOffset(nsIDOMSVGAnimatedLength * *aStartOffset)
{
  return mLengthAttributes[STARTOFFSET].ToDOMAnimatedLength(aStartOffset, this);
}


NS_IMETHODIMP nsSVGTextPathElement::GetMethod(nsIDOMSVGAnimatedEnumeration * *aMethod)
{
  return mEnumAttributes[METHOD].ToDOMAnimatedEnum(aMethod, this);
}


NS_IMETHODIMP nsSVGTextPathElement::GetSpacing(nsIDOMSVGAnimatedEnumeration * *aSpacing)
{
  return mEnumAttributes[SPACING].ToDOMAnimatedEnum(aSpacing, this);
}





NS_IMETHODIMP nsSVGTextPathElement::GetTextLength(nsIDOMSVGAnimatedLength * *aTextLength)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextPathElement::GetTextLength!");
  return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP nsSVGTextPathElement::GetLengthAdjust(nsIDOMSVGAnimatedEnumeration * *aLengthAdjust)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextPathElement::GetLengthAdjust!");
  return NS_ERROR_UNEXPECTED;
}


NS_IMETHODIMP nsSVGTextPathElement::GetNumberOfChars(PRInt32 *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetNumberOfChars(_retval);

  *_retval = 0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPathElement::GetComputedTextLength(float *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetComputedTextLength(_retval);

  *_retval = 0.0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPathElement::GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetSubStringLength(charnum, nchars, _retval);

  *_retval = 0.0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPathElement::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetStartPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTextPathElement::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetEndPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTextPathElement::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetExtentOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTextPathElement::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  *_retval = 0.0;

  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetRotationOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTextPathElement::GetCharNumAtPosition(nsIDOMSVGPoint *point,
                                                         PRInt32 *_retval)
{
  
  if (!point)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetCharNumAtPosition(point, _retval);

  *_retval = -1;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextPathElement::SelectSubString(PRUint32 charnum, PRUint32 nchars)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextPathElement::SelectSubString!");
  return NS_ERROR_UNEXPECTED;
}




NS_IMETHODIMP_(PRBool)
nsSVGTextPathElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGTextPathElementBase::IsAttributeMapped(name);
}




PRBool
nsSVGTextPathElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}

nsSVGElement::LengthAttributesInfo
nsSVGTextPathElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

nsSVGElement::EnumAttributesInfo
nsSVGTextPathElement::GetEnumInfo()
{
  return EnumAttributesInfo(mEnumAttributes, sEnumInfo,
                            NS_ARRAY_LENGTH(sEnumInfo));
}




already_AddRefed<nsISVGTextContentMetrics>
nsSVGTextPathElement::GetTextContentMetrics()
{
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame) {
    return nsnull;
  }
  
  nsISVGTextContentMetrics* metrics;
  CallQueryInterface(frame, &metrics);
  return metrics;
}
