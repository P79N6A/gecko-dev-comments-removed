



































#include "nsIDOMHTMLHeadElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"


class nsHTMLHeadElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLHeadElement
{
public:
  nsHTMLHeadElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLHeadElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLHEADELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Head)


nsHTMLHeadElement::nsHTMLHeadElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLHeadElement::~nsHTMLHeadElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLHeadElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLHeadElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLHeadElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLHeadElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLHeadElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLHeadElement)


NS_IMPL_URI_ATTR(nsHTMLHeadElement, Profile, profile)
