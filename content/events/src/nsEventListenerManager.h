




































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
  PRUint16                      mGroupFlags;
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

  NS_IMETHOD RemoveEventListener(const nsAString& aType,
                                 nsIDOMEventListener* aListener,
                                 PRBool aUseCapture);
  NS_IMETHOD DispatchEvent(nsIDOMEvent* aEvent, PRBool *_retval);

  



  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID, PRInt32 aFlags);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID, PRInt32 aFlags);
  NS_IMETHOD AddEventListenerByType(nsIDOMEventListener *aListener,
                                    const nsAString& type,
                                    PRInt32 aFlags,
                                    nsIDOMEventGroup* aEvtGroup);
  NS_IMETHOD RemoveEventListenerByType(nsIDOMEventListener *aListener,
                                       const nsAString& type,
                                       PRInt32 aFlags,
                                       nsIDOMEventGroup* aEvtGroup);
  NS_IMETHOD AddScriptEventListener(nsISupports *aObject,
                                    nsIAtom *aName,
                                    const nsAString& aFunc,
                                    PRUint32 aLanguage,
                                    PRBool aDeferCompilation,
                                    PRBool aPermitUntrustedEvents);
  NS_IMETHOD RegisterScriptEventListener(nsIScriptContext *aContext,
                                         void *aScopeObject,
                                         nsISupports *aObject,
                                         nsIAtom* aName);
  NS_IMETHOD RemoveScriptEventListener(nsIAtom *aName);
  NS_IMETHOD CompileScriptEventListener(nsIScriptContext *aContext,
                                        void *aScopeObject,
                                        nsISupports *aObject,
                                        nsIAtom* aName, PRBool *aDidCompile);

  nsresult HandleEvent(nsPresContext* aPresContext,
                       nsEvent* aEvent, 
                       nsIDOMEvent** aDOMEvent,
                       nsIDOMEventTarget* aCurrentTarget,
                       PRUint32 aFlags,
                       nsEventStatus* aEventStatus,
                       nsCxPusher* aPusher)
  {
    if (mListeners.IsEmpty() || aEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) {
      return NS_OK;
    }

    if (!mMayHaveCapturingListeners &&
        !(aEvent->flags & NS_EVENT_FLAG_BUBBLE)) {
      return NS_OK;
    }

    if (!mMayHaveSystemGroupListeners &&
        aFlags & NS_EVENT_FLAG_SYSTEM_EVENT) {
      return NS_OK;
    }

    
    if (mNoListenerForEvent == aEvent->message &&
        (mNoListenerForEvent != NS_USER_DEFINED_EVENT ||
         mNoListenerForEventAtom == aEvent->userType)) {
      return NS_OK;
    }
    return HandleEventInternal(aPresContext, aEvent, aDOMEvent, aCurrentTarget,
                               aFlags, aEventStatus, aPusher);
  }

  nsresult HandleEventInternal(nsPresContext* aPresContext,
                               nsEvent* aEvent, 
                               nsIDOMEvent** aDOMEvent,
                               nsIDOMEventTarget* aCurrentTarget,
                               PRUint32 aFlags,
                               nsEventStatus* aEventStatus,
                               nsCxPusher* aPusher);

  NS_IMETHOD Disconnect();

  NS_IMETHOD HasMutationListeners(PRBool* aListener);

  NS_IMETHOD GetSystemEventGroupLM(nsIDOMEventGroup** aGroup);

  virtual PRBool HasUnloadListeners();

  virtual PRUint32 MutationListenerBits();

  virtual PRBool HasListenersFor(const nsAString& aEventName);

  virtual PRBool HasListeners();

  virtual nsresult GetListenerInfo(nsCOMArray<nsIEventListenerInfo>* aList);

  static PRUint32 GetIdentifierForEvent(nsIAtom* aEvent);

  
  NS_DECL_NSIDOM3EVENTTARGET

  static void Shutdown();

  static nsIDOMEventGroup* GetSystemEventGroup();

  



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
  nsresult RemoveEventListener(nsIDOMEventListener *aListener,
                               PRUint32 aType,
                               nsIAtom* aUserType,
                               const EventTypeData* aTypeData,
                               PRInt32 aFlags,
                               nsIDOMEventGroup* aEvtGrp);
  nsresult RemoveAllListeners();
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
