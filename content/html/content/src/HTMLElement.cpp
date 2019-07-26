




#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

class HTMLElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLElement();

  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo* aNodeInfo,
                         nsINode** aResult) const MOZ_OVERRIDE;

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

HTMLElement::HTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLElement::~HTMLElement()
{
}

NS_IMPL_ELEMENT_CLONE(HTMLElement)

NS_IMETHODIMP
HTMLElement::GetInnerHTML(nsAString& aInnerHTML)
{
  






  if (mNodeInfo->Equals(nsGkAtoms::xmp) ||
      mNodeInfo->Equals(nsGkAtoms::plaintext)) {
    nsContentUtils::GetNodeTextContent(this, false, aInnerHTML);
    return NS_OK;
  }

  return nsGenericHTMLElement::GetInnerHTML(aInnerHTML);
}

JSObject*
HTMLElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
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
