





































#include "nsSVGPolyElement.h"
#include "nsIDOMSVGPolylineElement.h"

typedef nsSVGPolyElement nsSVGPolylineElementBase;

class nsSVGPolylineElement : public nsSVGPolylineElementBase,
                             public nsIDOMSVGPolylineElement
{
protected:
  friend nsresult NS_NewSVGPolylineElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGPolylineElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPOLYLINEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGPolylineElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGPolylineElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPolylineElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Polyline)




NS_IMPL_ADDREF_INHERITED(nsSVGPolylineElement,nsSVGPolylineElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolylineElement,nsSVGPolylineElementBase)

DOMCI_NODE_DATA(SVGPolylineElement, nsSVGPolylineElement)

NS_INTERFACE_TABLE_HEAD(nsSVGPolylineElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGPolylineElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGPolylineElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGPolylineElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolylineElementBase)




nsSVGPolylineElement::nsSVGPolylineElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGPolylineElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGPolylineElement)
