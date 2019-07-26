




#include "nsDocument.h"
#include "mozilla/dom/HTMLUnknownElement.h"
#include "mozilla/dom/HTMLElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Unknown)

namespace mozilla {
namespace dom {

JSObject*
HTMLUnknownElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  JS::Rooted<JSObject*> obj(aCx,
    HTMLUnknownElementBinding::Wrap(aCx, aScope, this));
  if (obj && Substring(NodeName(), 0, 2).LowerCaseEqualsLiteral("x-")) {
    
    JSAutoCompartment ac(aCx, obj);
    nsDocument* document = static_cast<nsDocument*>(OwnerDoc());
    JS::Rooted<JSObject*> prototype(aCx);
    document->GetCustomPrototype(LocalName(), &prototype);
    if (prototype) {
      NS_ENSURE_TRUE(JS_WrapObject(aCx, &prototype), nullptr);
      NS_ENSURE_TRUE(JS_SetPrototype(aCx, obj, prototype), nullptr);
    }
  }
  return obj;
}

NS_IMPL_ELEMENT_CLONE(HTMLUnknownElement)

} 
} 
