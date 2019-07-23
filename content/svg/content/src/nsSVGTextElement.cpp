





































#include "nsSVGGraphicElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTextElement.h"
#include "nsCOMPtr.h"
#include "nsSVGAnimatedLengthList.h"
#include "nsSVGAnimatedNumberList.h"
#include "nsSVGLengthList.h"
#include "nsSVGNumberList.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextContentElement.h"
#include "nsIFrame.h"
#include "nsDOMError.h"

typedef nsSVGGraphicElement nsSVGTextElementBase;

class nsSVGTextElement : public nsSVGTextElementBase,
                         public nsIDOMSVGTextElement,   
                         public nsSVGTextContentElement 
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

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGTextElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTextElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTextElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTCONTENTELEMENT(nsSVGTextContentElement::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  virtual nsSVGTextContainerFrame* GetTextContainerFrame() {
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

NS_INTERFACE_TABLE_HEAD(nsSVGTextElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGTextElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTextElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTextElement)
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











NS_IMETHODIMP nsSVGTextElement::GetX(nsIDOMSVGAnimatedLengthList * *aX)
{
  *aX = mX;
  NS_IF_ADDREF(*aX);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextElement::GetY(nsIDOMSVGAnimatedLengthList * *aY)
{
  *aY = mY;
  NS_IF_ADDREF(*aY);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextElement::GetDx(nsIDOMSVGAnimatedLengthList * *aDx)
{
  *aDx = mdX;
  NS_IF_ADDREF(*aDx);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextElement::GetDy(nsIDOMSVGAnimatedLengthList * *aDy)
{
  *aDy = mdY;
  NS_IF_ADDREF(*aDy);
  return NS_OK;
}


NS_IMETHODIMP nsSVGTextElement::GetRotate(nsIDOMSVGAnimatedNumberList * *aRotate)
{
  *aRotate = mRotate;
  NS_IF_ADDREF(*aRotate);
  return NS_OK;
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
