





































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGGElement.h"

typedef nsSVGGraphicElement nsSVGGElementBase;

class nsSVGGElement : public nsSVGGElementBase,
                      public nsIDOMSVGGElement
{
protected:
  friend nsresult NS_NewSVGGElement(nsIContent **aResult,
                                    nsINodeInfo *aNodeInfo);
  nsSVGGElement(nsINodeInfo *aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGGELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGGElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGGElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGGElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};





NS_IMPL_NS_NEW_SVG_ELEMENT(G)





NS_IMPL_ADDREF_INHERITED(nsSVGGElement,nsSVGGElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGGElement,nsSVGGElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGGElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGGElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGGElementBase)




nsSVGGElement::nsSVGGElement(nsINodeInfo *aNodeInfo)
  : nsSVGGElementBase(aNodeInfo)
{

}






NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGGElement)





NS_IMETHODIMP_(PRBool)
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
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGGElementBase::IsAttributeMapped(name);
}
