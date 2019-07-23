






































#include "nsCOMPtr.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLWindowKeyHandler.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDOMNSUIEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsXBLService.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsIXBLDocumentInfo.h"
#include "nsIDOMElement.h"
#include "nsINativeKeyBindings.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIDOMWindowInternal.h"
#include "nsIFocusController.h"
#include "nsPIWindowRoot.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsPIWindowRoot.h"
#include "nsPIDOMWindow.h"
#include "nsIFocusController.h"
#include "nsIDocShell.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsISelectionController.h"

static nsINativeKeyBindings *sNativeEditorBindings = nsnull;

class nsXBLSpecialDocInfo
{
public:
  nsCOMPtr<nsIXBLDocumentInfo> mHTMLBindings;
  nsCOMPtr<nsIXBLDocumentInfo> mUserHTMLBindings;

  static const char sHTMLBindingStr[];
  static const char sUserHTMLBindingStr[];

  PRBool mInitialized;

public:
  void LoadDocInfo();
  void GetAllHandlers(const char* aType,
                      nsXBLPrototypeHandler** handler,
                      nsXBLPrototypeHandler** userHandler);
  void GetHandlers(nsIXBLDocumentInfo* aInfo,
                   const nsACString& aRef,
                   nsXBLPrototypeHandler** aResult);

  nsXBLSpecialDocInfo() : mInitialized(PR_FALSE) {}
};

const char nsXBLSpecialDocInfo::sHTMLBindingStr[] =
  "chrome://global/content/platformHTMLBindings.xml";

void nsXBLSpecialDocInfo::LoadDocInfo()
{
  if (mInitialized)
    return;
  mInitialized = PR_TRUE;

  nsresult rv;
  nsCOMPtr<nsIXBLService> xblService = 
           do_GetService("@mozilla.org/xbl;1", &rv);
  if (NS_FAILED(rv) || !xblService)
    return;

  
  nsCOMPtr<nsIURI> bindingURI;
  NS_NewURI(getter_AddRefs(bindingURI), sHTMLBindingStr);
  if (!bindingURI) {
    return;
  }
  xblService->LoadBindingDocumentInfo(nsnull, nsnull,
                                      bindingURI,
                                      PR_TRUE, 
                                      getter_AddRefs(mHTMLBindings));

  const nsAdoptingCString& userHTMLBindingStr =
    nsContentUtils::GetCharPref("dom.userHTMLBindings.uri");
  if (!userHTMLBindingStr.IsEmpty()) {
    NS_NewURI(getter_AddRefs(bindingURI), userHTMLBindingStr);
    if (!bindingURI) {
      return;
    }

    xblService->LoadBindingDocumentInfo(nsnull, nsnull,
                                        bindingURI,
                                        PR_TRUE, 
                                        getter_AddRefs(mUserHTMLBindings));
  }
}





void
nsXBLSpecialDocInfo::GetHandlers(nsIXBLDocumentInfo* aInfo,
                                 const nsACString& aRef,
                                 nsXBLPrototypeHandler** aResult)
{
  nsXBLPrototypeBinding* binding;
  aInfo->GetPrototypeBinding(aRef, &binding);
  
  NS_ASSERTION(binding, "No binding found for the XBL window key handler.");
  if (!binding)
    return;

  *aResult = binding->GetPrototypeHandlers();
}

void
nsXBLSpecialDocInfo::GetAllHandlers(const char* aType,
                                    nsXBLPrototypeHandler** aHandler,
                                    nsXBLPrototypeHandler** aUserHandler)
{
  if (mUserHTMLBindings) {
    nsCAutoString type(aType);
    type.Append("User");
    GetHandlers(mUserHTMLBindings, type, aUserHandler);
  }
  if (mHTMLBindings) {
    GetHandlers(mHTMLBindings, nsDependentCString(aType), aHandler);
  }
}


nsXBLSpecialDocInfo* nsXBLWindowKeyHandler::sXBLSpecialDocInfo = nsnull;
PRUint32 nsXBLWindowKeyHandler::sRefCnt = 0;

nsXBLWindowKeyHandler::nsXBLWindowKeyHandler(nsIDOMElement* aElement,
                                             nsPIDOMEventTarget* aTarget)
  : mTarget(aTarget),
    mHandler(nsnull),
    mUserHandler(nsnull)
{
  mWeakPtrForElement = do_GetWeakReference(aElement);
  ++sRefCnt;
}

nsXBLWindowKeyHandler::~nsXBLWindowKeyHandler()
{
  
  if (mWeakPtrForElement)
    delete mHandler;

  --sRefCnt;
  if (!sRefCnt) {
    delete sXBLSpecialDocInfo;
    sXBLSpecialDocInfo = nsnull;
  }
}

NS_IMPL_ISUPPORTS2(nsXBLWindowKeyHandler,
                   nsIDOMKeyListener,
                   nsIDOMEventListener)

static void
BuildHandlerChain(nsIContent* aContent, nsXBLPrototypeHandler** aResult)
{
  *aResult = nsnull;

  
  
  
  for (PRUint32 j = aContent->GetChildCount(); j--; ) {
    nsIContent *key = aContent->GetChildAt(j);

    if (key->NodeInfo()->Equals(nsGkAtoms::key, kNameSpaceID_XUL)) {
      nsXBLPrototypeHandler* handler = new nsXBLPrototypeHandler(key);

      if (!handler)
        return;

      handler->SetNextHandler(*aResult);
      *aResult = handler;
    }
  }
}







nsresult
nsXBLWindowKeyHandler::EnsureHandlers(PRBool *aIsEditor)
{
  nsCOMPtr<nsIDOMElement> el = GetElement();
  NS_ENSURE_STATE(!mWeakPtrForElement || el);
  if (el) {
    
    if (aIsEditor)
      *aIsEditor = PR_FALSE;

    if (mHandler)
      return NS_OK;

    nsCOMPtr<nsIContent> content(do_QueryInterface(el));
    BuildHandlerChain(content, &mHandler);
  } else { 
    if (!sXBLSpecialDocInfo)
      sXBLSpecialDocInfo = new nsXBLSpecialDocInfo();
    if (!sXBLSpecialDocInfo) {
      if (aIsEditor) {
        *aIsEditor = PR_FALSE;
      }
      return NS_ERROR_OUT_OF_MEMORY;
    }
    sXBLSpecialDocInfo->LoadDocInfo();

    
    PRBool isEditor = IsEditor();
    if (isEditor) {
      sXBLSpecialDocInfo->GetAllHandlers("editor", &mHandler, &mUserHandler);
    }
    else {
      sXBLSpecialDocInfo->GetAllHandlers("browser", &mHandler, &mUserHandler);
    }

    if (aIsEditor)
      *aIsEditor = isEditor;
  }

  return NS_OK;
}

static nsINativeKeyBindings*
GetEditorKeyBindings()
{
  static PRBool noBindings = PR_FALSE;
  if (!sNativeEditorBindings && !noBindings) {
    CallGetService(NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "editor",
                   &sNativeEditorBindings);

    if (!sNativeEditorBindings) {
      noBindings = PR_TRUE;
    }
  }

  return sNativeEditorBindings;
}

static void
DoCommandCallback(const char *aCommand, void *aData)
{
  nsIControllers *controllers = NS_STATIC_CAST(nsIControllers*, aData);
  if (controllers) {
    nsCOMPtr<nsIController> controller;
    controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
    if (controller) {
      controller->DoCommand(aCommand);
    }
  }
}

nsresult
nsXBLWindowKeyHandler::WalkHandlers(nsIDOMEvent* aKeyEvent, nsIAtom* aEventType)
{
  nsCOMPtr<nsIDOMNSUIEvent> evt = do_QueryInterface(aKeyEvent);
  PRBool prevent;
  evt->GetPreventDefault(&prevent);
  if (prevent)
    return NS_OK;

  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  PRBool trustedEvent = PR_FALSE;

  if (domNSEvent) {
    
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  
  nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aKeyEvent));
  if (!keyEvent)
    return NS_OK;

  PRBool isEditor;
  nsresult rv = EnsureHandlers(&isEditor);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMElement> el = GetElement();
  if (!el) {
    if (mUserHandler) {
      WalkHandlersInternal(aKeyEvent, aEventType, mUserHandler);
      evt->GetPreventDefault(&prevent);
      if (prevent)
        return NS_OK; 
    }
  }

  WalkHandlersInternal(aKeyEvent, aEventType, mHandler);

  nsINativeKeyBindings *nativeBindings;
  if (isEditor && (nativeBindings = GetEditorKeyBindings())) {
    nsNativeKeyEvent nativeEvent;
    
    nativeEvent.charCode = 0;
    keyEvent->GetKeyCode(&nativeEvent.keyCode);
    keyEvent->GetAltKey(&nativeEvent.altKey);
    keyEvent->GetCtrlKey(&nativeEvent.ctrlKey);
    keyEvent->GetShiftKey(&nativeEvent.shiftKey);
    keyEvent->GetMetaKey(&nativeEvent.metaKey);

    
    nsCOMPtr<nsIControllers> controllers;
    nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(mTarget);
    if (root) {
      nsCOMPtr<nsIFocusController> fc;
      root->GetFocusController(getter_AddRefs(fc));
      if (fc) {
        fc->GetControllers(getter_AddRefs(controllers));
      }
    }

    PRBool handled;
    if (aEventType == nsGkAtoms::keypress) {
      keyEvent->GetCharCode(&nativeEvent.charCode);
      handled = sNativeEditorBindings->KeyPress(nativeEvent,
                                                DoCommandCallback, controllers);
    } else if (aEventType == nsGkAtoms::keyup) {
      handled = sNativeEditorBindings->KeyUp(nativeEvent,
                                             DoCommandCallback, controllers);
    } else {
      handled = sNativeEditorBindings->KeyDown(nativeEvent,
                                               DoCommandCallback, controllers);
    }

    if (handled)
      aKeyEvent->PreventDefault();

  }
  
  return NS_OK;
}

nsresult nsXBLWindowKeyHandler::KeyUp(nsIDOMEvent* aKeyEvent)
{
  return WalkHandlers(aKeyEvent, nsGkAtoms::keyup);
}

nsresult nsXBLWindowKeyHandler::KeyDown(nsIDOMEvent* aKeyEvent)
{
  return WalkHandlers(aKeyEvent, nsGkAtoms::keydown);
}

nsresult nsXBLWindowKeyHandler::KeyPress(nsIDOMEvent* aKeyEvent)
{
  return WalkHandlers(aKeyEvent, nsGkAtoms::keypress);
}







PRBool
nsXBLWindowKeyHandler::EventMatched(nsXBLPrototypeHandler* inHandler,
                                    nsIAtom* inEventType, nsIDOMEvent* inEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(inEvent));
  if (keyEvent)
    return inHandler->KeyEventMatched(inEventType, keyEvent);

  return PR_FALSE;
}

 void
nsXBLWindowKeyHandler::ShutDown()
{
  NS_IF_RELEASE(sNativeEditorBindings);
}






PRBool
nsXBLWindowKeyHandler::IsEditor()
{
  nsCOMPtr<nsPIWindowRoot> windowRoot(do_QueryInterface(mTarget));
  NS_ENSURE_TRUE(windowRoot, PR_FALSE);
  nsCOMPtr<nsIFocusController> focusController;
  windowRoot->GetFocusController(getter_AddRefs(focusController));
  if (!focusController) {
    NS_WARNING("********* Something went wrong! No focus controller on the root!!!\n");
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMWindowInternal> focusedWindow;
  focusController->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow)
    return PR_FALSE;
  
  nsCOMPtr<nsPIDOMWindow> piwin(do_QueryInterface(focusedWindow));
  nsIDocShell *docShell = piwin->GetDocShell();
  nsCOMPtr<nsIPresShell> presShell;
  if (docShell)
    docShell->GetPresShell(getter_AddRefs(presShell));

  if (presShell) {
    PRInt16 isEditor;
    presShell->GetSelectionFlags(&isEditor);
    return isEditor == nsISelectionDisplay::DISPLAY_ALL;
  }

  return PR_FALSE;
}








nsresult
nsXBLWindowKeyHandler::WalkHandlersInternal(nsIDOMEvent* aEvent,
                                            nsIAtom* aEventType, 
                                            nsXBLPrototypeHandler* aHandler)
{
  nsresult rv;
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aEvent));
  
  
  for (nsXBLPrototypeHandler *currHandler = aHandler; currHandler;
       currHandler = currHandler->GetNextHandler()) {
    PRBool stopped;
    privateEvent->IsDispatchStopped(&stopped);
    if (stopped) {
      
      return NS_OK;
    }

    if (!EventMatched(currHandler, aEventType, aEvent))
      continue;  

    
    
    
    nsCOMPtr<nsIContent> elt = currHandler->GetHandlerElement();
    nsCOMPtr<nsIDOMElement> commandElt;

    
    nsCOMPtr<nsIDOMElement> el = GetElement();
    if (el && elt) {
      
      nsAutoString command;
      elt->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
      if (!command.IsEmpty()) {
        
        
        NS_ASSERTION(elt->IsInDoc(), "elt must be in document");
        nsCOMPtr<nsIDOMDocument> domDoc(
           do_QueryInterface(elt->GetCurrentDoc()));
        if (domDoc)
          domDoc->GetElementById(command, getter_AddRefs(commandElt));

        if (!commandElt) {
          NS_ERROR("A XUL <key> is observing a command that doesn't exist. Unable to execute key binding!\n");
          continue;
        }
      }
    }

    if (!commandElt) {
      commandElt = do_QueryInterface(elt);
    }

    if (commandElt) {
      nsAutoString value;
      commandElt->GetAttribute(NS_LITERAL_STRING("disabled"), value);
      if (value.EqualsLiteral("true")) {
        continue;  
      }

      
      commandElt->GetAttribute(NS_LITERAL_STRING("oncommand"), value);
      if (value.IsEmpty()) {
        continue;  
      }
    }

    nsCOMPtr<nsPIDOMEventTarget> piTarget;
    nsCOMPtr<nsIDOMElement> element = GetElement();
    if (element) {
      piTarget = do_QueryInterface(commandElt);
    } else {
      piTarget = mTarget;
    }

    rv = currHandler->ExecuteHandler(piTarget, aEvent);
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
  }

  return NS_OK;
}

already_AddRefed<nsIDOMElement>
nsXBLWindowKeyHandler::GetElement()
{
  nsCOMPtr<nsIDOMElement> element = do_QueryReferent(mWeakPtrForElement);
  nsIDOMElement* el = nsnull;
  element.swap(el);
  return el;
}



nsresult
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement, nsPIDOMEventTarget* aTarget,
                          nsXBLWindowKeyHandler** aResult)
{
  *aResult = new nsXBLWindowKeyHandler(aElement, aTarget);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
