




































#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "nsEventListenerManager.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIDOM3EventTarget.h"
#include "nsHashtable.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"
#include "nsTObserverArray.h"
#include "nsGUIEvent.h"

class nsIDOMEvent;
class nsIAtom;
class nsIWidget;
struct nsPoint;
struct EventTypeData;
class nsEventTargetChainItem;
class nsPIDOMWindow;
class nsCxPusher;
class nsIEventListenerInfo;

typedef struct {
  nsRefPtr<nsIDOMEventListener> mListener;
  PRUint32                      mEventType;
  nsCOMPtr<nsIAtom>             mTypeAtom;
  PRUint16                      mFlags;
  PRBool                        mHandlerIsString;
  const EventTypeData*          mTypeData;
} nsListenerStruct;





class nsEventListenerManager
{

public:
  nsEventListenerManager(nsISupports* aTarget);
  virtual ~nsEventListenerManager();

  NS_INLINE_DECL_REFCOUNTING(nsEventListenerManager)

  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsEventListenerManager)

  nsresult AddEventListener(const nsAString& aType,
                            nsIDOMEventListener* aListener,
                            PRBool aUseCapture,
                            PRBool aWantsUntrusted);
  void RemoveEventListener(const nsAString& aType,
                           nsIDOMEventListener* aListener,
                           PRBool aUseCapture);

  



  nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                 const nsIID& aIID, PRInt32 aFlags);
  void RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                const nsIID& aIID, PRInt32 aFlags);
  nsresult AddEventListenerByType(nsIDOMEventListener *aListener,
                                  const nsAString& type,
                                  PRInt32 aFlags,
                                  nsIDOMEventGroup* aEvtGroup);
  void RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                 const nsAString& type,
                                 PRInt32 aFlags,
                                 nsIDOMEventGroup* aEvtGroup);
  nsresult AddScriptEventListener(nsISupports *aObject,
                                  nsIAtom *aName,
                                  const nsAString& aFunc,
                                  PRUint32 aLanguage,
                                  PRBool aDeferCompilation,
                                  PRBool aPermitUntrustedEvents);
  nsresult RegisterScriptEventListener(nsIScriptContext *aContext,
                                       void *aScopeObject,
                                       nsISupports *aObject,
                                       nsIAtom* aName);
  void RemoveScriptEventListener(nsIAtom *aName);
  nsresult CompileScriptEventListener(nsIScriptContext *aContext,
                                      void *aScopeObject,
                                      nsISupports *aObject,
                                      nsIAtom* aName, PRBool *aDidCompile);

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

  PRBool HasMutationListeners();

  PRBool HasUnloadListeners();

  PRUint32 MutationListenerBits();

  PRBool HasListenersFor(const nsAString& aEventName);

  PRBool HasListeners();

  nsresult GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList);

  PRUint32 GetIdentifierForEvent(nsIAtom* aEvent);

  
  NS_DECL_NSIDOM3EVENTTARGET

  static void Shutdown();

  



  PRBool MayHavePaintEventListener() { return mMayHavePaintEventListener; }

  



  PRBool MayHaveAudioAvailableEventListener() { return mMayHaveAudioAvailableEventListener; }

  



  PRBool MayHaveTouchEventListener() { return mMayHaveTouchEventListener; }

protected:
  nsresult HandleEventSubType(nsListenerStruct* aListenerStruct,
                              nsIDOMEventListener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              nsIDOMEventTarget* aCurrentTarget,
                              PRUint32 aPhaseFlags,
                              nsCxPusher* aPusher);
  nsresult CompileEventHandlerInternal(nsIScriptContext *aContext,
                                       void *aScopeObject,
                                       nsISupports *aObject,
                                       nsIAtom *aName,
                                       nsListenerStruct *aListenerStruct,
                                       nsISupports* aCurrentTarget,
                                       PRBool aNeedsCxPush);
  nsListenerStruct* FindJSEventListener(PRUint32 aEventType, nsIAtom* aTypeAtom);
  nsresult SetJSEventListener(nsIScriptContext *aContext,
                              void *aScopeGlobal,
                              nsISupports *aObject,
                              nsIAtom* aName, PRBool aIsString,
                              PRBool aPermitUntrustedEvents);
  nsresult AddEventListener(nsIDOMEventListener *aListener, 
                            PRUint32 aType,
                            nsIAtom* aTypeAtom,
                            const EventTypeData* aTypeData,
                            PRInt32 aFlags,
                            nsIDOMEventGroup* aEvtGrp);
  void RemoveEventListener(nsIDOMEventListener *aListener,
                           PRUint32 aType,
                           nsIAtom* aUserType,
                           const EventTypeData* aTypeData,
                           PRInt32 aFlags,
                           nsIDOMEventGroup* aEvtGrp);
  void RemoveAllListeners();
  const EventTypeData* GetTypeDataForIID(const nsIID& aIID);
  const EventTypeData* GetTypeDataForEventName(nsIAtom* aName);
  nsresult GetDOM2EventGroup(nsIDOMEventGroup** aGroup);
  PRBool ListenerCanHandle(nsListenerStruct* aLs, nsEvent* aEvent);
  nsPIDOMWindow* GetInnerWindowForTarget();

  PRUint32 mMayHavePaintEventListener : 1;
  PRUint32 mMayHaveMutationListeners : 1;
  PRUint32 mMayHaveCapturingListeners : 1;
  PRUint32 mMayHaveSystemGroupListeners : 1;
  PRUint32 mMayHaveAudioAvailableEventListener : 1;
  PRUint32 mMayHaveTouchEventListener : 1;
  PRUint32 mNoListenerForEvent : 26;

  nsAutoTObserverArray<nsListenerStruct, 2> mListeners;
  nsISupports*                              mTarget;  
  nsCOMPtr<nsIAtom>                         mNoListenerForEventAtom;

  static PRUint32                           mInstanceCount;
  static jsid                               sAddListenerID;

  friend class nsEventTargetChainItem;
  static PRUint32                           sCreatedCount;
};

#endif 
