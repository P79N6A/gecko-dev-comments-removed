







#include "nsDOMSettableTokenList.h"
#include "mozilla/dom/DOMSettableTokenListBinding.h"
#include "mozilla/dom/Element.h"

void
nsDOMSettableTokenList::SetValue(const nsAString& aValue, mozilla::ErrorResult& rv)
{
  if (!mElement) {
    return;
  }

  rv = mElement->SetAttr(kNameSpaceID_None, mAttrAtom, aValue, true);
}

JSObject*
nsDOMSettableTokenList::WrapObject(JSContext *cx, JSObject *scope)
{
  return mozilla::dom::DOMSettableTokenListBinding::Wrap(cx, scope, this);
}
