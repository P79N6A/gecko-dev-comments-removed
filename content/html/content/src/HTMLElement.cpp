




#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

class HTMLElement : public nsGenericHTMLElement,
                    public nsIDOMHTMLElement
{
public:
  HTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual void GetInnerHTML(nsAString& aInnerHTML,
                            mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo* aNodeInfo,
                         nsINode** aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

HTMLElement::HTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLElement::~HTMLElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLElement, Element)

NS_INTERFACE_TABLE_HEAD(HTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(HTMLElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLElement)

void
HTMLElement::GetInnerHTML(nsAString& aInnerHTML, ErrorResult& aError)
{
  






  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
    return;
  }

  nsGenericHTMLElement::GetInnerHTML(aInnerHTML, aError);
}

JSObject*
HTMLElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return dom::HTMLElementBinding::Wrap(aCx, aScope, this);
}

} 
} 



nsGenericHTMLElement*
NS_NewHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser)
{
  return new mozilla::dom::HTMLElement(aNodeInfo);
}
