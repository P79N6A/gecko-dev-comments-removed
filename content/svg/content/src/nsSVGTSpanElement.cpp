





































#include "nsSVGStylableElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGTSpanElement.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextPositioningElement.h"

typedef nsSVGStylableElement nsSVGTSpanElementBase;

class nsSVGTSpanElement : public nsSVGTSpanElementBase,
                          public nsIDOMSVGTSpanElement,
                          public nsSVGTextPositioningElement 
{
protected:
  friend nsresult NS_NewSVGTSpanElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGTSpanElement(nsINodeInfo* aNodeInfo);
  nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTSPANELEMENT

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTSpanElementBase::)
  NS_FORWARD_NSIDOMSVGTEXTCONTENTELEMENT(nsSVGTextContentElement::)
  NS_FORWARD_NSIDOMSVGTEXTPOSITIONINGELEMENT(nsSVGTextPositioningElement::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  virtual nsSVGTextContainerFrame* GetTextContainerFrame() {
    return do_QueryFrame(GetPrimaryFrame(Flush_Layout));
  }

  
  virtual PRBool IsEventName(nsIAtom* aName);
};


NS_IMPL_NS_NEW_SVG_ELEMENT(TSpan)





NS_IMPL_ADDREF_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTSpanElement,nsSVGTSpanElementBase)

DOMCI_DATA(SVGTSpanElement, nsSVGTSpanElement)

NS_INTERFACE_TABLE_HEAD(nsSVGTSpanElement)
  NS_NODE_INTERFACE_TABLE6(nsSVGTSpanElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTSpanElement,
                           nsIDOMSVGTextPositioningElement,
                           nsIDOMSVGTextContentElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGTSpanElement)
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

  rv = Initialise(this);
  NS_ENSURE_SUCCESS(rv,rv);

  return rv;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTSpanElement)










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
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SVGGraphic);
}
