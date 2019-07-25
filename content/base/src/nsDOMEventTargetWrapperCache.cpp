






































#include "nsContentUtils.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsIDocument.h"
#include "nsIJSContextStack.h"
#include "nsServiceManagerUtils.h"
#include "nsDOMJSUtils.h"
#include "nsWrapperCacheInlines.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMEventTargetWrapperCache)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(nsDOMEventTargetWrapperCache)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMEventTargetWrapperCache,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMEventTargetWrapperCache,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMEventTargetWrapperCache)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsDOMEventTargetWrapperCache, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsDOMEventTargetWrapperCache, nsDOMEventTargetHelper)

void
nsDOMEventTargetWrapperCache::Init(JSContext* aCx)
{
  
  JSContext* cx = aCx;
  if (!cx) {
    nsIJSContextStack* stack = nsContentUtils::ThreadJSContextStack();

    if (!stack)
      return;

    if (NS_FAILED(stack->Peek(&cx)) || !cx)
      return;
  }

  NS_ASSERTION(cx, "Should have returned earlier ...");
  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (context) {
    mScriptContext = context;
    nsCOMPtr<nsPIDOMWindow> window =
      do_QueryInterface(context->GetGlobalObject());
    if (window)
      mOwner = window->GetCurrentInnerWindow();
  }
}
