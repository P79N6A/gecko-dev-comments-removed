





































#ifndef nsDOMEventTargetHelper_h_
#define nsDOMEventTargetHelper_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsCycleCollectionParticipant.h"
#include "nsPIDOMWindow.h"
#include "nsIScriptGlobalObject.h"
#include "nsEventListenerManager.h"
#include "nsIScriptContext.h"

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

class nsDOMEventTargetHelper : public nsIDOMEventTarget
{
public:
  nsDOMEventTargetHelper() {}
  virtual ~nsDOMEventTargetHelper();
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsDOMEventTargetHelper)

  NS_DECL_NSIDOMEVENTTARGET

  bool HasListenersFor(const nsAString& aType)
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
  nsRefPtr<nsEventListenerManager> mListenerManager;
  
  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow>    mOwner; 
};

#define NS_UNMARK_LISTENER_WRAPPER(_event)                       \
  if (tmp->mOn##_event##Listener) {                              \
    nsCOMPtr<nsIXPConnectWrappedJS> wjs =                        \
      do_QueryInterface(tmp->mOn##_event##Listener->GetInner()); \
    xpc_UnmarkGrayObject(wjs);                                   \
  }

#endif 
