




#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "mozilla/BasicEvents.h"
#include "mozilla/dom/EventListenerBinding.h"
#include "mozilla/MemoryReporting.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGkAtoms.h"
#include "nsIDOMEventListener.h"
#include "nsIJSEventListener.h"
#include "nsTObserverArray.h"

class nsIDOMEvent;
struct EventTypeData;
class nsEventTargetChainItem;
class nsPIDOMWindow;
class nsIEventListenerInfo;
class nsIScriptContext;

class nsEventListenerManager;

template<class T> class nsCOMArray;

namespace mozilla {

namespace dom {
class EventTarget;
class Element;
} 

typedef dom::CallbackObjectHolder<dom::EventListener,
                                  nsIDOMEventListener> EventListenerHolder;

struct EventListenerFlags
{
  friend class ::nsEventListenerManager;
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

} 





class nsEventListenerManager
{
public:
  struct Listener
  {
    mozilla::EventListenerHolder mListener;
    nsCOMPtr<nsIAtom> mTypeAtom; 
    nsString mTypeString; 
    uint16_t mEventType;

    enum ListenerType MOZ_ENUM_TYPE(uint8_t)
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

    mozilla::EventListenerFlags mFlags;

    nsIJSEventListener* GetJSListener() const
    {
      return (mListenerType == eJSEventListener) ?
        static_cast<nsIJSEventListener *>(mListener.GetXPCOMCallback()) :
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
        static_cast<nsIJSEventListener*>(
          mListener.GetXPCOMCallback())->Disconnect();
      }
    }

    MOZ_ALWAYS_INLINE bool IsListening(const mozilla::WidgetEvent* aEvent) const
    {
      if (mFlags.mInSystemGroup != aEvent->mFlags.mInSystemGroup) {
        return false;
      }
      
      
      
      return ((mFlags.mCapture && aEvent->mFlags.mInCapturePhase) ||
              (!mFlags.mCapture && aEvent->mFlags.mInBubblingPhase));
    }
  };

  nsEventListenerManager(mozilla::dom::EventTarget* aTarget);
  virtual ~nsEventListenerManager();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsEventListenerManager)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsEventListenerManager)

  void AddEventListener(const nsAString& aType,
                        nsIDOMEventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted)
  {
    mozilla::EventListenerHolder holder(aListener);
    AddEventListener(aType, holder, aUseCapture, aWantsUntrusted);
  }
  void AddEventListener(const nsAString& aType,
                        mozilla::dom::EventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted)
  {
    mozilla::EventListenerHolder holder(aListener);
    AddEventListener(aType, holder, aUseCapture, aWantsUntrusted);
  }
  void RemoveEventListener(const nsAString& aType,
                           nsIDOMEventListener* aListener,
                           bool aUseCapture)
  {
    mozilla::EventListenerHolder holder(aListener);
    RemoveEventListener(aType, holder, aUseCapture);
  }
  void RemoveEventListener(const nsAString& aType,
                           mozilla::dom::EventListener* aListener,
                           bool aUseCapture)
  {
    mozilla::EventListenerHolder holder(aListener);
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
                              const mozilla::EventListenerFlags& aFlags)
  {
    mozilla::EventListenerHolder holder(aListener);
    AddEventListenerByType(holder, type, aFlags);
  }
  void AddEventListenerByType(const mozilla::EventListenerHolder& aListener,
                              const nsAString& type,
                              const mozilla::EventListenerFlags& aFlags);
  void RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                 const nsAString& type,
                                 const mozilla::EventListenerFlags& aFlags)
  {
    mozilla::EventListenerHolder holder(aListener);
    RemoveEventListenerByType(holder, type, aFlags);
  }
  void RemoveEventListenerByType(const mozilla::EventListenerHolder& aListener,
                                 const nsAString& type,
                                 const mozilla::EventListenerFlags& aFlags);

  







  
  
  nsresult SetEventHandler(nsIAtom *aName,
                           const nsAString& aFunc,
                           uint32_t aLanguage,
                           bool aDeferCompilation,
                           bool aPermitUntrustedEvents,
                           mozilla::dom::Element* aElement);
  


  void RemoveEventHandler(nsIAtom *aName, const nsAString& aTypeString);

  void HandleEvent(nsPresContext* aPresContext,
                   mozilla::WidgetEvent* aEvent, 
                   nsIDOMEvent** aDOMEvent,
                   mozilla::dom::EventTarget* aCurrentTarget,
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

  



  bool MayHaveAudioAvailableEventListener() { return mMayHaveAudioAvailableEventListener; }

  



  bool MayHaveTouchEventListener() { return mMayHaveTouchEventListener; }

  bool MayHaveMouseEnterLeaveEventListener() { return mMayHaveMouseEnterLeaveEventListener; }
  bool MayHavePointerEnterLeaveEventListener() { return mMayHavePointerEnterLeaveEventListener; }

  size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  uint32_t ListenerCount() const
  {
    return mListeners.Length();
  }

  void MarkForCC();

  mozilla::dom::EventTarget* GetTarget() { return mTarget; }
protected:
  void HandleEventInternal(nsPresContext* aPresContext,
                           mozilla::WidgetEvent* aEvent,
                           nsIDOMEvent** aDOMEvent,
                           mozilla::dom::EventTarget* aCurrentTarget,
                           nsEventStatus* aEventStatus);

  nsresult HandleEventSubType(Listener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              mozilla::dom::EventTarget* aCurrentTarget);

  





  nsresult CompileEventHandlerInternal(Listener* aListener,
                                       const nsAString* aBody,
                                       mozilla::dom::Element* aElement);

  


  Listener* FindEventHandler(uint32_t aEventType,
                             nsIAtom* aTypeAtom,
                             const nsAString& aTypeString);

  






  Listener* SetEventHandlerInternal(JS::Handle<JSObject*> aScopeGlobal,
                                    nsIAtom* aName,
                                    const nsAString& aTypeString,
                                    const nsEventHandler& aHandler,
                                    bool aPermitUntrustedEvents);

  bool IsDeviceType(uint32_t aType);
  void EnableDevice(uint32_t aType);
  void DisableDevice(uint32_t aType);

public:
  



  void SetEventHandler(nsIAtom* aEventName,
                       const nsAString& aTypeString,
                       mozilla::dom::EventHandlerNonNull* aHandler);
  void SetEventHandler(mozilla::dom::OnErrorEventHandlerNonNull* aHandler);
  void SetEventHandler(mozilla::dom::OnBeforeUnloadEventHandlerNonNull* aHandler);

  








  mozilla::dom::EventHandlerNonNull* GetEventHandler(nsIAtom *aEventName,
                                                     const nsAString& aTypeString)
  {
    const nsEventHandler* handler =
      GetEventHandlerInternal(aEventName, aTypeString);
    return handler ? handler->EventHandler() : nullptr;
  }
  mozilla::dom::OnErrorEventHandlerNonNull* GetOnErrorEventHandler()
  {
    const nsEventHandler* handler;
    if (mIsMainThreadELM) {
      handler = GetEventHandlerInternal(nsGkAtoms::onerror, EmptyString());
    } else {
      handler = GetEventHandlerInternal(nullptr, NS_LITERAL_STRING("error"));
    }
    return handler ? handler->OnErrorEventHandler() : nullptr;
  }
  mozilla::dom::OnBeforeUnloadEventHandlerNonNull* GetOnBeforeUnloadEventHandler()
  {
    const nsEventHandler* handler =
      GetEventHandlerInternal(nsGkAtoms::onbeforeunload, EmptyString());
    return handler ? handler->OnBeforeUnloadEventHandler() : nullptr;
  }

protected:
  



  const nsEventHandler* GetEventHandlerInternal(nsIAtom* aEventName,
                                                const nsAString& aTypeString);

  void AddEventListener(const nsAString& aType,
                        const mozilla::EventListenerHolder& aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted);
  void RemoveEventListener(const nsAString& aType,
                           const mozilla::EventListenerHolder& aListener,
                           bool aUseCapture);

  void AddEventListenerInternal(
         const mozilla::EventListenerHolder& aListener,
         uint32_t aType,
         nsIAtom* aTypeAtom,
         const nsAString& aTypeString,
         const mozilla::EventListenerFlags& aFlags,
         bool aHandler = false,
         bool aAllEvents = false);
  void RemoveEventListenerInternal(
         const mozilla::EventListenerHolder& aListener,
         uint32_t aType,
         nsIAtom* aUserType,
         const nsAString& aTypeString,
         const mozilla::EventListenerFlags& aFlags,
         bool aAllEvents = false);
  void RemoveAllListeners();
  const EventTypeData* GetTypeDataForIID(const nsIID& aIID);
  const EventTypeData* GetTypeDataForEventName(nsIAtom* aName);
  nsPIDOMWindow* GetInnerWindowForTarget();
  already_AddRefed<nsPIDOMWindow> GetTargetAsInnerWindow() const;

  bool ListenerCanHandle(Listener* aListener, mozilla::WidgetEvent* aEvent);

  already_AddRefed<nsIScriptGlobalObject>
  GetScriptGlobalAndDocument(nsIDocument** aDoc);

  uint32_t mMayHavePaintEventListener : 1;
  uint32_t mMayHaveMutationListeners : 1;
  uint32_t mMayHaveCapturingListeners : 1;
  uint32_t mMayHaveSystemGroupListeners : 1;
  uint32_t mMayHaveAudioAvailableEventListener : 1;
  uint32_t mMayHaveTouchEventListener : 1;
  uint32_t mMayHaveMouseEnterLeaveEventListener : 1;
  uint32_t mMayHavePointerEnterLeaveEventListener : 1;
  uint32_t mClearingListeners : 1;
  uint32_t mIsMainThreadELM : 1;
  uint32_t mNoListenerForEvent : 22;

  nsAutoTObserverArray<Listener, 2> mListeners;
  mozilla::dom::EventTarget*                mTarget;  
  nsCOMPtr<nsIAtom>                         mNoListenerForEventAtom;

  friend class ELMCreationDetector;
  static uint32_t                           sMainThreadCreatedCount;
};





inline nsresult
NS_AddSystemEventListener(mozilla::dom::EventTarget* aTarget,
                          const nsAString& aType,
                          nsIDOMEventListener *aListener,
                          bool aUseCapture,
                          bool aWantsUntrusted)
{
  nsEventListenerManager* listenerManager =
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
