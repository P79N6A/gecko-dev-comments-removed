




































#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLElement.h"


class nsHTMLElement : public nsGenericHTMLElement,
                      public nsIDOMHTMLElement
{
public:
  nsHTMLElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;
};



nsGenericHTMLElement*
NS_NewHTMLElement(nsINodeInfo *aNodeInfo, PRUint32 aFromParser)
{
  return new nsHTMLElement(aNodeInfo);
}

nsHTMLElement::nsHTMLElement(nsINodeInfo* aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLElement::~nsHTMLElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLElement, nsGenericElement)

DOMCI_DATA(HTMLElement, nsHTMLElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(nsHTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLElement)

nsresult
nsHTMLElement::GetInnerHTML(nsAString& aInnerHTML)
{
  






  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    nsContentUtils::GetNodeTextContent(this, PR_FALSE, aInnerHTML);
    return NS_OK;
  }

  return nsGenericHTMLElement::GetInnerHTML(aInnerHTML);
}

