





#include "mozilla/DebuggerOnGCRunnable.h"

#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/Move.h"
#include "js/Debug.h"

namespace mozilla {

 NS_METHOD
DebuggerOnGCRunnable::Enqueue(JSRuntime* aRt, const JS::GCDescription& aDesc)
{
  auto gcEvent = aDesc.toGCEvent(aRt);
  if (!gcEvent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsRefPtr<DebuggerOnGCRunnable> runOnGC =
    new DebuggerOnGCRunnable(Move(gcEvent));
  return NS_DispatchToCurrentThread(runOnGC);
}

NS_IMETHODIMP
DebuggerOnGCRunnable::Run()
{
  AutoJSAPI jsapi;
  jsapi.Init();
  if (!JS::dbg::FireOnGarbageCollectionHook(jsapi.cx(), Move(mGCData))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

NS_IMETHODIMP
DebuggerOnGCRunnable::Cancel()
{
  mGCData = nullptr;
  return NS_OK;
}

} 
