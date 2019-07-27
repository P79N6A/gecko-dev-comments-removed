




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

NS_IMPL_ELEMENT_CLONE(HTMLSpanElement)

JSObject*
HTMLSpanElement::WrapNode(JSContext *aCx)
{
  return HTMLSpanElementBinding::Wrap(aCx, this);
}

} 
} 
