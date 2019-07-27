





#include "PerformanceEntry.h"
#include "MainThreadUtils.h"
#include "mozilla/dom/PerformanceEntryBinding.h"
#include "nsIURI.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(PerformanceEntry, mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(PerformanceEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PerformanceEntry)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PerformanceEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

PerformanceEntry::PerformanceEntry(nsISupports* aParent,
                                   const nsAString& aName,
                                   const nsAString& aEntryType)
: mParent(aParent),
  mName(aName),
  mEntryType(aEntryType)
{
  
  MOZ_ASSERT(mParent || !NS_IsMainThread());
}

PerformanceEntry::~PerformanceEntry()
{
}

JSObject*
PerformanceEntry::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::PerformanceEntryBinding::Wrap(aCx, this, aGivenProto);
}
