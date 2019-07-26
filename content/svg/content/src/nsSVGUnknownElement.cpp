




#include "nsSVGElement.h"

using namespace mozilla;

typedef nsSVGElement nsSVGUnknownElementBase;

class nsSVGUnknownElement : public nsSVGUnknownElementBase
{
protected:
  friend nsresult NS_NewSVGUnknownElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Unknown)




NS_IMPL_ADDREF_INHERITED(nsSVGUnknownElement, nsSVGUnknownElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGUnknownElement, nsSVGUnknownElementBase)

DOMCI_NODE_DATA(SVGUnknownElement, nsSVGUnknownElement)

NS_INTERFACE_TABLE_HEAD(nsSVGUnknownElement)
  NS_NODE_INTERFACE_TABLE3(nsSVGUnknownElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGUnknownElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGUnknownElementBase)



nsSVGUnknownElement::nsSVGUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGUnknownElementBase(aNodeInfo)
{
}

NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGUnknownElement)
