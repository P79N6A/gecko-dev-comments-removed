




#ifndef nsDOMEvent_h__
#define nsDOMEvent_h__

#include "nsIDOMEvent.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsPIDOMWindow.h"
#include "nsPoint.h"
#include "nsGUIEvent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "nsIJSNativeInitializer.h"
#include "mozilla/dom/EventTarget.h"
#include "mozilla/dom/EventBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "nsIScriptGlobalObject.h"

class nsIContent;
class nsPresContext;
struct JSContext;
class JSObject;

class nsDOMEvent : public nsIDOMEvent,
                   public nsIJSNativeInitializer,
                   public nsWrapperCache
{
protected:
  nsDOMEvent(mozilla::dom::EventTarget* aOwner, nsPresContext* aPresContext,
             nsEvent* aEvent);
  virtual ~nsDOMEvent();
public:
  void GetParentObject(nsIScriptGlobalObject** aParentObject)
  {
    if (mOwner) {
      CallQueryInterface(mOwner, aParentObject);
    } else {
      *aParentObject = nullptr;
    }
  }

  static nsDOMEvent* FromSupports(nsISupports* aSupports)
  {
    nsIDOMEvent* event =
      static_cast<nsIDOMEvent*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsIDOMEvent> target_qi =
        do_QueryInterface(aSupports);

      
      
      
      MOZ_ASSERT(target_qi == event, "Uh, fix QI!");
    }
#endif
    return static_cast<nsDOMEvent*>(event);
  }

  static already_AddRefed<nsDOMEvent> CreateEvent(mozilla::dom::EventTarget* aOwner,
                                                  nsPresContext* aPresContext,
                                                  nsEvent* aEvent)
  {
    nsRefPtr<nsDOMEvent> e = new nsDOMEvent(aOwner, aPresContext, aEvent);
    e->SetIsDOMBinding();
    return e.forget();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(nsDOMEvent, nsIDOMEvent)

  nsISupports* GetParentObject()
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE
  {
    return mozilla::dom::EventBinding::Wrap(aCx, aScope, this);
  }

  
  NS_DECL_NSIDOMEVENT

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aCx, JSObject* aObj,
                        uint32_t aArgc, jsval* aArgv);

  virtual nsresult InitFromCtor(const nsAString& aType,
                                JSContext* aCx, jsval* aVal);

  void InitPresContextData(nsPresContext* aPresContext);

  static PopupControlState GetEventPopupControlState(nsEvent *aEvent);

  static void PopupAllowedEventsChanged();

  static void Shutdown();

  static const char* GetEventName(uint32_t aEventType);
  static nsIntPoint GetClientCoords(nsPresContext* aPresContext,
                                    nsEvent* aEvent,
                                    nsIntPoint aPoint,
                                    nsIntPoint aDefaultPoint);
  static nsIntPoint GetPageCoords(nsPresContext* aPresContext,
                                  nsEvent* aEvent,
                                  nsIntPoint aPoint,
                                  nsIntPoint aDefaultPoint);
  static nsIntPoint GetScreenCoords(nsPresContext* aPresContext,
                                    nsEvent* aEvent,
                                    nsIntPoint aPoint);

  static already_AddRefed<nsDOMEvent> Constructor(const mozilla::dom::GlobalObject& aGlobal,
                                                  const nsAString& aType,
                                                  const mozilla::dom::EventInit& aParam,
                                                  mozilla::ErrorResult& aRv);

  
  

  already_AddRefed<mozilla::dom::EventTarget> GetTarget()
  {
    nsCOMPtr<nsIDOMEventTarget> t;
    GetTarget(getter_AddRefs(t));
    nsCOMPtr<mozilla::dom::EventTarget> et = do_QueryInterface(t);
    return et.forget();
  }

  already_AddRefed<mozilla::dom::EventTarget> GetCurrentTarget()
  {
    nsCOMPtr<nsIDOMEventTarget> t;
    GetCurrentTarget(getter_AddRefs(t));
    nsCOMPtr<mozilla::dom::EventTarget> et = do_QueryInterface(t);
    return et.forget();
  }

  uint16_t EventPhase()
  {
    uint16_t p;
    GetEventPhase(&p);
    return p;
  }

  
  

  
  

  bool Bubbles()
  {
    bool b;
    GetBubbles(&b);
    return b;
  }

  bool Cancelable()
  {
    bool c;
    GetCancelable(&c);
    return c;
  }

  
  

  bool DefaultPrevented()
  {
    bool d;
    GetDefaultPrevented(&d);
    return d;
  }

  bool MultipleActionsPrevented()
  {
    return mEvent->mFlags.mMultipleActionsPrevented;
  }

  bool IsTrusted()
  {
    bool i;
    GetIsTrusted(&i);
    return i;
  }

  uint64_t TimeStamp()
  {
    uint64_t t;
    GetTimeStamp(&t);
    return t;
  }

  void InitEvent(const nsAString& aType, bool aBubbles, bool aCancelable,
                 mozilla::ErrorResult& aRv)
  {
    aRv = InitEvent(aType, aBubbles, aCancelable);
  }

  already_AddRefed<mozilla::dom::EventTarget> GetOriginalTarget()
  {
    nsCOMPtr<nsIDOMEventTarget> t;
    GetOriginalTarget(getter_AddRefs(t));
    nsCOMPtr<mozilla::dom::EventTarget> et = do_QueryInterface(t);
    return et.forget();
  }

  already_AddRefed<mozilla::dom::EventTarget> GetExplicitOriginalTarget()
  {
    nsCOMPtr<nsIDOMEventTarget> t;
    GetExplicitOriginalTarget(getter_AddRefs(t));
    nsCOMPtr<mozilla::dom::EventTarget> et = do_QueryInterface(t);
    return et.forget();
  }

  bool GetPreventDefault()
  {
    bool d;
    GetDefaultPrevented(&d);
    return d;
  }

protected:

  
  void SetEventType(const nsAString& aEventTypeArg);
  already_AddRefed<nsIContent> GetTargetFromFrame();

  nsEvent*                    mEvent;
  nsRefPtr<nsPresContext>     mPresContext;
  nsCOMPtr<nsIDOMEventTarget> mExplicitOriginalTarget;
  nsCOMPtr<nsPIDOMWindow>     mOwner; 
  nsString                    mCachedType;
  bool                        mEventIsInternal;
  bool                        mPrivateDataDuplicated;
};

#define NS_FORWARD_TO_NSDOMEVENT \
  NS_FORWARD_NSIDOMEVENT(nsDOMEvent::)

#define NS_FORWARD_NSIDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION(_to) \
  NS_IMETHOD GetType(nsAString& aType){ return _to GetType(aType); } \
  NS_IMETHOD GetTarget(nsIDOMEventTarget * *aTarget) { return _to GetTarget(aTarget); } \
  NS_IMETHOD GetCurrentTarget(nsIDOMEventTarget * *aCurrentTarget) { return _to GetCurrentTarget(aCurrentTarget); } \
  NS_IMETHOD GetEventPhase(uint16_t *aEventPhase) { return _to GetEventPhase(aEventPhase); } \
  NS_IMETHOD GetBubbles(bool *aBubbles) { return _to GetBubbles(aBubbles); } \
  NS_IMETHOD GetCancelable(bool *aCancelable) { return _to GetCancelable(aCancelable); } \
  NS_IMETHOD GetTimeStamp(DOMTimeStamp *aTimeStamp) { return _to GetTimeStamp(aTimeStamp); } \
  NS_IMETHOD StopPropagation(void) { return _to StopPropagation(); } \
  NS_IMETHOD PreventDefault(void) { return _to PreventDefault(); } \
  NS_IMETHOD InitEvent(const nsAString & eventTypeArg, bool canBubbleArg, bool cancelableArg) { return _to InitEvent(eventTypeArg, canBubbleArg, cancelableArg); } \
  NS_IMETHOD GetDefaultPrevented(bool *aDefaultPrevented) { return _to GetDefaultPrevented(aDefaultPrevented); } \
  NS_IMETHOD StopImmediatePropagation(void) { return _to StopImmediatePropagation(); } \
  NS_IMETHOD GetOriginalTarget(nsIDOMEventTarget** aOriginalTarget) { return _to GetOriginalTarget(aOriginalTarget); } \
  NS_IMETHOD GetExplicitOriginalTarget(nsIDOMEventTarget** aExplicitOriginalTarget) { return _to GetExplicitOriginalTarget(aExplicitOriginalTarget); } \
  NS_IMETHOD PreventBubble() { return _to PreventBubble(); } \
  NS_IMETHOD PreventCapture() { return _to PreventCapture(); } \
  NS_IMETHOD GetPreventDefault(bool* aRetval) { return _to GetPreventDefault(aRetval); } \
  NS_IMETHOD GetIsTrusted(bool* aIsTrusted) { return _to GetIsTrusted(aIsTrusted); } \
  NS_IMETHOD SetTarget(nsIDOMEventTarget *aTarget) { return _to SetTarget(aTarget); } \
  NS_IMETHOD_(bool) IsDispatchStopped(void) { return _to IsDispatchStopped(); } \
  NS_IMETHOD_(nsEvent *) GetInternalNSEvent(void) { return _to GetInternalNSEvent(); } \
  NS_IMETHOD_(void) SetTrusted(bool aTrusted) { _to SetTrusted(aTrusted); } \
  NS_IMETHOD_(void) SetOwner(mozilla::dom::EventTarget* aOwner) { _to SetOwner(aOwner); } \
  NS_IMETHOD_(nsDOMEvent *) InternalDOMEvent(void) { return _to InternalDOMEvent(); }

#define NS_FORWARD_TO_NSDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION \
  NS_FORWARD_NSIDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION(nsDOMEvent::)

inline nsISupports*
ToSupports(nsDOMEvent* e)
{
  return static_cast<nsIDOMEvent*>(e);
}

inline nsISupports*
ToCanonicalSupports(nsDOMEvent* e)
{
  return static_cast<nsIDOMEvent*>(e);
}

#endif 
