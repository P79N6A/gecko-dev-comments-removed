




#ifndef mozilla_dom_Event_h_
#define mozilla_dom_Event_h_

#include "mozilla/Attributes.h"
#include "mozilla/BasicEvents.h"
#include "nsIDOMEvent.h"
#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsPIDOMWindow.h"
#include "nsPoint.h"
#include "nsCycleCollectionParticipant.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/EventBinding.h"
#include "nsIScriptGlobalObject.h"
#include "Units.h"
#include "js/TypeDecls.h"
#include "nsIGlobalObject.h"

class nsIContent;
class nsIDOMEventTarget;
class nsPresContext;

namespace mozilla {
namespace dom {

class EventTarget;
class WantsPopupControlCheck;
#define GENERATED_EVENT(EventClass_) class EventClass_;
#include "mozilla/dom/GeneratedEventList.h"
#undef GENERATED_EVENT


namespace workers {
class ExtendableEvent;
} 



class EventBase : public nsIDOMEvent
{
};

class Event : public EventBase,
              public nsWrapperCache
{
public:
  Event(EventTarget* aOwner,
        nsPresContext* aPresContext,
        WidgetEvent* aEvent);
  explicit Event(nsPIDOMWindow* aWindow);

protected:
  virtual ~Event();

private:
  void ConstructorInit(EventTarget* aOwner,
                       nsPresContext* aPresContext,
                       WidgetEvent* aEvent);

public:
  static Event* FromSupports(nsISupports* aSupports)
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
    return static_cast<Event*>(event);
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Event)

  nsIGlobalObject* GetParentObject()
  {
    return mOwner;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override final;

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto);

#define GENERATED_EVENT(EventClass_) \
  virtual EventClass_* As##EventClass_()  \
  {                                       \
    return nullptr;                       \
  }
#include "mozilla/dom/GeneratedEventList.h"
#undef GENERATED_EVENT

  
  
  virtual workers::ExtendableEvent* AsExtendableEvent()
  {
    return nullptr;
  }

  
  NS_DECL_NSIDOMEVENT

  void InitPresContextData(nsPresContext* aPresContext);

  
  bool Init(EventTarget* aGlobal);

  static PopupControlState GetEventPopupControlState(WidgetEvent* aEvent,
                                                     nsIDOMEvent* aDOMEvent = nullptr);

  static void PopupAllowedEventsChanged();

  static void Shutdown();

  static const char* GetEventName(uint32_t aEventType);
  static CSSIntPoint GetClientCoords(nsPresContext* aPresContext,
                                     WidgetEvent* aEvent,
                                     LayoutDeviceIntPoint aPoint,
                                     CSSIntPoint aDefaultPoint);
  static CSSIntPoint GetPageCoords(nsPresContext* aPresContext,
                                   WidgetEvent* aEvent,
                                   LayoutDeviceIntPoint aPoint,
                                   CSSIntPoint aDefaultPoint);
  static LayoutDeviceIntPoint GetScreenCoords(nsPresContext* aPresContext,
                                              WidgetEvent* aEvent,
                                              LayoutDeviceIntPoint aPoint);
  static CSSIntPoint GetOffsetCoords(nsPresContext* aPresContext,
                                     WidgetEvent* aEvent,
                                     LayoutDeviceIntPoint aPoint,
                                     CSSIntPoint aDefaultPoint);

  static already_AddRefed<Event> Constructor(const GlobalObject& aGlobal,
                                             const nsAString& aType,
                                             const EventInit& aParam,
                                             ErrorResult& aRv);

  
  

  EventTarget* GetTarget() const;
  EventTarget* GetCurrentTarget() const;

  uint16_t EventPhase() const;

  
  

  
  

  bool Bubbles() const
  {
    return mEvent->mFlags.mBubbles;
  }

  bool Cancelable() const
  {
    return mEvent->mFlags.mCancelable;
  }

  
  

  
  
  
  
  void PreventDefault(JSContext* aCx);

  
  
  
  bool DefaultPrevented(JSContext* aCx) const;

  bool DefaultPrevented() const
  {
    return mEvent->mFlags.mDefaultPrevented;
  }

  bool MultipleActionsPrevented() const
  {
    return mEvent->mFlags.mMultipleActionsPrevented;
  }

  bool IsTrusted() const
  {
    return mEvent->mFlags.mIsTrusted;
  }

  bool IsSynthesized() const
  {
    return mEvent->mFlags.mIsSynthesizedForTests;
  }

  double TimeStamp() const;

  void InitEvent(const nsAString& aType, bool aBubbles, bool aCancelable,
                 ErrorResult& aRv)
  {
    aRv = InitEvent(aType, aBubbles, aCancelable);
  }

  EventTarget* GetOriginalTarget() const;
  EventTarget* GetExplicitOriginalTarget() const;
  EventTarget* GetComposedTarget() const;

  bool GetPreventDefault() const;

  





  void PreventDefaultInternal(bool aCalledByDefaultHandler);

  bool IsMainThreadEvent()
  {
    return mIsMainThreadEvent;
  }

  




  static nsIContent* GetShadowRelatedTarget(nsIContent* aCurrentTarget,
                                            nsIContent* aRelatedTarget);

protected:

  
  void SetEventType(const nsAString& aEventTypeArg);
  already_AddRefed<nsIContent> GetTargetFromFrame();

  friend class WantsPopupControlCheck;
  void SetWantsPopupControlCheck(bool aCheck)
  {
    mWantsPopupControlCheck = aCheck;
  }

  bool GetWantsPopupControlCheck()
  {
    return IsTrusted() && mWantsPopupControlCheck;
  }

  



  bool IsChrome(JSContext* aCx) const;

  mozilla::WidgetEvent*       mEvent;
  nsRefPtr<nsPresContext>     mPresContext;
  nsCOMPtr<EventTarget>       mExplicitOriginalTarget;
  nsCOMPtr<nsIGlobalObject>   mOwner;
  bool                        mEventIsInternal;
  bool                        mPrivateDataDuplicated;
  bool                        mIsMainThreadEvent;
  
  
  bool                        mWantsPopupControlCheck;
};

class MOZ_STACK_CLASS WantsPopupControlCheck
{
public:
  explicit WantsPopupControlCheck(nsIDOMEvent* aEvent) :
    mEvent(aEvent->InternalDOMEvent())
  {
    mOriginalWantsPopupControlCheck = mEvent->GetWantsPopupControlCheck();
    mEvent->SetWantsPopupControlCheck(mEvent->IsTrusted());
  }

  ~WantsPopupControlCheck()
  {
    mEvent->SetWantsPopupControlCheck(mOriginalWantsPopupControlCheck);
  }

private:
  Event* mEvent;
  bool mOriginalWantsPopupControlCheck;
};

} 
} 

#define NS_FORWARD_TO_EVENT \
  NS_FORWARD_NSIDOMEVENT(Event::)

#define NS_FORWARD_NSIDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION(_to) \
  NS_IMETHOD GetType(nsAString& aType) override { return _to GetType(aType); } \
  NS_IMETHOD GetTarget(nsIDOMEventTarget** aTarget) override { return _to GetTarget(aTarget); } \
  NS_IMETHOD GetCurrentTarget(nsIDOMEventTarget** aCurrentTarget) override { return _to GetCurrentTarget(aCurrentTarget); } \
  NS_IMETHOD GetEventPhase(uint16_t* aEventPhase) override { return _to GetEventPhase(aEventPhase); } \
  NS_IMETHOD GetBubbles(bool* aBubbles) override { return _to GetBubbles(aBubbles); } \
  NS_IMETHOD GetCancelable(bool* aCancelable) override { return _to GetCancelable(aCancelable); } \
  NS_IMETHOD GetTimeStamp(DOMTimeStamp* aTimeStamp) override { return _to GetTimeStamp(aTimeStamp); } \
  NS_IMETHOD StopPropagation(void) override { return _to StopPropagation(); } \
  NS_IMETHOD PreventDefault(void) override { return _to PreventDefault(); } \
  NS_IMETHOD InitEvent(const nsAString& eventTypeArg, bool canBubbleArg, bool cancelableArg) override { return _to InitEvent(eventTypeArg, canBubbleArg, cancelableArg); } \
  NS_IMETHOD GetDefaultPrevented(bool* aDefaultPrevented) override { return _to GetDefaultPrevented(aDefaultPrevented); } \
  NS_IMETHOD StopImmediatePropagation(void) override { return _to StopImmediatePropagation(); } \
  NS_IMETHOD GetOriginalTarget(nsIDOMEventTarget** aOriginalTarget) override { return _to GetOriginalTarget(aOriginalTarget); } \
  NS_IMETHOD GetExplicitOriginalTarget(nsIDOMEventTarget** aExplicitOriginalTarget) override { return _to GetExplicitOriginalTarget(aExplicitOriginalTarget); } \
  NS_IMETHOD GetPreventDefault(bool* aRetval) override { return _to GetPreventDefault(aRetval); } \
  NS_IMETHOD GetIsTrusted(bool* aIsTrusted) override { return _to GetIsTrusted(aIsTrusted); } \
  NS_IMETHOD SetTarget(nsIDOMEventTarget* aTarget) override { return _to SetTarget(aTarget); } \
  NS_IMETHOD_(bool) IsDispatchStopped(void) override { return _to IsDispatchStopped(); } \
  NS_IMETHOD_(WidgetEvent*) GetInternalNSEvent(void) override { return _to GetInternalNSEvent(); } \
  NS_IMETHOD_(void) SetTrusted(bool aTrusted) override { _to SetTrusted(aTrusted); } \
  NS_IMETHOD_(void) SetOwner(EventTarget* aOwner) override { _to SetOwner(aOwner); } \
  NS_IMETHOD_(Event*) InternalDOMEvent() override { return _to InternalDOMEvent(); }

#define NS_FORWARD_TO_EVENT_NO_SERIALIZATION_NO_DUPLICATION \
  NS_FORWARD_NSIDOMEVENT_NO_SERIALIZATION_NO_DUPLICATION(Event::)

inline nsISupports*
ToSupports(mozilla::dom::Event* e)
{
  return static_cast<nsIDOMEvent*>(e);
}

inline nsISupports*
ToCanonicalSupports(mozilla::dom::Event* e)
{
  return static_cast<nsIDOMEvent*>(e);
}

#endif 
