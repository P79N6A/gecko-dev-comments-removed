





































#include "nsSVGElement.h"
#include "nsIDOMSVGMetadataElement.h"

typedef nsSVGElement nsSVGMetadataElementBase;

class nsSVGMetadataElement : public nsSVGMetadataElementBase,
                             public nsIDOMSVGMetadataElement
{
protected:
  friend nsresult NS_NewSVGMetadataElement(nsIContent **aResult,
                                           nsINodeInfo *aNodeInfo);
  nsSVGMetadataElement(nsINodeInfo* aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGMETADATAELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGElement::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGElement::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Metadata)





NS_IMPL_ADDREF_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGMetadataElement, nsSVGMetadataElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGMetadataElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGMetadataElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGMetadataElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGMetadataElementBase)





nsSVGMetadataElement::nsSVGMetadataElement(nsINodeInfo *aNodeInfo)
  : nsSVGMetadataElementBase(aNodeInfo)
{
}


nsresult
nsSVGMetadataElement::Init()
{
  return NS_OK;
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGMetadataElement)
