





































#include "mozilla/Util.h"

#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGDefsElement.h"

using namespace mozilla;

typedef nsSVGGraphicElement nsSVGDefsElementBase;

class nsSVGDefsElement : public nsSVGDefsElementBase,
                         public nsIDOMSVGDefsElement
{
protected:
  friend nsresult NS_NewSVGDefsElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGDefsElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGDEFSELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGDefsElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGDefsElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGDefsElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};





NS_IMPL_NS_NEW_SVG_ELEMENT(Defs)





NS_IMPL_ADDREF_INHERITED(nsSVGDefsElement,nsSVGDefsElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGDefsElement,nsSVGDefsElementBase)

DOMCI_NODE_DATA(SVGDefsElement, nsSVGDefsElement)

NS_INTERFACE_TABLE_HEAD(nsSVGDefsElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGDefsElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGDefsElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGDefsElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGDefsElementBase)




nsSVGDefsElement::nsSVGDefsElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGDefsElementBase(aNodeInfo)
{

}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGDefsElement)





NS_IMETHODIMP_(bool)
nsSVGDefsElement::IsAttributeMapped(const nsIAtom* name) const
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
  
  return FindAttributeDependence(name, map, ArrayLength(map)) ||
    nsSVGDefsElementBase::IsAttributeMapped(name);
}
