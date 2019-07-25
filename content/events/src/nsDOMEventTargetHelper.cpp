





































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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mListenerManager,
                                                  nsEventListenerManager)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mListenerManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mScriptContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOwner)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMEventTargetHelper)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMEventTargetHelper)

NS_IMPL_DOMTARGET_DEFAULTS(nsDOMEventTargetHelper);

NS_IMETHODIMP
nsDOMEventTargetHelper::RemoveEventListener(const nsAString& aType,
                                            nsIDOMEventListener* aListener,
                                            PRBool aUseCapture)
{
  nsEventListenerManager* elm = GetListenerManager(PR_FALSE);
  if (elm) {
    elm->RemoveEventListener(aType, aListener, aUseCapture);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMEventTargetHelper::AddEventListener(const nsAString& aType,
                                         nsIDOMEventListener *aListener,
                                         PRBool aUseCapture,
                                         PRBool aWantsUntrusted,
                                         PRUint8 aOptionalArgc)
{
  NS_ASSERTION(!aWantsUntrusted || aOptionalArgc > 1,
               "Won't check if this is chrome, you want to set "
               "aWantsUntrusted to PR_FALSE or make the aWantsUntrusted "
               "explicit by making aOptionalArgc non-zero.");

  if (aOptionalArgc < 2) {
    nsresult rv;
    nsIScriptContext* context = GetContextForEventHandlers(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDocument> doc =
      nsContentUtils::GetDocumentFromScriptContext(context);
    aWantsUntrusted = doc && !nsContentUtils::IsChromeDoc(doc);
  }

  nsEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->AddEventListener(aType, aListener, aUseCapture, aWantsUntrusted);
}

NS_IMETHODIMP
nsDOMEventTargetHelper::DispatchEvent(nsIDOMEvent* aEvent, PRBool* aRetVal)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =
    nsEventDispatcher::DispatchDOMEvent(this, nsnull, aEvent, nsnull, &status);

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
    nsIDOMEventTarget::AddEventListener(aType, aCurrent, PR_FALSE);
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
    nsEventDispatcher::DispatchDOMEvent(this, aEvent, aDOMEvent, aPresContext,
                                        aEventStatus);
}

nsEventListenerManager*
nsDOMEventTargetHelper::GetListenerManager(PRBool aCreateIfNotFound)
{
  if (!mListenerManager && aCreateIfNotFound) {
    mListenerManager = new nsEventListenerManager(this);
  }

  return mListenerManager;
}

nsresult
nsDOMEventTargetHelper::AddEventListenerByIID(nsIDOMEventListener *aListener,
                                              const nsIID& aIID)
{
  nsEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
}

nsresult
nsDOMEventTargetHelper::RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                                 const nsIID& aIID)
{
  nsEventListenerManager* elm = GetListenerManager(PR_FALSE);
  if (elm) {
    elm->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
  }
  return NS_OK;
}

nsresult
nsDOMEventTargetHelper::GetSystemEventGroup(nsIDOMEventGroup** aGroup)
{
  nsEventListenerManager* elm = GetListenerManager(PR_TRUE);
  NS_ENSURE_STATE(elm);
  return elm->GetSystemEventGroupLM(aGroup);
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

