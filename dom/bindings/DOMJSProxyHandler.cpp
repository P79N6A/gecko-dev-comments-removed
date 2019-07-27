





#include "mozilla/dom/DOMJSProxyHandler.h"
#include "xpcpublic.h"
#include "xpcprivate.h"
#include "XPCQuickStubs.h"
#include "XPCWrapper.h"
#include "WrapperFactory.h"
#include "nsDOMClassInfo.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/dom/BindingUtils.h"

#include "jsapi.h"

using namespace JS;

namespace mozilla {
namespace dom {

jsid s_length_id = JSID_VOID;

bool
DefineStaticJSVals(JSContext* cx)
{
  return InternJSString(cx, s_length_id, "length");
}

const char DOMProxyHandler::family = 0;

js::DOMProxyShadowsResult
DOMProxyShadows(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id)
{
  JS::Value v = js::GetProxyExtra(proxy, JSPROXYSLOT_EXPANDO);
  if (v.isObject()) {
    bool hasOwn;
    Rooted<JSObject*> object(cx, &v.toObject());
    if (!JS_AlreadyHasOwnPropertyById(cx, object, id, &hasOwn))
      return js::ShadowCheckFailed;

    return hasOwn ? js::Shadows : js::DoesntShadow;
  }

  if (v.isUndefined()) {
    return js::DoesntShadow;
  }

  bool hasOwn;
  if (!GetProxyHandler(proxy)->hasOwn(cx, proxy, id, &hasOwn))
    return js::ShadowCheckFailed;

  return hasOwn ? js::Shadows : js::DoesntShadowUnique;
}


struct SetDOMProxyInformation
{
  SetDOMProxyInformation() {
    js::SetDOMProxyInformation((const void*) &DOMProxyHandler::family,
                               js::PROXY_EXTRA_SLOT + JSPROXYSLOT_EXPANDO, DOMProxyShadows);
  }
};

SetDOMProxyInformation gSetDOMProxyInformation;


JSObject*
DOMProxyHandler::GetAndClearExpandoObject(JSObject* obj)
{
  MOZ_ASSERT(IsDOMProxy(obj), "expected a DOM proxy object");
  JS::Value v = js::GetProxyExtra(obj, JSPROXYSLOT_EXPANDO);
  if (v.isUndefined()) {
    return nullptr;
  }

  if (v.isObject()) {
    js::SetProxyExtra(obj, JSPROXYSLOT_EXPANDO, UndefinedValue());
    xpc::ObjectScope(obj)->RemoveDOMExpandoObject(obj);
  } else {
    js::ExpandoAndGeneration* expandoAndGeneration =
      static_cast<js::ExpandoAndGeneration*>(v.toPrivate());
    v = expandoAndGeneration->expando;
    if (v.isUndefined()) {
      return nullptr;
    }
    expandoAndGeneration->expando = UndefinedValue();
  }


  return &v.toObject();
}


JSObject*
DOMProxyHandler::EnsureExpandoObject(JSContext* cx, JS::Handle<JSObject*> obj)
{
  NS_ASSERTION(IsDOMProxy(obj), "expected a DOM proxy object");
  JS::Value v = js::GetProxyExtra(obj, JSPROXYSLOT_EXPANDO);
  if (v.isObject()) {
    return &v.toObject();
  }

  js::ExpandoAndGeneration* expandoAndGeneration;
  if (!v.isUndefined()) {
    expandoAndGeneration = static_cast<js::ExpandoAndGeneration*>(v.toPrivate());
    if (expandoAndGeneration->expando.isObject()) {
      return &expandoAndGeneration->expando.toObject();
    }
  } else {
    expandoAndGeneration = nullptr;
  }

  JS::Rooted<JSObject*> parent(cx, js::GetObjectParent(obj));
  JS::Rooted<JSObject*> expando(cx,
    JS_NewObjectWithGivenProto(cx, nullptr, JS::NullPtr(), parent));
  if (!expando) {
    return nullptr;
  }

  nsISupports* native = UnwrapDOMObject<nsISupports>(obj);
  nsWrapperCache* cache;
  CallQueryInterface(native, &cache);
  if (expandoAndGeneration) {
    cache->PreserveWrapper(native);
    expandoAndGeneration->expando.setObject(*expando);

    return expando;
  }

  if (!xpc::ObjectScope(obj)->RegisterDOMExpandoObject(obj)) {
    return nullptr;
  }

  cache->SetPreservingWrapper(true);
  js::SetProxyExtra(obj, JSPROXYSLOT_EXPANDO, ObjectValue(*expando));

  return expando;
}

bool
DOMProxyHandler::isExtensible(JSContext *cx, JS::Handle<JSObject*> proxy, bool *extensible) const
{
  
  *extensible = true;
  return true;
}

bool
DOMProxyHandler::preventExtensions(JSContext *cx, JS::Handle<JSObject*> proxy) const
{
  
  JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                       JSMSG_CANT_CHANGE_EXTENSIBILITY);
  return false;
}

bool
BaseDOMProxyHandler::getPropertyDescriptor(JSContext* cx,
                                           JS::Handle<JSObject*> proxy,
                                           JS::Handle<jsid> id,
                                           MutableHandle<JSPropertyDescriptor> desc) const
{
  if (!getOwnPropertyDescriptor(cx, proxy, id, desc)) {
    return false;
  }
  if (desc.object()) {
    return true;
  }

  JS::Rooted<JSObject*> proto(cx);
  if (!js::GetObjectProto(cx, proxy, &proto)) {
    return false;
  }
  if (!proto) {
    desc.object().set(nullptr);
    return true;
  }

  return JS_GetPropertyDescriptorById(cx, proto, id, desc);
}

bool
BaseDOMProxyHandler::getOwnPropertyDescriptor(JSContext* cx,
                                              JS::Handle<JSObject*> proxy,
                                              JS::Handle<jsid> id,
                                              MutableHandle<JSPropertyDescriptor> desc) const
{
  return getOwnPropDescriptor(cx, proxy, id,  false,
                              desc);
}

bool
DOMProxyHandler::defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                                MutableHandle<JSPropertyDescriptor> desc, bool* defined) const
{
  if (desc.hasGetterObject() && desc.setter() == JS_StrictPropertyStub) {
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, nullptr,
                                        JSMSG_GETTER_ONLY);
  }

  if (xpc::WrapperFactory::IsXrayWrapper(proxy)) {
    return true;
  }

  JSObject* expando = EnsureExpandoObject(cx, proxy);
  if (!expando) {
    return false;
  }

  bool dummy;
  return js_DefineOwnProperty(cx, expando, id, desc, &dummy);
}

bool
DOMProxyHandler::set(JSContext *cx, Handle<JSObject*> proxy, Handle<JSObject*> receiver,
                     Handle<jsid> id, bool strict, MutableHandle<JS::Value> vp) const
{
  MOZ_ASSERT(!xpc::WrapperFactory::IsXrayWrapper(proxy),
             "Should not have a XrayWrapper here");
  bool done;
  if (!setCustom(cx, proxy, id, vp, &done)) {
    return false;
  }
  if (done) {
    return true;
  }

  
  
  JS::Rooted<JSPropertyDescriptor> desc(cx);
  if (!getOwnPropDescriptor(cx, proxy, id,  true,
                            &desc)) {
    return false;
  }
  bool descIsOwn = desc.object() != nullptr;
  if (!desc.object()) {
    
    
    
    JS::Rooted<JSObject*> proto(cx);
    if (!js::GetObjectProto(cx, proxy, &proto)) {
      return false;
    }
    if (proto && !JS_GetPropertyDescriptorById(cx, proto, id, &desc)) {
      return false;
    }
  }

  return js::SetPropertyIgnoringNamedGetter(cx, this, proxy, receiver, id,
                                            &desc, descIsOwn, strict, vp);
}

bool
DOMProxyHandler::delete_(JSContext* cx, JS::Handle<JSObject*> proxy,
                         JS::Handle<jsid> id, bool* bp) const
{
  JS::Rooted<JSObject*> expando(cx);
  if (!xpc::WrapperFactory::IsXrayWrapper(proxy) && (expando = GetExpandoObject(proxy))) {
    return JS_DeletePropertyById2(cx, expando, id, bp);
  }

  *bp = true;
  return true;
}

bool
BaseDOMProxyHandler::enumerate(JSContext* cx, JS::Handle<JSObject*> proxy,
                               AutoIdVector& props) const
{
  JS::Rooted<JSObject*> proto(cx);
  if (!JS_GetPrototype(cx, proxy, &proto))  {
    return false;
  }
  return keys(cx, proxy, props) &&
         (!proto || js::GetPropertyNames(cx, proto, 0, &props));
}

bool
BaseDOMProxyHandler::watch(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                           JS::Handle<JSObject*> callable) const
{
  return js::WatchGuts(cx, proxy, id, callable);
}

bool
BaseDOMProxyHandler::unwatch(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id) const
{
  return js::UnwatchGuts(cx, proxy, id);
}

bool
BaseDOMProxyHandler::getOwnPropertyNames(JSContext* cx,
                                         JS::Handle<JSObject*> proxy,
                                         JS::AutoIdVector& props) const
{
  return ownPropNames(cx, proxy, JSITER_OWNONLY | JSITER_HIDDEN, props);
}

bool
BaseDOMProxyHandler::keys(JSContext* cx,
                          JS::Handle<JSObject*> proxy,
                          JS::AutoIdVector& props) const
{
  return ownPropNames(cx, proxy, JSITER_OWNONLY, props);
}

bool
DOMProxyHandler::has(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id, bool* bp) const
{
  if (!hasOwn(cx, proxy, id, bp)) {
    return false;
  }

  if (*bp) {
    
    
    return true;
  }

  
  JS::Rooted<JSObject*> proto(cx);
  if (!js::GetObjectProto(cx, proxy, &proto)) {
    return false;
  }
  if (!proto) {
    return true;
  }
  bool protoHasProp;
  bool ok = JS_HasPropertyById(cx, proto, id, &protoHasProp);
  if (ok) {
    *bp = protoHasProp;
  }
  return ok;
}

int32_t
IdToInt32(JSContext* cx, JS::Handle<jsid> id)
{
  JS::Rooted<JS::Value> idval(cx);
  double array_index;
  int32_t i;
  if (!::JS_IdToValue(cx, id, &idval) ||
      !JS::ToNumber(cx, idval, &array_index) ||
      !::JS_DoubleIsInt32(array_index, &i)) {
    return -1;
  }

  return i;
}

bool
DOMProxyHandler::setCustom(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                           JS::MutableHandle<JS::Value> vp, bool *done) const
{
  *done = false;
  return true;
}


JSObject *
DOMProxyHandler::GetExpandoObject(JSObject *obj)
{
  MOZ_ASSERT(IsDOMProxy(obj), "expected a DOM proxy object");
  JS::Value v = js::GetProxyExtra(obj, JSPROXYSLOT_EXPANDO);
  if (v.isObject()) {
    return &v.toObject();
  }

  if (v.isUndefined()) {
    return nullptr;
  }

  js::ExpandoAndGeneration* expandoAndGeneration =
    static_cast<js::ExpandoAndGeneration*>(v.toPrivate());
  v = expandoAndGeneration->expando;
  return v.isUndefined() ? nullptr : &v.toObject();
}

} 
} 
