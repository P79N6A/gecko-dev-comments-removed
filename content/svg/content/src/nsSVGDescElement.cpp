





































#include "nsSVGStylableElement.h"
#include "nsIDOMSVGDescElement.h"

typedef nsSVGStylableElement nsSVGDescElementBase;

class nsSVGDescElement : public nsSVGDescElementBase,
                         public nsIDOMSVGDescElement
{
protected:
  friend nsresult NS_NewSVGDescElement(nsIContent **aResult,
                                       nsINodeInfo *aNodeInfo);
  nsSVGDescElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGDESCELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGDescElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGDescElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGDescElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Desc)





NS_IMPL_ADDREF_INHERITED(nsSVGDescElement, nsSVGDescElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGDescElement, nsSVGDescElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGDescElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGDescElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGDescElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGDescElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGDescElementBase)





nsSVGDescElement::nsSVGDescElement(nsINodeInfo *aNodeInfo)
  : nsSVGDescElementBase(aNodeInfo)
{
}


nsresult
nsSVGDescElement::Init()
{
  return nsSVGDescElementBase::Init();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGDescElement)

