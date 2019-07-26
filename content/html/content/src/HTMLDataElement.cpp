




#include "HTMLDataElement.h"
#include "mozilla/dom/HTMLDataElementBinding.h"
#include "nsGenericHTMLElement.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Data)

namespace mozilla {
namespace dom {

HTMLDataElement::HTMLDataElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLDataElement::~HTMLDataElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLDataElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLDataElement, Element)

NS_INTERFACE_TABLE_HEAD(HTMLDataElement)
  NS_HTML_CONTENT_INTERFACE_TABLE0(HTMLDataElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLDataElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLDataElement)

JSObject*
HTMLDataElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLDataElementBinding::Wrap(aCx, aScope, this);
}

void
HTMLDataElement::GetItemValueText(nsAString& text)
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
