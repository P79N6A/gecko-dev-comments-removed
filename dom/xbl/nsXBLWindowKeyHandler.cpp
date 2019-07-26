




#include "nsCOMPtr.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLWindowKeyHandler.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsXBLService.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsXBLDocumentInfo.h"
#include "nsIDOMElement.h"
#include "nsFocusManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsContentUtils.h"
#include "nsXBLPrototypeBinding.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIPresShell.h"
#include "mozilla/EventStateManager.h"
#include "nsISelectionController.h"
#include "mozilla/Preferences.h"
#include "mozilla/TextEvents.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/Event.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIDOMDocument.h"

using namespace mozilla;
using namespace mozilla::dom;

class nsXBLSpecialDocInfo : public nsIObserver
{
public:
  nsRefPtr<nsXBLDocumentInfo> mHTMLBindings;
  nsRefPtr<nsXBLDocumentInfo> mUserHTMLBindings;

  static const char sHTMLBindingStr[];
  static const char sUserHTMLBindingStr[];

  bool mInitialized;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  void LoadDocInfo();
  void GetAllHandlers(const char* aType,
                      nsXBLPrototypeHandler** handler,
                      nsXBLPrototypeHandler** userHandler);
  void GetHandlers(nsXBLDocumentInfo* aInfo,
                   const nsACString& aRef,
                   nsXBLPrototypeHandler** aResult);

  nsXBLSpecialDocInfo() : mInitialized(false) {}

  virtual ~nsXBLSpecialDocInfo() {}

};

const char nsXBLSpecialDocInfo::sHTMLBindingStr[] =
  "chrome://global/content/platformHTMLBindings.xml";

NS_IMPL_ISUPPORTS(nsXBLSpecialDocInfo, nsIObserver)

NS_IMETHODIMP
nsXBLSpecialDocInfo::Observe(nsISupports* aSubject,
                             const char* aTopic,
                             const char16_t* aData)
{
  MOZ_ASSERT(!strcmp(aTopic, "xpcom-shutdown"), "wrong topic");

  
  mHTMLBindings = nullptr;
  mUserHTMLBindings = nullptr;
  mInitialized = false;
  nsContentUtils::UnregisterShutdownObserver(this);

  return NS_OK;
}

void nsXBLSpecialDocInfo::LoadDocInfo()
{
  if (mInitialized)
    return;
  mInitialized = true;
  nsContentUtils::RegisterShutdownObserver(this);

  nsXBLService* xblService = nsXBLService::GetInstance();
  if (!xblService)
    return;

  
  nsCOMPtr<nsIURI> bindingURI;
  NS_NewURI(getter_AddRefs(bindingURI), sHTMLBindingStr);
  if (!bindingURI) {
    return;
  }
  xblService->LoadBindingDocumentInfo(nullptr, nullptr,
                                      bindingURI,
                                      nullptr,
                                      true, 
                                      getter_AddRefs(mHTMLBindings));

  const nsAdoptingCString& userHTMLBindingStr =
    Preferences::GetCString("dom.userHTMLBindings.uri");
  if (!userHTMLBindingStr.IsEmpty()) {
    NS_NewURI(getter_AddRefs(bindingURI), userHTMLBindingStr);
    if (!bindingURI) {
      return;
    }

    xblService->LoadBindingDocumentInfo(nullptr, nullptr,
                                        bindingURI,
                                        nullptr,
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
    nsAutoCString type(aType);
    type.AppendLiteral("User");
    GetHandlers(mUserHTMLBindings, type, aUserHandler);
  }
  if (mHTMLBindings) {
    GetHandlers(mHTMLBindings, nsDependentCString(aType), aHandler);
  }
}


nsXBLSpecialDocInfo* nsXBLWindowKeyHandler::sXBLSpecialDocInfo = nullptr;
uint32_t nsXBLWindowKeyHandler::sRefCnt = 0;

nsXBLWindowKeyHandler::nsXBLWindowKeyHandler(nsIDOMElement* aElement,
                                             EventTarget* aTarget)
  : mTarget(aTarget),
    mHandler(nullptr),
    mUserHandler(nullptr)
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
    NS_IF_RELEASE(sXBLSpecialDocInfo);
  }
}

NS_IMPL_ISUPPORTS(nsXBLWindowKeyHandler,
                  nsIDOMEventListener)

static void
BuildHandlerChain(nsIContent* aContent, nsXBLPrototypeHandler** aResult)
{
  *aResult = nullptr;

  
  
  
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
nsXBLWindowKeyHandler::EnsureHandlers()
{
  nsCOMPtr<Element> el = GetElement();
  NS_ENSURE_STATE(!mWeakPtrForElement || el);
  if (el) {
    
    if (mHandler)
      return NS_OK;

    nsCOMPtr<nsIContent> content(do_QueryInterface(el));
    BuildHandlerChain(content, &mHandler);
  } else { 
    if (!sXBLSpecialDocInfo) {
      sXBLSpecialDocInfo = new nsXBLSpecialDocInfo();
      NS_ADDREF(sXBLSpecialDocInfo);
    }
    sXBLSpecialDocInfo->LoadDocInfo();

    
    if (IsHTMLEditableFieldFocused()) {
      sXBLSpecialDocInfo->GetAllHandlers("editor", &mHandler, &mUserHandler);
    }
    else {
      sXBLSpecialDocInfo->GetAllHandlers("browser", &mHandler, &mUserHandler);
    }
  }

  return NS_OK;
}

nsresult
nsXBLWindowKeyHandler::WalkHandlers(nsIDOMKeyEvent* aKeyEvent, nsIAtom* aEventType)
{
  bool prevent;
  aKeyEvent->GetDefaultPrevented(&prevent);
  if (prevent)
    return NS_OK;

  bool trustedEvent = false;
  
  aKeyEvent->GetIsTrusted(&trustedEvent);

  if (!trustedEvent)
    return NS_OK;

  nsresult rv = EnsureHandlers();
  NS_ENSURE_SUCCESS(rv, rv);

  bool isDisabled;
  nsCOMPtr<Element> el = GetElement(&isDisabled);
  if (!el) {
    if (mUserHandler) {
      WalkHandlersInternal(aKeyEvent, aEventType, mUserHandler, true);
      aKeyEvent->GetDefaultPrevented(&prevent);
      if (prevent)
        return NS_OK; 
    }
  }

  
  if (el && isDisabled) {
    return NS_OK;
  }

  WalkHandlersInternal(aKeyEvent, aEventType, mHandler, true);

  return NS_OK;
}

NS_IMETHODIMP
nsXBLWindowKeyHandler::HandleEvent(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIDOMKeyEvent> keyEvent(do_QueryInterface(aEvent));
  NS_ENSURE_TRUE(keyEvent, NS_ERROR_INVALID_ARG);

  uint16_t eventPhase;
  aEvent->GetEventPhase(&eventPhase);
  if (eventPhase == nsIDOMEvent::CAPTURING_PHASE) {
    HandleEventOnCapture(keyEvent);
    return NS_OK;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);
  nsCOMPtr<nsIAtom> eventTypeAtom = do_GetAtom(eventType);
  NS_ENSURE_TRUE(eventTypeAtom, NS_ERROR_OUT_OF_MEMORY);

  return WalkHandlers(keyEvent, eventTypeAtom);
}

void
nsXBLWindowKeyHandler::HandleEventOnCapture(nsIDOMKeyEvent* aEvent)
{
  WidgetKeyboardEvent* widgetEvent =
    aEvent->GetInternalNSEvent()->AsKeyboardEvent();

  if (widgetEvent->mFlags.mNoCrossProcessBoundaryForwarding) {
    return;
  }

  nsCOMPtr<mozilla::dom::Element> originalTarget =
    do_QueryInterface(aEvent->GetInternalNSEvent()->originalTarget);
  if (!EventStateManager::IsRemoteTarget(originalTarget)) {
    return;
  }

  if (!HasHandlerForEvent(aEvent)) {
    return;
  }

  
  
  
  

  
  
  widgetEvent->mFlags.mWantReplyFromContentProcess = 1;
  aEvent->StopPropagation();
}






bool
nsXBLWindowKeyHandler::EventMatched(nsXBLPrototypeHandler* inHandler,
                                    nsIAtom* inEventType,
                                    nsIDOMKeyEvent* inEvent,
                                    uint32_t aCharCode, bool aIgnoreShiftKey)
{
  return inHandler->KeyEventMatched(inEventType, inEvent, aCharCode,
                                    aIgnoreShiftKey);
}

bool
nsXBLWindowKeyHandler::IsHTMLEditableFieldFocused()
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
  if (!docShell) {
    return false;
  }

  nsCOMPtr<nsIEditor> editor;
  docShell->GetEditor(getter_AddRefs(editor));
  nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(editor);
  if (!htmlEditor) {
    return false;
  }

  nsCOMPtr<nsIDOMDocument> domDocument;
  editor->GetDocument(getter_AddRefs(domDocument));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDocument);
  if (doc->HasFlag(NODE_IS_EDITABLE)) {
    
    return true;
  }

  nsCOMPtr<nsIDOMElement> focusedElement;
  fm->GetFocusedElement(getter_AddRefs(focusedElement));
  nsCOMPtr<nsINode> focusedNode = do_QueryInterface(focusedElement);
  if (focusedNode) {
    
    
    
    
    
    nsCOMPtr<Element> activeEditingHost = htmlEditor->GetActiveEditingHost();
    if (!activeEditingHost) {
      return false;
    }
    return nsContentUtils::ContentIsDescendantOf(focusedNode, activeEditingHost);
  }

  return false;
}









bool
nsXBLWindowKeyHandler::WalkHandlersInternal(nsIDOMKeyEvent* aKeyEvent,
                                            nsIAtom* aEventType, 
                                            nsXBLPrototypeHandler* aHandler,
                                            bool aExecute)
{
  nsAutoTArray<nsShortcutCandidate, 10> accessKeys;
  nsContentUtils::GetAccelKeyCandidates(aKeyEvent, accessKeys);

  if (accessKeys.IsEmpty()) {
    return WalkHandlersAndExecute(aKeyEvent, aEventType, aHandler,
                                  0, false, aExecute);
  }

  for (uint32_t i = 0; i < accessKeys.Length(); ++i) {
    nsShortcutCandidate &key = accessKeys[i];
    if (WalkHandlersAndExecute(aKeyEvent, aEventType, aHandler,
                               key.mCharCode, key.mIgnoreShift, aExecute))
      return true;
  }
  return false;
}

bool
nsXBLWindowKeyHandler::WalkHandlersAndExecute(nsIDOMKeyEvent* aKeyEvent,
                                              nsIAtom* aEventType,
                                              nsXBLPrototypeHandler* aHandler,
                                              uint32_t aCharCode,
                                              bool aIgnoreShiftKey,
                                              bool aExecute)
{
  nsresult rv;

  
  for (nsXBLPrototypeHandler *currHandler = aHandler; currHandler;
       currHandler = currHandler->GetNextHandler()) {
    bool stopped = aKeyEvent->IsDispatchStopped();
    if (stopped) {
      
      return false;
    }

    if (!EventMatched(currHandler, aEventType, aKeyEvent,
                      aCharCode, aIgnoreShiftKey))
      continue;  

    
    
    
    nsCOMPtr<nsIContent> elt = currHandler->GetHandlerElement();
    nsCOMPtr<Element> commandElt;

    
    nsCOMPtr<Element> el = GetElement();
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

    nsCOMPtr<EventTarget> piTarget;
    nsCOMPtr<Element> element = GetElement();
    if (element) {
      piTarget = commandElt;
    } else {
      piTarget = mTarget;
    }

    if (!aExecute) {
      return true;
    }

    rv = currHandler->ExecuteHandler(piTarget, aKeyEvent);
    if (NS_SUCCEEDED(rv)) {
      return true;
    }
  }

  return false;
}

bool
nsXBLWindowKeyHandler::HasHandlerForEvent(nsIDOMKeyEvent* aEvent)
{
  if (!aEvent->InternalDOMEvent()->IsTrusted()) {
    return false;
  }

  nsresult rv = EnsureHandlers();
  NS_ENSURE_SUCCESS(rv, false);

  bool isDisabled;
  nsCOMPtr<Element> el = GetElement(&isDisabled);
  if (el && isDisabled) {
    return false;
  }

  nsAutoString eventType;
  aEvent->GetType(eventType);
  nsCOMPtr<nsIAtom> eventTypeAtom = do_GetAtom(eventType);
  NS_ENSURE_TRUE(eventTypeAtom, false);

  return WalkHandlersInternal(aEvent, eventTypeAtom, mHandler, false);
}

already_AddRefed<Element>
nsXBLWindowKeyHandler::GetElement(bool* aIsDisabled)
{
  nsCOMPtr<Element> element = do_QueryReferent(mWeakPtrForElement);
  if (element && aIsDisabled) {
    *aIsDisabled = element->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                                        nsGkAtoms::_true, eCaseMatters);
  }
  return element.forget();
}



already_AddRefed<nsXBLWindowKeyHandler>
NS_NewXBLWindowKeyHandler(nsIDOMElement* aElement, EventTarget* aTarget)
{
  nsRefPtr<nsXBLWindowKeyHandler> result =
    new nsXBLWindowKeyHandler(aElement, aTarget);
  return result.forget();
}
