




#include "mozilla/dom/HTMLModElement.h"
#include "mozilla/dom/HTMLModElementBinding.h"
#include "nsStyleConsts.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Mod)

namespace mozilla {
namespace dom {

HTMLModElement::HTMLModElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLModElement::~HTMLModElement()
{
}


NS_IMPL_ADDREF_INHERITED(HTMLModElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLModElement, Element)


NS_INTERFACE_MAP_BEGIN(HTMLModElement)
  NS_HTML_CONTENT_INTERFACES(nsGenericHTMLElement)
NS_ELEMENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLModElement)

JSObject*
HTMLModElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLModElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
