





































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTSpanElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAnimatedLengthList.h"
#include "nsSVGLengthList.h"
#include "nsISVGSVGElement.h"
#include "nsSVGSVGElement.h"
#include "nsISVGTextContentMetrics.h"
#include "nsIFrame.h"
#include "nsDOMError.h"

typedef nsSVGStylableElement nsSVGTSpanElementBase;

class nsSVGTSpanElement : public nsSVGTSpanElementBase,
                          public nsIDOMSVGTSpanElement 
                                                       
{
protected:
  friend nsresult NS_NewSVGTSpanElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGTSpanElement(nsINodeInfo* aNodeInfo);
  nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTSPANELEMENT
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTSpanElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  
  virtual PRBool IsEventName(nsIAtom* aName);

  already_AddRefed<nsISVGTextContentMetrics> GetTextContentMetrics();

  
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mY;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdY;

};


NS_IMPL_NS_NEW_SVG_ELEMENT(TSpan)





NS_IMPL_ADDREF_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGTSpanElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTSpanElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTextPositioningElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTextContentElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTSpanElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTSpanElementBase)




nsSVGTSpanElement::nsSVGTSpanElement(nsINodeInfo *aNodeInfo)
  : nsSVGTSpanElementBase(aNodeInfo)
{

}

  
nsresult
nsSVGTSpanElement::Init()
{
  nsresult rv = nsSVGTSpanElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::x, mX);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::y, mY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::dx, mdX);
    NS_ENSURE_SUCCESS(rv,rv);
  }
  
  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::dy, mdY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTSpanElement)











NS_IMETHODIMP nsSVGTSpanElement::GetX(nsIDOMSVGAnimatedLengthList * *aX)
{
  *aX = mX;
  NS_IF_ADDREF(*aX);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetY(nsIDOMSVGAnimatedLengthList * *aY)
{
  *aY = mY;
  NS_IF_ADDREF(*aY);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetDx(nsIDOMSVGAnimatedLengthList * *aDx)
{
  *aDx = mdX;
  NS_IF_ADDREF(*aDx);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetDy(nsIDOMSVGAnimatedLengthList * *aDy)
{
  *aDy = mdY;
  NS_IF_ADDREF(*aDy);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetRotate(nsIDOMSVGAnimatedNumberList * *aRotate)
{
  NS_NOTYETIMPLEMENTED("nsSVGTSpanElement::GetRotate");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP nsSVGTSpanElement::GetTextLength(nsIDOMSVGAnimatedLength * *aTextLength)
{
  NS_NOTYETIMPLEMENTED("nsSVGTSpanElement::GetTextLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGTSpanElement::GetLengthAdjust(nsIDOMSVGAnimatedEnumeration * *aLengthAdjust)
{
  NS_NOTYETIMPLEMENTED("nsSVGTSpanElement::GetLengthAdjust");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsSVGTSpanElement::GetNumberOfChars(PRInt32 *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetNumberOfChars(_retval);

  *_retval = 0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetComputedTextLength(float *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetComputedTextLength(_retval);

  *_retval = 0.0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)
{
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (metrics)
    return metrics->GetSubStringLength(charnum, nchars, _retval);

  *_retval = 0.0;
  return NS_OK;
}


NS_IMETHODIMP nsSVGTSpanElement::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetStartPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTSpanElement::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetEndPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTSpanElement::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;
  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetExtentOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTSpanElement::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  *_retval = 0.0;

  nsCOMPtr<nsISVGTextContentMetrics> metrics = GetTextContentMetrics();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetRotationOfChar(charnum, _retval);
}


NS_IMETHODIMP nsSVGTSpanElement::GetCharNumAtPosition(nsIDOMSVGPoint *point,
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


NS_IMETHODIMP nsSVGTSpanElement::SelectSubString(PRUint32 charnum, PRUint32 nchars)
{
  NS_NOTYETIMPLEMENTED("nsSVGTSpanElement::SelectSubString");
  return NS_ERROR_NOT_IMPLEMENTED;
}




NS_IMETHODIMP_(PRBool)
nsSVGTSpanElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGTSpanElementBase::IsAttributeMapped(name);
}




PRBool
nsSVGTSpanElement::IsEventName(nsIAtom* aName)
{
  return IsGraphicElementEventName(aName);
}




already_AddRefed<nsISVGTextContentMetrics>
nsSVGTSpanElement::GetTextContentMetrics()
{
  nsIFrame* frame = GetPrimaryFrame(Flush_Layout);

  if (!frame) {
    return nsnull;
  }
  
  nsISVGTextContentMetrics* metrics;
  CallQueryInterface(frame, &metrics);
  return metrics;
}
