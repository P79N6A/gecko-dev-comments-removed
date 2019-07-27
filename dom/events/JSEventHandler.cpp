



#include "nsJSUtils.h"
#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsIMutableArray.h"
#include "nsVariant.h"
#include "nsIDOMBeforeUnloadEvent.h"
#include "nsGkAtoms.h"
#include "xpcpublic.h"
#include "nsJSEnvironment.h"
#include "nsDOMJSUtils.h"
#include "WorkerPrivate.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/HoldDropJSObjects.h"
#include "mozilla/JSEventHandler.h"
#include "mozilla/Likely.h"
#include "mozilla/dom/ErrorEvent.h"

namespace mozilla {

using namespace dom;

JSEventHandler::JSEventHandler(nsISupports* aTarget,
                               nsIAtom* aType,
                               const TypedEventHandler& aTypedHandler)
  : mEventName(aType)
  , mTypedHandler(aTypedHandler)
{
  nsCOMPtr<nsISupports> base = do_QueryInterface(aTarget);
  mTarget = base.get();
  
  HoldJSObjects(this);
}

JSEventHandler::~JSEventHandler()
{
  NS_ASSERTION(!mTarget, "Should have called Disconnect()!");
  DropJSObjects(this);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(JSEventHandler)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(JSEventHandler)
  tmp->mTypedHandler.ForgetHandler();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INTERNAL(JSEventHandler)
  if (MOZ_UNLIKELY(cb.WantDebugInfo()) && tmp->mEventName) {
    nsAutoCString name;
    name.AppendLiteral("JSEventHandler handlerName=");
    name.Append(
      NS_ConvertUTF16toUTF8(nsDependentAtomString(tmp->mEventName)).get());
    cb.DescribeRefCountedNode(tmp->mRefCnt.get(), name.get());
  } else {
    NS_IMPL_CYCLE_COLLECTION_DESCRIBE(JSEventHandler, tmp->mRefCnt.get())
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mTypedHandler.Ptr())
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(JSEventHandler)
  if (tmp->IsBlackForCC()) {
    return true;
  }
  
  if (tmp->mTarget) {
    nsXPCOMCycleCollectionParticipant* cp = nullptr;
    CallQueryInterface(tmp->mTarget, &cp);
    nsISupports* canonical = nullptr;
    tmp->mTarget->QueryInterface(NS_GET_IID(nsCycleCollectionISupports),
                                 reinterpret_cast<void**>(&canonical));
    
    
    if (cp && canonical && cp->CanSkip(canonical, true)) {
      return tmp->IsBlackForCC();
    }
  }
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(JSEventHandler)
  return tmp->IsBlackForCC();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(JSEventHandler)
  return tmp->IsBlackForCC();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(JSEventHandler)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(JSEventHandler)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(JSEventHandler)
NS_IMPL_CYCLE_COLLECTING_RELEASE(JSEventHandler)

bool
JSEventHandler::IsBlackForCC()
{
  
  
  return !mTypedHandler.HasEventHandler() ||
         !mTypedHandler.Ptr()->HasGrayCallable();
}

nsresult
JSEventHandler::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<EventTarget> target = do_QueryInterface(mTarget);
  if (!target || !mTypedHandler.HasEventHandler() ||
      !GetTypedEventHandler().Ptr()->CallbackPreserveColor()) {
    return NS_ERROR_FAILURE;
  }

  Event* event = aEvent->InternalDOMEvent();
  bool isMainThread = event->IsMainThreadEvent();
  bool isChromeHandler =
    isMainThread ?
      nsContentUtils::ObjectPrincipal(
        GetTypedEventHandler().Ptr()->CallbackPreserveColor()) ==
        nsContentUtils::GetSystemPrincipal() :
      mozilla::dom::workers::IsCurrentThreadRunningChromeWorker();

  if (mTypedHandler.Type() == TypedEventHandler::eOnError) {
    MOZ_ASSERT_IF(mEventName, mEventName == nsGkAtoms::onerror);

    nsString errorMsg, file;
    EventOrString msgOrEvent;
    Optional<nsAString> fileName;
    Optional<uint32_t> lineNumber;
    Optional<uint32_t> columnNumber;
    Optional<JS::Handle<JS::Value>> error;

    NS_ENSURE_TRUE(aEvent, NS_ERROR_UNEXPECTED);
    ErrorEvent* scriptEvent = aEvent->InternalDOMEvent()->AsErrorEvent();
    if (scriptEvent) {
      scriptEvent->GetMessage(errorMsg);
      msgOrEvent.SetAsString().Rebind(errorMsg.Data(), errorMsg.Length());

      scriptEvent->GetFilename(file);
      fileName = &file;

      lineNumber.Construct();
      lineNumber.Value() = scriptEvent->Lineno();

      columnNumber.Construct();
      columnNumber.Value() = scriptEvent->Colno();

      ThreadsafeAutoJSContext cx;
      error.Construct(cx);
      scriptEvent->GetError(cx, &error.Value());
    } else {
      msgOrEvent.SetAsEvent() = aEvent->InternalDOMEvent();
    }

    nsRefPtr<OnErrorEventHandlerNonNull> handler =
      mTypedHandler.OnErrorEventHandler();
    ErrorResult rv;
    bool handled = handler->Call(mTarget, msgOrEvent, fileName, lineNumber,
                                 columnNumber, error, rv);
    if (rv.Failed()) {
      return rv.StealNSResult();
    }

    if (handled) {
      event->PreventDefaultInternal(isChromeHandler);
    }
    return NS_OK;
  }

  if (mTypedHandler.Type() == TypedEventHandler::eOnBeforeUnload) {
    MOZ_ASSERT(mEventName == nsGkAtoms::onbeforeunload);

    nsRefPtr<OnBeforeUnloadEventHandlerNonNull> handler =
      mTypedHandler.OnBeforeUnloadEventHandler();
    ErrorResult rv;
    nsString retval;
    handler->Call(mTarget, *(aEvent->InternalDOMEvent()), retval, rv);
    if (rv.Failed()) {
      return rv.StealNSResult();
    }

    nsCOMPtr<nsIDOMBeforeUnloadEvent> beforeUnload = do_QueryInterface(aEvent);
    NS_ENSURE_STATE(beforeUnload);

    if (!DOMStringIsNull(retval)) {
      event->PreventDefaultInternal(isChromeHandler);

      nsAutoString text;
      beforeUnload->GetReturnValue(text);

      
      
      
      if (text.IsEmpty()) {
        beforeUnload->SetReturnValue(retval);
      }
    }

    return NS_OK;
  }

  MOZ_ASSERT(mTypedHandler.Type() == TypedEventHandler::eNormal);
  ErrorResult rv;
  nsRefPtr<EventHandlerNonNull> handler = mTypedHandler.NormalEventHandler();
  JS::Rooted<JS::Value> retval(CycleCollectedJSRuntime::Get()->Runtime());
  handler->Call(mTarget, *(aEvent->InternalDOMEvent()), &retval, rv);
  if (rv.Failed()) {
    return rv.StealNSResult();
  }

  
  
  
  if (retval.isBoolean() &&
      retval.toBoolean() == (mEventName == nsGkAtoms::onerror ||
                             mEventName == nsGkAtoms::onmouseover)) {
    event->PreventDefaultInternal(isChromeHandler);
  }

  return NS_OK;
}

} 

using namespace mozilla;





nsresult
NS_NewJSEventHandler(nsISupports* aTarget,
                     nsIAtom* aEventType,
                     const TypedEventHandler& aTypedHandler,
                     JSEventHandler** aReturn)
{
  NS_ENSURE_ARG(aEventType || !NS_IsMainThread());
  JSEventHandler* it =
    new JSEventHandler(aTarget, aEventType, aTypedHandler);
  NS_ADDREF(*aReturn = it);

  return NS_OK;
}
