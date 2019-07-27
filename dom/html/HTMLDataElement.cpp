




#include "HTMLDataElement.h"
#include "mozilla/dom/HTMLDataElementBinding.h"
#include "nsGenericHTMLElement.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Data)

namespace mozilla {
namespace dom {

HTMLDataElement::HTMLDataElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLDataElement::~HTMLDataElement()
{
}

NS_IMPL_ELEMENT_CLONE(HTMLDataElement)

JSObject*
HTMLDataElement::WrapNode(JSContext* aCx)
{
  return HTMLDataElementBinding::Wrap(aCx, this);
}

void
HTMLDataElement::GetItemValueText(DOMString& text)
{
  GetValue(text);
}

void
HTMLDataElement::SetItemValueText(const nsAString& text)
{
  ErrorResult rv;
  SetValue(text, rv);
}

} 
} 
