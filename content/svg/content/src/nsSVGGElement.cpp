




#include "mozilla/Util.h"

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGGElement.h"
#include "DOMSVGTests.h"

using namespace mozilla;

typedef nsSVGGraphicElement nsSVGGElementBase;

class nsSVGGElement : public nsSVGGElementBase,
                      public nsIDOMSVGGElement,
                      public DOMSVGTests
{
protected:
  friend nsresult NS_NewSVGGElement(nsIContent **aResult,
                                    already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGGElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGGELEMENT

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT(nsSVGGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGGElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};





NS_IMPL_NS_NEW_SVG_ELEMENT(G)





NS_IMPL_ADDREF_INHERITED(nsSVGGElement,nsSVGGElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGElement,nsSVGGElementBase)

DOMCI_NODE_DATA(SVGGElement, nsSVGGElement)

NS_INTERFACE_TABLE_HEAD(nsSVGGElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGGElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTests,
                           nsIDOMSVGGElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGElementBase)




nsSVGGElement::nsSVGGElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGGElementBase(aNodeInfo)
{

}






NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGGElement)





NS_IMETHODIMP_(bool)
nsSVGGElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGGElementBase::IsAttributeMapped(name);
}
