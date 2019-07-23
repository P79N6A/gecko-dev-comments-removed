



































#include "nsIDOMHTMLHtmlElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDocument.h"


class nsHTMLHtmlElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLHtmlElement
{
public:
  nsHTMLHtmlElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLHtmlElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLHTMLELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Html)


nsHTMLHtmlElement::nsHTMLHtmlElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLHtmlElement::~nsHTMLHtmlElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLHtmlElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLHtmlElement, nsGenericElement) 




NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLHtmlElement, nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLHtmlElement, nsIDOMHTMLHtmlElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLHtmlElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLHtmlElement)


NS_IMPL_STRING_ATTR(nsHTMLHtmlElement, Version, version)
