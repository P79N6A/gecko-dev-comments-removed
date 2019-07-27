





#include "nsCxPusher.h"

#include "nsIScriptContext.h"
#include "mozilla/dom/EventTarget.h"
#include "nsDOMJSUtils.h"
#include "xpcprivate.h"
#include "WorkerPrivate.h"

using mozilla::dom::EventTarget;
using mozilla::DebugOnly;

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

void
nsCxPusher::Push(JSContext *cx)
{
  mPusher.construct(cx);
}

void
nsCxPusher::PushNull()
{
  
  
  mPusher.construct(static_cast<JSContext*>(nullptr),  true);
}

void
nsCxPusher::Pop()
{
  if (!mPusher.empty())
    mPusher.destroy();
}

namespace mozilla {

AutoCxPusher::AutoCxPusher(JSContext* cx, bool allowNull)
{
  MOZ_ASSERT_IF(!allowNull, cx);

  
  
  
  if (cx)
    mScx = GetScriptContextFromJSContext(cx);

  XPCJSContextStack *stack = XPCJSRuntime::Get()->GetJSContextStack();
  if (!stack->Push(cx)) {
    MOZ_CRASH();
  }
  mStackDepthAfterPush = stack->Count();

#ifdef DEBUG
  mPushedContext = cx;
  mCompartmentDepthOnEntry = cx ? js::GetEnterCompartmentDepth(cx) : 0;
#endif

  
  
  if (cx) {
    mAutoRequest.construct(cx);

    
    JSObject *compartmentObject = mScx ? mScx->GetWindowProxy()
                                       : js::DefaultObjectForContextOrNull(cx);
    if (compartmentObject)
      mAutoCompartment.construct(cx, compartmentObject);
  }
}

AutoCxPusher::~AutoCxPusher()
{
  
  
  
  
  
  
  if (mScx && !mAutoCompartment.empty())
    JS_MaybeGC(nsXPConnect::XPConnect()->GetCurrentJSContext());

  
  mAutoCompartment.destroyIfConstructed();
  mAutoRequest.destroyIfConstructed();

  
  
  
  
  
  MOZ_ASSERT_IF(mPushedContext, mCompartmentDepthOnEntry ==
                                js::GetEnterCompartmentDepth(mPushedContext));
  DebugOnly<JSContext*> stackTop;
  MOZ_ASSERT(mPushedContext == nsXPConnect::XPConnect()->GetCurrentJSContext());
  XPCJSRuntime::Get()->GetJSContextStack()->Pop();
  mScx = nullptr;
}

bool
AutoCxPusher::IsStackTop() const
{
  uint32_t currentDepth = XPCJSRuntime::Get()->GetJSContextStack()->Count();
  MOZ_ASSERT(currentDepth >= mStackDepthAfterPush);
  return currentDepth == mStackDepthAfterPush;
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
  JS::AutoSuppressGCAnalysis nogc;
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

ThreadsafeAutoJSContext::ThreadsafeAutoJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  if (NS_IsMainThread()) {
    mCx = nullptr;
    mAutoJSContext.construct();
  } else {
    mCx = mozilla::dom::workers::GetCurrentThreadJSContext();
    mRequest.construct(mCx);
  }
}

ThreadsafeAutoJSContext::operator JSContext*() const
{
  if (mCx) {
    return mCx;
  } else {
    return mAutoJSContext.ref();
  }
}

AutoSafeJSContext::AutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
  : AutoJSContext(true MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  , mAc(mCx, XPCJSRuntime::Get()->GetJSContextStack()->GetSafeJSContextGlobal())
{
}

ThreadsafeAutoSafeJSContext::ThreadsafeAutoSafeJSContext(MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;

  if (NS_IsMainThread()) {
    mCx = nullptr;
    mAutoSafeJSContext.construct();
  } else {
    mCx = mozilla::dom::workers::GetCurrentThreadJSContext();
    mRequest.construct(mCx);
  }
}

ThreadsafeAutoSafeJSContext::operator JSContext*() const
{
  if (mCx) {
    return mCx;
  } else {
    return mAutoSafeJSContext.ref();
  }
}

} 
