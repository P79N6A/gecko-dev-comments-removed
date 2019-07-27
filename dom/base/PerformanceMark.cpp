





#include "PerformanceMark.h"
#include "MainThreadUtils.h"
#include "mozilla/dom/PerformanceMarkBinding.h"

using namespace mozilla::dom;

PerformanceMark::PerformanceMark(nsISupports* aParent,
                                 const nsAString& aName,
                                 DOMHighResTimeStamp aStartTime)
  : PerformanceEntry(aParent, aName, NS_LITERAL_STRING("mark"))
  , mStartTime(aStartTime)
{
  
  MOZ_ASSERT(mParent || !NS_IsMainThread());
}

PerformanceMark::~PerformanceMark()
{
}

JSObject*
PerformanceMark::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return PerformanceMarkBinding::Wrap(aCx, this, aGivenProto);
}
