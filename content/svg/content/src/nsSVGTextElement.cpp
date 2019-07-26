




#include "mozilla/Util.h"

#include "SVGGraphicsElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTextElement.h"
#include "nsCOMPtr.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextPositioningElement.h"
#include "nsError.h"
#include "SVGAnimatedLengthList.h"
#include "DOMSVGAnimatedLengthList.h"
#include "SVGLengthList.h"
#include "SVGNumberList.h"
#include "SVGAnimatedNumberList.h"
#include "DOMSVGAnimatedNumberList.h"
#include "DOMSVGPoint.h"

using namespace mozilla;

typedef dom::SVGGraphicsElement nsSVGTextElementBase;













class nsSVGTextElement : public nsSVGTextElementBase,
                         public nsIDOMSVGTextElement 
{
protected:
  friend nsresult NS_NewSVGTextElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGTextElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTEXTELEMENT
  NS_DECL_NSIDOMSVGTEXTPOSITIONINGELEMENT
  NS_DECL_NSIDOMSVGTEXTCONTENTELEMENT

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTextElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  nsSVGTextContainerFrame* GetTextContainerFrame() {
    return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
  }

  virtual LengthListAttributesInfo GetLengthListInfo();
  virtual NumberListAttributesInfo GetNumberListInfo();

  

  enum { X, Y, DX, DY };
  SVGAnimatedLengthList mLengthListAttributes[4];
  static LengthListInfo sLengthListInfo[4];

  enum { ROTATE };
  SVGAnimatedNumberList mNumberListAttributes[1];
  static NumberListInfo sNumberListInfo[1];
};


NS_IMPL_NS_NEW_SVG_ELEMENT(Text)





NS_IMPL_ADDREF_INHERITED(nsSVGTextElement,nsSVGTextElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTextElement,nsSVGTextElementBase)

DOMCI_NODE_DATA(SVGTextElement, nsSVGTextElement)

NS_INTERFACE_TABLE_HEAD(nsSVGTextElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGTextElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTextElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTextElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTextElementBase)




nsSVGTextElement::nsSVGTextElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGTextElementBase(aNodeInfo)
{

}
  




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTextElement)












NS_IMETHODIMP
nsSVGTextElement::GetX(nsISupports * *aX)
{
  *aX = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[X],
                                                this, X, SVGContentUtils::X).get();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetY(nsISupports * *aY)
{
  *aY = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[Y],
                                                this, Y, SVGContentUtils::Y).get();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetDx(nsISupports * *aDx)
{
  *aDx = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[DX],
                                                 this, DX, SVGContentUtils::X).get();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetDy(nsISupports * *aDy)
{
  *aDy = DOMSVGAnimatedLengthList::GetDOMWrapper(&mLengthListAttributes[DY],
                                                 this, DY, SVGContentUtils::Y).get();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetRotate(nsISupports * *aRotate)
{
  *aRotate = DOMSVGAnimatedNumberList::GetDOMWrapper(&mNumberListAttributes[ROTATE],
                                                     this, ROTATE).get();
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
nsSVGTextElement::GetNumberOfChars(int32_t *_retval)
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
nsSVGTextElement::GetSubStringLength(uint32_t charnum, uint32_t nchars, float *_retval)
{
  *_retval = 0.0f;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (!metrics)
    return NS_OK;

  uint32_t charcount = metrics->GetNumberOfChars();
  if (charcount <= charnum || nchars > charcount - charnum)
    return NS_ERROR_DOM_INDEX_SIZE_ERR;

  if (nchars == 0)
    return NS_OK;

  *_retval = metrics->GetSubStringLength(charnum, nchars);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::GetStartPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  *_retval = nullptr;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetStartPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetEndPositionOfChar(uint32_t charnum, nsISupports **_retval)
{
  *_retval = nullptr;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetEndPositionOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetExtentOfChar(uint32_t charnum, nsIDOMSVGRect **_retval)
{
  *_retval = nullptr;
  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetExtentOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetRotationOfChar(uint32_t charnum, float *_retval)
{
  *_retval = 0.0;

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();

  if (!metrics) return NS_ERROR_FAILURE;

  return metrics->GetRotationOfChar(charnum, _retval);
}


NS_IMETHODIMP
nsSVGTextElement::GetCharNumAtPosition(nsISupports *point, int32_t *_retval)
{
  *_retval = -1;

  nsCOMPtr<DOMSVGPoint> domPoint = do_QueryInterface(point);
  if (!domPoint) {
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;
  }

  nsSVGTextContainerFrame* metrics = GetTextContainerFrame();
  if (metrics)
    *_retval = metrics->GetCharNumAtPosition(domPoint);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGTextElement::SelectSubString(uint32_t charnum, uint32_t nchars)
{
  NS_NOTYETIMPLEMENTED("nsSVGTextElement::SelectSubString");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP_(bool)
nsSVGTextElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sTextContentElementsMap,
    sFontSpecificationMap
  };

  return FindAttributeDependence(name, map) ||
    nsSVGTextElementBase::IsAttributeMapped(name);
}




nsSVGElement::LengthListInfo nsSVGTextElement::sLengthListInfo[4] =
{
  { &nsGkAtoms::x,  SVGContentUtils::X, false },
  { &nsGkAtoms::y,  SVGContentUtils::Y, false },
  { &nsGkAtoms::dx, SVGContentUtils::X, true },
  { &nsGkAtoms::dy, SVGContentUtils::Y, true }
};

nsSVGElement::LengthListAttributesInfo
nsSVGTextElement::GetLengthListInfo()
{
  return LengthListAttributesInfo(mLengthListAttributes, sLengthListInfo,
                                  ArrayLength(sLengthListInfo));
}

nsSVGElement::NumberListInfo nsSVGTextElement::sNumberListInfo[1] =
{
  { &nsGkAtoms::rotate }
};

nsSVGElement::NumberListAttributesInfo
nsSVGTextElement::GetNumberListInfo()
{
  return NumberListAttributesInfo(mNumberListAttributes, sNumberListInfo,
                                  ArrayLength(sNumberListInfo));
}

