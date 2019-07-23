



































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGSwitchElement.h"

typedef nsSVGGraphicElement nsSVGSwitchElementBase;

class nsSVGSwitchElement : public nsSVGSwitchElementBase,
                           public nsIDOMSVGSwitchElement
{
protected:
  friend nsresult NS_NewSVGSwitchElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGSwitchElement(nsINodeInfo *aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSWITCHELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGSwitchElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGSwitchElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGSwitchElementBase::)

  
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};





NS_IMPL_NS_NEW_SVG_ELEMENT(Switch)





NS_IMPL_ADDREF_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGSwitchElement,nsSVGSwitchElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGSwitchElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGSwitchElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGSwitchElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGSwitchElementBase)




nsSVGSwitchElement::nsSVGSwitchElement(nsINodeInfo *aNodeInfo)
  : nsSVGSwitchElementBase(aNodeInfo)
{

}






NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGSwitchElement)





NS_IMETHODIMP_(PRBool)
nsSVGSwitchElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGSwitchElementBase::IsAttributeMapped(name);
}
