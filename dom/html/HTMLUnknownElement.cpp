




#include "nsDocument.h"
#include "mozilla/dom/HTMLUnknownElement.h"
#include "mozilla/dom/HTMLElementBinding.h"
#include "jsapi.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Unknown)

namespace mozilla {
namespace dom {

JSObject*
HTMLUnknownElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLUnknownElementBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_ELEMENT_CLONE(HTMLUnknownElement)

} 
} 
