





































#include "nsSVGStylableElement.h"
#include "nsIDOMSVGTitleElement.h"

typedef nsSVGStylableElement nsSVGTitleElementBase;

class nsSVGTitleElement : public nsSVGTitleElementBase,
                          public nsIDOMSVGTitleElement
{
protected:
  friend nsresult NS_NewSVGTitleElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGTitleElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTITLEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGTitleElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTitleElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTitleElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Title)





NS_IMPL_ADDREF_INHERITED(nsSVGTitleElement, nsSVGTitleElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTitleElement, nsSVGTitleElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGTitleElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGTitleElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTitleElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTitleElementBase)





nsSVGTitleElement::nsSVGTitleElement(nsINodeInfo *aNodeInfo)
  : nsSVGTitleElementBase(aNodeInfo)
{
}


nsresult
nsSVGTitleElement::Init()
{
  return nsSVGTitleElementBase::Init();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTitleElement)

