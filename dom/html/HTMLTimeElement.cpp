





#include "HTMLTimeElement.h"
#include "mozilla/dom/HTMLTimeElementBinding.h"
#include "nsGenericHTMLElement.h"
#include "nsVariant.h"
#include "nsGkAtoms.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Time)

namespace mozilla {
namespace dom {

HTMLTimeElement::HTMLTimeElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLTimeElement::~HTMLTimeElement()
{
}

NS_IMPL_ELEMENT_CLONE(HTMLTimeElement)

JSObject*
HTMLTimeElement::WrapNode(JSContext* cx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLTimeElementBinding::Wrap(cx, this, aGivenProto);
}

void
HTMLTimeElement::GetItemValueText(DOMString& text)
{
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::datetime)) {
    GetDateTime(text);
  } else {
    ErrorResult rv;
    GetTextContentInternal(text, rv);
  }
}

void
HTMLTimeElement::SetItemValueText(const nsAString& text)
{
  ErrorResult rv;
  SetDateTime(text, rv);
}

} 
} 
