





































#include "mozilla/Util.h"

#include "nsSVGStylableElement.h"
#include "nsIDOMSVGStopElement.h"
#include "nsSVGNumber2.h"
#include "nsSVGUtils.h"
#include "nsGenericHTMLElement.h"

using namespace mozilla;

typedef nsSVGStylableElement nsSVGStopElementBase;

class nsSVGStopElement : public nsSVGStopElementBase,
                         public nsIDOMSVGStopElement
{
protected:
  friend nsresult NS_NewSVGStopElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGStopElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTOPELEMENT

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGStopElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGStopElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGStopElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:

  virtual NumberAttributesInfo GetNumberInfo();
  
  nsSVGNumber2 mOffset;
  static NumberInfo sNumberInfo;
};

nsSVGElement::NumberInfo nsSVGStopElement::sNumberInfo =
{ &nsGkAtoms::offset, 0, true };

NS_IMPL_NS_NEW_SVG_ELEMENT(Stop)




NS_IMPL_ADDREF_INHERITED(nsSVGStopElement,nsSVGStopElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGStopElement,nsSVGStopElementBase)

DOMCI_NODE_DATA(SVGStopElement, nsSVGStopElement)

NS_INTERFACE_TABLE_HEAD(nsSVGStopElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGStopElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGStopElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGStopElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGStopElementBase)




nsSVGStopElement::nsSVGStopElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGStopElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGStopElement)





NS_IMETHODIMP nsSVGStopElement::GetOffset(nsIDOMSVGAnimatedNumber * *aOffset)
{
  return mOffset.ToDOMAnimatedNumber(aOffset,this);
}




nsSVGElement::NumberAttributesInfo
nsSVGStopElement::GetNumberInfo()
{
  return NumberAttributesInfo(&mOffset, &sNumberInfo, 1);
}




NS_IMETHODIMP_(bool)
nsSVGStopElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sGradientStopMap
  };
  
  return FindAttributeDependence(name, map, ArrayLength(map)) ||
    nsSVGStopElementBase::IsAttributeMapped(name);
}


