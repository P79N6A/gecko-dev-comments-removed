




#include "BroadcastChannelChild.h"
#include "BroadcastChannel.h"
#include "jsapi.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/dom/MessageEventBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "WorkerPrivate.h"

namespace mozilla {

using namespace ipc;

namespace dom {

using namespace workers;

BroadcastChannelChild::BroadcastChannelChild(const nsAString& aOrigin,
                                             const nsAString& aChannel)
  : mOrigin(aOrigin)
  , mChannel(aChannel)
  , mActorDestroyed(false)
{
}

BroadcastChannelChild::~BroadcastChannelChild()
{
  MOZ_ASSERT(!mEventTarget);
}

bool
BroadcastChannelChild::RecvNotify(const nsString& aMessage)
{
  
  if (!mEventTarget) {
    return true;
  }

  if (NS_IsMainThread()) {
    DOMEventTargetHelper* deth =
      DOMEventTargetHelper::FromSupports(mEventTarget);
    MOZ_ASSERT(deth);

    AutoJSAPI autoJS;
    if (!autoJS.Init(deth->GetParentObject())) {
      NS_WARNING("Dropping message");
      return true;
    }

    Notify(autoJS.cx(), aMessage);
    return true;
  }

  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  Notify(workerPrivate->GetJSContext(), aMessage);
  return true;
}

void
BroadcastChannelChild::Notify(JSContext* aCx, const nsString& aMessage)
{
  JS::Rooted<JSString*> str(aCx,
                            JS_NewUCStringCopyN(aCx, aMessage.get(),
                                                aMessage.Length()));
  if (!str) {
    
    NS_WARNING("Failed allocating a JS string. Probably OOM.");
    return;
  }

  JS::Rooted<JS::Value> value(aCx, JS::StringValue(str));

  RootedDictionary<MessageEventInit> init(aCx);
  init.mBubbles = false;
  init.mCancelable = false;
  init.mOrigin.Construct(mOrigin);
  init.mData = value;

  ErrorResult rv;
  nsRefPtr<MessageEvent> event =
    MessageEvent::Constructor(mEventTarget, NS_LITERAL_STRING("message"),
                              init, rv);
  if (rv.Failed()) {
    NS_WARNING("Failed to create a MessageEvent object.");
    return;
  }

  event->SetTrusted(true);

  bool status;
  mEventTarget->DispatchEvent(static_cast<Event*>(event.get()), &status);
}

void
BroadcastChannelChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

} 
} 
