












































#include "nsIContent.h"
#include "nsIFocusController.h"
#include "nsIControllers.h"
#include "nsIDOMDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMXULElement.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"
#include "nsRDFCID.h"
#include "nsXULCommandDispatcher.h"
#include "prlog.h"
#include "nsIDOMEventTarget.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "nsDOMError.h"
#include "nsEventDispatcher.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog;
#endif



nsXULCommandDispatcher::nsXULCommandDispatcher(nsIDocument* aDocument)
    : mFocusController(nsnull), mDocument(aDocument), mUpdaters(nsnull)
{

#ifdef PR_LOGGING
  if (! gLog)
    gLog = PR_NewLogModule("nsXULCommandDispatcher");
#endif
}

nsXULCommandDispatcher::~nsXULCommandDispatcher()
{
  while (mUpdaters) {
    Updater* doomed = mUpdaters;
    mUpdaters = mUpdaters->mNext;
    delete doomed;
  }
}


NS_INTERFACE_MAP_BEGIN(nsXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY(nsIDOMXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMXULCommandDispatcher)
    NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(XULCommandDispatcher)
NS_INTERFACE_MAP_END


NS_IMPL_ADDREF(nsXULCommandDispatcher)
NS_IMPL_RELEASE(nsXULCommandDispatcher)


NS_IMETHODIMP
nsXULCommandDispatcher::Create(nsIDocument* aDoc, nsIDOMXULCommandDispatcher** aResult)
{
  nsXULCommandDispatcher* dispatcher = new nsXULCommandDispatcher(aDoc);
  if (!dispatcher)
    return NS_ERROR_OUT_OF_MEMORY;

  *aResult = dispatcher;
  NS_ADDREF(*aResult);
  return NS_OK;
}

void
nsXULCommandDispatcher::EnsureFocusController()
{
  if (!mFocusController) {
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mDocument->GetScriptGlobalObject()));
  
    
    
    
    
    mFocusController = win->GetRootFocusController(); 
  }
}




NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedElement(nsIDOMElement** aElement)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  nsresult rv = mFocusController->GetFocusedElement(aElement);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (*aElement && !nsContentUtils::CanCallerAccess(*aElement)) {
    
    
    
    NS_RELEASE(*aElement);
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetFocusedWindow(nsIDOMWindow** aWindow)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMWindowInternal> window;
  nsresult rv = mFocusController->GetFocusedWindow(getter_AddRefs(window));
  NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && window, rv);

  rv = CallQueryInterface(window, aWindow);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIDOMDocument> domdoc;
  rv = (*aWindow)->GetDocument(getter_AddRefs(domdoc));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (domdoc && !nsContentUtils::CanCallerAccess(domdoc)) {
    NS_RELEASE(*aWindow);
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedElement(nsIDOMElement* aElement)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  return mFocusController->SetFocusedElement(aElement);
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetFocusedWindow(nsIDOMWindow* aWindow)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(aWindow));

  return mFocusController->SetFocusedWindow(window);
}

NS_IMETHODIMP
nsXULCommandDispatcher::AdvanceFocus()
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_TRUE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::RewindFocus()
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_FALSE, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AdvanceFocusIntoSubtree(nsIDOMElement* aElt)
{
  EnsureFocusController();
  if (mFocusController)
    return mFocusController->MoveFocus(PR_TRUE, aElt);
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::AddCommandUpdater(nsIDOMElement* aElement,
                                          const nsAString& aEvents,
                                          const nsAString& aTargets)
{
  NS_PRECONDITION(aElement != nsnull, "null ptr");
  if (! aElement)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIDOMNode> doc(do_QueryInterface(mDocument));

  nsresult rv = nsContentUtils::CheckSameOrigin(doc, aElement);

  if (NS_FAILED(rv)) {
    return rv;
  }

  Updater* updater = mUpdaters;
  Updater** link = &mUpdaters;

  while (updater) {
    if (updater->mElement == aElement) {

#ifdef NS_DEBUG
      if (PR_LOG_TEST(gLog, PR_LOG_NOTICE)) {
        nsCAutoString eventsC, targetsC, aeventsC, atargetsC; 
        eventsC.AssignWithConversion(updater->mEvents);
        targetsC.AssignWithConversion(updater->mTargets);
        CopyUTF16toUTF8(aEvents, aeventsC);
        CopyUTF16toUTF8(aTargets, atargetsC);
        PR_LOG(gLog, PR_LOG_NOTICE,
               ("xulcmd[%p] replace %p(events=%s targets=%s) with (events=%s targets=%s)",
                this, aElement,
                eventsC.get(),
                targetsC.get(),
                aeventsC.get(),
                atargetsC.get()));
      }
#endif

      
      
      
      updater->mEvents  = aEvents;
      updater->mTargets = aTargets;
      return NS_OK;
    }

    link = &(updater->mNext);
    updater = updater->mNext;
  }
#ifdef NS_DEBUG
  if (PR_LOG_TEST(gLog, PR_LOG_NOTICE)) {
    nsCAutoString aeventsC, atargetsC; 
    CopyUTF16toUTF8(aEvents, aeventsC);
    CopyUTF16toUTF8(aTargets, atargetsC);

    PR_LOG(gLog, PR_LOG_NOTICE,
           ("xulcmd[%p] add     %p(events=%s targets=%s)",
            this, aElement,
            aeventsC.get(),
            atargetsC.get()));
  }
#endif

  
  updater = new Updater(aElement, aEvents, aTargets);
  if (! updater)
      return NS_ERROR_OUT_OF_MEMORY;

  *link = updater;
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::RemoveCommandUpdater(nsIDOMElement* aElement)
{
  NS_PRECONDITION(aElement != nsnull, "null ptr");
  if (! aElement)
    return NS_ERROR_NULL_POINTER;

  Updater* updater = mUpdaters;
  Updater** link = &mUpdaters;

  while (updater) {
    if (updater->mElement == aElement) {
#ifdef NS_DEBUG
      if (PR_LOG_TEST(gLog, PR_LOG_NOTICE)) {
        nsCAutoString eventsC, targetsC; 
        eventsC.AssignWithConversion(updater->mEvents);
        targetsC.AssignWithConversion(updater->mTargets);
        PR_LOG(gLog, PR_LOG_NOTICE,
               ("xulcmd[%p] remove  %p(events=%s targets=%s)",
                this, aElement,
                eventsC.get(),
                targetsC.get()));
      }
#endif

      *link = updater->mNext;
      delete updater;
      return NS_OK;
    }

    link = &(updater->mNext);
    updater = updater->mNext;
  }

  
  return NS_OK;
}

NS_IMETHODIMP
nsXULCommandDispatcher::UpdateCommands(const nsAString& aEventName)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  nsAutoString id;
  nsCOMPtr<nsIDOMElement> element;
  mFocusController->GetFocusedElement(getter_AddRefs(element));
  if (element) {
    nsresult rv = element->GetAttribute(NS_LITERAL_STRING("id"), id);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to get element's id");
    if (NS_FAILED(rv)) return rv;
  }

#if 0
  {
    char*   actionString = ToNewCString(aEventName);
    printf("Doing UpdateCommands(\"%s\")\n", actionString);
    free(actionString);    
  }
#endif
  
  for (Updater* updater = mUpdaters; updater != nsnull; updater = updater->mNext) {
    
    
    if (! Matches(updater->mEvents, aEventName))
      continue;

    if (! Matches(updater->mTargets, id))
      continue;

    nsCOMPtr<nsIContent> content = do_QueryInterface(updater->mElement);
    NS_ASSERTION(content != nsnull, "not an nsIContent");
    if (! content)
      return NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIDocument> document = content->GetDocument();

    NS_ASSERTION(document != nsnull, "element has no document");
    if (! document)
      continue;

#ifdef NS_DEBUG
    if (PR_LOG_TEST(gLog, PR_LOG_NOTICE)) {
      nsCAutoString aeventnameC; 
      CopyUTF16toUTF8(aEventName, aeventnameC);
      PR_LOG(gLog, PR_LOG_NOTICE,
             ("xulcmd[%p] update %p event=%s",
              this, updater->mElement,
              aeventnameC.get()));
    }
#endif

    PRUint32 count = document->GetNumberOfShells();
    for (PRUint32 i = 0; i < count; i++) {
      nsIPresShell *shell = document->GetShellAt(i);

      
      nsCOMPtr<nsPresContext> context = shell->GetPresContext();

      
      nsEventStatus status = nsEventStatus_eIgnore;

      nsEvent event(PR_TRUE, NS_XUL_COMMAND_UPDATE);

      nsEventDispatcher::Dispatch(content, context, &event, nsnull, &status);
    }
  }
  return NS_OK;
}

PRBool
nsXULCommandDispatcher::Matches(const nsString& aList, 
                                const nsAString& aElement)
{
  if (aList.EqualsLiteral("*"))
    return PR_TRUE; 

  PRInt32 indx = aList.Find(PromiseFlatString(aElement));
  if (indx == -1)
    return PR_FALSE; 

  
  
  if (indx > 0) {
    PRUnichar ch = aList[indx - 1];
    if (! nsCRT::IsAsciiSpace(ch) && ch != PRUnichar(','))
      return PR_FALSE;
  }

  if (indx + aElement.Length() < aList.Length()) {
    PRUnichar ch = aList[indx + aElement.Length()];
    if (! nsCRT::IsAsciiSpace(ch) && ch != PRUnichar(','))
      return PR_FALSE;
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllers(nsIControllers** aResult)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  return mFocusController->GetControllers(aResult);
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetControllerForCommand(const char *aCommand, nsIController** _retval)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  return mFocusController->GetControllerForCommand(aCommand, _retval);
}

NS_IMETHODIMP
nsXULCommandDispatcher::GetSuppressFocusScroll(PRBool* aSuppressFocusScroll)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  return mFocusController->GetSuppressFocusScroll(aSuppressFocusScroll);
}

NS_IMETHODIMP
nsXULCommandDispatcher::SetSuppressFocusScroll(PRBool aSuppressFocusScroll)
{
  EnsureFocusController();
  NS_ENSURE_TRUE(mFocusController, NS_ERROR_FAILURE);

  return mFocusController->SetSuppressFocusScroll(aSuppressFocusScroll);
}

