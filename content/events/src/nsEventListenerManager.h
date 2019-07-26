




#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "nsEventListenerManager.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTObserverArray.h"
#include "nsGUIEvent.h"
#include "nsIJSEventListener.h"

class nsIDOMEvent;
class nsIAtom;
class nsIWidget;
struct nsPoint;
struct EventTypeData;
class nsEventTargetChainItem;
class nsPIDOMWindow;
class nsCxPusher;
class nsIEventListenerInfo;

struct nsListenerStruct;
class nsEventListenerManager;

namespace mozilla {
namespace dom {

struct EventListenerFlags
{
  friend struct ::nsListenerStruct;
  friend class  ::nsEventListenerManager;
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
} 

typedef enum
{
    eNativeListener = 0,
    eJSEventListener,
    eWrappedJSListener
} nsListenerType;

struct nsListenerStruct
{
  nsRefPtr<nsIDOMEventListener> mListener;
  nsCOMPtr<nsIAtom>             mTypeAtom;
  uint32_t                      mEventType;
  uint8_t                       mListenerType;
  bool                          mListenerIsHandler : 1;
  bool                          mHandlerIsString : 1;
  bool                          mAllEvents : 1;

  mozilla::dom::EventListenerFlags mFlags;

  nsIJSEventListener* GetJSListener() const {
    return (mListenerType == eJSEventListener) ?
      static_cast<nsIJSEventListener *>(mListener.get()) : nullptr;
  }

  ~nsListenerStruct()
  {
    if ((mListenerType == eJSEventListener) && mListener) {
      static_cast<nsIJSEventListener*>(mListener.get())->Disconnect();
    }
  }

  MOZ_ALWAYS_INLINE bool IsListening(const nsEvent* aEvent) const
  {
    if (mFlags.mInSystemGroup != aEvent->mFlags.mInSystemGroup) {
      return false;
    }
    
    
    
    return ((mFlags.mCapture && aEvent->mFlags.mInCapturePhase) ||
            (!mFlags.mCapture && aEvent->mFlags.mInBubblingPhase));
  }
};





class nsEventListenerManager
{

public:
  nsEventListenerManager(nsISupports* aTarget);
  virtual ~nsEventListenerManager();

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(nsEventListenerManager)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsEventListenerManager)

  void AddEventListener(const nsAString& aType,
                        nsIDOMEventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted);
  void RemoveEventListener(const nsAString& aType,
                           nsIDOMEventListener* aListener,
                           bool aUseCapture);

  void AddListenerForAllEvents(nsIDOMEventListener* aListener,
                               bool aUseCapture,
                               bool aWantsUntrusted,
                               bool aSystemEventGroup);
  void RemoveListenerForAllEvents(nsIDOMEventListener* aListener,
                                  bool aUseCapture,
                                  bool aSystemEventGroup);

  



  void AddEventListenerByType(nsIDOMEventListener *aListener,
                              const nsAString& type,
                              const mozilla::dom::EventListenerFlags& aFlags);
  void RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                 const nsAString& type,
                                 const mozilla::dom::EventListenerFlags& aFlags);

  





  
  
  nsresult SetEventHandler(nsIAtom *aName,
                           const nsAString& aFunc,
                           uint32_t aLanguage,
                           bool aDeferCompilation,
                           bool aPermitUntrustedEvents);
  


  void RemoveEventHandler(nsIAtom *aName);

  void HandleEvent(nsPresContext* aPresContext,
                   nsEvent* aEvent, 
                   nsIDOMEvent** aDOMEvent,
                   nsIDOMEventTarget* aCurrentTarget,
                   nsEventStatus* aEventStatus,
                   nsCxPusher* aPusher)
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
                        aEventStatus, aPusher);
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

  size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const;

  void MarkForCC();

  nsISupports* GetTarget() { return mTarget; }
protected:
  void HandleEventInternal(nsPresContext* aPresContext,
                           nsEvent* aEvent, 
                           nsIDOMEvent** aDOMEvent,
                           nsIDOMEventTarget* aCurrentTarget,
                           nsEventStatus* aEventStatus,
                           nsCxPusher* aPusher);

  nsresult HandleEventSubType(nsListenerStruct* aListenerStruct,
                              nsIDOMEventListener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              nsIDOMEventTarget* aCurrentTarget,
                              nsCxPusher* aPusher);

  




  nsresult CompileEventHandlerInternal(nsListenerStruct *aListenerStruct,
                                       bool aNeedsCxPush,
                                       const nsAString* aBody);

  


  nsListenerStruct* FindEventHandler(uint32_t aEventType, nsIAtom* aTypeAtom);

  







  nsresult SetEventHandlerInternal(nsIScriptContext *aContext,
                                   JSObject* aScopeGlobal,
                                   nsIAtom* aName,
                                   const nsEventHandler& aHandler,
                                   bool aPermitUntrustedEvents,
                                   nsListenerStruct **aListenerStruct);

  bool IsDeviceType(uint32_t aType);
  void EnableDevice(uint32_t aType);
  void DisableDevice(uint32_t aType);

public:
  



  nsresult SetEventHandler(nsIAtom* aEventName,
                           mozilla::dom::EventHandlerNonNull* aHandler);
  nsresult SetEventHandler(mozilla::dom::OnErrorEventHandlerNonNull* aHandler);
  nsresult SetEventHandler(mozilla::dom::BeforeUnloadEventHandlerNonNull* aHandler);

  








  mozilla::dom::EventHandlerNonNull* GetEventHandler(nsIAtom *aEventName)
  {
    const nsEventHandler* handler = GetEventHandlerInternal(aEventName);
    return handler ? handler->EventHandler() : nullptr;
  }
  mozilla::dom::OnErrorEventHandlerNonNull* GetOnErrorEventHandler()
  {
    const nsEventHandler* handler = GetEventHandlerInternal(nsGkAtoms::onerror);
    return handler ? handler->OnErrorEventHandler() : nullptr;
  }
  mozilla::dom::BeforeUnloadEventHandlerNonNull* GetOnBeforeUnloadEventHandler()
  {
    const nsEventHandler* handler =
      GetEventHandlerInternal(nsGkAtoms::onbeforeunload);
    return handler ? handler->BeforeUnloadEventHandler() : nullptr;
  }

protected:
  



  const nsEventHandler* GetEventHandlerInternal(nsIAtom* aEventName);

  void AddEventListenerInternal(
         nsIDOMEventListener* aListener,
         uint32_t aType,
         nsIAtom* aTypeAtom,
         const mozilla::dom::EventListenerFlags& aFlags,
         bool aHandler = false,
         bool aAllEvents = false);
  void RemoveEventListenerInternal(
         nsIDOMEventListener* aListener,
         uint32_t aType,
         nsIAtom* aUserType,
         const mozilla::dom::EventListenerFlags& aFlags,
         bool aAllEvents = false);
  void RemoveAllListeners();
  const EventTypeData* GetTypeDataForIID(const nsIID& aIID);
  const EventTypeData* GetTypeDataForEventName(nsIAtom* aName);
  nsPIDOMWindow* GetInnerWindowForTarget();
  already_AddRefed<nsPIDOMWindow> GetTargetAsInnerWindow() const;

  uint32_t mMayHavePaintEventListener : 1;
  uint32_t mMayHaveMutationListeners : 1;
  uint32_t mMayHaveCapturingListeners : 1;
  uint32_t mMayHaveSystemGroupListeners : 1;
  uint32_t mMayHaveAudioAvailableEventListener : 1;
  uint32_t mMayHaveTouchEventListener : 1;
  uint32_t mMayHaveMouseEnterLeaveEventListener : 1;
  uint32_t mClearingListeners : 1;
  uint32_t mNoListenerForEvent : 24;

  nsAutoTObserverArray<nsListenerStruct, 2> mListeners;
  nsISupports*                              mTarget;  
  nsCOMPtr<nsIAtom>                         mNoListenerForEventAtom;

  static uint32_t                           mInstanceCount;
  static jsid                               sAddListenerID;

  friend class nsEventTargetChainItem;
  static uint32_t                           sCreatedCount;
};





inline nsresult
NS_AddSystemEventListener(nsIDOMEventTarget* aTarget,
                          const nsAString& aType,
                          nsIDOMEventListener *aListener,
                          bool aUseCapture,
                          bool aWantsUntrusted)
{
  nsEventListenerManager* listenerManager = aTarget->GetListenerManager(true);
  NS_ENSURE_STATE(listenerManager);
  mozilla::dom::EventListenerFlags flags;
  flags.mInSystemGroup = true;
  flags.mCapture = aUseCapture;
  flags.mAllowUntrustedEvents = aWantsUntrusted;
  listenerManager->AddEventListenerByType(aListener, aType, flags);
  return NS_OK;
}

#endif 
