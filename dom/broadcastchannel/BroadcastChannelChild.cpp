




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
  MOZ_ASSERT(!mBC);
}

bool
BroadcastChannelChild::RecvNotify(const nsString& aMessage)
{
  nsCOMPtr<DOMEventTargetHelper> helper = mBC;
  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(helper);

  
  if (!eventTarget) {
    return true;
  }

  
  
  if (NS_FAILED(mBC->CheckInnerWindowCorrectness())) {
    return true;
  }

  if (NS_IsMainThread()) {
    AutoJSAPI autoJS;
    if (!autoJS.Init(mBC->GetParentObject())) {
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
    MessageEvent::Constructor(mBC, NS_LITERAL_STRING("message"), init, rv);
  if (rv.Failed()) {
    NS_WARNING("Failed to create a MessageEvent object.");
    return;
  }

  event->SetTrusted(true);

  bool status;
  mBC->DispatchEvent(static_cast<Event*>(event.get()), &status);
}

void
BroadcastChannelChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

} 
} 
