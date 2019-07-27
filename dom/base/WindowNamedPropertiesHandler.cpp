





#include "WindowNamedPropertiesHandler.h"
#include "mozilla/dom/EventTargetBinding.h"
#include "mozilla/dom/WindowBinding.h"
#include "nsDOMClassInfo.h"
#include "nsGlobalWindow.h"
#include "nsHTMLDocument.h"
#include "nsJSUtils.h"
#include "xpcprivate.h"

namespace mozilla {
namespace dom {

static bool
ShouldExposeChildWindow(nsString& aNameBeingResolved, nsIDOMWindow *aChild)
{
  
  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aChild);
  NS_ENSURE_TRUE(sop, false);
  if (nsContentUtils::SubjectPrincipal()->Equals(sop->GetPrincipal())) {
    return true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aChild);
  NS_ENSURE_TRUE(piWin, false);
  Element* e = piWin->GetFrameElementInternal();
  return e && e->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                             aNameBeingResolved, eCaseMatters);
}

bool
WindowNamedPropertiesHandler::getOwnPropDescriptor(JSContext* aCx,
                                                   JS::Handle<JSObject*> aProxy,
                                                   JS::Handle<jsid> aId,
                                                   bool ,
                                                   JS::MutableHandle<JSPropertyDescriptor> aDesc)
                                                   const
{
  if (!JSID_IS_STRING(aId)) {
    
    return true;
  }

  bool hasOnPrototype;
  if (!HasPropertyOnPrototype(aCx, aProxy, aId, &hasOnPrototype)) {
    return false;
  }
  if (hasOnPrototype) {
    return true;
  }

  nsAutoJSString str;
  if (!str.init(aCx, JSID_TO_STRING(aId))) {
    return false;
  }

  
  JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForObject(aCx, aProxy));
  nsGlobalWindow* win = xpc::WindowOrNull(global);
  if (win->Length() > 0) {
    nsCOMPtr<nsIDOMWindow> childWin = win->GetChildWindow(str);
    if (childWin && ShouldExposeChildWindow(str, childWin)) {
      
      
      
      JS::Rooted<JS::Value> v(aCx);
      if (!WrapObject(aCx, childWin, &v)) {
        return false;
      }
      aDesc.object().set(aProxy);
      aDesc.value().set(v);
      aDesc.setAttributes(JSPROP_ENUMERATE);
      return true;
    }
  }

  
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(win->GetExtantDoc());
  if (!htmlDoc) {
    return true;
  }
  nsHTMLDocument* document = static_cast<nsHTMLDocument*>(htmlDoc.get());

  Element* element = document->GetElementById(str);
  if (element) {
    JS::Rooted<JS::Value> v(aCx);
    if (!WrapObject(aCx, element, &v)) {
      return false;
    }
    aDesc.object().set(aProxy);
    aDesc.value().set(v);
    aDesc.setAttributes(JSPROP_ENUMERATE);
    return true;
  }

  nsWrapperCache* cache;
  nsISupports* result = document->ResolveName(str, &cache);
  if (!result) {
    return true;
  }

  JS::Rooted<JS::Value> v(aCx);
  if (!WrapObject(aCx, result, cache, nullptr, &v)) {
    return false;
  }
  aDesc.object().set(aProxy);
  aDesc.value().set(v);
  aDesc.setAttributes(JSPROP_ENUMERATE);
  return true;
}

bool
WindowNamedPropertiesHandler::defineProperty(JSContext* aCx,
                                             JS::Handle<JSObject*> aProxy,
                                             JS::Handle<jsid> aId,
                                             JS::MutableHandle<JSPropertyDescriptor> aDesc) const
{
  ErrorResult rv;
  rv.ThrowTypeError(MSG_DEFINEPROPERTY_ON_GSP);
  rv.ReportErrorWithMessage(aCx);
  return false;
}

bool
WindowNamedPropertiesHandler::ownPropNames(JSContext* aCx,
                                           JS::Handle<JSObject*> aProxy,
                                           unsigned flags,
                                           JS::AutoIdVector& aProps) const
{
  
  nsGlobalWindow* win = xpc::WindowOrNull(JS_GetGlobalForObject(aCx, aProxy));
  nsTArray<nsString> names;
  win->GetSupportedNames(names);
  
  
  for (size_t i = names.Length(); i > 0; ) {
    --i; 
    nsIDOMWindow* childWin = win->GetChildWindow(names[i]);
    if (!childWin || !ShouldExposeChildWindow(names[i], childWin)) {
      names.RemoveElementAt(i);
    }
  }
  if (!AppendNamedPropertyIds(aCx, aProxy, names, false, aProps)) {
    return false;
  }

  names.Clear();
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(win->GetExtantDoc());
  if (!htmlDoc) {
    return true;
  }
  nsHTMLDocument* document = static_cast<nsHTMLDocument*>(htmlDoc.get());
  document->GetSupportedNames(flags, names);

  JS::AutoIdVector docProps(aCx);
  if (!AppendNamedPropertyIds(aCx, aProxy, names, false, docProps)) {
    return false;
  }

  return js::AppendUnique(aCx, aProps, docProps);
}

bool
WindowNamedPropertiesHandler::delete_(JSContext* aCx,
                                      JS::Handle<JSObject*> aProxy,
                                      JS::Handle<jsid> aId, bool* aBp) const
{
  *aBp = false;
  return true;
}

static bool
ResolveWindowNamedProperty(JSContext* aCx, JS::Handle<JSObject*> aWrapper,
                           JS::Handle<JSObject*> aObj, JS::Handle<jsid> aId,
                           JS::MutableHandle<JSPropertyDescriptor> aDesc)
{
  {
    JSAutoCompartment ac(aCx, aObj);
    if (!js::GetProxyHandler(aObj)->getOwnPropertyDescriptor(aCx, aObj, aId,
                                                             aDesc)) {
      return false;
    }
  }

  if (aDesc.object()) {
    aDesc.object().set(aWrapper);

    return JS_WrapPropertyDescriptor(aCx, aDesc);
  }

  return true;
}

static bool
EnumerateWindowNamedProperties(JSContext* aCx, JS::Handle<JSObject*> aWrapper,
                               JS::Handle<JSObject*> aObj,
                               JS::AutoIdVector& aProps)
{
  JSAutoCompartment ac(aCx, aObj);
  return js::GetProxyHandler(aObj)->ownPropertyKeys(aCx, aObj, aProps);
}

const NativePropertyHooks sWindowNamedPropertiesNativePropertyHooks[] = { {
  ResolveWindowNamedProperty,
  EnumerateWindowNamedProperties,
  { nullptr, nullptr },
  prototypes::id::_ID_Count,
  constructors::id::_ID_Count,
  nullptr
} };

static const DOMIfaceAndProtoJSClass WindowNamedPropertiesClass = {
  PROXY_CLASS_DEF("WindowProperties",
                  JSCLASS_IS_DOMIFACEANDPROTOJSCLASS),
  eNamedPropertiesObject,
  sWindowNamedPropertiesNativePropertyHooks,
  "[object WindowProperties]",
  prototypes::id::_ID_Count,
  0,
  EventTargetBinding::GetProtoObject
};


JSObject*
WindowNamedPropertiesHandler::Create(JSContext* aCx,
                                     JS::Handle<JSObject*> aProto)
{
  
  
  
  JS::Rooted<JSObject*> gsp(aCx);
  js::ProxyOptions options;
  options.setSingleton(true);
  options.setClass(&WindowNamedPropertiesClass.mBase);
  return js::NewProxyObject(aCx, WindowNamedPropertiesHandler::getInstance(),
                            JS::NullHandleValue, aProto,
                            js::GetGlobalForObjectCrossCompartment(aProto),
                            options);
}

} 
} 
