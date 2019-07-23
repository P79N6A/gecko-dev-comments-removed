





































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGDefsElement.h"

typedef nsSVGGraphicElement nsSVGDefsElementBase;

class nsSVGDefsElement : public nsSVGDefsElementBase,
                         public nsIDOMSVGDefsElement
{
protected:
  friend nsresult NS_NewSVGDefsElement(nsIContent **aResult,
                                    nsINodeInfo *aNodeInfo);
  nsSVGDefsElement(nsINodeInfo *aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGDEFSELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGDefsElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGDefsElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGDefsElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};





NS_IMPL_NS_NEW_SVG_ELEMENT(Defs)





NS_IMPL_ADDREF_INHERITED(nsSVGDefsElement,nsSVGDefsElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGDefsElement,nsSVGDefsElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGDefsElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGDefsElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGDefsElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGDefsElementBase)




nsSVGDefsElement::nsSVGDefsElement(nsINodeInfo *aNodeInfo)
  : nsSVGDefsElementBase(aNodeInfo)
{

}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGDefsElement)





NS_IMETHODIMP_(PRBool)
nsSVGDefsElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGDefsElementBase::IsAttributeMapped(name);
}
