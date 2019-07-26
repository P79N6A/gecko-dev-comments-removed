





#undef CreateEvent

#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventListenerManager.h"
#ifdef MOZ_B2G
#include "mozilla/Hal.h"
#endif 
#include "mozilla/HalSensor.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/JSEventHandler.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Event.h"

#include "EventListenerService.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsDOMCID.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIDocument.h"
#include "nsIDOMEventListener.h"
#include "nsIScriptGlobalObject.h"
#include "nsISupports.h"
#include "nsIXPConnect.h"
#include "nsJSUtils.h"
#include "nsNameSpaceManager.h"
#include "nsPIDOMWindow.h"
#include "nsSandboxFlags.h"
#include "xpcpublic.h"

namespace mozilla {

using namespace dom;
using namespace hal;

#define EVENT_TYPE_EQUALS(ls, type, userType, typeString, allEvents) \
  ((ls->mEventType == type &&                                        \
    (ls->mEventType != NS_USER_DEFINED_EVENT ||                      \
    (mIsMainThreadELM && ls->mTypeAtom == userType) ||               \
    (!mIsMainThreadELM && ls->mTypeString.Equals(typeString)))) ||   \
   (allEvents && ls->mAllEvents))

static const uint32_t kAllMutationBits =
  NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED |
  NS_EVENT_BITS_MUTATION_NODEINSERTED |
  NS_EVENT_BITS_MUTATION_NODEREMOVED |
  NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT |
  NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT |
  NS_EVENT_BITS_MUTATION_ATTRMODIFIED |
  NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED;

static uint32_t
MutationBitForEventType(uint32_t aEventType)
{
  switch (aEventType) {
    case NS_MUTATION_SUBTREEMODIFIED:
      return NS_EVENT_BITS_MUTATION_SUBTREEMODIFIED;
    case NS_MUTATION_NODEINSERTED:
      return NS_EVENT_BITS_MUTATION_NODEINSERTED;
    case NS_MUTATION_NODEREMOVED:
      return NS_EVENT_BITS_MUTATION_NODEREMOVED;
    case NS_MUTATION_NODEREMOVEDFROMDOCUMENT:
      return NS_EVENT_BITS_MUTATION_NODEREMOVEDFROMDOCUMENT;
    case NS_MUTATION_NODEINSERTEDINTODOCUMENT:
      return NS_EVENT_BITS_MUTATION_NODEINSERTEDINTODOCUMENT;
    case NS_MUTATION_ATTRMODIFIED:
      return NS_EVENT_BITS_MUTATION_ATTRMODIFIED;
    case NS_MUTATION_CHARACTERDATAMODIFIED:
      return NS_EVENT_BITS_MUTATION_CHARACTERDATAMODIFIED;
    default:
      break;
  }
  return 0;
}

uint32_t EventListenerManager::sMainThreadCreatedCount = 0;

EventListenerManager::EventListenerManager(EventTarget* aTarget)
  : mMayHavePaintEventListener(false)
  , mMayHaveMutationListeners(false)
  , mMayHaveCapturingListeners(false)
  , mMayHaveSystemGroupListeners(false)
  , mMayHaveAudioAvailableEventListener(false)
  , mMayHaveTouchEventListener(false)
  , mMayHaveMouseEnterLeaveEventListener(false)
  , mMayHavePointerEnterLeaveEventListener(false)
  , mClearingListeners(false)
  , mIsMainThreadELM(NS_IsMainThread())
  , mNoListenerForEvent(0)
  , mTarget(aTarget)
{
  NS_ASSERTION(aTarget, "unexpected null pointer");

  if (mIsMainThreadELM) {
    ++sMainThreadCreatedCount;
  }
}

EventListenerManager::~EventListenerManager()
{
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(!mTarget, "didn't call Disconnect");
  RemoveAllListeners();
}

void
EventListenerManager::RemoveAllListeners()
{
  if (mClearingListeners) {
    return;
  }
  mClearingListeners = true;
  mListeners.Clear();
  mClearingListeners = false;
}

void
EventListenerManager::Shutdown()
{
  Event::Shutdown();
}

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(EventListenerManager, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(EventListenerManager, Release)

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            EventListenerManager::Listener& aField,
                            const char* aName,
                            unsigned aFlags)
{
  if (MOZ_UNLIKELY(aCallback.WantDebugInfo())) {
    nsAutoCString name;
    name.AppendASCII(aName);
    if (aField.mTypeAtom) {
      name.AppendASCII(" event=");
      name.Append(nsAtomCString(aField.mTypeAtom));
      name.AppendASCII(" listenerType=");
      name.AppendInt(aField.mListenerType);
      name.AppendASCII(" ");
    }
    CycleCollectionNoteChild(aCallback, aField.mListener.GetISupports(), name.get(),
                             aFlags);
  } else {
    CycleCollectionNoteChild(aCallback, aField.mListener.GetISupports(), aName,
                             aFlags);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(EventListenerManager)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(EventListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mListeners)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(EventListenerManager)
  tmp->Disconnect();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


nsPIDOMWindow*
EventListenerManager::GetInnerWindowForTarget()
{
  nsCOMPtr<nsINode> node = do_QueryInterface(mTarget);
  if (node) {
    
    
    return node->OwnerDoc()->GetInnerWindow();
  }

  nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
  return window;
}

already_AddRefed<nsPIDOMWindow>
EventListenerManager::GetTargetAsInnerWindow() const
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mTarget);
  if (!window) {
    return nullptr;
  }

  NS_ASSERTION(window->IsInnerWindow(), "Target should not be an outer window");
  return window.forget();
}

void
EventListenerManager::AddEventListenerInternal(
                        const EventListenerHolder& aListenerHolder,
                        uint32_t aType,
                        nsIAtom* aTypeAtom,
                        const nsAString& aTypeString,
                        const EventListenerFlags& aFlags,
                        bool aHandler,
                        bool aAllEvents)
{
  MOZ_ASSERT((NS_IsMainThread() && aType && aTypeAtom) || 
             (!NS_IsMainThread() && aType && !aTypeString.IsEmpty()) || 
             aAllEvents, "Missing type"); 

  if (!aListenerHolder || mClearingListeners) {
    return;
  }

  
  
  

  Listener* listener;
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; i++) {
    listener = &mListeners.ElementAt(i);
    
    if (listener->mListenerIsHandler == aHandler &&
        listener->mFlags == aFlags &&
        EVENT_TYPE_EQUALS(listener, aType, aTypeAtom, aTypeString,
                          aAllEvents) &&
        listener->mListener == aListenerHolder) {
      return;
    }
  }

  mNoListenerForEvent = NS_EVENT_NULL;
  mNoListenerForEventAtom = nullptr;

  listener = aAllEvents ? mListeners.InsertElementAt(0) :
                          mListeners.AppendElement();
  listener->mListener = aListenerHolder;
  MOZ_ASSERT(aType < PR_UINT16_MAX);
  listener->mEventType = aType;
  listener->mTypeString = aTypeString;
  listener->mTypeAtom = aTypeAtom;
  listener->mFlags = aFlags;
  listener->mListenerIsHandler = aHandler;
  listener->mHandlerIsString = false;
  listener->mAllEvents = aAllEvents;

  
  nsCOMPtr<nsIXPConnectWrappedJS> wjs;
  if (aFlags.mListenerIsJSListener) {
    MOZ_ASSERT(!aListenerHolder.HasWebIDLCallback());
    listener->mListenerType = Listener::eJSEventListener;
  } else if (aListenerHolder.HasWebIDLCallback()) {
    listener->mListenerType = Listener::eWebIDLListener;
  } else if ((wjs = do_QueryInterface(aListenerHolder.GetXPCOMCallback()))) {
    listener->mListenerType = Listener::eWrappedJSListener;
  } else {
    listener->mListenerType = Listener::eNativeListener;
  }


  if (aFlags.mInSystemGroup) {
    mMayHaveSystemGroupListeners = true;
  }
  if (aFlags.mCapture) {
    mMayHaveCapturingListeners = true;
  }

  if (aType == NS_AFTERPAINT) {
    mMayHavePaintEventListener = true;
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      window->SetHasPaintEventListeners();
    }
  } else if (aType == NS_MOZAUDIOAVAILABLE) {
    mMayHaveAudioAvailableEventListener = true;
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      window->SetHasAudioAvailableEventListeners();
    }
  } else if (aType >= NS_MUTATION_START && aType <= NS_MUTATION_END) {
    
    
    mMayHaveMutationListeners = true;
    
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      nsCOMPtr<nsIDocument> doc = window->GetExtantDoc();
      if (doc) {
        doc->WarnOnceAbout(nsIDocument::eMutationEvent);
      }
      
      
      window->SetMutationListeners((aType == NS_MUTATION_SUBTREEMODIFIED) ?
                                   kAllMutationBits :
                                   MutationBitForEventType(aType));
    }
  } else if (aTypeAtom == nsGkAtoms::ondeviceorientation) {
    EnableDevice(NS_DEVICE_ORIENTATION);
  } else if (aTypeAtom == nsGkAtoms::ondeviceproximity || aTypeAtom == nsGkAtoms::onuserproximity) {
    EnableDevice(NS_DEVICE_PROXIMITY);
  } else if (aTypeAtom == nsGkAtoms::ondevicelight) {
    EnableDevice(NS_DEVICE_LIGHT);
  } else if (aTypeAtom == nsGkAtoms::ondevicemotion) {
    EnableDevice(NS_DEVICE_MOTION);
#ifdef MOZ_B2G
  } else if (aTypeAtom == nsGkAtoms::onmoztimechange) {
    nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
    if (window) {
      window->EnableTimeChangeNotifications();
    }
  } else if (aTypeAtom == nsGkAtoms::onmoznetworkupload) {
    nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
    if (window) {
      window->EnableNetworkEvent(NS_NETWORK_UPLOAD_EVENT);
    }
  } else if (aTypeAtom == nsGkAtoms::onmoznetworkdownload) {
    nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
    if (window) {
      window->EnableNetworkEvent(NS_NETWORK_DOWNLOAD_EVENT);
    }
#endif
  } else if (aTypeAtom == nsGkAtoms::ontouchstart ||
             aTypeAtom == nsGkAtoms::ontouchend ||
             aTypeAtom == nsGkAtoms::ontouchmove ||
             aTypeAtom == nsGkAtoms::ontouchenter ||
             aTypeAtom == nsGkAtoms::ontouchleave ||
             aTypeAtom == nsGkAtoms::ontouchcancel) {
    mMayHaveTouchEventListener = true;
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    
    
    if (window && !aFlags.mInSystemGroup) {
      window->SetHasTouchEventListeners();
    }
  } else if (aType >= NS_POINTER_EVENT_START && aType <= NS_POINTER_LOST_CAPTURE) {
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (aTypeAtom == nsGkAtoms::onpointerenter ||
        aTypeAtom == nsGkAtoms::onpointerleave) {
      mMayHavePointerEnterLeaveEventListener = true;
      if (window) {
#ifdef DEBUG
        nsCOMPtr<nsIDocument> d = window->GetExtantDoc();
        NS_WARN_IF_FALSE(!nsContentUtils::IsChromeDoc(d),
                         "Please do not use pointerenter/leave events in chrome. "
                         "They are slower than pointerover/out!");
#endif
        window->SetHasPointerEnterLeaveEventListeners();
      }
    }
  } else if (aTypeAtom == nsGkAtoms::onmouseenter ||
             aTypeAtom == nsGkAtoms::onmouseleave) {
    mMayHaveMouseEnterLeaveEventListener = true;
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
#ifdef DEBUG
      nsCOMPtr<nsIDocument> d = window->GetExtantDoc();
      NS_WARN_IF_FALSE(!nsContentUtils::IsChromeDoc(d),
                       "Please do not use mouseenter/leave events in chrome. "
                       "They are slower than mouseover/out!");
#endif
      window->SetHasMouseEnterLeaveEventListeners();
    }
#ifdef MOZ_GAMEPAD
  } else if (aType >= NS_GAMEPAD_START &&
             aType <= NS_GAMEPAD_END) {
    nsPIDOMWindow* window = GetInnerWindowForTarget();
    if (window) {
      window->SetHasGamepadEventListener();
    }
#endif
  }
  if (aTypeAtom && mTarget) {
    mTarget->EventListenerAdded(aTypeAtom);
  }
}

bool
EventListenerManager::IsDeviceType(uint32_t aType)
{
  switch (aType) {
    case NS_DEVICE_ORIENTATION:
    case NS_DEVICE_MOTION:
    case NS_DEVICE_LIGHT:
    case NS_DEVICE_PROXIMITY:
    case NS_USER_PROXIMITY:
      return true;
    default:
      break;
  }
  return false;
}

void
EventListenerManager::EnableDevice(uint32_t aType)
{
  nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
  if (!window) {
    return;
  }

  switch (aType) {
    case NS_DEVICE_ORIENTATION:
      window->EnableDeviceSensor(SENSOR_ORIENTATION);
      break;
    case NS_DEVICE_PROXIMITY:
    case NS_USER_PROXIMITY:
      window->EnableDeviceSensor(SENSOR_PROXIMITY);
      break;
    case NS_DEVICE_LIGHT:
      window->EnableDeviceSensor(SENSOR_LIGHT);
      break;
    case NS_DEVICE_MOTION:
      window->EnableDeviceSensor(SENSOR_ACCELERATION);
      window->EnableDeviceSensor(SENSOR_LINEAR_ACCELERATION);
      window->EnableDeviceSensor(SENSOR_GYROSCOPE);
      break;
    default:
      NS_WARNING("Enabling an unknown device sensor.");
      break;
  }
}

void
EventListenerManager::DisableDevice(uint32_t aType)
{
  nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
  if (!window) {
    return;
  }

  switch (aType) {
    case NS_DEVICE_ORIENTATION:
      window->DisableDeviceSensor(SENSOR_ORIENTATION);
      break;
    case NS_DEVICE_MOTION:
      window->DisableDeviceSensor(SENSOR_ACCELERATION);
      window->DisableDeviceSensor(SENSOR_LINEAR_ACCELERATION);
      window->DisableDeviceSensor(SENSOR_GYROSCOPE);
      break;
    case NS_DEVICE_PROXIMITY:
    case NS_USER_PROXIMITY:
      window->DisableDeviceSensor(SENSOR_PROXIMITY);
      break;
    case NS_DEVICE_LIGHT:
      window->DisableDeviceSensor(SENSOR_LIGHT);
      break;
    default:
      NS_WARNING("Disabling an unknown device sensor.");
      break;
  }
}

void
EventListenerManager::RemoveEventListenerInternal(
                        const EventListenerHolder& aListenerHolder,
                        uint32_t aType,
                        nsIAtom* aUserType,
                        const nsAString& aTypeString,
                        const EventListenerFlags& aFlags,
                        bool aAllEvents)
{
  if (!aListenerHolder || !aType || mClearingListeners) {
    return;
  }

  Listener* listener;

  uint32_t count = mListeners.Length();
  uint32_t typeCount = 0;
  bool deviceType = IsDeviceType(aType);
#ifdef MOZ_B2G
  bool timeChangeEvent = (aType == NS_MOZ_TIME_CHANGE_EVENT);
  bool networkEvent = (aType == NS_NETWORK_UPLOAD_EVENT ||
                       aType == NS_NETWORK_DOWNLOAD_EVENT);
#endif 

  for (uint32_t i = 0; i < count; ++i) {
    listener = &mListeners.ElementAt(i);
    if (EVENT_TYPE_EQUALS(listener, aType, aUserType, aTypeString,
                          aAllEvents)) {
      ++typeCount;
      if (listener->mListener == aListenerHolder &&
          listener->mFlags.EqualsIgnoringTrustness(aFlags)) {
        nsRefPtr<EventListenerManager> kungFuDeathGrip(this);
        mListeners.RemoveElementAt(i);
        --count;
        mNoListenerForEvent = NS_EVENT_NULL;
        mNoListenerForEventAtom = nullptr;
        if (mTarget && aUserType) {
          mTarget->EventListenerRemoved(aUserType);
        }

        if (!deviceType
#ifdef MOZ_B2G
            && !timeChangeEvent && !networkEvent
#endif 
            ) {
          return;
        }
        --typeCount;
      }
    }
  }

  if (!aAllEvents && deviceType && typeCount == 0) {
    DisableDevice(aType);
#ifdef MOZ_B2G
  } else if (timeChangeEvent && typeCount == 0) {
    nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
    if (window) {
      window->DisableTimeChangeNotifications();
    }
  } else if (!aAllEvents && networkEvent && typeCount == 0) {
    nsCOMPtr<nsPIDOMWindow> window = GetTargetAsInnerWindow();
    if (window) {
      window->DisableNetworkEvent(aType);
    }
#endif
  }
}

bool
EventListenerManager::ListenerCanHandle(Listener* aListener,
                                        WidgetEvent* aEvent)
{
  
  
  
  
  if (aListener->mAllEvents) {
    return true;
  }
  if (aEvent->message == NS_USER_DEFINED_EVENT) {
    if (mIsMainThreadELM) {
      return aListener->mTypeAtom == aEvent->userType;
    }
    return aListener->mTypeString.Equals(aEvent->typeString);
  }
  MOZ_ASSERT(mIsMainThreadELM);
  return aListener->mEventType == aEvent->message;
}

void
EventListenerManager::AddEventListenerByType(
                        const EventListenerHolder& aListenerHolder,
                        const nsAString& aType,
                        const EventListenerFlags& aFlags)
{
  nsCOMPtr<nsIAtom> atom =
    mIsMainThreadELM ? do_GetAtom(NS_LITERAL_STRING("on") + aType) : nullptr;
  uint32_t type = nsContentUtils::GetEventId(atom);
  AddEventListenerInternal(aListenerHolder, type, atom, aType, aFlags);
}

void
EventListenerManager::RemoveEventListenerByType(
                        const EventListenerHolder& aListenerHolder,
                        const nsAString& aType,
                        const EventListenerFlags& aFlags)
{
  nsCOMPtr<nsIAtom> atom =
    mIsMainThreadELM ? do_GetAtom(NS_LITERAL_STRING("on") + aType) : nullptr;
  uint32_t type = nsContentUtils::GetEventId(atom);
  RemoveEventListenerInternal(aListenerHolder, type, atom, aType, aFlags);
}

EventListenerManager::Listener*
EventListenerManager::FindEventHandler(uint32_t aEventType,
                                       nsIAtom* aTypeAtom,
                                       const nsAString& aTypeString)
{
  
  
  Listener* listener;
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    listener = &mListeners.ElementAt(i);
    if (listener->mListenerIsHandler &&
        EVENT_TYPE_EQUALS(listener, aEventType, aTypeAtom, aTypeString,
                          false)) {
      return listener;
    }
  }
  return nullptr;
}

EventListenerManager::Listener*
EventListenerManager::SetEventHandlerInternal(
                        nsIAtom* aName,
                        const nsAString& aTypeString,
                        const TypedEventHandler& aTypedHandler,
                        bool aPermitUntrustedEvents)
{
  MOZ_ASSERT(aName || !aTypeString.IsEmpty());

  uint32_t eventType = nsContentUtils::GetEventId(aName);
  Listener* listener = FindEventHandler(eventType, aName, aTypeString);

  if (!listener) {
    
    
    EventListenerFlags flags;
    flags.mListenerIsJSListener = true;

    nsCOMPtr<nsJSEventListener> jsListener;
    NS_NewJSEventHandler(mTarget, aName,
                         aTypedHandler, getter_AddRefs(jsListener));
    EventListenerHolder listenerHolder(jsListener);
    AddEventListenerInternal(listenerHolder, eventType, aName, aTypeString,
                             flags, true);

    listener = FindEventHandler(eventType, aName, aTypeString);
  } else {
    nsJSEventListener* jsListener = listener->GetJSListener();
    MOZ_ASSERT(jsListener,
               "How can we have an event handler with no nsJSEventListener?");

    bool same = jsListener->GetTypedEventHandler() == aTypedHandler;
    
    jsListener->SetHandler(aTypedHandler);
    if (mTarget && !same && aName) {
      mTarget->EventListenerRemoved(aName);
      mTarget->EventListenerAdded(aName);
    }
  }

  
  listener->mHandlerIsString = !aTypedHandler.HasEventHandler();
  if (aPermitUntrustedEvents) {
    listener->mFlags.mAllowUntrustedEvents = true;
  }

  return listener;
}

nsresult
EventListenerManager::SetEventHandler(nsIAtom* aName,
                                      const nsAString& aBody,
                                      uint32_t aLanguage,
                                      bool aDeferCompilation,
                                      bool aPermitUntrustedEvents,
                                      Element* aElement)
{
  NS_PRECONDITION(aLanguage != nsIProgrammingLanguage::UNKNOWN,
                  "Must know the language for the script event listener");

  
  
  
  
  
  if (aPermitUntrustedEvents && 
      aLanguage != nsIProgrammingLanguage::JAVASCRIPT) {
    NS_WARNING("Discarding non-JS event listener from untrusted source");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIScriptGlobalObject> global =
    GetScriptGlobalAndDocument(getter_AddRefs(doc));

  if (!global) {
    
    
    return NS_OK;
  }

#ifdef DEBUG
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(global);
  if (win) {
    MOZ_ASSERT(win->IsInnerWindow(), "We should not have an outer window here!");
  }
#endif

  nsresult rv = NS_OK;
  
  
  if (doc) {
    
    
    if (doc->GetSandboxFlags() & SANDBOXED_SCRIPTS) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    nsCOMPtr<nsIContentSecurityPolicy> csp;
    rv = doc->NodePrincipal()->GetCsp(getter_AddRefs(csp));
    NS_ENSURE_SUCCESS(rv, rv);

    if (csp) {
      bool inlineOK = true;
      bool reportViolations = false;
      rv = csp->GetAllowsInlineScript(&reportViolations, &inlineOK);
      NS_ENSURE_SUCCESS(rv, rv);

      if (reportViolations) {
        
        nsIURI* uri = doc->GetDocumentURI();
        nsAutoCString asciiSpec;
        if (uri)
          uri->GetAsciiSpec(asciiSpec);
        nsAutoString scriptSample, attr, tagName(NS_LITERAL_STRING("UNKNOWN"));
        aName->ToString(attr);
        nsCOMPtr<nsIDOMNode> domNode(do_QueryInterface(mTarget));
        if (domNode)
          domNode->GetNodeName(tagName);
        
        scriptSample.Assign(attr);
        scriptSample.AppendLiteral(" attribute on ");
        scriptSample.Append(tagName);
        scriptSample.AppendLiteral(" element");
        csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_SCRIPT,
                                 NS_ConvertUTF8toUTF16(asciiSpec),
                                 scriptSample,
                                 0,
                                 EmptyString(),
                                 EmptyString());
      }

      
      if (!inlineOK) {
        return NS_OK;
      }
    }
  }

  
  
  if (NS_FAILED(global->EnsureScriptEnvironment())) {
    NS_WARNING("Failed to setup script environment for this language");
    
  }

  nsIScriptContext* context = global->GetScriptContext();
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);
  NS_ENSURE_STATE(global->GetGlobalJSObject());

  Listener* listener = SetEventHandlerInternal(aName,
                                               EmptyString(),
                                               TypedEventHandler(),
                                               aPermitUntrustedEvents);

  if (!aDeferCompilation) {
    return CompileEventHandlerInternal(listener, &aBody, aElement);
  }

  return NS_OK;
}

void
EventListenerManager::RemoveEventHandler(nsIAtom* aName,
                                         const nsAString& aTypeString)
{
  if (mClearingListeners) {
    return;
  }

  uint32_t eventType = nsContentUtils::GetEventId(aName);
  Listener* listener = FindEventHandler(eventType, aName, aTypeString);

  if (listener) {
    mListeners.RemoveElementAt(uint32_t(listener - &mListeners.ElementAt(0)));
    mNoListenerForEvent = NS_EVENT_NULL;
    mNoListenerForEventAtom = nullptr;
    if (mTarget && aName) {
      mTarget->EventListenerRemoved(aName);
    }
  }
}

nsresult
EventListenerManager::CompileEventHandlerInternal(Listener* aListener,
                                                  const nsAString* aBody,
                                                  Element* aElement)
{
  MOZ_ASSERT(aListener->GetJSListener());
  MOZ_ASSERT(aListener->mHandlerIsString, "Why are we compiling a non-string JS listener?");
  nsJSEventListener* jsListener = aListener->GetJSListener();
  MOZ_ASSERT(!jsListener->GetTypedEventHandler().HasEventHandler(),
             "What is there to compile?");

  nsresult result = NS_OK;
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIScriptGlobalObject> global =
    GetScriptGlobalAndDocument(getter_AddRefs(doc));
  NS_ENSURE_STATE(global);

  nsIScriptContext* context = global->GetScriptContext();
  NS_ENSURE_STATE(context);

  
  AutoPushJSContextForErrorReporting cx(context->GetNativeContext());

  nsCOMPtr<nsIAtom> typeAtom = aListener->mTypeAtom;
  nsIAtom* attrName = typeAtom;

  
  
  aListener->mHandlerIsString = false;

  
  
  
  
  
  
  
  nsCOMPtr<Element> element = do_QueryInterface(mTarget);
  MOZ_ASSERT(element || aBody, "Where will we get our body?");
  nsAutoString handlerBody;
  const nsAString* body = aBody;
  if (!aBody) {
    if (aListener->mTypeAtom == nsGkAtoms::onSVGLoad) {
      attrName = nsGkAtoms::onload;
    } else if (aListener->mTypeAtom == nsGkAtoms::onSVGUnload) {
      attrName = nsGkAtoms::onunload;
    } else if (aListener->mTypeAtom == nsGkAtoms::onSVGResize) {
      attrName = nsGkAtoms::onresize;
    } else if (aListener->mTypeAtom == nsGkAtoms::onSVGScroll) {
      attrName = nsGkAtoms::onscroll;
    } else if (aListener->mTypeAtom == nsGkAtoms::onSVGZoom) {
      attrName = nsGkAtoms::onzoom;
    } else if (aListener->mTypeAtom == nsGkAtoms::onbeginEvent) {
      attrName = nsGkAtoms::onbegin;
    } else if (aListener->mTypeAtom == nsGkAtoms::onrepeatEvent) {
      attrName = nsGkAtoms::onrepeat;
    } else if (aListener->mTypeAtom == nsGkAtoms::onendEvent) {
      attrName = nsGkAtoms::onend;
    }

    element->GetAttr(kNameSpaceID_None, attrName, handlerBody);
    body = &handlerBody;
    aElement = element;
  }
  aListener = nullptr;

  uint32_t lineNo = 0;
  nsAutoCString url (NS_LITERAL_CSTRING("-moz-evil:lying-event-listener"));
  MOZ_ASSERT(body);
  MOZ_ASSERT(aElement);
  nsIURI *uri = aElement->OwnerDoc()->GetDocumentURI();
  if (uri) {
    uri->GetSpec(url);
    lineNo = 1;
  }

  uint32_t argCount;
  const char **argNames;
  nsContentUtils::GetEventArgNames(aElement->GetNameSpaceID(),
                                   typeAtom,
                                   &argCount, &argNames);

  
  
  
  
  
  
  JS::Rooted<JSObject*> wrapScope(cx, context->GetWindowProxy());
  JS::Rooted<JS::Value> v(cx);
  {
    JSAutoCompartment ac(cx, wrapScope);
    nsresult rv = nsContentUtils::WrapNative(cx, wrapScope, mTarget, &v,
                                              false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }
  JS::Rooted<JSObject*> target(cx, &v.toObject());
  JSAutoCompartment ac(cx, target);

  nsDependentAtomString str(attrName);
  
  
  
  JS::Rooted<JSString*> jsStr(cx, JS_NewUCStringCopyN(cx,
                                                      str.BeginReading(),
                                                      str.Length()));
  NS_ENSURE_TRUE(jsStr, NS_ERROR_OUT_OF_MEMORY);

  
  if (NS_WARN_IF(!WrapNewBindingObject(cx, target, aElement, &v))) {
    return NS_ERROR_FAILURE;
  }
  JS::CompileOptions options(cx);
  options.setIntroductionType("eventHandler")
         .setFileAndLine(url.get(), lineNo)
         .setVersion(SCRIPTVERSION_DEFAULT)
         .setElement(&v.toObject())
         .setElementAttributeName(jsStr)
         .setDefineOnScope(false);

  JS::Rooted<JSObject*> handler(cx);
  result = nsJSUtils::CompileFunction(cx, target, options,
                                      nsAtomCString(typeAtom),
                                      argCount, argNames, *body, handler.address());
  NS_ENSURE_SUCCESS(result, result);
  NS_ENSURE_TRUE(handler, NS_ERROR_FAILURE);

  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(mTarget);
  if (jsListener->EventName() == nsGkAtoms::onerror && win) {
    nsRefPtr<OnErrorEventHandlerNonNull> handlerCallback =
      new OnErrorEventHandlerNonNull(handler,  nullptr);
    jsListener->SetHandler(handlerCallback);
  } else if (jsListener->EventName() == nsGkAtoms::onbeforeunload && win) {
    nsRefPtr<OnBeforeUnloadEventHandlerNonNull> handlerCallback =
      new OnBeforeUnloadEventHandlerNonNull(handler,  nullptr);
    jsListener->SetHandler(handlerCallback);
  } else {
    nsRefPtr<EventHandlerNonNull> handlerCallback =
      new EventHandlerNonNull(handler,  nullptr);
    jsListener->SetHandler(handlerCallback);
  }

  return result;
}

nsresult
EventListenerManager::HandleEventSubType(Listener* aListener,
                                         nsIDOMEvent* aDOMEvent,
                                         EventTarget* aCurrentTarget)
{
  nsresult result = NS_OK;
  EventListenerHolder listenerHolder(aListener->mListener);  

  
  
  if ((aListener->mListenerType == Listener::eJSEventListener) &&
      aListener->mHandlerIsString) {
    result = CompileEventHandlerInternal(aListener, nullptr, nullptr);
    aListener = nullptr;
  }

  if (NS_SUCCEEDED(result)) {
    if (mIsMainThreadELM) {
      nsContentUtils::EnterMicroTask();
    }
    
    if (listenerHolder.HasWebIDLCallback()) {
      ErrorResult rv;
      listenerHolder.GetWebIDLCallback()->
        HandleEvent(aCurrentTarget, *(aDOMEvent->InternalDOMEvent()), rv);
      result = rv.ErrorCode();
    } else {
      result = listenerHolder.GetXPCOMCallback()->HandleEvent(aDOMEvent);
    }
    if (mIsMainThreadELM) {
      nsContentUtils::LeaveMicroTask();
    }
  }

  return result;
}






void
EventListenerManager::HandleEventInternal(nsPresContext* aPresContext,
                                          WidgetEvent* aEvent,
                                          nsIDOMEvent** aDOMEvent,
                                          EventTarget* aCurrentTarget,
                                          nsEventStatus* aEventStatus)
{
  
  if (*aEventStatus == nsEventStatus_eConsumeNoDefault) {
    aEvent->mFlags.mDefaultPrevented = true;
  }

  nsAutoTObserverArray<Listener, 2>::EndLimitedIterator iter(mListeners);
  Maybe<nsAutoPopupStatePusher> popupStatePusher;
  if (mIsMainThreadELM) {
    popupStatePusher.construct(Event::GetEventPopupControlState(aEvent));
  }

  bool hasListener = false;
  while (iter.HasMore()) {
    if (aEvent->mFlags.mImmediatePropagationStopped) {
      break;
    }
    Listener* listener = &iter.GetNext();
    
    
    if (ListenerCanHandle(listener, aEvent)) {
      hasListener = true;
      if (listener->IsListening(aEvent) &&
          (aEvent->mFlags.mIsTrusted ||
           listener->mFlags.mAllowUntrustedEvents)) {
        if (!*aDOMEvent) {
          
          nsCOMPtr<EventTarget> et =
            do_QueryInterface(aEvent->originalTarget);
          EventDispatcher::CreateEvent(et, aPresContext,
                                       aEvent, EmptyString(), aDOMEvent);
        }
        if (*aDOMEvent) {
          if (!aEvent->currentTarget) {
            aEvent->currentTarget = aCurrentTarget->GetTargetForDOMEvent();
            if (!aEvent->currentTarget) {
              break;
            }
          }

          if (NS_FAILED(HandleEventSubType(listener, *aDOMEvent,
                                           aCurrentTarget))) {
            aEvent->mFlags.mExceptionHasBeenRisen = true;
          }
        }
      }
    }
  }

  aEvent->currentTarget = nullptr;

  if (mIsMainThreadELM && !hasListener) {
    mNoListenerForEvent = aEvent->message;
    mNoListenerForEventAtom = aEvent->userType;
  }

  if (aEvent->mFlags.mDefaultPrevented) {
    *aEventStatus = nsEventStatus_eConsumeNoDefault;
  }
}

void
EventListenerManager::Disconnect()
{
  mTarget = nullptr;
  RemoveAllListeners();
}

void
EventListenerManager::AddEventListener(
                        const nsAString& aType,
                        const EventListenerHolder& aListenerHolder,
                        bool aUseCapture,
                        bool aWantsUntrusted)
{
  EventListenerFlags flags;
  flags.mCapture = aUseCapture;
  flags.mAllowUntrustedEvents = aWantsUntrusted;
  return AddEventListenerByType(aListenerHolder, aType, flags);
}

void
EventListenerManager::RemoveEventListener(
                        const nsAString& aType,
                        const EventListenerHolder& aListenerHolder,
                        bool aUseCapture)
{
  EventListenerFlags flags;
  flags.mCapture = aUseCapture;
  RemoveEventListenerByType(aListenerHolder, aType, flags);
}

void
EventListenerManager::AddListenerForAllEvents(nsIDOMEventListener* aDOMListener,
                                              bool aUseCapture,
                                              bool aWantsUntrusted,
                                              bool aSystemEventGroup)
{
  EventListenerFlags flags;
  flags.mCapture = aUseCapture;
  flags.mAllowUntrustedEvents = aWantsUntrusted;
  flags.mInSystemGroup = aSystemEventGroup;
  EventListenerHolder listenerHolder(aDOMListener);
  AddEventListenerInternal(listenerHolder, NS_EVENT_ALL, nullptr, EmptyString(),
                           flags, false, true);
}

void
EventListenerManager::RemoveListenerForAllEvents(
                        nsIDOMEventListener* aDOMListener,
                        bool aUseCapture,
                        bool aSystemEventGroup)
{
  EventListenerFlags flags;
  flags.mCapture = aUseCapture;
  flags.mInSystemGroup = aSystemEventGroup;
  EventListenerHolder listenerHolder(aDOMListener);
  RemoveEventListenerInternal(listenerHolder, NS_EVENT_ALL, nullptr,
                              EmptyString(), flags, true);
}

bool
EventListenerManager::HasMutationListeners()
{
  if (mMayHaveMutationListeners) {
    uint32_t count = mListeners.Length();
    for (uint32_t i = 0; i < count; ++i) {
      Listener* listener = &mListeners.ElementAt(i);
      if (listener->mEventType >= NS_MUTATION_START &&
          listener->mEventType <= NS_MUTATION_END) {
        return true;
      }
    }
  }

  return false;
}

uint32_t
EventListenerManager::MutationListenerBits()
{
  uint32_t bits = 0;
  if (mMayHaveMutationListeners) {
    uint32_t count = mListeners.Length();
    for (uint32_t i = 0; i < count; ++i) {
      Listener* listener = &mListeners.ElementAt(i);
      if (listener->mEventType >= NS_MUTATION_START &&
          listener->mEventType <= NS_MUTATION_END) {
        if (listener->mEventType == NS_MUTATION_SUBTREEMODIFIED) {
          return kAllMutationBits;
        }
        bits |= MutationBitForEventType(listener->mEventType);
      }
    }
  }
  return bits;
}

bool
EventListenerManager::HasListenersFor(const nsAString& aEventName)
{
  nsCOMPtr<nsIAtom> atom = do_GetAtom(NS_LITERAL_STRING("on") + aEventName);
  return HasListenersFor(atom);
}

bool
EventListenerManager::HasListenersFor(nsIAtom* aEventNameWithOn)
{
#ifdef DEBUG
  nsAutoString name;
  aEventNameWithOn->ToString(name);
#endif
  NS_ASSERTION(StringBeginsWith(name, NS_LITERAL_STRING("on")),
               "Event name does not start with 'on'");
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    Listener* listener = &mListeners.ElementAt(i);
    if (listener->mTypeAtom == aEventNameWithOn) {
      return true;
    }
  }
  return false;
}

bool
EventListenerManager::HasListeners()
{
  return !mListeners.IsEmpty();
}

nsresult
EventListenerManager::GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList)
{
  nsCOMPtr<EventTarget> target = do_QueryInterface(mTarget);
  NS_ENSURE_STATE(target);
  aList->Clear();
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    const Listener& listener = mListeners.ElementAt(i);
    
    
    if (listener.mListenerType == Listener::eJSEventListener &&
        listener.mHandlerIsString) {
      CompileEventHandlerInternal(const_cast<Listener*>(&listener), nullptr,
                                  nullptr);
    }
    nsAutoString eventType;
    if (listener.mAllEvents) {
      eventType.SetIsVoid(true);
    } else {
      eventType.Assign(Substring(nsDependentAtomString(listener.mTypeAtom), 2));
    }
    
    
    nsRefPtr<EventListenerInfo> info =
      new EventListenerInfo(eventType, listener.mListener.ToXPCOMCallback(),
                            listener.mFlags.mCapture,
                            listener.mFlags.mAllowUntrustedEvents,
                            listener.mFlags.mInSystemGroup);
    aList->AppendObject(info);
  }
  return NS_OK;
}

bool
EventListenerManager::HasUnloadListeners()
{
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    Listener* listener = &mListeners.ElementAt(i);
    if (listener->mEventType == NS_PAGE_UNLOAD ||
        listener->mEventType == NS_BEFORE_PAGE_UNLOAD) {
      return true;
    }
  }
  return false;
}

void
EventListenerManager::SetEventHandler(nsIAtom* aEventName,
                                      const nsAString& aTypeString,
                                      EventHandlerNonNull* aHandler)
{
  if (!aHandler) {
    RemoveEventHandler(aEventName, aTypeString);
    return;
  }

  
  
  SetEventHandlerInternal(aEventName, aTypeString, TypedEventHandler(aHandler),
                          !mIsMainThreadELM ||
                          !nsContentUtils::IsCallerChrome());
}

void
EventListenerManager::SetEventHandler(OnErrorEventHandlerNonNull* aHandler)
{
  if (mIsMainThreadELM) {
    if (!aHandler) {
      RemoveEventHandler(nsGkAtoms::onerror, EmptyString());
      return;
    }

    
    
    SetEventHandlerInternal(nsGkAtoms::onerror, EmptyString(),
                            TypedEventHandler(aHandler),
                            !nsContentUtils::IsCallerChrome());
  } else {
    if (!aHandler) {
      RemoveEventHandler(nullptr, NS_LITERAL_STRING("error"));
      return;
    }

    
    SetEventHandlerInternal(nullptr, NS_LITERAL_STRING("error"),
                            TypedEventHandler(aHandler), true);
  }
}

void
EventListenerManager::SetEventHandler(
                        OnBeforeUnloadEventHandlerNonNull* aHandler)
{
  if (!aHandler) {
    RemoveEventHandler(nsGkAtoms::onbeforeunload, EmptyString());
    return;
  }

  
  
  SetEventHandlerInternal(nsGkAtoms::onbeforeunload, EmptyString(),
                          TypedEventHandler(aHandler),
                          !mIsMainThreadELM ||
                          !nsContentUtils::IsCallerChrome());
}

const TypedEventHandler*
EventListenerManager::GetTypedEventHandler(nsIAtom* aEventName,
                                           const nsAString& aTypeString)
{
  uint32_t eventType = nsContentUtils::GetEventId(aEventName);
  Listener* listener = FindEventHandler(eventType, aEventName, aTypeString);

  if (!listener) {
    return nullptr;
  }

  nsJSEventListener* jsListener = listener->GetJSListener();

  if (listener->mHandlerIsString) {
    CompileEventHandlerInternal(listener, nullptr, nullptr);
  }

  const TypedEventHandler& typedHandler = jsListener->GetTypedEventHandler();
  return typedHandler.HasEventHandler() ? &typedHandler : nullptr;
}

size_t
EventListenerManager::SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t n = aMallocSizeOf(this);
  n += mListeners.SizeOfExcludingThis(aMallocSizeOf);
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    nsJSEventListener* jsl = mListeners.ElementAt(i).GetJSListener();
    if (jsl) {
      n += jsl->SizeOfIncludingThis(aMallocSizeOf);
    }
  }
  return n;
}

void
EventListenerManager::MarkForCC()
{
  uint32_t count = mListeners.Length();
  for (uint32_t i = 0; i < count; ++i) {
    const Listener& listener = mListeners.ElementAt(i);
    nsJSEventListener* jsListener = listener.GetJSListener();
    if (jsListener) {
      const TypedEventHandler& typedHandler =
        jsListener->GetTypedEventHandler();
      if (typedHandler.HasEventHandler()) {
        JS::ExposeObjectToActiveJS(typedHandler.Ptr()->Callable());
      }
    } else if (listener.mListenerType == Listener::eWrappedJSListener) {
      xpc_TryUnmarkWrappedGrayObject(listener.mListener.GetXPCOMCallback());
    } else if (listener.mListenerType == Listener::eWebIDLListener) {
      
      listener.mListener.GetWebIDLCallback()->Callback();
    }
  }
  if (mRefCnt.IsPurple()) {
    mRefCnt.RemovePurple();
  }
}

already_AddRefed<nsIScriptGlobalObject>
EventListenerManager::GetScriptGlobalAndDocument(nsIDocument** aDoc)
{
  nsCOMPtr<nsINode> node(do_QueryInterface(mTarget));
  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIScriptGlobalObject> global;
  if (node) {
    
    
    
    doc = node->OwnerDoc();
    if (doc->IsLoadedAsData()) {
      return nullptr;
    }

    
    
    global = do_QueryInterface(doc->GetScopeObject());
  } else {
    nsCOMPtr<nsPIDOMWindow> win = GetTargetAsInnerWindow();
    if (win) {
      doc = win->GetExtantDoc();
      global = do_QueryInterface(win);
    } else {
      global = do_QueryInterface(mTarget);
    }
  }

  doc.forget(aDoc);
  return global.forget();
}

} 
