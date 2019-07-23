





































#include "nsDOMEventTargetHelper.h"
#include "nsContentUtils.h"
#include "nsEventDispatcher.h"
#include "nsGUIEvent.h"
#include "nsIDocument.h"

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMEventListenerWrapper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMEventListenerWrapper)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
NS_INTERFACE_MAP_END_AGGREGATED(mListener)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMEventListenerWrapper)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMEventListenerWrapper)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMEventListenerWrapper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mListener)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMEventListenerWrapper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mListener)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP
nsDOMEventListenerWrapper::HandleEvent(nsIDOMEvent* aEvent)
{
  return mListener->HandleEvent(aEvent);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMEventTargetHelper)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEventTarget)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsDOMEventTargetHelper,
                                          nsPIDOMEventTarget)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsDOMEventTargetHelper,
                                           nsPIDOMEventTarget)


NS_IMETHODIMP
nsDOMEventTargetHelper::AddEventListener(const nsAString& aType,
                                         nsIDOMEventListener* aListener,
                                         PRBool aUseCapture)
{
  nsresult rv;
  nsIScriptContext* context =
    GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDocument> doc = nsContentUtils::GetDocumentFromScriptContext(context);
  PRBool wantsUntrusted = doc && !nsContentUtils::IsChromeDoc(doc);
  return AddEventListener(aType, aListener, aUseCapture, wantsUntrusted);
}

NS_IMETHODIMP
nsDOMEventTargetHelper::RemoveEventListener(const nsAString& aType,
                                            nsIDOMEventListener* aListener,
                                            PRBool aUseCapture)
{
  nsCOMPtr<nsIEventListenerManager> elm;
  GetListenerManager(PR_FALSE, getter_AddRefs(elm));
  if (elm) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
    elm->RemoveEventListenerByType(aListener, aType, flags, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEventTargetHelper::AddEventListener(const nsAString& aType,
                                         nsIDOMEventListener *aListener,
                                         PRBool aUseCapture,
                                         PRBool aWantsUntrusted)
{
  nsCOMPtr<nsIEventListenerManager> elm;
  GetListenerManager(PR_TRUE, getter_AddRefs(elm));
  NS_ENSURE_STATE(elm);
  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }
  return elm->AddEventListenerByType(aListener, aType, flags, nsnull);
}

NS_IMETHODIMP
nsDOMEventTargetHelper::GetScriptTypeID(PRUint32 *aLang)
{
  *aLang = mLang;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEventTargetHelper::SetScriptTypeID(PRUint32 aLang)
{
  mLang = aLang;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMEventTargetHelper::DispatchEvent(nsIDOMEvent* aEvent, PRBool* aRetVal)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(static_cast<nsPIDOMEventTarget*>(this),
                                        nsnull, aEvent, nsnull, &status);

  *aRetVal = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}

nsresult
nsDOMEventTargetHelper::RemoveAddEventListener(const nsAString& aType,
                                               nsRefPtr<nsDOMEventListenerWrapper>& aCurrent,
                                               nsIDOMEventListener* aNew)
{
  if (aCurrent) {
    RemoveEventListener(aType, aCurrent, PR_FALSE);
    aCurrent = nsnull;
  }
  if (aNew) {
    aCurrent = new nsDOMEventListenerWrapper(aNew);
    NS_ENSURE_TRUE(aCurrent, NS_ERROR_OUT_OF_MEMORY);
    AddEventListener(aType, aCurrent, PR_FALSE);
  }
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::GetInnerEventListener(nsRefPtr<nsDOMEventListenerWrapper>& aWrapper,
                                              nsIDOMEventListener** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  if (aWrapper) {
    NS_ADDREF(*aListener = aWrapper->GetInner());
  } else {
    *aListener = nsnull;
  }
  return NS_OK;
}


nsresult
nsDOMEventTargetHelper::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_TRUE;
  aVisitor.mParentTarget = nsnull;
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::DispatchDOMEvent(nsEvent* aEvent,
                                         nsIDOMEvent* aDOMEvent,
                                         nsPresContext* aPresContext,
                                         nsEventStatus* aEventStatus)
{
  return
    nsEventDispatcher::DispatchDOMEvent(static_cast<nsPIDOMEventTarget*>(this),
                                        aEvent, aDOMEvent, aPresContext,
                                        aEventStatus);
}

nsresult
nsDOMEventTargetHelper::GetListenerManager(PRBool aCreateIfNotFound,
                                           nsIEventListenerManager** aResult)
{
  if (!mListenerManager) {
    if (!aCreateIfNotFound) {
      *aResult = nsnull;
      return NS_OK;
    }
    nsresult rv = NS_NewEventListenerManager(getter_AddRefs(mListenerManager));
    NS_ENSURE_SUCCESS(rv, rv);
    mListenerManager->SetListenerTarget(static_cast<nsPIDOMEventTarget*>(this));
  }

  NS_ADDREF(*aResult = mListenerManager);
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                              const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> elm;
  GetListenerManager(PR_TRUE, getter_AddRefs(elm));
  if (elm) {
    elm->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
  }
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                                 const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> elm;
  GetListenerManager(PR_FALSE, getter_AddRefs(elm));
  if (elm) {
    return elm->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
  }
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::GetSystemEventGroup(nsIDOMEventGroup** aGroup)
{
  nsCOMPtr<nsIEventListenerManager> elm;
  nsresult rv = GetListenerManager(PR_TRUE, getter_AddRefs(elm));
  if (elm) {
    return elm->GetSystemEventGroupLM(aGroup);
  }
  return rv;
}

nsIScriptContext*
nsDOMEventTargetHelper::GetContextForEventHandlers(nsresult* aRv)
{
  *aRv = CheckInnerWindowCorrectness();
  if (NS_FAILED(*aRv)) {
    return nsnull;
  }
  return mScriptContext;
}

