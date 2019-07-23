





































#ifndef nsDOMEventTargetHelper_h_
#define nsDOMEventTargetHelper_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsPIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMEventTarget.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsIEventListenerManager.h"

class nsDOMEventListenerWrapper : public nsIDOMEventListener
{
public:
  nsDOMEventListenerWrapper(nsIDOMEventListener* aListener)
  : mListener(aListener) {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMEventListenerWrapper)

  NS_DECL_NSIDOMEVENTLISTENER

  nsIDOMEventListener* GetInner() { return mListener; }
protected:
  nsCOMPtr<nsIDOMEventListener> mListener;
};

class nsDOMEventTargetHelper : public nsPIDOMEventTarget,
                               public nsIDOMEventTarget,
                               public nsIDOMNSEventTarget
{
public:
  nsDOMEventTargetHelper() : mLang(nsIProgrammingLanguage::JAVASCRIPT) {}
  virtual ~nsDOMEventTargetHelper() {}
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMEventTargetHelper,
                                           nsPIDOMEventTarget)

  NS_DECL_NSIDOMNSEVENTTARGET
  NS_DECL_NSIDOMEVENTTARGET
  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsresult GetListenerManager(PRBool aCreateIfNotFound,
                                      nsIEventListenerManager** aResult);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv);

  PRBool HasListenersFor(const nsAString& aType)
  {
    return mListenerManager && mListenerManager->HasListenersFor(aType);
  }
  nsresult RemoveAddEventListener(const nsAString& aType,
                                  nsRefPtr<nsDOMEventListenerWrapper>& aCurrent,
                                  nsIDOMEventListener* aNew);

  nsresult GetInnerEventListener(nsRefPtr<nsDOMEventListenerWrapper>& aWrapper,
                                 nsIDOMEventListener** aListener);

  nsresult CheckInnerWindowCorrectness()
  {
    if (mOwner) {
      NS_ASSERTION(mOwner->IsInnerWindow(), "Should have inner window here!\n");
      nsPIDOMWindow* outer = mOwner->GetOuterWindow();
      if (!outer || outer->GetCurrentInnerWindow() != mOwner) {
        return NS_ERROR_FAILURE;
      }
    }
    return NS_OK;
  }
protected:
  nsCOMPtr<nsIEventListenerManager> mListenerManager;
  PRUint32 mLang;
  
  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow>    mOwner; 
};

#endif 
