





































#include "nsSVGGraphicElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTextElement.h"
#include "nsCOMPtr.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextPositioningElement.h"
#include "nsIFrame.h"
#include "nsDOMError.h"
#include "nsSVGLengthList.h"
#include "nsSVGAnimatedLengthList.h"
#include "nsSVGNumberList.h"
#include "nsSVGAnimatedNumberList.h"

typedef nsSVGGraphicElement nsSVGTextElementBase;













class nsSVGTextElement : public nsSVGTextElementBase,
                         public nsIDOMSVGTextElement 
{
protected:
  friend nsresult NS_NewSVGTextElement(nsIContent **aResult,
                                       nsINodeInfo *aNodeInfo);
  nsSVGTextElement(nsINodeInfo* aNodeInfo);
  nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTEXTELEMENT
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGTextElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTextElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTextElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  nsSVGTextContainerFrame* GetTextContainerFrame() {
    return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
  }

  
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mY;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdX;
  nsCOMPtr<nsIDOMSVGAnimatedLengthList> mdY;
  nsCOMPtr<nsIDOMSVGAnimatedNumberList> mRotate;
};


NS_IMPL_NS_NEW_SVG_ELEMENT(Text)





NS_IMPL_ADDREF_INHERITED(nsSVGTextElement,nsSVGTextElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTextElement,nsSVGTextElementBase)

DOMCI_DATA(SVGTextElement, nsSVGTextElement)

NS_INTERFACE_TABLE_HEAD(nsSVGTextElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGTextElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTextElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTextElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTextElementBase)




nsSVGTextElement::nsSVGTextElement(nsINodeInfo* aNodeInfo)
  : nsSVGTextElementBase(aNodeInfo)
{

}
  
nsresult
nsSVGTextElement::Init()
{
  nsresult rv = nsSVGTextElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this, nsSVGUtils::X);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::x, mX);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this, nsSVGUtils::Y);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::y, mY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this, nsSVGUtils::X);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdX),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::dx, mdX);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGLengthList> lengthList;
    rv = NS_NewSVGLengthList(getter_AddRefs(lengthList), this, nsSVGUtils::Y);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedLengthList(getter_AddRefs(mdY),
                                     lengthList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::dy, mdY);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGNumberList> numberList;
    rv = NS_NewSVGNumberList(getter_AddRefs(numberList));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedNumberList(getter_AddRefs(mRotate),
                                     numberList);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::rotate, mRotate);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTextElement)












NS_IMETHODIMP
nsSVGTextElement::GetX(nsIDOMSVGAnimatedLengthList * *aX)
{
  *aX = mX;
  NS_IF_ADDREF(*aX);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetY(nsIDOMSVGAnimatedLengthList * *aY)
{
  *aY = mY;
  NS_IF_ADDREF(*aY);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetDx(nsIDOMSVGAnimatedLengthList * *aDx)
{
  *aDx = mdX;
  NS_IF_ADDREF(*aDx);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetDy(nsIDOMSVGAnimatedLengthList * *aDy)
{
  *aDy = mdY;
  NS_IF_ADDREF(*aDy);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetRotate(nsIDOMSVGAnimatedNumberList * *aRotate)
{
  *aRotate = mRotate;
  NS_IF_ADDREF(*aRotate);
  return NS_OK;
}






NS_IMETHODIMP
nsSVGTextElement::GetTextLength(nsIDOMSVGAnimatedLength * *aTextLength)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextElement::GetTextLength");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGTextElement::GetLengthAdjust(nsIDOMSVGAnimatedEnumeration * *aLengthAdjust)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextElement::GetLengthAdjust");
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsSVGTextElement::GetNumberOfChars(PRInt32 *_retval)
{
  *_retval = 0;

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (metrics)
    *_retval = metrics->GetNumberOfChars();

  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetComputedTextLength(float *_retval)
{
  *_retval = 0.0;

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (metrics)
    *_retval = metrics->GetComputedTextLength();

  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval)
{
  *_retval = 0.0f;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (!metrics)
    return NS_OK;

  PRUint32 charcount = metrics->GetNumberOfChars();
  if (charcount <= charnum || nchars > charcount - charnum)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  if (nchars == 0)
    return NS_OK;

  *_retval = metrics->GetSubStringLength(charnum, nchars);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetStartPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval)
{
  *_retval = nsnull;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetEndPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nsnull;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetExtentOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetRotationOfChar(PRUint32 charnum, float *_retval)
{
  *_retval = 0.0;

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetRotationOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval)
{
  if (!point)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  *_retval = -1;

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (metrics)
    *_retval = metrics->GetCharNumAtPosition(point);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::SelectSubString(PRUint32 charnum, PRUint32 nchars)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextElement::SelectSubString");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP_(PRBool)
nsSVGTextElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sTextContentElementsMap,
    sFontSpecificationMap
  };

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGTextElementBase::IsAttributeMapped(name);
}
