






































#include "nsCOMPtr.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLWindowKeyHandler.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsXBLService.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsXBLDocumentInfo.h"
#include "nsIDOMElement.h"
#include "nsINativeKeyBindings.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsFocusManager.h"
#include "nsPIWindowRoot.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMDocument.h"
#include "nsPIWindowRoot.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIPresShell.h"
#include "nsIPrivateDOMEvent.h"
#include "nsISelectionController.h"
#include "nsGUIEvent.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Element.h"

using namespace mozilla;

static nsINativeKeyBindings *sNativeEditorBindings = nsnull;

class nsXBLSpecialDocInfo
{
public:
  nsRefPtr<nsXBLDocumentInfo> mHTMLBindings;
  nsRefPtr<nsXBLDocumentInfo> mUserHTMLBindings;

  static const char sHTMLBindingStr[];
  static const char sUserHTMLBindingStr[];

  bool mInitialized;

public:
  void LoadDocInfo();
  void GetAllHandlers(const char* aType,
                      nsXBLPrototypeHandler** handler,
                      nsXBLPrototypeHandler** userHandler);
  void GetHandlers(nsXBLDocumentInfo* aInfo,
                   const nsACString& aRef,
                   nsXBLPrototypeHandler** aResult);

  nsXBLSpecialDocInfo() : mInitialized(false) {}
};

const char nsXBLSpecialDocInfo::sHTMLBindingStr[] =
  "chrome://global/content/platformHTMLBindings.xml";

void nsXBLSpecialDocInfo::LoadDocInfo()
{
  if (mInitialized)
    return;
  mInitialized = true;

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
                                      nsnull,
                                      true, 
                                      getter_AddRefs(mHTMLBindings));

  const nsAdoptingCString& userHTMLBindingStr =
    Preferences::GetCString("dom.userHTMLBindings.uri");
  if (!userHTMLBindingStr.IsEmpty()) {
    NS_NewURI(getter_AddRefs(bindingURI), userHTMLBindingStr);
    if (!bindingURI) {
      return;
    }

    xblService->LoadBindingDocumentInfo(nsnull, nsnull,
                                        bindingURI,
                                        nsnull,
                                        true, 
                                        getter_AddRefs(mUserHTMLBindings));
  }
}





void
nsXBLSpecialDocInfo::GetHandlers(nsXBLDocumentInfo* aInfo,
                                 const nsACString& aRef,
                                 nsXBLPrototypeHandler** aResult)
{
  nsXBLPrototypeBinding* binding = aInfo->GetPrototypeBinding(aRef);
  
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
                                             nsIDOMEventTarget* aTarget)
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

NS_IMPL_ISUPPORTS1(nsXBLWindowKeyHandler,
                   nsIDOMEventListener)

static void
BuildHandlerChain(nsIContent* aContent, nsXBLPrototypeHandler** aResult)
{
  *aResult = nsnull;

  
  
  
  for (nsIContent* key = aContent->GetLastChild();
       key;
       key = key->GetPreviousSibling()) {

    if (key->NodeInfo()->Equals(nsGkAtoms::key, kNameSpaceID_XUL)) {
      
      
      
      nsAutoString valKey, valCharCode, valKeyCode;
      bool attrExists =
        key->GetAttr(kNameSpaceID_None, nsGkAtoms::key, valKey) ||
        key->GetAttr(kNameSpaceID_None, nsGkAtoms::charcode, valCharCode) ||
        key->GetAttr(kNameSpaceID_None, nsGkAtoms::keycode, valKeyCode);
      if (attrExists &&
          valKey.IsEmpty() && valCharCode.IsEmpty() && valKeyCode.IsEmpty())
        continue;

      nsXBLPrototypeHandler* handler = new nsXBLPrototypeHandler(key);

      if (!handler)
        return;

      handler->SetNextHandler(*aResult);
      *aResult = handler;
    }
  }
}







nsresult
nsXBLWindowKeyHandler::EnsureHandlers(bool *aIsEditor)
{
  nsCOMPtr<nsIDOMElement> el = GetElement();
  NS_ENSURE_STATE(!mWeakPtrForElement || el);
  if (el) {
    
    if (aIsEditor)
      *aIsEditor = false;

    if (mHandler)
      return NS_OK;

    nsCOMPtr<nsIContent> content(do_QueryInterface(el));
    BuildHandlerChain(content, &mHandler);
  } else { 
    if (!sXBLSpecialDocInfo)
      sXBLSpecialDocInfo = new nsXBLSpecialDocInfo();
    if (!sXBLSpecialDocInfo) {
      if (aIsEditor) {
        *aIsEditor = false;
      }
      return NS_ERROR_OUT_OF_MEMORY;
    }
    sXBLSpecialDocInfo->LoadDocInfo();

    
    bool isEditor = IsEditor();
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
  static bool noBindings = false;
  if (!sNativeEditorBindings && !noBindings) {
    CallGetService(NS_NATIVEKEYBINDINGS_CONTRACTID_PREFIX "editor",
                   &sNativeEditorBindings);

    if (!sNativeEditorBindings) {
      noBindings = true;
    }
  }

  return sNativeEditorBindings;
}

static void
DoCommandCallback(const char *aCommand, void *aData)
{
  nsIControllers *controllers = static_cast<nsIControllers*>(aData);
  if (controllers) {
    nsCOMPtr<nsIController> controller;
    controllers->GetControllerForCommand(aCommand, getter_AddRefs(controller));
    if (controller) {
      controller->DoCommand(aCommand);
    }
  }
}

nsresult
nsXBLWindowKeyHandler::WalkHandlers(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType)
{
  nsCOMPtr<nsIDOMNSEvent> domNSEvent = do_QueryInterface(aKeyEvent);
  bool prevent;
  domNSEvent->GetPreventDefault(&prevent);
  if (prevent)
    return NS_OK;

  bool trustedEvent = false;
  if (domNSEvent) {
    
    domNSEvent->GetIsTrusted(&trustedEvent);
  }

  if (!trustedEvent)
    return NS_OK;

  bool isEditor;
  nsresult rv = EnsureHandlers(&isEditor);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMElement> el = GetElement();
  if (!el) {
    if (mUserHandler) {
      WalkHandlersInternal(aKeyEvent, aEventType, mUserHandler);
      domNSEvent->GetPreventDefault(&prevent);
      if (prevent)
        return NS_OK; 
    }
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(el);
  
  if (content && content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                                      nsGkAtoms::_true, eCaseMatters)) {
    return NS_OK;
  }

  WalkHandlersInternal(aKeyEvent, aEventType, mHandler);

  if (isEditor && GetEditorKeyBindings()) {
    nsNativeKeyEvent nativeEvent;
    
    nsCOMPtr<nsIControllers> controllers;
    nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(mTarget);
    if (root) {
      root->GetControllers(getter_AddRefs(controllers));
    }

    bool handled = false;
    if (aEventType == nsGkAtoms::keypress) {
      if (nsContentUtils::DOMEventToNativeKeyEvent(aKeyEvent, &nativeEvent, true))
        handled = sNativeEditorBindings->KeyPress(nativeEvent,
                                                  DoCommandCallback, controllers);
    } else if (aEventType == nsGkAtoms::keyup) {
      if (nsContentUtils::DOMEventToNativeKeyEvent(aKeyEvent, &nativeEvent, false))
        handled = sNativeEditorBindings->KeyUp(nativeEvent,
                                               DoCommandCallback, controllers);
    } else {
      NS_ASSERTION(aEventType == nsGkAtoms::keydown, "unknown key event type");
      if (nsContentUtils::DOMEventToNativeKeyEvent(aKeyEvent, &nativeEvent, false))
        handled = sNativeEditorBindings->KeyDown(nativeEvent,
                                                 DoCommandCallback, controllers);
    }

    if (handled)
      aKeyEvent->PreventDefault();

  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsXBLWindowKeyHandler::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aEvent));
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_INVALID_ARG);

  nsAutoString eventType;
  aEvent->GetType(eventType);
  nsCOMPtr<nsIAtom> eventTypeAtom = do_GetAtom(eventType);
  NS_ENSURE_TRUE(eventTypeAtom, NS_ERROR_OUT_OF_MEMORY);

  return WalkHandlers(keyEvent, eventTypeAtom);
}






bool
nsXBLWindowKeyHandler::EventMatched(nsXBLPrototypeHandler* inHandler,
                                    nsIAtom* inEventType,
                                    nsIDOMKeyEvent* inEvent,
                                    PRUint32 aCharCode, bool aIgnoreShiftKey)
{
  return inHandler->KeyEventMatched(inEventType, inEvent, aCharCode,
                                    aIgnoreShiftKey);
}

 void
nsXBLWindowKeyHandler::ShutDown()
{
  NS_IF_RELEASE(sNativeEditorBindings);
}






bool
nsXBLWindowKeyHandler::IsEditor()
{
  
  
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (!fm)
    return false;

  nsCOMPtr<nsIDOMWindow> focusedWindow;
  fm->GetFocusedWindow(getter_AddRefs(focusedWindow));
  if (!focusedWindow)
    return false;

  nsCOMPtr<nsPIDOMWindow> piwin(do_QueryInterface(focusedWindow));
  nsIDocShell *docShell = piwin->GetDocShell();
  nsCOMPtr<nsIPresShell> presShell;
  if (docShell)
    docShell->GetPresShell(getter_AddRefs(presShell));

  if (presShell) {
    return presShell->GetSelectionFlags() == nsISelectionDisplay::DISPLAY_ALL;
  }

  return false;
}








nsresult
nsXBLWindowKeyHandler::WalkHandlersInternal(nsIDOMKeyEvent* aKeyEvent,
                                            nsIAtom* aEventType, 
                                            nsXBLPrototypeHandler* aHandler)
{
  nsAutoTArray<nsShortcutCandidate, 10> accessKeys;
  nsContentUtils::GetAccelKeyCandidates(aKeyEvent, accessKeys);

  if (accessKeys.IsEmpty()) {
    WalkHandlersAndExecute(aKeyEvent, aEventType, aHandler, 0, false);
    return NS_OK;
  }

  for (PRUint32 i = 0; i < accessKeys.Length(); ++i) {
    nsShortcutCandidate &key = accessKeys[i];
    if (WalkHandlersAndExecute(aKeyEvent, aEventType, aHandler,
                               key.mCharCode, key.mIgnoreShift))
      return NS_OK;
  }
  return NS_OK;
}

bool
nsXBLWindowKeyHandler::WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent,
                                              nsIAtom* aEventType,
                                              nsXBLPrototypeHandler* aHandler,
                                              PRUint32 aCharCode,
                                              bool aIgnoreShiftKey)
{
  nsresult rv;
  nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(aKeyEvent));

  
  for (nsXBLPrototypeHandler *currHandler = aHandler; currHandler;
       currHandler = currHandler->GetNextHandler()) {
    bool stopped = privateEvent->IsDispatchStopped();
    if (stopped) {
      
      return NS_OK;
    }

    if (!EventMatched(currHandler, aEventType, aKeyEvent,
                      aCharCode, aIgnoreShiftKey))
      continue;  

    
    
    
    nsCOMPtr<nsIContent> elt = currHandler->GetHandlerElement();
    nsCOMPtr<nsIDOMElement> commandElt;

    
    nsCOMPtr<nsIDOMElement> el = GetElement();
    if (el && elt) {
      
      nsAutoString command;
      elt->GetAttr(kNameSpaceID_None, nsGkAtoms::command, command);
      if (!command.IsEmpty()) {
        
        
        NS_ASSERTION(elt->IsInDoc(), "elt must be in document");
        nsIDocument *doc = elt->GetCurrentDoc();
        if (doc)
          commandElt = do_QueryInterface(doc->GetElementById(command));

        if (!commandElt) {
          NS_ERROR("A XUL <key> is observing a command that doesn't exist. Unable to execute key binding!");
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

    nsCOMPtr<nsIDOMEventTarget> piTarget;
    nsCOMPtr<nsIDOMElement> element = GetElement();
    if (element) {
      piTarget = do_QueryInterface(commandElt);
    } else {
      piTarget = mTarget;
    }

    rv = currHandler->ExecuteHandler(piTarget, aKeyEvent);
    if (NS_SUCCEEDED(rv)) {
      return true;
    }
  }

  return false;
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
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement, nsIDOMEventTarget* aTarget,
                          nsXBLWindowKeyHandler** aResult)
{
  *aResult = new nsXBLWindowKeyHandler(aElement, aTarget);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
