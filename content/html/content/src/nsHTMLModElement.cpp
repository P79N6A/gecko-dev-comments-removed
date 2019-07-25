





































#include "nsIDOMHTMLModElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"

class nsHTMLModElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLModElement
{
public:
  nsHTMLModElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLModElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLMODELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Mod)

nsHTMLModElement::nsHTMLModElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLModElement::~nsHTMLModElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLModElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLModElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLModElement, nsHTMLModElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLModElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLModElement,
                                   nsIDOMHTMLModElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLModElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLModElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLModElement)


NS_IMPL_URI_ATTR(nsHTMLModElement, Cite, cite)
NS_IMPL_STRING_ATTR(nsHTMLModElement, DateTime, datetime)
