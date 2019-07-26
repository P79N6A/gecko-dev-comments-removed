




#include "nsSVGElement.h"
#include "nsIDOMSVGMetadataElement.h"

typedef nsSVGElement nsSVGMetadataElementBase;

class nsSVGMetadataElement : public nsSVGMetadataElementBase,
                             public nsIDOMSVGMetadataElement
{
protected:
  friend nsresult NS_NewSVGMetadataElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo);
  nsSVGMetadataElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMETADATAELEMENT

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Metadata)





NS_IMPL_ADDREF_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)

DOMCI_NODE_DATA(SVGMetadataElement, nsSVGMetadataElement)

NS_INTERFACE_TABLE_HEAD(nsSVGMetadataElement)
  NS_NODE_INTERFACE_TABLE4(nsSVGMetadataElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGMetadataElement)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGMetadataElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMetadataElementBase)





nsSVGMetadataElement::nsSVGMetadataElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGMetadataElementBase(aNodeInfo)
{
}


nsresult
nsSVGMetadataElement::Init()
{
  return NS_OK;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMetadataElement)
