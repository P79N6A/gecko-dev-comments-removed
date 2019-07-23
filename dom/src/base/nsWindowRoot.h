






































#ifndef nsWindowRoot_h__
#define nsWindowRoot_h__

class nsIDOMWindow;
class nsIDOMEventListener;
class nsIEventListenerManager;
class nsIDOMEvent;
class nsEventChainPreVisitor;
class nsEventChainPostVisitor;

#include "nsIDOMEventTarget.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIEventListenerManager.h"
#include "nsPIWindowRoot.h"
#include "nsIFocusController.h"
#include "nsIDOMEventTarget.h"
#include "nsCycleCollectionParticipant.h"

class nsWindowRoot : public nsIDOMEventTarget,
                     public nsIDOM3EventTarget,
                     public nsIDOMNSEventTarget,
                     public nsPIWindowRoot
{
public:
  nsWindowRoot(nsIDOMWindow* aWindow);
  virtual ~nsWindowRoot();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET
  NS_DECL_NSIDOM3EVENTTARGET
  NS_DECL_NSIDOMNSEVENTTARGET

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent,
                                    nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsresult GetListenerManager(PRBool aCreateIfNotFound,
                                      nsIEventListenerManager** aResult);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);

  
  NS_IMETHOD GetFocusController(nsIFocusController** aResult);

  virtual nsIDOMWindow* GetWindow();

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsWindowRoot, nsIDOMEventTarget)

protected:
  
  nsIDOMWindow* mWindow; 
  nsCOMPtr<nsIEventListenerManager> mListenerManager; 
                                                      
  nsCOMPtr<nsIFocusController> mFocusController; 
};

extern nsresult
NS_NewWindowRoot(nsIDOMWindow* aWindow,
                 nsPIDOMEventTarget** aResult);

#endif
