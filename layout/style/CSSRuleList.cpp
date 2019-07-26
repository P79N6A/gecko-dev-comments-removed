





#include "mozilla/dom/CSSRuleList.h"

#include "mozilla/dom/CSSRuleListBinding.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(CSSRuleList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CSSRuleList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(CSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(CSSRuleList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CSSRuleList)

 JSObject*
CSSRuleList::WrapObject(JSContext* aCx)
{
  return CSSRuleListBinding::Wrap(aCx, this);
}

} 
} 
