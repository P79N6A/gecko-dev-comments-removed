




#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLElement.h"

#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

class nsHTMLElement : public nsGenericHTMLElement,
                      public nsIDOMHTMLElement
{
public:
  nsHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)
  virtual void GetInnerHTML(nsAString& aInnerHTML,
                            mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};



nsGenericHTMLElement*
NS_NewHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                  FromParser aFromParser)
{
  return new nsHTMLElement(aNodeInfo);
}

nsHTMLElement::nsHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLElement::~nsHTMLElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLElement, nsHTMLElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(nsHTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLElement)

void
nsHTMLElement::GetInnerHTML(nsAString& aInnerHTML, ErrorResult& aError)
{
  






  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
    return;
  }

  nsGenericHTMLElement::GetInnerHTML(aInnerHTML, aError);
}

