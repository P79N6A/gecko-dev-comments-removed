






































#ifndef nsWindowRoot_h__
#define nsWindowRoot_h__

class nsPIDOMWindow;
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
#include "nsIDOMEventTarget.h"
#include "nsCycleCollectionParticipant.h"

class nsWindowRoot : public nsIDOMEventTarget,
                     public nsIDOM3EventTarget,
                     public nsIDOMNSEventTarget,
                     public nsPIWindowRoot
{
public:
  nsWindowRoot(nsPIDOMWindow* aWindow);
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
  virtual nsIEventListenerManager* GetListenerManager(PRBool aCreateIfNotFound);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv)
  {
    *aRv = NS_OK;
    return nsnull;
  }

  

  virtual nsPIDOMWindow* GetWindow();

  virtual nsresult GetControllers(nsIControllers** aResult);
  virtual nsresult GetControllerForCommand(const char * aCommand,
                                           nsIController** _retval);

  virtual nsIDOMNode* GetPopupNode();
  virtual void SetPopupNode(nsIDOMNode* aNode);

  virtual void SetParentTarget(nsPIDOMEventTarget* aTarget)
  {
    mParent = aTarget;
  }
  virtual nsPIDOMEventTarget* GetParentTarget() { return mParent; }

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsWindowRoot, nsIDOMEventTarget)

protected:
  
  nsPIDOMWindow* mWindow; 
  nsCOMPtr<nsIEventListenerManager> mListenerManager; 
                                                      

  nsCOMPtr<nsIDOMNode> mPopupNode; 

  nsCOMPtr<nsPIDOMEventTarget> mParent;
};

extern nsresult
NS_NewWindowRoot(nsPIDOMWindow* aWindow,
                 nsPIDOMEventTarget** aResult);

#endif
