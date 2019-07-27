




#include "mozilla/dom/HTMLModElement.h"
#include "mozilla/dom/HTMLModElementBinding.h"
#include "nsStyleConsts.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Mod)

namespace mozilla {
namespace dom {

HTMLModElement::HTMLModElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

HTMLModElement::~HTMLModElement()
{
}

NS_IMPL_ELEMENT_CLONE(HTMLModElement)

JSObject*
HTMLModElement::WrapNode(JSContext* aCx)
{
  return HTMLModElementBinding::Wrap(aCx, this);
}

} 
} 
