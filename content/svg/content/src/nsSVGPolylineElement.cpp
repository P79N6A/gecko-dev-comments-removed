





































#include "nsSVGPolyElement.h"
#include "nsIDOMSVGPolylineElement.h"

typedef nsSVGPolyElement nsSVGPolylineElementBase;

class nsSVGPolylineElement : public nsSVGPolylineElementBase,
                             public nsIDOMSVGPolylineElement
{
protected:
  friend nsresult NS_NewSVGPolylineElement(nsIContent **aResult,
                                           nsINodeInfo *aNodeInfo);
  nsSVGPolylineElement(nsINodeInfo* aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGPOLYLINEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGPolylineElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGPolylineElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGPolylineElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Polyline)




NS_IMPL_ADDREF_INHERITED(nsSVGPolylineElement,nsSVGPolylineElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGPolylineElement,nsSVGPolylineElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGPolylineElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPolylineElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPolylineElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGPolylineElementBase)




nsSVGPolylineElement::nsSVGPolylineElement(nsINodeInfo* aNodeInfo)
  : nsSVGPolylineElementBase(aNodeInfo)
{

}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGPolylineElement)
