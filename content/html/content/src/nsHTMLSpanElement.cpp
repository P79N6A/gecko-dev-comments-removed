



































#include "nsIDOMHTMLElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

class nsHTMLSpanElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLElement
{
public:
  nsHTMLSpanElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLSpanElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);
  virtual nsresult SetInnerHTML(const nsAString& aInnerHTML);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Span)


nsHTMLSpanElement::nsHTMLSpanElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLSpanElement::~nsHTMLSpanElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLSpanElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLSpanElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLSpanElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLSpanElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLSpanElement)


nsresult
nsHTMLSpanElement::GetInnerHTML(nsAString& aInnerHTML)
{
  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    nsContentUtils::GetNodeTextContent(this, PR_FALSE, aInnerHTML);
    return NS_OK;
  }

  return nsGenericHTMLElement::GetInnerHTML(aInnerHTML);  
}

nsresult
nsHTMLSpanElement::SetInnerHTML(const nsAString& aInnerHTML)
{
  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    return nsContentUtils::SetNodeTextContent(this, aInnerHTML, PR_TRUE);
  }

  return nsGenericHTMLElement::SetInnerHTML(aInnerHTML);
}



class nsHTMLUnknownElement : public nsHTMLSpanElement
{
public:
  nsHTMLUnknownElement(nsINodeInfo *aNodeInfo);

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

NS_INTERFACE_MAP_BEGIN(nsHTMLUnknownElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLUnknownElement)
NS_INTERFACE_MAP_END_INHERITING(nsHTMLSpanElement)

nsHTMLUnknownElement::nsHTMLUnknownElement(nsINodeInfo *aNodeInfo)
  : nsHTMLSpanElement(aNodeInfo)
{
}


NS_IMPL_NS_NEW_HTML_ELEMENT(Unknown)


NS_IMPL_ELEMENT_CLONE(nsHTMLUnknownElement)
