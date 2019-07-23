



































#include "nsIDOMHTMLTitleElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLDocument.h"


class nsHTMLTitleElement : public nsGenericHTMLElement,
                           public nsIDOMHTMLTitleElement
{
public:
  nsHTMLTitleElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLTitleElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTITLEELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Title)


nsHTMLTitleElement::nsHTMLTitleElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTitleElement::~nsHTMLTitleElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTitleElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTitleElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTitleElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTitleElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTitleElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLTitleElement)


NS_IMETHODIMP 
nsHTMLTitleElement::GetText(nsAString& aTitle)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aTitle);

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTitleElement::SetText(const nsAString& aTitle)
{
  nsCOMPtr<nsIDOMHTMLDocument> htmlDoc(do_QueryInterface(GetCurrentDoc()));

  if (htmlDoc) {
    htmlDoc->SetTitle(aTitle);
  }   

  return nsContentUtils::SetNodeTextContent(this, aTitle, PR_TRUE);
}
