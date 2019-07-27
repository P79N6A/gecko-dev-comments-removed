




#include "BroadcastChannelChild.h"
#include "BroadcastChannel.h"
#include "jsapi.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/dom/MessageEventBinding.h"
#include "mozilla/dom/StructuredCloneUtils.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerScope.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "WorkerPrivate.h"

namespace mozilla {

using namespace ipc;

namespace dom {

using namespace workers;

BroadcastChannelChild::BroadcastChannelChild(const nsAString& aOrigin)
  : mOrigin(aOrigin)
  , mActorDestroyed(false)
{
}

BroadcastChannelChild::~BroadcastChannelChild()
{
  MOZ_ASSERT(!mBC);
}

bool
BroadcastChannelChild::RecvNotify(const ClonedMessageData& aData)
{
  
  
  nsTArray<nsRefPtr<File>> files;
  if (!aData.blobsChild().IsEmpty()) {
    files.SetCapacity(aData.blobsChild().Length());

    for (uint32_t i = 0, len = aData.blobsChild().Length(); i < len; ++i) {
      nsRefPtr<FileImpl> impl =
        static_cast<BlobChild*>(aData.blobsChild()[i])->GetBlobImpl();

      nsRefPtr<File> file = new File(mBC ? mBC->GetOwner() : nullptr, impl);
      files.AppendElement(file);
    }
  }

  nsCOMPtr<DOMEventTargetHelper> helper = mBC;
  nsCOMPtr<EventTarget> eventTarget = do_QueryInterface(helper);

  
  
  if (!eventTarget || mBC->IsClosed()) {
    return true;
  }

  
  
  if (NS_FAILED(mBC->CheckInnerWindowCorrectness())) {
    return true;
  }

  AutoJSAPI jsapi;
  nsCOMPtr<nsIGlobalObject> globalObject;

  if (NS_IsMainThread()) {
    globalObject = do_QueryInterface(mBC->GetParentObject());
  } else {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(workerPrivate);
    globalObject = workerPrivate->GlobalScope();
  }

  if (!globalObject || !jsapi.Init(globalObject)) {
    NS_WARNING("Failed to initialize AutoJSAPI object.");
    return true;
  }

  JSContext* cx = jsapi.cx();

  const SerializedStructuredCloneBuffer& buffer = aData.data();
  StructuredCloneData cloneData;
  cloneData.mData = buffer.data;
  cloneData.mDataLength = buffer.dataLength;
  cloneData.mClosure.mBlobs.SwapElements(files);

  JS::Rooted<JS::Value> value(cx, JS::NullValue());
  if (cloneData.mDataLength && !ReadStructuredClone(cx, cloneData, &value)) {
    JS_ClearPendingException(cx);
    return false;
  }

  RootedDictionary<MessageEventInit> init(cx);
  init.mBubbles = false;
  init.mCancelable = false;
  init.mOrigin.Construct(mOrigin);
  init.mData = value;

  ErrorResult rv;
  nsRefPtr<MessageEvent> event =
    MessageEvent::Constructor(mBC, NS_LITERAL_STRING("message"), init, rv);
  if (rv.Failed()) {
    NS_WARNING("Failed to create a MessageEvent object.");
    return true;
  }

  event->SetTrusted(true);

  bool status;
  mBC->DispatchEvent(static_cast<Event*>(event.get()), &status);

  return true;
}

void
BroadcastChannelChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

} 
} 
