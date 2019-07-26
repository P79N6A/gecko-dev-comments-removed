





#include "WindowNamedPropertiesHandler.h"
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
  if (nsContentUtils::GetSubjectPrincipal()->Equals(sop->GetPrincipal())) {
    return true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aChild);
  NS_ENSURE_TRUE(piWin, false);
  Element* e = piWin->GetFrameElementInternal();
  return e && e->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                             aNameBeingResolved, eCaseMatters);
}

static nsGlobalWindow*
GetWindowFromGlobal(JSObject* aGlobal)
{
  nsGlobalWindow* win;
  if (NS_SUCCEEDED(UNWRAP_OBJECT(Window, aGlobal, win))) {
    return win;
  }
  XPCWrappedNative* wrapper = XPCWrappedNative::Get(aGlobal);
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryWrappedNative(wrapper);
  MOZ_ASSERT(piWin);
  return static_cast<nsGlobalWindow*>(piWin.get());
}

bool
WindowNamedPropertiesHandler::getOwnPropertyDescriptor(JSContext* aCx,
                                                       JS::Handle<JSObject*> aProxy,
                                                       JS::Handle<jsid> aId,
                                                       JS::MutableHandle<JSPropertyDescriptor> aDesc,
                                                       unsigned aFlags)
{
  if (!JSID_IS_STRING(aId)) {
    
    return true;
  }

  JS::Rooted<JSObject*> global(aCx, JS_GetGlobalForObject(aCx, aProxy));
  if (HasPropertyOnPrototype(aCx, aProxy, aId)) {
    return true;
  }

  nsDependentJSString str(aId);

  
  nsGlobalWindow* win = GetWindowFromGlobal(global);
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
                                             JS::MutableHandle<JSPropertyDescriptor> aDesc)
{
  ErrorResult rv;
  rv.ThrowTypeError(MSG_DEFINEPROPERTY_ON_GSP);
  rv.ReportTypeError(aCx);
  return false;
}

bool
WindowNamedPropertiesHandler::getOwnPropertyNames(JSContext* aCx,
                                                  JS::Handle<JSObject*> aProxy,
                                                  JS::AutoIdVector& aProps)
{
  
  nsGlobalWindow* win = GetWindowFromGlobal(JS_GetGlobalForObject(aCx, aProxy));
  nsTArray<nsString> names;
  win->GetSupportedNames(names);
  if (!AppendNamedPropertyIds(aCx, aProxy, names, false, aProps)) {
    return false;
  }

  names.Clear();
  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(win->GetExtantDoc());
  if (!htmlDoc) {
    return true;
  }
  nsHTMLDocument* document = static_cast<nsHTMLDocument*>(htmlDoc.get());
  document->GetSupportedNames(names);

  JS::AutoIdVector docProps(aCx);
  if (!AppendNamedPropertyIds(aCx, aProxy, names, false, docProps)) {
    return false;
  }

  return js::AppendUnique(aCx, aProps, docProps);
}

bool
WindowNamedPropertiesHandler::delete_(JSContext* aCx,
                                      JS::Handle<JSObject*> aProxy,
                                      JS::Handle<jsid> aId, bool* aBp)
{
  *aBp = false;
  return true;
}


void
WindowNamedPropertiesHandler::Install(JSContext* aCx,
                                      JS::Handle<JSObject*> aProto)
{
  JS::Rooted<JSObject*> protoProto(aCx);
  if (!::JS_GetPrototype(aCx, aProto, &protoProto)) {
    return;
  }

  
  
  
  JS::Rooted<JSObject*> gsp(aCx);
  js::ProxyOptions options;
  options.setSingleton(true);
  gsp = js::NewProxyObject(aCx, WindowNamedPropertiesHandler::getInstance(),
                           JS::NullHandleValue, protoProto,
                           js::GetGlobalForObjectCrossCompartment(aProto),
                           options);
  if (!gsp) {
    return;
  }

  
  
  ::JS_SplicePrototype(aCx, aProto, gsp);
}

} 
} 
