





#include "nsCxPusher.h"

#include "nsIScriptContext.h"
#include "mozilla/dom/EventTarget.h"
#include "nsJSUtils.h"
#include "nsDOMJSUtils.h"
#include "mozilla/Util.h"
#include "xpcprivate.h"

using mozilla::dom::EventTarget;
using mozilla::DebugOnly;

NS_EXPORT
nsCxPusher::nsCxPusher()
    : mScriptIsRunning(false),
      mPushedSomething(false)
{
}

NS_EXPORT
nsCxPusher::~nsCxPusher()
{
  Pop();
}

bool
nsCxPusher::Push(EventTarget *aCurrentTarget)
{
  if (mPushedSomething) {
    NS_ERROR("Whaaa! No double pushing with nsCxPusher::Push()!");

    return false;
  }

  NS_ENSURE_TRUE(aCurrentTarget, false);
  nsresult rv;
  nsIScriptContext* scx =
    aCurrentTarget->GetContextForEventHandlers(&rv);
#ifdef DEBUG_smaug
  NS_ENSURE_SUCCESS(rv, false);
#else
  if(NS_FAILED(rv)) {
    return false;
  }
#endif

  if (!scx) {
    
    JSContext* cx = aCurrentTarget->GetJSContextForEventHandlers();
    if (cx) {
      DoPush(cx);
    }

    
    
    return true;
  }

  JSContext* cx = scx ? scx->GetNativeContext() : nullptr;

  
  
  
  
  Push(cx);
  return true;
}

bool
nsCxPusher::RePush(EventTarget *aCurrentTarget)
{
  if (!mPushedSomething) {
    return Push(aCurrentTarget);
  }

  if (aCurrentTarget) {
    nsresult rv;
    nsIScriptContext* scx =
      aCurrentTarget->GetContextForEventHandlers(&rv);
    if (NS_FAILED(rv)) {
      Pop();
      return false;
    }

    
    
    if (scx && scx == mScx &&
        scx->GetNativeContext()) {
      return true;
    }
  }

  Pop();
  return Push(aCurrentTarget);
}

NS_EXPORT_(void)
nsCxPusher::Push(JSContext *cx)
{
  MOZ_ASSERT(!mPushedSomething, "No double pushing with nsCxPusher::Push()!");
  MOZ_ASSERT(cx);

  DoPush(cx);
}

void
nsCxPusher::DoPush(JSContext* cx)
{
  
  
  
  if (cx)
    mScx = GetScriptContextFromJSContext(cx);

  
  if (cx && nsJSUtils::GetDynamicScriptContext(cx) &&
      xpc::IsJSContextOnStack(cx))
  {
    
    
    mScriptIsRunning = true;
  }

  if (!xpc::PushJSContext(cx)) {
    MOZ_CRASH();
  }

  
  
  
  if (cx) {
    mAutoRequest.construct(cx);
    xpc_UnmarkGrayContext(cx);
  }

  mPushedSomething = true;
#ifdef DEBUG
  mPushedContext = cx;
  if (cx)
    mCompartmentDepthOnEntry = js::GetEnterCompartmentDepth(cx);
#endif
}

void
nsCxPusher::PushNull()
{
  DoPush(nullptr);
}

NS_EXPORT_(void)
nsCxPusher::Pop()
{
  if (!mPushedSomething) {
    mScx = nullptr;
    mPushedSomething = false;

    NS_ASSERTION(!mScriptIsRunning, "Huh, this can't be happening, "
                 "mScriptIsRunning can't be set here!");

    return;
  }

  
  mAutoRequest.destroyIfConstructed();

  
  
  
  
  
  MOZ_ASSERT_IF(mPushedContext, mCompartmentDepthOnEntry ==
                                js::GetEnterCompartmentDepth(mPushedContext));
  DebugOnly<JSContext*> stackTop;
  MOZ_ASSERT(mPushedContext == nsXPConnect::XPConnect()->GetCurrentJSContext());
  xpc::PopJSContext();

  if (!mScriptIsRunning && mScx) {
    
    

    mScx->ScriptEvaluated(true);
  }

  mScx = nullptr;
  mScriptIsRunning = false;
  mPushedSomething = false;
}

namespace mozilla {

AutoJSContext::AutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(false MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

AutoJSContext::AutoJSContext(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mCx(nullptr)
{
  Init(aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT);
}

void
AutoJSContext::Init(bool aSafe MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
{
  MOZ_ASSERT(!mCx, "mCx should not be initialized!");

  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  nsXPConnect *xpc = nsXPConnect::XPConnect();
  if (!aSafe) {
    mCx = xpc->GetCurrentJSContext();
  }

  if (!mCx) {
    mCx = xpc->GetSafeJSContext();
    mPusher.Push(mCx);
  }
}

AutoJSContext::operator JSContext*() const
{
  return mCx;
}

AutoSafeJSContext::AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : AutoJSContext(true MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
{
}

AutoPushJSContext::AutoPushJSContext(JSContext *aCx) : mCx(aCx)
{
  if (mCx && mCx != nsXPConnect::XPConnect()->GetCurrentJSContext()) {
    mPusher.Push(mCx);
  }
}

} 
