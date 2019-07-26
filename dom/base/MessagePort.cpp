




#include "MessagePort.h"
#include "mozilla/dom/MessageChannel.h"
#include "mozilla/dom/MessagePortBinding.h"
#include "mozilla/dom/StructuredCloneTags.h"
#include "nsGlobalWindow.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsPresContext.h"
#include "nsDOMEvent.h"

#include "nsIDocument.h"
#include "nsIDOMFile.h"
#include "nsIDOMFileList.h"
#include "nsIDOMMessageEvent.h"
#include "nsIPresShell.h"

namespace mozilla {
namespace dom {

class DispatchEventRunnable : public nsRunnable
{
  friend class MessagePort;

  public:
    DispatchEventRunnable(MessagePort* aPort)
      : mPort(aPort)
    {
    }

    NS_IMETHOD
    Run()
    {
      nsRefPtr<DispatchEventRunnable> mKungFuDeathGrip(this);

      mPort->mDispatchRunnable = nullptr;
      mPort->Dispatch();

      return NS_OK;
    }

  private:
    nsRefPtr<MessagePort> mPort;
};

class PostMessageRunnable : public nsRunnable
{
  friend class MessagePort;

  public:
    NS_DECL_NSIRUNNABLE

    PostMessageRunnable()
      : mMessage(nullptr)
      , mMessageLen(0)
    {
    }

    ~PostMessageRunnable()
    {
      
      if (mMessage) {
        JSAutoStructuredCloneBuffer buffer;
        buffer.adopt(mMessage, mMessageLen);
      }
    }

    void SetJSData(JSAutoStructuredCloneBuffer& aBuffer)
    {
      NS_ASSERTION(!mMessage && mMessageLen == 0, "Don't call twice!");
      aBuffer.steal(&mMessage, &mMessageLen);
    }

    bool StoreISupports(nsISupports* aSupports)
    {
      mSupportsArray.AppendElement(aSupports);
      return true;
    }

    void Dispatch(MessagePort* aPort)
    {
      mPort = aPort;
      NS_DispatchToCurrentThread(this);
    }

  private:
    nsRefPtr<MessagePort> mPort;
    uint64_t* mMessage;
    size_t mMessageLen;

    nsTArray<nsCOMPtr<nsISupports> > mSupportsArray;
};

namespace {

struct StructuredCloneInfo
{
  PostMessageRunnable* mEvent;
  MessagePort* mPort;
};

static JSObject*
PostMessageReadStructuredClone(JSContext* cx,
                               JSStructuredCloneReader* reader,
                               uint32_t tag,
                               uint32_t data,
                               void* closure)
{
  StructuredCloneInfo* scInfo = static_cast<StructuredCloneInfo*>(closure);
  NS_ASSERTION(scInfo, "Must have scInfo!");

  if (tag == SCTAG_DOM_BLOB || tag == SCTAG_DOM_FILELIST) {
    NS_ASSERTION(!data, "Data should be empty");

    nsISupports* supports;
    if (JS_ReadBytes(reader, &supports, sizeof(supports))) {
      JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
      if (global) {
        JS::Rooted<JS::Value> val(cx);
        nsCOMPtr<nsIXPConnectJSObjectHolder> wrapper;
        if (NS_SUCCEEDED(nsContentUtils::WrapNative(cx, global, supports,
                                                    val.address(),
                                                    getter_AddRefs(wrapper)))) {
          return JSVAL_TO_OBJECT(val);
        }
      }
    }
  }

  if (tag == SCTAG_DOM_MESSAGEPORT) {
    NS_ASSERTION(!data, "Data should be empty");

    MessagePort* port;
    if (JS_ReadBytes(reader, &port, sizeof(port))) {
      JS::Rooted<JSObject*> global(cx, JS::CurrentGlobalOrNull(cx));
      if (global) {
        JS::Rooted<JSObject*> obj(cx, port->WrapObject(cx, global));
        if (JS_WrapObject(cx, obj.address())) {
          port->BindToOwner(scInfo->mPort->GetOwner());
          return obj;
        }
      }
    }
  }

  const JSStructuredCloneCallbacks* runtimeCallbacks =
    js::GetContextStructuredCloneCallbacks(cx);

  if (runtimeCallbacks) {
    return runtimeCallbacks->read(cx, reader, tag, data, nullptr);
  }

  return nullptr;
}

static bool
PostMessageWriteStructuredClone(JSContext* cx,
                                JSStructuredCloneWriter* writer,
                                JS::Handle<JSObject*> obj,
                                void *closure)
{
  StructuredCloneInfo* scInfo = static_cast<StructuredCloneInfo*>(closure);
  NS_ASSERTION(scInfo, "Must have scInfo!");

  nsCOMPtr<nsIXPConnectWrappedNative> wrappedNative;
  nsContentUtils::XPConnect()->
    GetWrappedNativeOfJSObject(cx, obj, getter_AddRefs(wrappedNative));
  if (wrappedNative) {
    uint32_t scTag = 0;
    nsISupports* supports = wrappedNative->Native();

    nsCOMPtr<nsIDOMBlob> blob = do_QueryInterface(supports);
    if (blob) {
      scTag = SCTAG_DOM_BLOB;
    }

    nsCOMPtr<nsIDOMFileList> list = do_QueryInterface(supports);
    if (list) {
      scTag = SCTAG_DOM_FILELIST;
    }

    if (scTag) {
      return JS_WriteUint32Pair(writer, scTag, 0) &&
             JS_WriteBytes(writer, &supports, sizeof(supports)) &&
             scInfo->mEvent->StoreISupports(supports);
    }
  }

  MessagePort* port = nullptr;
  nsresult rv = UNWRAP_OBJECT(MessagePort, cx, obj, port);
  if (NS_SUCCEEDED(rv)) {
    nsRefPtr<MessagePort> newPort = port->Clone();

    return JS_WriteUint32Pair(writer, SCTAG_DOM_MESSAGEPORT, 0) &&
           JS_WriteBytes(writer, &newPort, sizeof(newPort)) &&
           scInfo->mEvent->StoreISupports(newPort);
  }

  const JSStructuredCloneCallbacks* runtimeCallbacks =
    js::GetContextStructuredCloneCallbacks(cx);

  if (runtimeCallbacks) {
    return runtimeCallbacks->write(cx, writer, obj, nullptr);
  }

  return false;
}

JSStructuredCloneCallbacks kPostMessageCallbacks = {
  PostMessageReadStructuredClone,
  PostMessageWriteStructuredClone,
  nullptr
};

} 

NS_IMETHODIMP
PostMessageRunnable::Run()
{
  MOZ_ASSERT(mPort);

  
  JSAutoStructuredCloneBuffer buffer;
  buffer.adopt(mMessage, mMessageLen);
  mMessage = nullptr;
  mMessageLen = 0;

  
  nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(mPort->GetOwner());
  NS_ENSURE_STATE(sgo);
  nsCOMPtr<nsIScriptContext> scriptContext = sgo->GetContext();
  AutoPushJSContext cx(scriptContext ? scriptContext->GetNativeContext()
                                     : nsContentUtils::GetSafeJSContext());

  MOZ_ASSERT(cx);

  
  JS::Rooted<JS::Value> messageData(cx);
  {
    StructuredCloneInfo scInfo;
    scInfo.mEvent = this;
    scInfo.mPort = mPort;

    if (!buffer.read(cx, messageData.address(), &kPostMessageCallbacks,
                     &scInfo)) {
      return NS_ERROR_DOM_DATA_CLONE_ERR;
    }
  }

  
  nsIDocument* doc = mPort->GetOwner()->GetExtantDoc();
  if (!doc) {
    return NS_OK;
  }

  ErrorResult error;
  nsRefPtr<nsDOMEvent> event =
    doc->CreateEvent(NS_LITERAL_STRING("MessageEvent"), error);
  if (error.Failed()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMMessageEvent> message = do_QueryInterface(event);
  nsresult rv = message->InitMessageEvent(NS_LITERAL_STRING("message"),
                                          false ,
                                          true ,
                                          messageData,
                                          EmptyString(),
                                          EmptyString(),
                                          mPort->GetOwner());
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  message->SetTrusted(true);

  bool status;
  mPort->DispatchEvent(event, &status);
  return status ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(MessagePort)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(MessagePort,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mEntangledPort)

  
  
  while (!tmp->mMessageQueue.IsEmpty()) {
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageQueue[0]->mPort);
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageQueue[0]->mSupportsArray);
    tmp->mMessageQueue.RemoveElementAt(0);
  }

  if (tmp->mDispatchRunnable) {
    NS_IMPL_CYCLE_COLLECTION_UNLINK(mDispatchRunnable->mPort);
  }

NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(MessagePort,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEntangledPort)

  
  
  for (uint32_t i = 0, len = tmp->mMessageQueue.Length(); i < len; ++i) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessageQueue[i]->mPort);
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessageQueue[i]->mSupportsArray);
  }

  if (tmp->mDispatchRunnable) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDispatchRunnable->mPort);
  }

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MessagePort)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(MessagePort, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MessagePort, nsDOMEventTargetHelper)

MessagePort::MessagePort(nsPIDOMWindow* aWindow)
  : nsDOMEventTargetHelper(aWindow)
  , mMessageQueueEnabled(false)
{
  MOZ_COUNT_CTOR(MessagePort);
  SetIsDOMBinding();
}

MessagePort::~MessagePort()
{
  MOZ_COUNT_DTOR(MessagePort);
  Close();
}

JSObject*
MessagePort::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return MessagePortBinding::Wrap(aCx, aScope, this);
}

void
MessagePort::PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                         const Optional<JS::Handle<JS::Value> >& aTransfer,
                         ErrorResult& aRv)
{
  nsRefPtr<PostMessageRunnable> event = new PostMessageRunnable();

  
  
  JSAutoStructuredCloneBuffer buffer;
  StructuredCloneInfo scInfo;
  scInfo.mEvent = event;
  scInfo.mPort = this;

  JS::Handle<JS::Value> transferable = aTransfer.WasPassed()
                                       ? aTransfer.Value()
                                       : JS::UndefinedHandleValue;

  if (!buffer.write(aCx, aMessage, transferable, &kPostMessageCallbacks,
                    &scInfo)) {
    aRv.Throw(NS_ERROR_DOM_DATA_CLONE_ERR);
    return;
  }

  event->SetJSData(buffer);

  if (!mEntangledPort) {
    return;
  }

  mEntangledPort->mMessageQueue.AppendElement(event);
  mEntangledPort->Dispatch();
}

void
MessagePort::Start()
{
  if (mMessageQueueEnabled) {
    return;
  }

  mMessageQueueEnabled = true;
  Dispatch();
}

void
MessagePort::Dispatch()
{
  if (!mMessageQueueEnabled || mMessageQueue.IsEmpty() || mDispatchRunnable) {
    return;
  }

  nsRefPtr<PostMessageRunnable> event = mMessageQueue.ElementAt(0);
  mMessageQueue.RemoveElementAt(0);

  event->Dispatch(this);

  mDispatchRunnable = new DispatchEventRunnable(this);
  NS_DispatchToCurrentThread(mDispatchRunnable);
}

void
MessagePort::Close()
{
  if (!mEntangledPort) {
    return;
  }

  
  nsRefPtr<MessagePort> port = mEntangledPort;
  mEntangledPort = nullptr;

  
  port->Close();
}

EventHandlerNonNull*
MessagePort::GetOnmessage()
{
  if (NS_IsMainThread()) {
    return GetEventHandler(nsGkAtoms::onmessage, EmptyString());
  }
  return GetEventHandler(nullptr, NS_LITERAL_STRING("message"));
}

void
MessagePort::SetOnmessage(EventHandlerNonNull* aCallback)
{
  if (NS_IsMainThread()) {
    SetEventHandler(nsGkAtoms::onmessage, EmptyString(), aCallback);
  } else {
    SetEventHandler(nullptr, NS_LITERAL_STRING("message"), aCallback);
  }

  
  Start();
}

void
MessagePort::Entangle(MessagePort* aMessagePort)
{
  MOZ_ASSERT(aMessagePort);
  MOZ_ASSERT(aMessagePort != this);

  Close();

  mEntangledPort = aMessagePort;
}

already_AddRefed<MessagePort>
MessagePort::Clone()
{
  nsRefPtr<MessagePort> newPort = new MessagePort(nullptr);

  
  newPort->mMessageQueue.SwapElements(mMessageQueue);

  if (mEntangledPort) {
    nsRefPtr<MessagePort> port = mEntangledPort;
    mEntangledPort = nullptr;

    newPort->Entangle(port);
    port->Entangle(newPort);
  }

  return newPort.forget();
}

} 
} 
