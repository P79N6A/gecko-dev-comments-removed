





#include "TextLeafAccessibleWrap.h"

#include "sdnTextAccessible.h"
#include "Statistics.h"

using namespace mozilla::a11y;

IMPL_IUNKNOWN_QUERY_HEAD(TextLeafAccessibleWrap)
  if (aIID == IID_ISimpleDOMText) {
    statistics::ISimpleDOMUsed();
    *aInstancePtr = static_cast<ISimpleDOMText*>(new sdnTextAccessible(this));
    static_cast<IUnknown*>(*aInstancePtr)->AddRef();
    return S_OK;
  }
IMPL_IUNKNOWN_QUERY_TAIL_INHERITED(AccessibleWrap)
