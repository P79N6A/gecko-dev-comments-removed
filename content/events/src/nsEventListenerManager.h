




































#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "nsEventListenerManager.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsHashtable.h"
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
class nsIDocument;

struct nsListenerStruct
{
  nsRefPtr<nsIDOMEventListener> mListener;
  PRUint32                      mEventType;
  nsCOMPtr<nsIAtom>             mTypeAtom;
  PRUint16                      mFlags;
  bool                          mHandlerIsString;

  nsIJSEventListener* GetJSListener() const {
    return (mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) ?
      static_cast<nsIJSEventListener *>(mListener.get()) : nsnull;
  }

  ~nsListenerStruct()
  {
    if ((mFlags & NS_PRIV_EVENT_FLAG_SCRIPT) && mListener) {
      static_cast<nsIJSEventListener*>(mListener.get())->Disconnect();
    }
  }
};





class nsEventListenerManager
{

public:
  nsEventListenerManager(nsISupports* aTarget);
  virtual ~nsEventListenerManager();

  NS_INLINE_DECL_REFCOUNTING(nsEventListenerManager)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsEventListenerManager)

  void AddEventListener(const nsAString& aType,
                        nsIDOMEventListener* aListener,
                        bool aUseCapture,
                        bool aWantsUntrusted);
  void RemoveEventListener(const nsAString& aType,
                           nsIDOMEventListener* aListener,
                           bool aUseCapture);

  



  void AddEventListenerByType(nsIDOMEventListener *aListener,
                              const nsAString& type,
                              PRInt32 aFlags);
  void RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                 const nsAString& type,
                                 PRInt32 aFlags);

  





  
  
  nsresult AddScriptEventListener(nsIAtom *aName,
                                  const nsAString& aFunc,
                                  PRUint32 aLanguage,
                                  bool aDeferCompilation,
                                  bool aPermitUntrustedEvents);
  


  void RemoveScriptEventListener(nsIAtom *aName);

  void HandleEvent(nsPresContext* aPresContext,
                   nsEvent* aEvent, 
                   nsIDOMEvent** aDOMEvent,
                   nsIDOMEventTarget* aCurrentTarget,
                   PRUint32 aFlags,
                   nsEventStatus* aEventStatus,
                   nsCxPusher* aPusher)
  {
    if (mListeners.IsEmpty() || aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
      return;
    }

    if (!mMayHaveCapturingListeners &&
        !(aEvent->flags & NS_EVENT_FLAG_BUBBLE)) {
      return;
    }

    if (!mMayHaveSystemGroupListeners &&
        aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) {
      return;
    }

    
    if (mNoListenerForEvent == aEvent->message &&
        (mNoListenerForEvent != NS_USER_DEFINED_EVENT ||
         mNoListenerForEventAtom == aEvent->userType)) {
      return;
    }
    HandleEventInternal(aPresContext, aEvent, aDOMEvent, aCurrentTarget,
                        aFlags, aEventStatus, aPusher);
  }

  void HandleEventInternal(nsPresContext* aPresContext,
                           nsEvent* aEvent, 
                           nsIDOMEvent** aDOMEvent,
                           nsIDOMEventTarget* aCurrentTarget,
                           PRUint32 aFlags,
                           nsEventStatus* aEventStatus,
                           nsCxPusher* aPusher);

  



  void Disconnect();

  


  bool HasMutationListeners();

  



  bool HasUnloadListeners();

  






  PRUint32 MutationListenerBits();

  


  bool HasListenersFor(const nsAString& aEventName);

  


  bool HasListeners();

  



  nsresult GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList);

  PRUint32 GetIdentifierForEvent(nsIAtom* aEvent);

  static void Shutdown();

  



  bool MayHavePaintEventListener() { return mMayHavePaintEventListener; }

  



  bool MayHaveAudioAvailableEventListener() { return mMayHaveAudioAvailableEventListener; }

  



  bool MayHaveTouchEventListener() { return mMayHaveTouchEventListener; }

  bool MayHaveMouseEnterLeaveEventListener() { return mMayHaveMouseEnterLeaveEventListener; }

  PRInt64 SizeOf() const;
protected:
  nsresult HandleEventSubType(nsListenerStruct* aListenerStruct,
                              nsIDOMEventListener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              nsIDOMEventTarget* aCurrentTarget,
                              PRUint32 aPhaseFlags,
                              nsCxPusher* aPusher);

  




  nsresult CompileEventHandlerInternal(nsListenerStruct *aListenerStruct,
                                       bool aNeedsCxPush,
                                       const nsAString* aBody);

  


  nsListenerStruct* FindJSEventListener(PRUint32 aEventType, nsIAtom* aTypeAtom);

  





  nsresult SetJSEventListener(nsIScriptContext *aContext,
                              JSObject* aScopeGlobal,
                              nsIAtom* aName,
                              JSObject *aHandler,
                              bool aPermitUntrustedEvents,
                              nsListenerStruct **aListenerStruct);

public:
  




  nsresult SetJSEventListenerToJsval(nsIAtom *aEventName, JSContext *cx,
                                     JSObject *aScope, const jsval &v);
  



  void GetJSEventListener(nsIAtom *aEventName, jsval *vp);

protected:
  void AddEventListener(nsIDOMEventListener *aListener, 
                        PRUint32 aType,
                        nsIAtom* aTypeAtom,
                        PRInt32 aFlags);
  void RemoveEventListener(nsIDOMEventListener *aListener,
                           PRUint32 aType,
                           nsIAtom* aUserType,
                           PRInt32 aFlags);
  void RemoveAllListeners();
  const EventTypeData* GetTypeDataForIID(const nsIID& aIID);
  const EventTypeData* GetTypeDataForEventName(nsIAtom* aName);
  nsPIDOMWindow* GetInnerWindowForTarget();

  PRUint32 mMayHavePaintEventListener : 1;
  PRUint32 mMayHaveMutationListeners : 1;
  PRUint32 mMayHaveCapturingListeners : 1;
  PRUint32 mMayHaveSystemGroupListeners : 1;
  PRUint32 mMayHaveAudioAvailableEventListener : 1;
  PRUint32 mMayHaveTouchEventListener : 1;
  PRUint32 mMayHaveMouseEnterLeaveEventListener : 1;
  PRUint32 mNoListenerForEvent : 25;

  nsAutoTObserverArray<nsListenerStruct, 2> mListeners;
  nsISupports*                              mTarget;  
  nsCOMPtr<nsIAtom>                         mNoListenerForEventAtom;

  static PRUint32                           mInstanceCount;
  static jsid                               sAddListenerID;

  friend class nsEventTargetChainItem;
  static PRUint32                           sCreatedCount;
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
  PRUint32 flags = NS_EVENT_FLAG_SYSTEM_EVENT;
  flags |= aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }
  listenerManager->AddEventListenerByType(aListener, aType, flags);
  return NS_OK;
}

#endif 
