






































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
#include "nsIEventListenerManager.h"
#include "nsPIWindowRoot.h"
#include "nsIDOMEventTarget.h"
#include "nsCycleCollectionParticipant.h"

class nsWindowRoot : public nsIDOM3EventTarget,
                     public nsPIWindowRoot
{
public:
  nsWindowRoot(nsPIDOMWindow* aWindow);
  virtual ~nsWindowRoot();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMEVENTTARGET
  NS_DECL_NSIDOM3EVENTTARGET

  

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
