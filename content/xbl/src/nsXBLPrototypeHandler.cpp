




#include "mozilla/Util.h"

#include "nsCOMPtr.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLPrototypeBinding.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsINameSpaceManager.h"
#include "nsIScriptContext.h"
#include "nsIDocument.h"
#include "nsIJSEventListener.h"
#include "nsIController.h"
#include "nsIControllers.h"
#include "nsIDOMXULElement.h"
#include "nsIURI.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsFocusManager.h"
#include "nsEventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEventListener.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsIDOMWindow.h"
#include "nsIServiceManager.h"
#include "nsIScriptError.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsGkAtoms.h"
#include "nsGUIEvent.h"
#include "nsIXPConnect.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsDOMCID.h"
#include "nsUnicharUtils.h"
#include "nsCRT.h"
#include "nsXBLEventHandler.h"
#include "nsXBLSerialize.h"
#include "nsEventDispatcher.h"
#include "nsJSUtils.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/EventHandlerBinding.h"

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);

uint32_t nsXBLPrototypeHandler::gRefCnt = 0;

int32_t nsXBLPrototypeHandler::kMenuAccessKey = -1;
int32_t nsXBLPrototypeHandler::kAccelKey = -1;

const int32_t nsXBLPrototypeHandler::cShift = (1<<0);
const int32_t nsXBLPrototypeHandler::cAlt = (1<<1);
const int32_t nsXBLPrototypeHandler::cControl = (1<<2);
const int32_t nsXBLPrototypeHandler::cMeta = (1<<3);
const int32_t nsXBLPrototypeHandler::cOS = (1<<4);

const int32_t nsXBLPrototypeHandler::cShiftMask = (1<<5);
const int32_t nsXBLPrototypeHandler::cAltMask = (1<<6);
const int32_t nsXBLPrototypeHandler::cControlMask = (1<<7);
const int32_t nsXBLPrototypeHandler::cMetaMask = (1<<8);
const int32_t nsXBLPrototypeHandler::cOSMask = (1<<9);

const int32_t nsXBLPrototypeHandler::cAllModifiers =
  cShiftMask | cAltMask | cControlMask | cMetaMask | cOSMask;

nsXBLPrototypeHandler::nsXBLPrototypeHandler(const PRUnichar* aEvent,
                                             const PRUnichar* aPhase,
                                             const PRUnichar* aAction,
                                             const PRUnichar* aCommand,
                                             const PRUnichar* aKeyCode,
                                             const PRUnichar* aCharCode,
                                             const PRUnichar* aModifiers,
                                             const PRUnichar* aButton,
                                             const PRUnichar* aClickCount,
                                             const PRUnichar* aGroup,
                                             const PRUnichar* aPreventDefault,
                                             const PRUnichar* aAllowUntrusted,
                                             nsXBLPrototypeBinding* aBinding,
                                             uint32_t aLineNumber)
  : mHandlerText(nullptr),
    mLineNumber(aLineNumber),
    mNextHandler(nullptr),
    mPrototypeBinding(aBinding)
{
  Init();

  ConstructPrototype(nullptr, aEvent, aPhase, aAction, aCommand, aKeyCode,
                     aCharCode, aModifiers, aButton, aClickCount,
                     aGroup, aPreventDefault, aAllowUntrusted);
}

nsXBLPrototypeHandler::nsXBLPrototypeHandler(nsIContent* aHandlerElement)
  : mHandlerElement(nullptr),
    mLineNumber(0),
    mNextHandler(nullptr),
    mPrototypeBinding(nullptr)
{
  Init();

  
  ConstructPrototype(aHandlerElement);
}

nsXBLPrototypeHandler::nsXBLPrototypeHandler(nsXBLPrototypeBinding* aBinding)
  : mHandlerText(nullptr),
    mLineNumber(0),
    mNextHandler(nullptr),
    mPrototypeBinding(aBinding)
{
  Init();
}

nsXBLPrototypeHandler::~nsXBLPrototypeHandler()
{
  --gRefCnt;
  if (mType & NS_HANDLER_TYPE_XUL) {
    NS_IF_RELEASE(mHandlerElement);
  } else if (mHandlerText) {
    nsMemory::Free(mHandlerText);
  }

  
  NS_CONTENT_DELETE_LIST_MEMBER(nsXBLPrototypeHandler, this, mNextHandler);
}

already_AddRefed<nsIContent>
nsXBLPrototypeHandler::GetHandlerElement()
{
  if (mType & NS_HANDLER_TYPE_XUL) {
    nsCOMPtr<nsIContent> element = do_QueryReferent(mHandlerElement);
    nsIContent* el = nullptr;
    element.swap(el);
    return el;
  }

  return nullptr;
}

void
nsXBLPrototypeHandler::AppendHandlerText(const nsAString& aText) 
{
  if (mHandlerText) {
    
    PRUnichar* temp = mHandlerText;
    mHandlerText = ToNewUnicode(nsDependentString(temp) + aText);
    nsMemory::Free(temp);
  }
  else {
    mHandlerText = ToNewUnicode(aText);
  }
}




void
nsXBLPrototypeHandler::InitAccessKeys()
{
  if (kAccelKey >= 0 && kMenuAccessKey >= 0)
    return;

  
  
#ifdef XP_MACOSX
  kMenuAccessKey = 0;
  kAccelKey = nsIDOMKeyEvent::DOM_VK_META;
#else
  kMenuAccessKey = nsIDOMKeyEvent::DOM_VK_ALT;
  kAccelKey = nsIDOMKeyEvent::DOM_VK_CONTROL;
#endif

  
  kMenuAccessKey =
    Preferences::GetInt("ui.key.menuAccessKey", kMenuAccessKey);
  kAccelKey = Preferences::GetInt("ui.key.accelKey", kAccelKey);
}

nsresult
nsXBLPrototypeHandler::ExecuteHandler(nsIDOMEventTarget* aTarget,
                                      nsIDOMEvent* aEvent)
{
  nsresult rv = NS_ERROR_FAILURE;

  
  if (mType & NS_HANDLER_TYPE_PREVENTDEFAULT) {
    aEvent->PreventDefault();
    
    
    rv = NS_OK;
  }

  if (!mHandlerElement) 
    return rv;

  
  bool isXULKey = !!(mType & NS_HANDLER_TYPE_XUL);
  bool isXBLCommand = !!(mType & NS_HANDLER_TYPE_XBL_COMMAND);
  NS_ASSERTION(!(isXULKey && isXBLCommand),
               "can't be both a key and xbl command handler");

  
  
  if (isXULKey || isXBLCommand) {
    bool trustedEvent = false;
    aEvent->GetIsTrusted(&trustedEvent);

    if (!trustedEvent)
      return NS_OK;
  }
    
  if (isXBLCommand) {
    return DispatchXBLCommand(aTarget, aEvent);
  }

  
  
  
  if (isXULKey) {
    return DispatchXULKeyCommand(aEvent);
  }

  
  
  nsCOMPtr<nsIAtom> onEventAtom = do_GetAtom(NS_LITERAL_STRING("onxbl") +
                                             nsDependentAtomString(mEventName));

  
  nsCOMPtr<nsIScriptGlobalObject> boundGlobal;
  nsCOMPtr<nsPIWindowRoot> winRoot(do_QueryInterface(aTarget));
  nsCOMPtr<nsPIDOMWindow> window;

  if (winRoot) {
    window = winRoot->GetWindow();
  }

  if (window) {
    window = window->GetCurrentInnerWindow();
    NS_ENSURE_TRUE(window, NS_ERROR_UNEXPECTED);

    boundGlobal = do_QueryInterface(window->GetPrivateRoot());
  }
  else boundGlobal = do_QueryInterface(aTarget);

  if (!boundGlobal) {
    nsCOMPtr<nsIDocument> boundDocument(do_QueryInterface(aTarget));
    if (!boundDocument) {
      
      nsCOMPtr<nsIContent> content(do_QueryInterface(aTarget));
      if (!content)
        return NS_OK;
      boundDocument = content->OwnerDoc();
    }

    boundGlobal = boundDocument->GetScopeObject();
  }

  if (!boundGlobal)
    return NS_OK;

  nsIScriptContext *boundContext = boundGlobal->GetScriptContext();
  if (!boundContext)
    return NS_OK;

  nsScriptObjectHolder<JSObject> handler(boundContext);
  nsISupports *scriptTarget;

  if (winRoot) {
    scriptTarget = boundGlobal;
  } else {
    scriptTarget = aTarget;
  }

  
  
  
  nsCxPusher pusher;
  NS_ENSURE_STATE(pusher.Push(aTarget));

  rv = EnsureEventHandler(boundGlobal, boundContext, onEventAtom, handler);
  NS_ENSURE_SUCCESS(rv, rv);

  AutoPushJSContext cx(boundContext->GetNativeContext());
  JSAutoRequest ar(cx);
  JSObject* globalObject = boundGlobal->GetGlobalJSObject();
  JSObject* scopeObject = xpc::GetXBLScope(cx, globalObject);

  
  
  

  
  
  JSAutoCompartment ac(cx, scopeObject);
  JSObject* genericHandler = handler.get();
  bool ok = JS_WrapObject(cx, &genericHandler);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  MOZ_ASSERT(!js::IsCrossCompartmentWrapper(genericHandler));

  
  
  
  JS::Value targetV = JS::UndefinedValue();
  rv = nsContentUtils::WrapNative(cx, scopeObject, scriptTarget, &targetV, nullptr,
                                   true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  JSObject* bound = JS_CloneFunctionObject(cx, genericHandler, &targetV.toObject());
  NS_ENSURE_TRUE(bound, NS_ERROR_FAILURE);

  
  JSAutoCompartment ac2(cx, globalObject);
  if (!JS_WrapObject(cx, &bound)) {
    return NS_ERROR_FAILURE;
  }
  nsScriptObjectHolder<JSObject> boundHandler(boundContext, bound);

  nsRefPtr<EventHandlerNonNull> handlerCallback =
    new EventHandlerNonNull(cx, globalObject, boundHandler.get(), &ok);
  if (!ok) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsEventHandler eventHandler(handlerCallback);

  
  nsCOMPtr<nsIJSEventListener> eventListener;
  rv = NS_NewJSEventListener(nullptr, globalObject,
                             scriptTarget, onEventAtom,
                             eventHandler,
                             getter_AddRefs(eventListener));
  NS_ENSURE_SUCCESS(rv, rv);

  
  eventListener->HandleEvent(aEvent);
  eventListener->Disconnect();
  return NS_OK;
}

nsresult
nsXBLPrototypeHandler::EnsureEventHandler(nsIScriptGlobalObject* aGlobal,
                                          nsIScriptContext *aBoundContext,
                                          nsIAtom *aName,
                                          nsScriptObjectHolder<JSObject>& aHandler)
{
  
  nsCOMPtr<nsPIDOMWindow> pWindow = do_QueryInterface(aGlobal);
  if (pWindow) {
    JSObject* cachedHandler = pWindow->GetCachedXBLPrototypeHandler(this);
    if (cachedHandler) {
      xpc_UnmarkGrayObject(cachedHandler);
      aHandler.set(cachedHandler);
      NS_ENSURE_TRUE(aHandler, NS_ERROR_FAILURE);
      return NS_OK;
    }
  }

  
  nsDependentString handlerText(mHandlerText);
  NS_ENSURE_TRUE(!handlerText.IsEmpty(), NS_ERROR_FAILURE);

  AutoPushJSContext cx(aBoundContext->GetNativeContext());
  JSObject* globalObject = aGlobal->GetGlobalJSObject();
  JSObject* scopeObject = xpc::GetXBLScope(cx, globalObject);

  nsAutoCString bindingURI;
  mPrototypeBinding->DocURI()->GetSpec(bindingURI);

  uint32_t argCount;
  const char **argNames;
  nsContentUtils::GetEventArgNames(kNameSpaceID_XBL, aName, &argCount,
                                   &argNames);

  
  JSAutoRequest ar(cx);
  JSAutoCompartment ac(cx, scopeObject);
  JS::CompileOptions options(cx);
  options.setFileAndLine(bindingURI.get(), mLineNumber)
         .setVersion(JSVERSION_LATEST)
         .setUserBit(true); 

  JS::RootedObject rootedNull(cx, nullptr); 
  JSObject* handlerFun = nullptr;
  nsresult rv = nsJSUtils::CompileFunction(cx, rootedNull, options,
                                           nsAtomCString(aName), argCount,
                                           argNames, handlerText, &handlerFun);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(handlerFun, NS_ERROR_FAILURE);

  
  
  JSAutoCompartment ac2(cx, globalObject);
  bool ok = JS_WrapObject(cx, &handlerFun);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);
  aHandler.set(handlerFun);
  NS_ENSURE_TRUE(aHandler, NS_ERROR_FAILURE);

  if (pWindow) {
    pWindow->CacheXBLPrototypeHandler(this, aHandler);
  }

  return NS_OK;
}

nsresult
nsXBLPrototypeHandler::DispatchXBLCommand(nsIDOMEventTarget* aTarget, nsIDOMEvent* aEvent)
{
  
  

  if (aEvent) {
    
    bool preventDefault = false;
    aEvent->GetPreventDefault(&preventDefault);
    if (preventDefault) {
      return NS_OK;
    }
    bool dispatchStopped = aEvent->IsDispatchStopped();
    if (dispatchStopped) {
      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIController> controller;

  nsCOMPtr<nsPIDOMWindow> privateWindow;
  nsCOMPtr<nsPIWindowRoot> windowRoot(do_QueryInterface(aTarget));
  if (windowRoot) {
    privateWindow = windowRoot->GetWindow();
  }
  else {
    privateWindow = do_QueryInterface(aTarget);
    if (!privateWindow) {
      nsCOMPtr<nsIContent> elt(do_QueryInterface(aTarget));
      nsCOMPtr<nsIDocument> doc;
      
      
      
      
      if (elt)
        doc = elt->OwnerDoc();

      if (!doc)
        doc = do_QueryInterface(aTarget);

      if (!doc)
        return NS_ERROR_FAILURE;

      privateWindow = do_QueryInterface(doc->GetScriptGlobalObject());
      if (!privateWindow)
        return NS_ERROR_FAILURE;
    }

    windowRoot = privateWindow->GetTopWindowRoot();
  }

  NS_LossyConvertUTF16toASCII command(mHandlerText);
  if (windowRoot)
    windowRoot->GetControllerForCommand(command.get(), getter_AddRefs(controller));
  else
    controller = GetController(aTarget); 

  if (mEventName == nsGkAtoms::keypress &&
      mDetail == nsIDOMKeyEvent::DOM_VK_SPACE &&
      mMisc == 1) {
    
    

    nsCOMPtr<nsPIDOMWindow> windowToCheck;
    if (windowRoot)
      windowToCheck = windowRoot->GetWindow();
    else
      windowToCheck = privateWindow->GetPrivateRoot();

    nsCOMPtr<nsIContent> focusedContent;
    if (windowToCheck) {
      nsCOMPtr<nsPIDOMWindow> focusedWindow;
      focusedContent =
        nsFocusManager::GetFocusedDescendant(windowToCheck, true, getter_AddRefs(focusedWindow));
    }

    bool isLink = false;
    nsIContent *content = focusedContent;

    
    
    
    
    if (focusedContent && focusedContent->GetParent()) {
      while (content) {
        if (content->Tag() == nsGkAtoms::a && content->IsHTML()) {
          isLink = true;
          break;
        }

        if (content->HasAttr(kNameSpaceID_XLink, nsGkAtoms::type)) {
          isLink = content->AttrValueIs(kNameSpaceID_XLink, nsGkAtoms::type,
                                        nsGkAtoms::simple, eCaseMatters);

          if (isLink) {
            break;
          }
        }

        content = content->GetParent();
      }

      if (!isLink)
        return NS_OK;
    }
  }

  
  
  aEvent->PreventDefault();
  
  if (controller)
    controller->DoCommand(command.get());

  return NS_OK;
}

nsresult
nsXBLPrototypeHandler::DispatchXULKeyCommand(nsIDOMEvent* aEvent)
{
  nsCOMPtr<nsIContent> handlerElement = GetHandlerElement();
  NS_ENSURE_STATE(handlerElement);
  if (handlerElement->AttrValueIs(kNameSpaceID_None,
                                  nsGkAtoms::disabled,
                                  nsGkAtoms::_true,
                                  eCaseMatters)) {
    
    return NS_OK;
  }

  aEvent->PreventDefault();

  
  nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
  if (!keyEvent) {
    NS_ERROR("Trying to execute a key handler for a non-key event!");
    return NS_ERROR_FAILURE;
  }

  

  bool isAlt = false;
  bool isControl = false;
  bool isShift = false;
  bool isMeta = false;
  keyEvent->GetAltKey(&isAlt);
  keyEvent->GetCtrlKey(&isControl);
  keyEvent->GetShiftKey(&isShift);
  keyEvent->GetMetaKey(&isMeta);

  nsContentUtils::DispatchXULCommand(handlerElement, true,
                                     nullptr, nullptr,
                                     isControl, isAlt, isShift, isMeta);
  return NS_OK;
}

already_AddRefed<nsIAtom>
nsXBLPrototypeHandler::GetEventName()
{
  nsIAtom* eventName = mEventName;
  NS_IF_ADDREF(eventName);
  return eventName;
}

already_AddRefed<nsIController>
nsXBLPrototypeHandler::GetController(nsIDOMEventTarget* aTarget)
{
  
  
  nsCOMPtr<nsIControllers> controllers;

  nsCOMPtr<nsIDOMXULElement> xulElement(do_QueryInterface(aTarget));
  if (xulElement)
    xulElement->GetControllers(getter_AddRefs(controllers));

  if (!controllers) {
    nsCOMPtr<nsIDOMHTMLTextAreaElement> htmlTextArea(do_QueryInterface(aTarget));
    if (htmlTextArea)
      htmlTextArea->GetControllers(getter_AddRefs(controllers));
  }

  if (!controllers) {
    nsCOMPtr<nsIDOMHTMLInputElement> htmlInputElement(do_QueryInterface(aTarget));
    if (htmlInputElement)
      htmlInputElement->GetControllers(getter_AddRefs(controllers));
  }

  if (!controllers) {
    nsCOMPtr<nsIDOMWindow> domWindow(do_QueryInterface(aTarget));
    if (domWindow)
      domWindow->GetControllers(getter_AddRefs(controllers));
  }

  
  
  
  nsIController* controller;
  if (controllers) {
    controllers->GetControllerAt(0, &controller);  
  }
  else controller = nullptr;

  return controller;
}

bool
nsXBLPrototypeHandler::KeyEventMatched(nsIDOMKeyEvent* aKeyEvent,
                                       uint32_t aCharCode,
                                       bool aIgnoreShiftKey)
{
  if (mDetail != -1) {
    
    uint32_t code;

    if (mMisc) {
      if (aCharCode)
        code = aCharCode;
      else
        aKeyEvent->GetCharCode(&code);
      if (IS_IN_BMP(code))
        code = ToLowerCase(PRUnichar(code));
    }
    else
      aKeyEvent->GetKeyCode(&code);

    if (code != uint32_t(mDetail))
      return false;
  }

  return ModifiersMatchMask(aKeyEvent, aIgnoreShiftKey);
}

bool
nsXBLPrototypeHandler::MouseEventMatched(nsIDOMMouseEvent* aMouseEvent)
{
  if (mDetail == -1 && mMisc == 0 && (mKeyMask & cAllModifiers) == 0)
    return true; 

  uint16_t button;
  aMouseEvent->GetButton(&button);
  if (mDetail != -1 && (button != mDetail))
    return false;

  int32_t clickcount;
  aMouseEvent->GetDetail(&clickcount);
  if (mMisc != 0 && (clickcount != mMisc))
    return false;

  return ModifiersMatchMask(aMouseEvent);
}

struct keyCodeData {
  const char* str;
  size_t strlength;
  uint32_t keycode;
};




static const keyCodeData gKeyCodes[] = {

#define NS_DEFINE_VK(aDOMKeyName, aDOMKeyCode) \
  { #aDOMKeyName, sizeof(#aDOMKeyName) - 1, aDOMKeyCode }
#include "nsVKList.h"
#undef NS_DEFINE_VK
};

int32_t nsXBLPrototypeHandler::GetMatchingKeyCode(const nsAString& aKeyName)
{
  nsAutoCString keyName;
  keyName.AssignWithConversion(aKeyName);
  ToUpperCase(keyName); 
                        

  uint32_t keyNameLength = keyName.Length();
  const char* keyNameStr = keyName.get();
  for (uint16_t i = 0; i < (sizeof(gKeyCodes) / sizeof(gKeyCodes[0])); ++i)
    if (keyNameLength == gKeyCodes[i].strlength &&
        !nsCRT::strcmp(gKeyCodes[i].str, keyNameStr))
      return gKeyCodes[i].keycode;

  return 0;
}

int32_t nsXBLPrototypeHandler::KeyToMask(int32_t key)
{
  switch (key)
  {
    case nsIDOMKeyEvent::DOM_VK_META:
      return cMeta | cMetaMask;

    case nsIDOMKeyEvent::DOM_VK_WIN:
      return cOS | cOSMask;

    case nsIDOMKeyEvent::DOM_VK_ALT:
      return cAlt | cAltMask;

    case nsIDOMKeyEvent::DOM_VK_CONTROL:
    default:
      return cControl | cControlMask;
  }
  return cControl | cControlMask;  
}

void
nsXBLPrototypeHandler::GetEventType(nsAString& aEvent)
{
  nsCOMPtr<nsIContent> handlerElement = GetHandlerElement();
  if (!handlerElement) {
    aEvent.Truncate();
    return;
  }
  handlerElement->GetAttr(kNameSpaceID_None, nsGkAtoms::event, aEvent);
  
  if (aEvent.IsEmpty() && (mType & NS_HANDLER_TYPE_XUL))
    
    aEvent.AssignLiteral("keypress");
}

void
nsXBLPrototypeHandler::ConstructPrototype(nsIContent* aKeyElement, 
                                          const PRUnichar* aEvent,
                                          const PRUnichar* aPhase,
                                          const PRUnichar* aAction,
                                          const PRUnichar* aCommand,
                                          const PRUnichar* aKeyCode,
                                          const PRUnichar* aCharCode,
                                          const PRUnichar* aModifiers,
                                          const PRUnichar* aButton,
                                          const PRUnichar* aClickCount,
                                          const PRUnichar* aGroup,
                                          const PRUnichar* aPreventDefault,
                                          const PRUnichar* aAllowUntrusted)
{
  mType = 0;

  if (aKeyElement) {
    mType |= NS_HANDLER_TYPE_XUL;
    nsCOMPtr<nsIWeakReference> weak = do_GetWeakReference(aKeyElement);
    if (!weak) {
      return;
    }
    weak.swap(mHandlerElement);
  }
  else {
    mType |= aCommand ? NS_HANDLER_TYPE_XBL_COMMAND : NS_HANDLER_TYPE_XBL_JS;
    mHandlerText = nullptr;
  }

  mDetail = -1;
  mMisc = 0;
  mKeyMask = 0;
  mPhase = NS_PHASE_BUBBLING;

  if (aAction)
    mHandlerText = ToNewUnicode(nsDependentString(aAction));
  else if (aCommand)
    mHandlerText = ToNewUnicode(nsDependentString(aCommand));

  nsAutoString event(aEvent);
  if (event.IsEmpty()) {
    if (mType & NS_HANDLER_TYPE_XUL)
      GetEventType(event);
    if (event.IsEmpty())
      return;
  }

  mEventName = do_GetAtom(event);

  if (aPhase) {
    const nsDependentString phase(aPhase);
    if (phase.EqualsLiteral("capturing"))
      mPhase = NS_PHASE_CAPTURING;
    else if (phase.EqualsLiteral("target"))
      mPhase = NS_PHASE_TARGET;
  }

  
  
  if (aButton && *aButton)
    mDetail = *aButton - '0';

  if (aClickCount && *aClickCount)
    mMisc = *aClickCount - '0';

  
  nsAutoString modifiers(aModifiers);
  if (mType & NS_HANDLER_TYPE_XUL)
    aKeyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::modifiers, modifiers);
  
  if (!modifiers.IsEmpty()) {
    mKeyMask = cAllModifiers;
    char* str = ToNewCString(modifiers);
    char* newStr;
    char* token = nsCRT::strtok( str, ", \t", &newStr );
    while( token != NULL ) {
      if (PL_strcmp(token, "shift") == 0)
        mKeyMask |= cShift | cShiftMask;
      else if (PL_strcmp(token, "alt") == 0)
        mKeyMask |= cAlt | cAltMask;
      else if (PL_strcmp(token, "meta") == 0)
        mKeyMask |= cMeta | cMetaMask;
      else if (PL_strcmp(token, "os") == 0)
        mKeyMask |= cOS | cOSMask;
      else if (PL_strcmp(token, "control") == 0)
        mKeyMask |= cControl | cControlMask;
      else if (PL_strcmp(token, "accel") == 0)
        mKeyMask |= KeyToMask(kAccelKey);
      else if (PL_strcmp(token, "access") == 0)
        mKeyMask |= KeyToMask(kMenuAccessKey);
      else if (PL_strcmp(token, "any") == 0)
        mKeyMask &= ~(mKeyMask << 5);
    
      token = nsCRT::strtok( newStr, ", \t", &newStr );
    }

    nsMemory::Free(str);
  }

  nsAutoString key(aCharCode);
  if (key.IsEmpty()) {
    if (mType & NS_HANDLER_TYPE_XUL) {
      aKeyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::key, key);
      if (key.IsEmpty()) 
        aKeyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::charcode, key);
    }
  }

  if (!key.IsEmpty()) {
    if (mKeyMask == 0)
      mKeyMask = cAllModifiers;
    ToLowerCase(key);

    
    mMisc = 1;
    mDetail = key[0];
    const uint8_t GTK2Modifiers = cShift | cControl | cShiftMask | cControlMask;
    if ((mKeyMask & GTK2Modifiers) == GTK2Modifiers &&
        modifiers.First() != PRUnichar(',') &&
        (mDetail == 'u' || mDetail == 'U'))
      ReportKeyConflict(key.get(), modifiers.get(), aKeyElement, "GTK2Conflict");
    const uint8_t WinModifiers = cControl | cAlt | cControlMask | cAltMask;
    if ((mKeyMask & WinModifiers) == WinModifiers &&
        modifiers.First() != PRUnichar(',') &&
        (('A' <= mDetail && mDetail <= 'Z') ||
         ('a' <= mDetail && mDetail <= 'z')))
      ReportKeyConflict(key.get(), modifiers.get(), aKeyElement, "WinConflict");
  }
  else {
    key.Assign(aKeyCode);
    if (mType & NS_HANDLER_TYPE_XUL)
      aKeyElement->GetAttr(kNameSpaceID_None, nsGkAtoms::keycode, key);
    
    if (!key.IsEmpty()) {
      if (mKeyMask == 0)
        mKeyMask = cAllModifiers;
      mDetail = GetMatchingKeyCode(key);
    }
  }

  if (aGroup && nsDependentString(aGroup).EqualsLiteral("system"))
    mType |= NS_HANDLER_TYPE_SYSTEM;

  if (aPreventDefault &&
      nsDependentString(aPreventDefault).EqualsLiteral("true"))
    mType |= NS_HANDLER_TYPE_PREVENTDEFAULT;

  if (aAllowUntrusted) {
    mType |= NS_HANDLER_HAS_ALLOW_UNTRUSTED_ATTR;
    if (nsDependentString(aAllowUntrusted).EqualsLiteral("true")) {
      mType |= NS_HANDLER_ALLOW_UNTRUSTED;
    } else {
      mType &= ~NS_HANDLER_ALLOW_UNTRUSTED;
    }
  }
}

void
nsXBLPrototypeHandler::ReportKeyConflict(const PRUnichar* aKey, const PRUnichar* aModifiers, nsIContent* aKeyElement, const char *aMessageName)
{
  nsCOMPtr<nsIDocument> doc;
  if (mPrototypeBinding) {
    nsXBLDocumentInfo* docInfo = mPrototypeBinding->XBLDocumentInfo();
    if (docInfo) {
      doc = docInfo->GetDocument();
    }
  } else if (aKeyElement) {
    doc = aKeyElement->OwnerDoc();
  }

  const PRUnichar* params[] = { aKey, aModifiers };
  nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                  "XBL Prototype Handler", doc,
                                  nsContentUtils::eXBL_PROPERTIES,
                                  aMessageName,
                                  params, ArrayLength(params),
                                  nullptr, EmptyString(), mLineNumber);
}

bool
nsXBLPrototypeHandler::ModifiersMatchMask(nsIDOMUIEvent* aEvent,
                                          bool aIgnoreShiftKey)
{
  nsEvent* event = aEvent->GetInternalNSEvent();
  NS_ENSURE_TRUE(event && NS_IS_INPUT_EVENT(event), false);
  nsInputEvent* inputEvent = static_cast<nsInputEvent*>(event);

  if (mKeyMask & cMetaMask) {
    if (inputEvent->IsMeta() != ((mKeyMask & cMeta) != 0)) {
      return false;
    }
  }

  if (mKeyMask & cOSMask) {
    if (inputEvent->IsOS() != ((mKeyMask & cOS) != 0)) {
      return false;
    }
  }

  if (mKeyMask & cShiftMask && !aIgnoreShiftKey) {
    if (inputEvent->IsShift() != ((mKeyMask & cShift) != 0)) {
      return false;
    }
  }

  if (mKeyMask & cAltMask) {
    if (inputEvent->IsAlt() != ((mKeyMask & cAlt) != 0)) {
      return false;
    }
  }

  if (mKeyMask & cControlMask) {
    if (inputEvent->IsControl() != ((mKeyMask & cControl) != 0)) {
      return false;
    }
  }

  return true;
}

nsresult
nsXBLPrototypeHandler::Read(nsIScriptContext* aContext, nsIObjectInputStream* aStream)
{
  nsresult rv = aStream->Read8(&mPhase);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Read8(&mType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Read8(&mMisc);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->Read32(reinterpret_cast<uint32_t*>(&mKeyMask));
  NS_ENSURE_SUCCESS(rv, rv);
  uint32_t detail; 
  rv = aStream->Read32(&detail);
  NS_ENSURE_SUCCESS(rv, rv);
  mDetail = detail;

  nsAutoString name;
  rv = aStream->ReadString(name);
  NS_ENSURE_SUCCESS(rv, rv);
  mEventName = do_GetAtom(name);

  rv = aStream->Read32(&mLineNumber);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString handlerText;
  rv = aStream->ReadString(handlerText);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!handlerText.IsEmpty())
    mHandlerText = ToNewUnicode(handlerText);

  return NS_OK;
}

nsresult
nsXBLPrototypeHandler::Write(nsIScriptContext* aContext, nsIObjectOutputStream* aStream)
{
  
  
  if ((mType & NS_HANDLER_TYPE_XUL) || !mEventName)
    return NS_OK;

  XBLBindingSerializeDetails type = XBLBinding_Serialize_Handler;

  nsresult rv = aStream->Write8(type);
  rv = aStream->Write8(mPhase);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Write8(mType);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Write8(mMisc);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Write32(static_cast<uint32_t>(mKeyMask));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aStream->Write32(mDetail);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->WriteWStringZ(nsDependentAtomString(mEventName).get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->Write32(mLineNumber);
  NS_ENSURE_SUCCESS(rv, rv);
  return aStream->WriteWStringZ(mHandlerText ? mHandlerText : EmptyString().get());
}
