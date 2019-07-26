




#include "mozilla/dom/HTMLSpanElement.h"
#include "mozilla/dom/HTMLSpanElementBinding.h"

#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Span)

namespace mozilla {
namespace dom {

HTMLSpanElement::~HTMLSpanElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLSpanElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLSpanElement, Element)


NS_INTERFACE_MAP_BEGIN(HTMLSpanElement)
  NS_HTML_CONTENT_INTERFACES(nsGenericHTMLElement)
NS_ELEMENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLSpanElement)

JSObject*
HTMLSpanElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLSpanElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
