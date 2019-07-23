



































#include "nsIDOMHTMLModElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"


class nsHTMLModElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLModElement
{
public:
  nsHTMLModElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLModElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLMODELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Mod)

nsHTMLModElement::nsHTMLModElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLModElement::~nsHTMLModElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLModElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLModElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLModElement, nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLModElement, nsIDOMHTMLModElement)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLDelElement, del)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLInsElement, ins)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLModElement)


NS_IMPL_URI_ATTR(nsHTMLModElement, Cite, cite)
NS_IMPL_STRING_ATTR(nsHTMLModElement, DateTime, datetime)
