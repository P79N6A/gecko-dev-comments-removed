




#include "mozilla/Util.h"

#include "nsGkAtoms.h"
#include "nsIDOMSVGTSpanElement.h"
#include "nsSVGSVGElement.h"
#include "SVGTextPositioningElement.h"
#include "nsContentUtils.h"

using namespace mozilla;

typedef dom::SVGTextPositioningElement nsSVGTSpanElementBase;

class nsSVGTSpanElement : public nsSVGTSpanElementBase, 
                          public nsIDOMSVGTSpanElement
{
protected:
  friend nsresult NS_NewSVGTSpanElement(nsIContent **aResult,
                                        already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTSPANELEMENT

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTCONTENTELEMENT(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTPOSITIONINGELEMENT(nsSVGTSpanElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual bool IsEventAttributeName(nsIAtom* aName) MOZ_OVERRIDE;
};


NS_IMPL_NS_NEW_SVG_ELEMENT(TSpan)





NS_IMPL_ADDREF_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)

DOMCI_NODE_DATA(SVGTSpanElement, nsSVGTSpanElement)

NS_INTERFACE_TABLE_HEAD(nsSVGTSpanElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGTSpanElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTSpanElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTSpanElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTSpanElementBase)




nsSVGTSpanElement::nsSVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGTSpanElementBase(aNodeInfo)
{

}

  




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTSpanElement)










NS_IMETHODIMP_(bool)
nsSVGTSpanElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sColorMap,
    sFillStrokeMap,
    sFontSpecificationMap,
    sGraphicsMap,
    sTextContentElementsMap
  };
  
  return FindAttributeDependence(name, map) ||
    nsSVGTSpanElementBase::IsAttributeMapped(name);
}


bool
nsSVGTSpanElement::IsEventAttributeName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}





