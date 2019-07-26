




#include "mozilla/dom/HTMLModElement.h"
#include "mozilla/dom/HTMLModElementBinding.h"
#include "nsStyleConsts.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Mod)

namespace mozilla {
namespace dom {

HTMLModElement::HTMLModElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLModElement::~HTMLModElement()
{
}


NS_IMPL_ADDREF_INHERITED(HTMLModElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLModElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLModElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLModElement,
                                   nsIDOMHTMLModElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLModElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLModElement)


NS_IMPL_URI_ATTR(HTMLModElement, Cite, cite)
NS_IMPL_STRING_ATTR(HTMLModElement, DateTime, datetime)

JSObject*
HTMLModElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLModElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
