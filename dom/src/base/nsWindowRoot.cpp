






































#include "nsCOMPtr.h"
#include "nsWindowRoot.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIEventListenerManager.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsLayoutCID.h"
#include "nsContentCID.h"
#include "nsIEventStateManager.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMWindowInternal.h"
#include "nsFocusController.h"
#include "nsString.h"
#include "nsEventDispatcher.h"
#include "nsIProgrammingLanguage.h"

#include "nsCycleCollectionParticipant.h"

static NS_DEFINE_CID(kEventListenerManagerCID,    NS_EVENTLISTENERMANAGER_CID);

nsWindowRoot::nsWindowRoot(nsIDOMWindow* aWindow)
{
  mWindow = aWindow;

  
  nsFocusController::Create(getter_AddRefs(mFocusController));

  nsCOMPtr<nsIDOMFocusListener> focusListener(do_QueryInterface(mFocusController));
  mRefCnt.incr(NS_STATIC_CAST(nsIDOMEventTarget*, this));
  AddEventListener(NS_LITERAL_STRING("focus"), focusListener, PR_TRUE);
  AddEventListener(NS_LITERAL_STRING("blur"), focusListener, PR_TRUE);
  mRefCnt.decr(NS_STATIC_CAST(nsIDOMEventTarget*, this));
}

nsWindowRoot::~nsWindowRoot()
{
  if (mListenerManager) {
    mListenerManager->Disconnect();
  }
}

NS_IMPL_CYCLE_COLLECTION_2(nsWindowRoot, mListenerManager, mFocusController)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsWindowRoot)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsPIWindowRoot)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOM3EventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEventTarget)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsWindowRoot, nsIDOMEventTarget)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsWindowRoot, nsIDOMEventTarget)

NS_IMETHODIMP
nsWindowRoot::AddEventListener(const nsAString& aType, nsIDOMEventListener* aListener, PRBool aUseCapture)
{
  return AddGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsWindowRoot::RemoveEventListener(const nsAString& aType, nsIDOMEventListener* aListener, PRBool aUseCapture)
{
  return RemoveGroupedEventListener(aType, aListener, aUseCapture, nsnull);
}

NS_IMETHODIMP
nsWindowRoot::DispatchEvent(nsIDOMEvent* aEvt, PRBool *_retval)
{
  nsEventStatus status = nsEventStatus_eIgnore;
  nsresult rv =  nsEventDispatcher::DispatchDOMEvent(
    NS_STATIC_CAST(nsPIDOMEventTarget*, this), nsnull, aEvt, nsnull, &status);
  *_retval = (status != nsEventStatus_eConsumeNoDefault);
  return rv;
}

nsresult
nsWindowRoot::DispatchDOMEvent(nsEvent* aEvent,
                               nsIDOMEvent* aDOMEvent,
                               nsPresContext* aPresContext,
                               nsEventStatus* aEventStatus)
{
  return nsEventDispatcher::DispatchDOMEvent(NS_STATIC_CAST(nsPIDOMEventTarget*, this),
                                             aEvent, aDOMEvent,
                                             aPresContext, aEventStatus);
}

NS_IMETHODIMP
nsWindowRoot::AddGroupedEventListener(const nsAString & aType, nsIDOMEventListener *aListener, 
                                          PRBool aUseCapture, nsIDOMEventGroup *aEvtGrp)
{
  nsCOMPtr<nsIEventListenerManager> manager;

  if (NS_SUCCEEDED(GetListenerManager(PR_TRUE, getter_AddRefs(manager)))) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
    manager->AddEventListenerByType(aListener, aType, flags, aEvtGrp);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsWindowRoot::RemoveGroupedEventListener(const nsAString & aType, nsIDOMEventListener *aListener, 
                                             PRBool aUseCapture, nsIDOMEventGroup *aEvtGrp)
{
  if (mListenerManager) {
    PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;
    mListenerManager->RemoveEventListenerByType(aListener, aType, flags, aEvtGrp);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsWindowRoot::CanTrigger(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindowRoot::IsRegisteredHere(const nsAString & type, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsWindowRoot::AddEventListener(const nsAString& aType,
                               nsIDOMEventListener *aListener,
                               PRBool aUseCapture, PRBool aWantsUntrusted)
{
  nsCOMPtr<nsIEventListenerManager> manager;
  nsresult rv = GetListenerManager(PR_TRUE, getter_AddRefs(manager));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 flags = aUseCapture ? NS_EVENT_FLAG_CAPTURE : NS_EVENT_FLAG_BUBBLE;

  if (aWantsUntrusted) {
    flags |= NS_PRIV_EVENT_UNTRUSTED_PERMITTED;
  }

  return manager->AddEventListenerByType(aListener, aType, flags, nsnull);
}

nsresult
nsWindowRoot::AddEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> manager;
  GetListenerManager(PR_TRUE, getter_AddRefs(manager));
  if (manager) {
    manager->AddEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}
  
nsresult
nsWindowRoot::RemoveEventListenerByIID(nsIDOMEventListener *aListener, const nsIID& aIID)
{
  nsCOMPtr<nsIEventListenerManager> manager;
  GetListenerManager(PR_TRUE, getter_AddRefs(manager));
  if (manager) {
    manager->RemoveEventListenerByIID(aListener, aIID, NS_EVENT_FLAG_BUBBLE);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

nsresult
nsWindowRoot::GetListenerManager(PRBool aCreateIfNotFound,
                                 nsIEventListenerManager** aResult)
{
  if (!mListenerManager) {
    if (!aCreateIfNotFound) {
      *aResult = nsnull;
      return NS_OK;
    }
    nsresult rv;
    mListenerManager = do_CreateInstance(kEventListenerManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;
    mListenerManager->SetListenerTarget(
      NS_STATIC_CAST(nsPIDOMEventTarget*, this));
  }

  *aResult = mListenerManager;
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
nsWindowRoot::GetSystemEventGroup(nsIDOMEventGroup **aGroup)
{
  nsCOMPtr<nsIEventListenerManager> manager;
  if (NS_SUCCEEDED(GetListenerManager(PR_TRUE, getter_AddRefs(manager))) &&
    manager) {
    return manager->GetSystemEventGroupLM(aGroup);
  }
  return NS_ERROR_FAILURE;
}


nsresult
nsWindowRoot::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = PR_TRUE;
  aVisitor.mForceContentDispatch = PR_TRUE; 
  
  aVisitor.mItemData = mWindow;
  return NS_OK;
}

nsresult
nsWindowRoot::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return NS_OK;
}

NS_IMETHODIMP
nsWindowRoot::GetFocusController(nsIFocusController** aResult)
{
  *aResult = mFocusController;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

nsIDOMWindow*
nsWindowRoot::GetWindow()
{
  return mWindow;
}

NS_IMETHODIMP
nsWindowRoot::GetScriptTypeID(PRUint32 *aScriptType)
{
    NS_ERROR("No default script type here - ask some element");
    return nsIProgrammingLanguage::UNKNOWN;
}

NS_IMETHODIMP
nsWindowRoot::SetScriptTypeID(PRUint32 aScriptType)
{
    NS_ERROR("Can't change default script type for a document");
    return NS_ERROR_NOT_IMPLEMENTED;
}



nsresult
NS_NewWindowRoot(nsIDOMWindow* aWindow, nsPIDOMEventTarget** aResult)
{
  *aResult = new nsWindowRoot(aWindow);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
