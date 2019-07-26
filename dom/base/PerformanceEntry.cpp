




#include "PerformanceEntry.h"
#include "nsIURI.h"
#include "mozilla/dom/PerformanceEntryBinding.h"

using namespace mozilla::dom;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(PerformanceEntry, mPerformance)

NS_IMPL_CYCLE_COLLECTING_ADDREF(PerformanceEntry)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PerformanceEntry)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PerformanceEntry)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

PerformanceEntry::PerformanceEntry(nsPerformance* aPerformance)
: mPerformance(aPerformance)
{
  MOZ_ASSERT(aPerformance, "Parent performance object should be provided");
  SetIsDOMBinding();
}

PerformanceEntry::~PerformanceEntry()
{
}

JSObject*
PerformanceEntry::WrapObject(JSContext* aCx)
{
  return mozilla::dom::PerformanceEntryBinding::Wrap(aCx, this);
}
