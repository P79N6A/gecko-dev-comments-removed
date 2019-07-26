





#include "WindowNamedPropertiesHandler.h"
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
  return piWin->GetFrameElementInternal()->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                                                       aNameBeingResolved, eCaseMatters);
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

  
  XPCWrappedNative* wrapper = XPCWrappedNative::Get(global);
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryWrappedNative(wrapper);
  MOZ_ASSERT(piWin);
  nsGlobalWindow* win = static_cast<nsGlobalWindow*>(piWin.get());
  if (win->GetLength() > 0) {
    nsCOMPtr<nsIDOMWindow> childWin = win->GetChildWindow(str);
    if (childWin && ShouldExposeChildWindow(str, childWin)) {
      
      
      
      JS::Rooted<JS::Value> v(aCx);
      if (!WrapObject(aCx, aProxy, childWin, &v)) {
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
    if (!WrapObject(aCx, aProxy, element, &v)) {
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
  if (!WrapObject(aCx, aProxy, result, cache, nullptr, &v)) {
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
  
  JSObject* global = JS_GetGlobalForObject(aCx, aProxy);
  XPCWrappedNative* wrapper = XPCWrappedNative::Get(global);
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryWrappedNative(wrapper);
  MOZ_ASSERT(piWin);
  nsGlobalWindow* win = static_cast<nsGlobalWindow*>(piWin.get());
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
