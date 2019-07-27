




#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

class HTMLElement MOZ_FINAL : public nsGenericHTMLElement
{
public:
  HTMLElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual ~HTMLElement();

  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo,
                         nsINode** aResult) const MOZ_OVERRIDE;

protected:
  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;
};

HTMLElement::HTMLElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
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
    if (!nsContentUtils::GetNodeTextContent(this, false, aInnerHTML)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    return NS_OK;
  }

  return nsGenericHTMLElement::GetInnerHTML(aInnerHTML);
}

JSObject*
HTMLElement::WrapNode(JSContext *aCx)
{
  return dom::HTMLElementBinding::Wrap(aCx, this);
}

} 
} 



nsGenericHTMLElement*
NS_NewHTMLElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  mozilla::dom::FromParser aFromParser)
{
  return new mozilla::dom::HTMLElement(aNodeInfo);
}
