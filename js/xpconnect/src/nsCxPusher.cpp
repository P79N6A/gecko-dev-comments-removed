





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
nsCxPusher::~nsCxPusher() {}

bool
nsCxPusher::Push(EventTarget *aCurrentTarget)
{
  MOZ_ASSERT(mPusher.empty());
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
      mPusher.construct(cx);
    }

    
    
    return true;
  }

  mPusher.construct(scx->GetNativeContext());
  return true;
}

bool
nsCxPusher::RePush(EventTarget *aCurrentTarget)
{
  if (mPusher.empty()) {
    return Push(aCurrentTarget);
  }

  if (aCurrentTarget) {
    nsresult rv;
    nsIScriptContext* scx =
      aCurrentTarget->GetContextForEventHandlers(&rv);
    if (NS_FAILED(rv)) {
      mPusher.destroy();
      return false;
    }

    
    
    if (scx && scx == mPusher.ref().GetScriptContext() &&
        scx->GetNativeContext()) {
      return true;
    }
  }

  mPusher.destroy();
  return Push(aCurrentTarget);
}

NS_EXPORT_(void)
nsCxPusher::Push(JSContext *cx)
{
  mPusher.construct(cx);
}

void
nsCxPusher::PushNull()
{
  
  
  mPusher.construct(static_cast<JSContext*>(nullptr),  true);
}

NS_EXPORT_(void)
nsCxPusher::Pop()
{
  if (!mPusher.empty())
    mPusher.destroy();
}

namespace mozilla {

AutoCxPusher::AutoCxPusher(JSContext* cx, bool allowNull) : mScriptIsRunning(false)
{
  MOZ_ASSERT_IF(!allowNull, cx);

  
  
  
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

#ifdef DEBUG
  mPushedContext = cx;
  mCompartmentDepthOnEntry = cx ? js::GetEnterCompartmentDepth(cx) : 0;
#endif
}

AutoCxPusher::~AutoCxPusher()
{
  
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
}

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
    mPusher.construct(mCx);
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
    mPusher.construct(mCx);
  }
}

} 
