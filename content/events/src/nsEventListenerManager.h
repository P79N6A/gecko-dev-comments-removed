




































#ifndef nsEventListenerManager_h__
#define nsEventListenerManager_h__

#include "nsIEventListenerManager.h"
#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventReceiver.h"
#include "nsIDOM3EventTarget.h"
#include "nsHashtable.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"

class nsIDOMEvent;
class nsVoidArray;
class nsIAtom;
struct EventTypeData;

typedef struct {
  nsRefPtr<nsIDOMEventListener> mListener;
  PRUint32                      mEventType;
  nsCOMPtr<nsIAtom>             mTypeAtom;
  PRUint16                      mFlags;
  PRUint16                      mGroupFlags;
  PRBool                        mHandlerIsString;
  const EventTypeData*          mTypeData;
} nsListenerStruct;





class nsEventListenerManager : public nsIEventListenerManager,
                               public nsIDOMEventReceiver,
                               public nsIDOM3EventTarget
{

public:
  nsEventListenerManager();
  virtual ~nsEventListenerManager();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  



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

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsEvent* aEvent, 
                         nsIDOMEvent** aDOMEvent,
                         nsISupports* aCurrentTarget,
                         PRUint32 aFlags,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD Disconnect();

  NS_IMETHOD SetListenerTarget(nsISupports* aTarget);

  NS_IMETHOD HasMutationListeners(PRBool* aListener);

  NS_IMETHOD GetSystemEventGroupLM(nsIDOMEventGroup** aGroup);

  virtual PRBool HasUnloadListeners();

  virtual PRUint32 MutationListenerBits();

  virtual PRBool HasListenersFor(const nsAString& aEventName);

  static PRUint32 GetIdentifierForEvent(nsIAtom* aEvent);

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOM3EVENTTARGET

  
  NS_IMETHOD AddEventListenerByIID(nsIDOMEventListener *aListener,
                                   const nsIID& aIID);
  NS_IMETHOD RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                      const nsIID& aIID);
  NS_IMETHOD GetListenerManager(PRBool aCreateIfNotFound,
                                nsIEventListenerManager** aResult);
  NS_IMETHOD GetSystemEventGroup(nsIDOMEventGroup** aGroup);

  static void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEventListenerManager,
                                           nsIEventListenerManager)

protected:
  nsresult HandleEventSubType(nsListenerStruct* aListenerStruct,
                              nsIDOMEventListener* aListener,
                              nsIDOMEvent* aDOMEvent,
                              nsISupports* aCurrentTarget,
                              PRUint32 aPhaseFlags);
  nsresult CompileEventHandlerInternal(nsIScriptContext *aContext,
                                       void *aScopeObject,
                                       nsISupports *aObject,
                                       nsIAtom *aName,
                                       nsListenerStruct *aListenerStruct,
                                       nsISupports* aCurrentTarget);
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
  nsresult FixContextMenuEvent(nsPresContext* aPresContext,
                               nsISupports* aCurrentTarget,
                               nsEvent* aEvent,
                               nsIDOMEvent** aDOMEvent);
  PRBool PrepareToUseCaretPosition(nsIWidget* aEventWidget,
                                   nsIPresShell* aShell,
                                   nsPoint& aTargetPt);
  void GetCoordinatesFor(nsIDOMElement *aCurrentEl, nsPresContext *aPresContext,
                         nsIPresShell *aPresShell, nsPoint& aTargetPt);
  nsresult GetDOM2EventGroup(nsIDOMEventGroup** aGroup);
  PRBool ListenerCanHandle(nsListenerStruct* aLs, nsEvent* aEvent);

  nsVoidArray       mListeners;
  nsISupports*      mTarget;  
  PRPackedBool      mListenersRemoved;
  PRPackedBool      mListenerRemoved;
  PRPackedBool      mHandlingEvent;
  PRPackedBool      mMayHaveMutationListeners;
  
  
  
  PRUint32          mNoListenerForEvent;
  nsCOMPtr<nsIAtom> mNoListenerForEventAtom;

  static PRUint32   mInstanceCount;
  static jsval      sAddListenerID;
};

#endif 
