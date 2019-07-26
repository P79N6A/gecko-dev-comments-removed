





#include "nsICSSRuleList.h"

#include "mozilla/dom/CSSRuleListBinding.h"

using namespace mozilla;


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(nsICSSRuleList)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsICSSRuleList)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsICSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSRuleList)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsICSSRuleList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsICSSRuleList)

 JSObject*
nsICSSRuleList::WrapObject(JSContext* aCx)
{
  return dom::CSSRuleListBinding::Wrap(aCx, this);
}
