




#ifndef mozilla_EventListenerManager_h_
#define mozilla_EventListenerManager_h_

#include "mozilla/BasicEvents.h"
#include "mozilla/dom/EventListenerBinding.h"
#include "mozilla/JSEventHandler.h"
#include "mozilla/MemoryReporting.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventListener.h"
#include "nsTObserverArray.h"

class nsIDocShell;
class nsIDOMEvent;
class nsIEventListenerInfo;
class nsPIDOMWindow;
class JSTracer;

struct EventTypeData;

template<class T> class nsCOMArray;

namespace mozilla {

class ELMCreationDetector;
class EventListenerManager;

namespace dom {
class EventTarget;
class Element;
} 

typedef dom::CallbackObjectHolder<dom::EventListener,
                                  nsIDOMEventListener> EventListenerHolder;

struct EventListenerFlags
{
  friend class EventListenerManager;
private:
  
  
  bool mListenerIsJSListener : 1;

public:
  
  
  bool mCapture : 1;
  
  
  bool mInSystemGroup : 1;
  
  
  bool mAllowUntrustedEvents : 1;

  EventListenerFlags() :
    mListenerIsJSListener(false),
    mCapture(false), mInSystemGroup(false), mAllowUntrustedEvents(false)
  {
  }

  bool Equals(const EventListenerFlags& aOther) const
  {
    return (mCapture == aOther.mCapture &&
            mInSystemGroup == aOther.mInSystemGroup &&
            mListenerIsJSListener == aOther.mListenerIsJSListener &&
            mAllowUntrustedEvents == aOther.mAllowUntrustedEvents);
  }

  bool EqualsIgnoringTrustness(const EventListenerFlags& aOther) const
  {
    return (mCapture == aOther.mCapture &&
            mInSystemGroup == aOther.mInSystemGroup &&
            mListenerIsJSListener == aOther.mListenerIsJSListener);
  }

  bool operator==(const EventListenerFlags& aOther) const
  {
    return Equals(aOther);
  }
};

inline EventListenerFlags TrustedEventsAtBubble()
{
  EventListenerFlags flags;
  return flags;
}

inline EventListenerFlags TrustedEventsAtCapture()
{
  EventListenerFlags flags;
  flags.mCapture = true;
  return flags;
}

inline EventListenerFlags AllEventsAtBubbe()
{
  EventListenerFlags flags;
  flags.mAllowUntrustedEvents = true;
  return flags;
}

inline EventListenerFlags AllEventsAtCapture()
{
  EventListenerFlags flags;
  flags.mCapture = true;
  flags.mAllowUntrustedEvents = true;
  return flags;
}

inline EventListenerFlags TrustedEventsAtSystemGroupBubble()
{
  EventListenerFlags flags;
  flags.mInSystemGroup = true;
  return flags;
}

inline EventListenerFlags TrustedEventsAtSystemGroupCapture()
{
  EventListenerFlags flags;
  flags.mCapture = true;
  flags.mInSystemGroup = true;
  return flags;
}

inline EventListenerFlags AllEventsAtSystemGroupBubble()
{
  EventListenerFlags flags;
  flags.mInSystemGroup = true;
  flags.mAllowUntrustedEvents = true;
  return flags;
}

inline EventListenerFlags AllEventsAtSystemGroupCapture()
{
  EventListenerFlags flags;
  flags.mCapture = true;
  flags.mInSystemGroup = true;
  flags.mAllowUntrustedEvents = true;
  return flags;
}





class EventListenerManager final
{
  ~EventListenerManager();

public:
  struct Listener
  {
    EventListenerHolder mListener;
    nsCOMPtr<nsIAtom> mTypeAtom; 
    nsString mTypeString; 
    uint16_t mEventType;

    enum ListenerType : uint8_t
    {
      eNativeListener = 0,
      eJSEventListener,
      eWrappedJSListener,
      eWebIDLListener,
      eListenerTypeCount
    };
    uint8_t mListenerType;

    bool mListenerIsHandler : 1;
    bool mHandlerIsString : 1;
    bool mAllEvents : 1;

    EventListenerFlags mFlags;

    JSEventHandler* GetJSEventHandler() const
    {
      return (mListenerType == eJSEventListener) ?
        static_cast<JSEventHandler*>(mListener.GetXPCOMCallback()) :
        nullptr;
    }

    Listener()
    {
      MOZ_ASSERT(sizeof(mListenerType) == 1);
      MOZ_ASSERT(eListenerTypeCount < 255);
    }

    ~Listener()
    {
      if ((mListenerType == eJSEventListener) && mListener) {
        static_cast<JSEventHandler*>(
          mListener.GetXPCOMCallback())->Disconnect();
      }
    }

    MOZ_ALWAYS_INLINE bool IsListening(const WidgetEvent* aEvent) const
    {
      if (mFlags.mInSystemGroup != aEvent->mFlags.mInSystemGroup) {
        return false;
      }
      
      
      
      return ((mFlags.mCapture && aEvent->mFlags.mInCapturePhase) ||
              (!mFlags.mCapture && aEvent->mFlags.mInBubblingPhase));
    }
  };

  explicit EventListenerManager(dom::EventTarget* aTarget);

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(EventListenerManager)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(EventListenerManager)

  void AddEventListener(const nsAString& aType,
                        nsIDOMEventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted)
  {
    EventListenerHolder holder(aListener);
    AddEventListener(aType, holder, aUseCapture, aWantsUntrusted);
  }
  void AddEventListener(const nsAString& aType,
                        dom::EventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted)
  {
    EventListenerHolder holder(aListener);
    AddEventListener(aType, holder, aUseCapture, aWantsUntrusted);
  }
  void RemoveEventListener(const nsAString& aType,
                           nsIDOMEventListener* aListener,
                           bool aUseCapture)
  {
    EventListenerHolder holder(aListener);
    RemoveEventListener(aType, holder, aUseCapture);
  }
  void RemoveEventListener(const nsAString& aType,
                           dom::EventListener* aListener,
                           bool aUseCapture)
  {
    EventListenerHolder holder(aListener);
    RemoveEventListener(aType, holder, aUseCapture);
  }

  void AddListenerForAllEvents(nsIDOMEventListener* aListener,
                               bool aUseCapture,
                               bool aWantsUntrusted,
                               bool aSystemEventGroup);
  void RemoveListenerForAllEvents(nsIDOMEventListener* aListener,
                                  bool aUseCapture,
                                  bool aSystemEventGroup);

  



  void AddEventListenerByType(nsIDOMEventListener *aListener,
                              const nsAString& type,
                              const EventListenerFlags& aFlags)
  {
    EventListenerHolder holder(aListener);
    AddEventListenerByType(holder, type, aFlags);
  }
  void AddEventListenerByType(const EventListenerHolder& aListener,
                              const nsAString& type,
                              const EventListenerFlags& aFlags);
  void RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                 const nsAString& type,
                                 const EventListenerFlags& aFlags)
  {
    EventListenerHolder holder(aListener);
    RemoveEventListenerByType(holder, type, aFlags);
  }
  void RemoveEventListenerByType(const EventListenerHolder& aListener,
                                 const nsAString& type,
                                 const EventListenerFlags& aFlags);

  







  
  
  nsresult SetEventHandler(nsIAtom *aName,
                           const nsAString& aFunc,
                           bool aDeferCompilation,
                           bool aPermitUntrustedEvents,
                           dom::Element* aElement);
  


  void RemoveEventHandler(nsIAtom *aName, const nsAString& aTypeString);

  void HandleEvent(nsPresContext* aPresContext,
                   WidgetEvent* aEvent, 
                   nsIDOMEvent** aDOMEvent,
                   dom::EventTarget* aCurrentTarget,
                   nsEventStatus* aEventStatus)
  {
    if (mListeners.IsEmpty() || aEvent->mFlags.mPropagationStopped) {
      return;
    }

    if (!mMayHaveCapturingListeners && !aEvent->mFlags.mInBubblingPhase) {
      return;
    }

    if (!mMayHaveSystemGroupListeners && aEvent->mFlags.mInSystemGroup) {
      return;
    }

    
    if (mNoListenerForEvent == aEvent->message &&
        (mNoListenerForEvent != NS_USER_DEFINED_EVENT ||
         mNoListenerForEventAtom == aEvent->userType)) {
      return;
    }
    HandleEventInternal(aPresContext, aEvent, aDOMEvent, aCurrentTarget,
                        aEventStatus);
  }

  



  void Disconnect();

  


  bool HasMutationListeners();

  



  bool HasUnloadListeners();

  






  uint32_t MutationListenerBits();

  


  bool HasListenersFor(const nsAString& aEventName);

  



  bool HasListenersFor(nsIAtom* aEventNameWithOn);

  


  bool HasListeners();

  



  nsresult GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList);

  uint32_t GetIdentifierForEvent(nsIAtom* aEvent);

  static void Shutdown();

  



  bool MayHavePaintEventListener() { return mMayHavePaintEventListener; }

  



  bool MayHaveTouchEventListener() { return mMayHaveTouchEventListener; }

  bool MayHaveMouseEnterLeaveEventListener() { return mMayHaveMouseEnterLeaveEventListener; }
  bool MayHavePointerEnterLeaveEventListener() { return mMayHavePointerEnterLeaveEventListener; }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;

  uint32_t ListenerCount() const
  {
    return mListeners.Length();
  }

  void MarkForCC();

  void TraceListeners(JSTracer* aTrc);

  dom::EventTarget* GetTarget() { return mTarget; }

protected:
  void HandleEventInternal(nsPresContext* aPresContext,
                           WidgetEvent* aEvent,
                           nsIDOMEvent** aDOMEvent,
                           dom::EventTarget* aCurrentTarget,
                           nsEventStatus* aEventStatus);

  nsresult HandleEventSubType(Listener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              dom::EventTarget* aCurrentTarget);

  nsIDocShell* GetDocShellForTarget();

  





  nsresult CompileEventHandlerInternal(Listener* aListener,
                                       const nsAString* aBody,
                                       dom::Element* aElement);

  


  Listener* FindEventHandler(uint32_t aEventType,
                             nsIAtom* aTypeAtom,
                             const nsAString& aTypeString);

  






  Listener* SetEventHandlerInternal(nsIAtom* aName,
                                    const nsAString& aTypeString,
                                    const TypedEventHandler& aHandler,
                                    bool aPermitUntrustedEvents);

  bool IsDeviceType(uint32_t aType);
  void EnableDevice(uint32_t aType);
  void DisableDevice(uint32_t aType);

public:
  



  void SetEventHandler(nsIAtom* aEventName,
                       const nsAString& aTypeString,
                       dom::EventHandlerNonNull* aHandler);
  void SetEventHandler(dom::OnErrorEventHandlerNonNull* aHandler);
  void SetEventHandler(dom::OnBeforeUnloadEventHandlerNonNull* aHandler);

  








  dom::EventHandlerNonNull* GetEventHandler(nsIAtom* aEventName,
                                            const nsAString& aTypeString)
  {
    const TypedEventHandler* typedHandler =
      GetTypedEventHandler(aEventName, aTypeString);
    return typedHandler ? typedHandler->NormalEventHandler() : nullptr;
  }

  dom::OnErrorEventHandlerNonNull* GetOnErrorEventHandler()
  {
    const TypedEventHandler* typedHandler = mIsMainThreadELM ?
      GetTypedEventHandler(nsGkAtoms::onerror, EmptyString()) :
      GetTypedEventHandler(nullptr, NS_LITERAL_STRING("error"));
    return typedHandler ? typedHandler->OnErrorEventHandler() : nullptr;
  }

  dom::OnBeforeUnloadEventHandlerNonNull* GetOnBeforeUnloadEventHandler()
  {
    const TypedEventHandler* typedHandler =
      GetTypedEventHandler(nsGkAtoms::onbeforeunload, EmptyString());
    return typedHandler ? typedHandler->OnBeforeUnloadEventHandler() : nullptr;
  }

protected:
  



  const TypedEventHandler* GetTypedEventHandler(nsIAtom* aEventName,
                                                const nsAString& aTypeString);

  void AddEventListener(const nsAString& aType,
                        const EventListenerHolder& aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted);
  void RemoveEventListener(const nsAString& aType,
                           const EventListenerHolder& aListener,
                           bool aUseCapture);

  void AddEventListenerInternal(const EventListenerHolder& aListener,
                                uint32_t aType,
                                nsIAtom* aTypeAtom,
                                const nsAString& aTypeString,
                                const EventListenerFlags& aFlags,
                                bool aHandler = false,
                                bool aAllEvents = false);
  void RemoveEventListenerInternal(const EventListenerHolder& aListener,
                                   uint32_t aType,
                                   nsIAtom* aUserType,
                                   const nsAString& aTypeString,
                                   const EventListenerFlags& aFlags,
                                   bool aAllEvents = false);
  void RemoveAllListeners();
  const EventTypeData* GetTypeDataForIID(const nsIID& aIID);
  const EventTypeData* GetTypeDataForEventName(nsIAtom* aName);
  nsPIDOMWindow* GetInnerWindowForTarget();
  already_AddRefed<nsPIDOMWindow> GetTargetAsInnerWindow() const;

  bool ListenerCanHandle(Listener* aListener, WidgetEvent* aEvent);

  already_AddRefed<nsIScriptGlobalObject>
  GetScriptGlobalAndDocument(nsIDocument** aDoc);

  uint32_t mMayHavePaintEventListener : 1;
  uint32_t mMayHaveMutationListeners : 1;
  uint32_t mMayHaveCapturingListeners : 1;
  uint32_t mMayHaveSystemGroupListeners : 1;
  uint32_t mMayHaveTouchEventListener : 1;
  uint32_t mMayHaveMouseEnterLeaveEventListener : 1;
  uint32_t mMayHavePointerEnterLeaveEventListener : 1;
  uint32_t mClearingListeners : 1;
  uint32_t mIsMainThreadELM : 1;
  uint32_t mNoListenerForEvent : 23;

  nsAutoTObserverArray<Listener, 2> mListeners;
  dom::EventTarget* MOZ_NON_OWNING_REF mTarget;
  nsCOMPtr<nsIAtom> mNoListenerForEventAtom;

  friend class ELMCreationDetector;
  static uint32_t sMainThreadCreatedCount;
};

} 





inline nsresult
NS_AddSystemEventListener(mozilla::dom::EventTarget* aTarget,
                          const nsAString& aType,
                          nsIDOMEventListener *aListener,
                          bool aUseCapture,
                          bool aWantsUntrusted)
{
  mozilla::EventListenerManager* listenerManager =
    aTarget->GetOrCreateListenerManager();
  NS_ENSURE_STATE(listenerManager);
  mozilla::EventListenerFlags flags;
  flags.mInSystemGroup = true;
  flags.mCapture = aUseCapture;
  flags.mAllowUntrustedEvents = aWantsUntrusted;
  listenerManager->AddEventListenerByType(aListener, aType, flags);
  return NS_OK;
}

#endif 
