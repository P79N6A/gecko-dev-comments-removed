




#include "AccessCheck.h"
#include "base/basictypes.h"
#include "ipc/IPCMessageUtils.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/ShadowRoot.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/MiscEvents.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"
#include "nsContentUtils.h"
#include "nsCOMPtr.h"
#include "nsDeviceContext.h"
#include "nsError.h"
#include "nsGlobalWindow.h"
#include "nsIFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIScrollableFrame.h"
#include "nsJSEnvironment.h"
#include "nsLayoutUtils.h"
#include "nsPerformance.h"
#include "nsPIWindowRoot.h"
#include "WorkerPrivate.h"

namespace mozilla {
namespace dom {

namespace workers {
extern bool IsCurrentThreadRunningChromeWorker();
} 

static char *sPopupAllowedEvents;

static bool sReturnHighResTimeStamp = false;
static bool sReturnHighResTimeStampIsSet = false;

Event::Event(EventTarget* aOwner,
             nsPresContext* aPresContext,
             WidgetEvent* aEvent)
{
  ConstructorInit(aOwner, aPresContext, aEvent);
}

Event::Event(nsPIDOMWindow* aParent)
{
  ConstructorInit(static_cast<nsGlobalWindow *>(aParent), nullptr, nullptr);
}

void
Event::ConstructorInit(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       WidgetEvent* aEvent)
{
  SetIsDOMBinding();
  SetOwner(aOwner);
  mIsMainThreadEvent = mOwner || NS_IsMainThread();
  if (mIsMainThreadEvent) {
    nsJSContext::LikelyShortLivingObjectCreated();
  }

  if (mIsMainThreadEvent && !sReturnHighResTimeStampIsSet) {
    Preferences::AddBoolVarCache(&sReturnHighResTimeStamp,
                                 "dom.event.highrestimestamp.enabled",
                                 sReturnHighResTimeStamp);
    sReturnHighResTimeStampIsSet = true;
  }

  mPrivateDataDuplicated = false;

  if (aEvent) {
    mEvent = aEvent;
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    























    mEvent = new WidgetEvent(false, 0);
    mEvent->time = PR_Now();
  }

  InitPresContextData(aPresContext);
}

void
Event::InitPresContextData(nsPresContext* aPresContext)
{
  mPresContext = aPresContext;
  
  {
    nsCOMPtr<nsIContent> content = GetTargetFromFrame();
    mExplicitOriginalTarget = content;
    if (content && content->IsInAnonymousSubtree()) {
      mExplicitOriginalTarget = nullptr;
    }
  }
}

Event::~Event() 
{
  NS_ASSERT_OWNINGTHREAD(Event);

  if (mEventIsInternal && mEvent) {
    delete mEvent;
  }
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Event)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEvent)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Event)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Event)

NS_IMPL_CYCLE_COLLECTION_CLASS(Event)

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(Event)
  NS_IMPL_CYCLE_COLLECTION_TRACE_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(Event)
  if (tmp->mEventIsInternal) {
    tmp->mEvent->target = nullptr;
    tmp->mEvent->currentTarget = nullptr;
    tmp->mEvent->originalTarget = nullptr;
    switch (tmp->mEvent->mClass) {
      case eMouseEventClass:
      case eMouseScrollEventClass:
      case eWheelEventClass:
      case eSimpleGestureEventClass:
      case ePointerEventClass:
        tmp->mEvent->AsMouseEventBase()->relatedTarget = nullptr;
        break;
      case eDragEventClass: {
        WidgetDragEvent* dragEvent = tmp->mEvent->AsDragEvent();
        dragEvent->dataTransfer = nullptr;
        dragEvent->relatedTarget = nullptr;
        break;
      }
      case eClipboardEventClass:
        tmp->mEvent->AsClipboardEvent()->clipboardData = nullptr;
        break;
      case eMutationEventClass:
        tmp->mEvent->AsMutationEvent()->mRelatedNode = nullptr;
        break;
      case eFocusEventClass:
        tmp->mEvent->AsFocusEvent()->relatedTarget = nullptr;
        break;
      default:
        break;
    }
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPresContext);
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mExplicitOriginalTarget);
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOwner);
  NS_IMPL_CYCLE_COLLECTION_UNLINK_PRESERVED_WRAPPER
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(Event)
  if (tmp->mEventIsInternal) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEvent->target)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEvent->currentTarget)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEvent->originalTarget)
    switch (tmp->mEvent->mClass) {
      case eMouseEventClass:
      case eMouseScrollEventClass:
      case eWheelEventClass:
      case eSimpleGestureEventClass:
      case ePointerEventClass:
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->relatedTarget");
        cb.NoteXPCOMChild(tmp->mEvent->AsMouseEventBase()->relatedTarget);
        break;
      case eDragEventClass: {
        WidgetDragEvent* dragEvent = tmp->mEvent->AsDragEvent();
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->dataTransfer");
        cb.NoteXPCOMChild(dragEvent->dataTransfer);
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->relatedTarget");
        cb.NoteXPCOMChild(dragEvent->relatedTarget);
        break;
      }
      case eClipboardEventClass:
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->clipboardData");
        cb.NoteXPCOMChild(tmp->mEvent->AsClipboardEvent()->clipboardData);
        break;
      case eMutationEventClass:
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->mRelatedNode");
        cb.NoteXPCOMChild(tmp->mEvent->AsMutationEvent()->mRelatedNode);
        break;
      case eFocusEventClass:
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mEvent->relatedTarget");
        cb.NoteXPCOMChild(tmp->mEvent->AsFocusEvent()->relatedTarget);
        break;
      default:
        break;
    }
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPresContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mExplicitOriginalTarget)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOwner)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

bool
Event::IsChrome(JSContext* aCx) const
{
  return mIsMainThreadEvent ?
    xpc::AccessCheck::isChrome(js::GetContextCompartment(aCx)) :
    mozilla::dom::workers::IsCurrentThreadRunningChromeWorker();
}


NS_METHOD
Event::GetType(nsAString& aType)
{
  if (!mIsMainThreadEvent || !mEvent->typeString.IsEmpty()) {
    aType = mEvent->typeString;
    return NS_OK;
  }
  const char* name = GetEventName(mEvent->message);

  if (name) {
    CopyASCIItoUTF16(name, aType);
    return NS_OK;
  } else if (mEvent->message == NS_USER_DEFINED_EVENT && mEvent->userType) {
    aType = Substring(nsDependentAtomString(mEvent->userType), 2); 
    mEvent->typeString = aType;
    return NS_OK;
  }

  aType.Truncate();
  return NS_OK;
}

static EventTarget*
GetDOMEventTarget(nsIDOMEventTarget* aTarget)
{
  return aTarget ? aTarget->GetTargetForDOMEvent() : nullptr;
}

EventTarget*
Event::GetTarget() const
{
  return GetDOMEventTarget(mEvent->target);
}

NS_METHOD
Event::GetTarget(nsIDOMEventTarget** aTarget)
{
  NS_IF_ADDREF(*aTarget = GetTarget());
  return NS_OK;
}

EventTarget*
Event::GetCurrentTarget() const
{
  return GetDOMEventTarget(mEvent->currentTarget);
}

NS_IMETHODIMP
Event::GetCurrentTarget(nsIDOMEventTarget** aCurrentTarget)
{
  NS_IF_ADDREF(*aCurrentTarget = GetCurrentTarget());
  return NS_OK;
}




already_AddRefed<nsIContent>
Event::GetTargetFromFrame()
{
  if (!mPresContext) { return nullptr; }

  
  nsIFrame* targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  if (!targetFrame) { return nullptr; }

  
  nsCOMPtr<nsIContent> realEventContent;
  targetFrame->GetContentForEvent(mEvent, getter_AddRefs(realEventContent));
  return realEventContent.forget();
}

EventTarget*
Event::GetExplicitOriginalTarget() const
{
  if (mExplicitOriginalTarget) {
    return mExplicitOriginalTarget;
  }
  return GetTarget();
}

NS_IMETHODIMP
Event::GetExplicitOriginalTarget(nsIDOMEventTarget** aRealEventTarget)
{
  NS_IF_ADDREF(*aRealEventTarget = GetExplicitOriginalTarget());
  return NS_OK;
}

EventTarget*
Event::GetOriginalTarget() const
{
  if (mEvent->originalTarget) {
    return GetDOMEventTarget(mEvent->originalTarget);
  }

  return GetTarget();
}

NS_IMETHODIMP
Event::GetOriginalTarget(nsIDOMEventTarget** aOriginalTarget)
{
  NS_IF_ADDREF(*aOriginalTarget = GetOriginalTarget());
  return NS_OK;
}

NS_IMETHODIMP_(void)
Event::SetTrusted(bool aTrusted)
{
  mEvent->mFlags.mIsTrusted = aTrusted;
}

bool
Event::Init(mozilla::dom::EventTarget* aGlobal)
{
  if (!mIsMainThreadEvent) {
    return nsContentUtils::ThreadsafeIsCallerChrome();
  }
  bool trusted = false;
  nsCOMPtr<nsPIDOMWindow> w = do_QueryInterface(aGlobal);
  if (w) {
    nsCOMPtr<nsIDocument> d = w->GetExtantDoc();
    if (d) {
      trusted = nsContentUtils::IsChromeDoc(d);
      nsIPresShell* s = d->GetShell();
      if (s) {
        InitPresContextData(s->GetPresContext());
      }
    }
  }
  return trusted;
}


already_AddRefed<Event>
Event::Constructor(const GlobalObject& aGlobal,
                   const nsAString& aType,
                   const EventInit& aParam,
                   ErrorResult& aRv)
{
  nsCOMPtr<mozilla::dom::EventTarget> t = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<Event> e = new Event(t, nullptr, nullptr);
  bool trusted = e->Init(t);
  aRv = e->InitEvent(aType, aParam.mBubbles, aParam.mCancelable);
  e->SetTrusted(trusted);
  return e.forget();
}

uint16_t
Event::EventPhase() const
{
  
  
  if ((mEvent->currentTarget &&
       mEvent->currentTarget == mEvent->target) ||
       mEvent->mFlags.InTargetPhase()) {
    return nsIDOMEvent::AT_TARGET;
  }
  if (mEvent->mFlags.mInCapturePhase) {
    return nsIDOMEvent::CAPTURING_PHASE;
  }
  if (mEvent->mFlags.mInBubblingPhase) {
    return nsIDOMEvent::BUBBLING_PHASE;
  }
  return nsIDOMEvent::NONE;
}

NS_IMETHODIMP
Event::GetEventPhase(uint16_t* aEventPhase)
{
  *aEventPhase = EventPhase();
  return NS_OK;
}

NS_IMETHODIMP
Event::GetBubbles(bool* aBubbles)
{
  *aBubbles = Bubbles();
  return NS_OK;
}

NS_IMETHODIMP
Event::GetCancelable(bool* aCancelable)
{
  *aCancelable = Cancelable();
  return NS_OK;
}

NS_IMETHODIMP
Event::GetTimeStamp(uint64_t* aTimeStamp)
{
  *aTimeStamp = mEvent->time;
  return NS_OK;
}

NS_IMETHODIMP
Event::StopPropagation()
{
  mEvent->mFlags.mPropagationStopped = true;
  return NS_OK;
}

NS_IMETHODIMP
Event::StopImmediatePropagation()
{
  mEvent->mFlags.mPropagationStopped = true;
  mEvent->mFlags.mImmediatePropagationStopped = true;
  return NS_OK;
}

NS_IMETHODIMP
Event::GetIsTrusted(bool* aIsTrusted)
{
  *aIsTrusted = IsTrusted();
  return NS_OK;
}

NS_IMETHODIMP
Event::PreventDefault()
{
  
  
  PreventDefaultInternal(true);
  return NS_OK;
}

void
Event::PreventDefault(JSContext* aCx)
{
  MOZ_ASSERT(aCx, "JS context must be specified");

  
  
  
  
  PreventDefaultInternal(IsChrome(aCx));
}

void
Event::PreventDefaultInternal(bool aCalledByDefaultHandler)
{
  if (!mEvent->mFlags.mCancelable) {
    return;
  }

  mEvent->mFlags.mDefaultPrevented = true;

  
  
  
  
  if (!aCalledByDefaultHandler) {
    mEvent->mFlags.mDefaultPreventedByContent = true;
  }

  if (!IsTrusted()) {
    return;
  }

  WidgetDragEvent* dragEvent = mEvent->AsDragEvent();
  if (!dragEvent) {
    return;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(mEvent->currentTarget);
  if (!node) {
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(mEvent->currentTarget);
    if (!win) {
      return;
    }
    node = win->GetExtantDoc();
  }
  if (!nsContentUtils::IsChromeDoc(node->OwnerDoc())) {
    dragEvent->mDefaultPreventedOnContent = true;
  }
}

void
Event::SetEventType(const nsAString& aEventTypeArg)
{
  if (mIsMainThreadEvent) {
    mEvent->typeString.Truncate();
    mEvent->userType =
      nsContentUtils::GetEventIdAndAtom(aEventTypeArg, mEvent->mClass,
                                        &(mEvent->message));
  } else {
    mEvent->userType = nullptr;
    mEvent->message = NS_USER_DEFINED_EVENT;
    mEvent->typeString = aEventTypeArg;
  }
}

NS_IMETHODIMP
Event::InitEvent(const nsAString& aEventTypeArg,
                 bool aCanBubbleArg,
                 bool aCancelableArg)
{
  
  NS_ENSURE_TRUE(!mEvent->mFlags.mIsBeingDispatched, NS_OK);

  if (IsTrusted()) {
    
    if (!nsContentUtils::ThreadsafeIsCallerChrome()) {
      SetTrusted(false);
    }
  }

  SetEventType(aEventTypeArg);

  mEvent->mFlags.mBubbles = aCanBubbleArg;
  mEvent->mFlags.mCancelable = aCancelableArg;

  mEvent->mFlags.mDefaultPrevented = false;

  
  
  mEvent->target = nullptr;
  mEvent->originalTarget = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
Event::DuplicatePrivateData()
{
  NS_ASSERTION(mEvent, "No WidgetEvent for Event duplication!");
  if (mEventIsInternal) {
    return NS_OK;
  }

  mEvent = mEvent->Duplicate();
  mPresContext = nullptr;
  mEventIsInternal = true;
  mPrivateDataDuplicated = true;

  return NS_OK;
}

NS_IMETHODIMP
Event::SetTarget(nsIDOMEventTarget* aTarget)
{
#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aTarget);

    NS_ASSERTION(!win || !win->IsInnerWindow(),
                 "Uh, inner window set as event target!");
  }
#endif

  mEvent->target = do_QueryInterface(aTarget);
  return NS_OK;
}

NS_IMETHODIMP_(bool)
Event::IsDispatchStopped()
{
  return mEvent->mFlags.mPropagationStopped;
}

NS_IMETHODIMP_(WidgetEvent*)
Event::GetInternalNSEvent()
{
  return mEvent;
}

NS_IMETHODIMP_(Event*)
Event::InternalDOMEvent()
{
  return this;
}



static bool
PopupAllowedForEvent(const char *eventName)
{
  if (!sPopupAllowedEvents) {
    Event::PopupAllowedEventsChanged();

    if (!sPopupAllowedEvents) {
      return false;
    }
  }

  nsDependentCString events(sPopupAllowedEvents);

  nsAFlatCString::const_iterator start, end;
  nsAFlatCString::const_iterator startiter(events.BeginReading(start));
  events.EndReading(end);

  while (startiter != end) {
    nsAFlatCString::const_iterator enditer(end);

    if (!FindInReadable(nsDependentCString(eventName), startiter, enditer))
      return false;

    
    if ((startiter == start || *--startiter == ' ') &&
        (enditer == end || *enditer == ' ')) {
      return true;
    }

    
    
    
    startiter = enditer;
  }

  return false;
}


PopupControlState
Event::GetEventPopupControlState(WidgetEvent* aEvent)
{
  
  
  PopupControlState abuse = openAbused;

  switch(aEvent->mClass) {
  case eBasicEventClass:
    
    
    
    if (EventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_SELECTED :
        if (PopupAllowedForEvent("select")) {
          abuse = openControlled;
        }
        break;
      case NS_FORM_CHANGE :
        if (PopupAllowedForEvent("change")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  case eEditorInputEventClass:
    
    
    
    if (EventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_EDITOR_INPUT:
        if (PopupAllowedForEvent("input")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  case eInputEventClass:
    
    
    
    if (EventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_CHANGE :
        if (PopupAllowedForEvent("change")) {
          abuse = openControlled;
        }
        break;
      case NS_XUL_COMMAND:
        abuse = openControlled;
        break;
      }
    }
    break;
  case eKeyboardEventClass:
    if (aEvent->mFlags.mIsTrusted) {
      uint32_t key = aEvent->AsKeyboardEvent()->keyCode;
      switch(aEvent->message) {
      case NS_KEY_PRESS :
        
        if (key == nsIDOMKeyEvent::DOM_VK_RETURN) {
          abuse = openAllowed;
        } else if (PopupAllowedForEvent("keypress")) {
          abuse = openControlled;
        }
        break;
      case NS_KEY_UP :
        
        if (key == nsIDOMKeyEvent::DOM_VK_SPACE) {
          abuse = openAllowed;
        } else if (PopupAllowedForEvent("keyup")) {
          abuse = openControlled;
        }
        break;
      case NS_KEY_DOWN :
        if (PopupAllowedForEvent("keydown")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  case eTouchEventClass:
    if (aEvent->mFlags.mIsTrusted) {
      switch (aEvent->message) {
      case NS_TOUCH_START :
        if (PopupAllowedForEvent("touchstart")) {
          abuse = openControlled;
        }
        break;
      case NS_TOUCH_END :
        if (PopupAllowedForEvent("touchend")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  case eMouseEventClass:
    if (aEvent->mFlags.mIsTrusted &&
        aEvent->AsMouseEvent()->button == WidgetMouseEvent::eLeftButton) {
      switch(aEvent->message) {
      case NS_MOUSE_BUTTON_UP :
        if (PopupAllowedForEvent("mouseup")) {
          abuse = openControlled;
        }
        break;
      case NS_MOUSE_BUTTON_DOWN :
        if (PopupAllowedForEvent("mousedown")) {
          abuse = openControlled;
        }
        break;
      case NS_MOUSE_CLICK :
        



        if (PopupAllowedForEvent("click")) {
          abuse = openAllowed;
        }
        break;
      case NS_MOUSE_DOUBLECLICK :
        if (PopupAllowedForEvent("dblclick")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  case eFormEventClass:
    
    
    
    if (EventStateManager::IsHandlingUserInput()) {
      switch(aEvent->message) {
      case NS_FORM_SUBMIT :
        if (PopupAllowedForEvent("submit")) {
          abuse = openControlled;
        }
        break;
      case NS_FORM_RESET :
        if (PopupAllowedForEvent("reset")) {
          abuse = openControlled;
        }
        break;
      }
    }
    break;
  default:
    break;
  }

  return abuse;
}


void
Event::PopupAllowedEventsChanged()
{
  if (sPopupAllowedEvents) {
    nsMemory::Free(sPopupAllowedEvents);
  }

  nsAdoptingCString str = Preferences::GetCString("dom.popup_allowed_events");

  
  
  sPopupAllowedEvents = ToNewCString(str);
}


void
Event::Shutdown()
{
  if (sPopupAllowedEvents) {
    nsMemory::Free(sPopupAllowedEvents);
  }
}

nsIntPoint
Event::GetScreenCoords(nsPresContext* aPresContext,
                       WidgetEvent* aEvent,
                       LayoutDeviceIntPoint aPoint)
{
  if (EventStateManager::sIsPointerLocked) {
    return EventStateManager::sLastScreenPoint;
  }

  if (!aEvent || 
       (aEvent->mClass != eMouseEventClass &&
        aEvent->mClass != eMouseScrollEventClass &&
        aEvent->mClass != eWheelEventClass &&
        aEvent->mClass != ePointerEventClass &&
        aEvent->mClass != eTouchEventClass &&
        aEvent->mClass != eDragEventClass &&
        aEvent->mClass != eSimpleGestureEventClass)) {
    return nsIntPoint(0, 0);
  }

  WidgetGUIEvent* guiEvent = aEvent->AsGUIEvent();
  if (!guiEvent->widget) {
    return LayoutDeviceIntPoint::ToUntyped(aPoint);
  }

  LayoutDeviceIntPoint offset = aPoint +
    LayoutDeviceIntPoint::FromUntyped(guiEvent->widget->WidgetToScreenOffset());
  nscoord factor = aPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel();
  return nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(offset.x * factor),
                    nsPresContext::AppUnitsToIntCSSPixels(offset.y * factor));
}


CSSIntPoint
Event::GetPageCoords(nsPresContext* aPresContext,
                     WidgetEvent* aEvent,
                     LayoutDeviceIntPoint aPoint,
                     CSSIntPoint aDefaultPoint)
{
  CSSIntPoint pagePoint =
    Event::GetClientCoords(aPresContext, aEvent, aPoint, aDefaultPoint);

  
  if (aPresContext && aPresContext->GetPresShell()) {
    nsIPresShell* shell = aPresContext->GetPresShell();
    nsIScrollableFrame* scrollframe = shell->GetRootScrollFrameAsScrollable();
    if (scrollframe) {
      pagePoint += CSSIntPoint::FromAppUnitsRounded(scrollframe->GetScrollPosition());
    }
  }

  return pagePoint;
}


CSSIntPoint
Event::GetClientCoords(nsPresContext* aPresContext,
                       WidgetEvent* aEvent,
                       LayoutDeviceIntPoint aPoint,
                       CSSIntPoint aDefaultPoint)
{
  if (EventStateManager::sIsPointerLocked) {
    return EventStateManager::sLastClientPoint;
  }

  if (!aEvent ||
      (aEvent->mClass != eMouseEventClass &&
       aEvent->mClass != eMouseScrollEventClass &&
       aEvent->mClass != eWheelEventClass &&
       aEvent->mClass != eTouchEventClass &&
       aEvent->mClass != eDragEventClass &&
       aEvent->mClass != ePointerEventClass &&
       aEvent->mClass != eSimpleGestureEventClass) ||
      !aPresContext ||
      !aEvent->AsGUIEvent()->widget) {
    return aDefaultPoint;
  }

  nsIPresShell* shell = aPresContext->GetPresShell();
  if (!shell) {
    return CSSIntPoint(0, 0);
  }

  nsIFrame* rootFrame = shell->GetRootFrame();
  if (!rootFrame) {
    return CSSIntPoint(0, 0);
  }
  nsPoint pt =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
      LayoutDeviceIntPoint::ToUntyped(aPoint), rootFrame);

  return CSSIntPoint::FromAppUnitsRounded(pt);
}




const char*
Event::GetEventName(uint32_t aEventType)
{
  switch(aEventType) {
#define ID_TO_EVENT(name_, _id, _type, _struct) \
  case _id: return #name_;
#include "mozilla/EventNameList.h"
#undef ID_TO_EVENT
  default:
    break;
  }
  
  
  
  
  
  return nullptr;
}

bool
Event::DefaultPrevented(JSContext* aCx) const
{
  MOZ_ASSERT(aCx, "JS context must be specified");

  NS_ENSURE_TRUE(mEvent, false);

  
  if (!mEvent->mFlags.mDefaultPrevented) {
    return false;
  }

  
  
  
  return mEvent->mFlags.mDefaultPreventedByContent || IsChrome(aCx);
}

double
Event::TimeStamp() const
{
  if (!sReturnHighResTimeStamp) {
    return static_cast<double>(mEvent->time);
  }

  if (mEvent->timeStamp.IsNull()) {
    return 0.0;
  }

  if (mIsMainThreadEvent) {
    if (NS_WARN_IF(!mOwner)) {
      return 0.0;
    }

    nsPerformance* perf = mOwner->GetPerformance();
    if (NS_WARN_IF(!perf)) {
      return 0.0;
    }

    return perf->GetDOMTiming()->TimeStampToDOMHighRes(mEvent->timeStamp);
  }

  
  
  
  workers::WorkerPrivate* workerPrivate =
    workers::GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  TimeDuration duration =
    mEvent->timeStamp - workerPrivate->NowBaseTimeStamp();
  return duration.ToMilliseconds();
}

bool
Event::GetPreventDefault() const
{
  if (mOwner) {
    if (nsIDocument* doc = mOwner->GetExtantDoc()) {
      doc->WarnOnceAbout(nsIDocument::eGetPreventDefault);
    }
  }
  
  
  
  return DefaultPrevented();
}

NS_IMETHODIMP
Event::GetPreventDefault(bool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = GetPreventDefault();
  return NS_OK;
}

NS_IMETHODIMP
Event::GetDefaultPrevented(bool* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  
  
  
  *aReturn = DefaultPrevented();
  return NS_OK;
}

NS_IMETHODIMP_(void)
Event::Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("event"));
  }

  nsString type;
  GetType(type);
  IPC::WriteParam(aMsg, type);

  IPC::WriteParam(aMsg, Bubbles());
  IPC::WriteParam(aMsg, Cancelable());
  IPC::WriteParam(aMsg, IsTrusted());

  
}

NS_IMETHODIMP_(bool)
Event::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  nsString type;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &type), false);

  bool bubbles = false;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &bubbles), false);

  bool cancelable = false;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &cancelable), false);

  bool trusted = false;
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &trusted), false);

  nsresult rv = InitEvent(type, bubbles, cancelable);
  NS_ENSURE_SUCCESS(rv, false);
  SetTrusted(trusted);

  return true;
}

NS_IMETHODIMP_(void)
Event::SetOwner(mozilla::dom::EventTarget* aOwner)
{
  mOwner = nullptr;

  if (!aOwner) {
    return;
  }

  nsCOMPtr<nsINode> n = do_QueryInterface(aOwner);
  if (n) {
    mOwner = do_QueryInterface(n->OwnerDoc()->GetScopeObject());
    return;
  }

  nsCOMPtr<nsPIDOMWindow> w = do_QueryInterface(aOwner);
  if (w) {
    if (w->IsOuterWindow()) {
      mOwner = w->GetCurrentInnerWindow();
    } else {
      mOwner.swap(w);
    }
    return;
  }

  nsCOMPtr<DOMEventTargetHelper> eth = do_QueryInterface(aOwner);
  if (eth) {
    mOwner = eth->GetOwner();
    return;
  }

#ifdef DEBUG
  nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(aOwner);
  MOZ_ASSERT(root, "Unexpected EventTarget!");
#endif
}


nsIContent*
Event::GetShadowRelatedTarget(nsIContent* aCurrentTarget,
                              nsIContent* aRelatedTarget)
{
  if (!aCurrentTarget || !aRelatedTarget) {
    return nullptr;
  }

  
  
  
  
  
  ShadowRoot* currentTargetShadow = aCurrentTarget->GetContainingShadow();
  if (!currentTargetShadow) {
    return nullptr;
  }

  nsIContent* relatedTarget = aCurrentTarget;
  while (relatedTarget) {
    ShadowRoot* ancestorShadow = relatedTarget->GetContainingShadow();
    if (currentTargetShadow == ancestorShadow) {
      return relatedTarget;
    }

    
    
    if (!ancestorShadow) {
      return nullptr;
    }

    relatedTarget = ancestorShadow->GetHost();
  }

  return nullptr;
}

} 
} 

using namespace mozilla;
using namespace mozilla::dom;

nsresult
NS_NewDOMEvent(nsIDOMEvent** aInstancePtrResult,
               EventTarget* aOwner,
               nsPresContext* aPresContext,
               WidgetEvent* aEvent) 
{
  Event* it = new Event(aOwner, aPresContext, aEvent);
  NS_ADDREF(it);
  *aInstancePtrResult = static_cast<Event*>(it);
  return NS_OK;
}
