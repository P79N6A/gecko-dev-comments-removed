





#include "mozilla/DeferredFinalize.h"

#include "mozilla/Assertions.h"
#include "mozilla/CycleCollectedJSRuntime.h"

void
mozilla::DeferredFinalize(nsISupports* aSupports)
{
  CycleCollectedJSRuntime* rt = CycleCollectedJSRuntime::Get();
  MOZ_ASSERT(rt, "Should have a CycleCollectedJSRuntime by now");
  rt->DeferredFinalize(aSupports);
}

void
mozilla::DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                          DeferredFinalizeFunction aFunc,
                          void* aThing)
{
  CycleCollectedJSRuntime* rt = CycleCollectedJSRuntime::Get();
  MOZ_ASSERT(rt, "Should have a CycleCollectedJSRuntime by now");
  rt->DeferredFinalize(aAppendFunc, aFunc, aThing);
}
