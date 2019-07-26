





#include "mozilla/Util.h"

#include "DOMJSProxyHandler.h"
#include "xpcpublic.h"
#include "xpcprivate.h"
#include "XPCQuickStubs.h"
#include "XPCWrapper.h"
#include "WrapperFactory.h"
#include "nsDOMClassInfo.h"
#include "nsGlobalWindow.h"
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
  JSAutoRequest ar(cx);

  return InternJSString(cx, s_length_id, "length");
}


int HandlerFamily;


struct SetListBaseInformation
{
  SetListBaseInformation() {
    js::SetListBaseInformation((void*) &HandlerFamily, js::JSSLOT_PROXY_EXTRA + JSPROXYSLOT_EXPANDO);
  }
};

SetListBaseInformation gSetListBaseInformation;



JSObject*
DOMProxyHandler::EnsureExpandoObject(JSContext* cx, JSObject* obj)
{
  NS_ASSERTION(IsDOMProxy(obj), "expected a DOM proxy object");
  JSObject* expando = GetExpandoObject(obj);
  if (!expando) {
    expando = JS_NewObjectWithGivenProto(cx, nullptr, nullptr,
                                         js::GetObjectParent(obj));
    if (!expando) {
      return NULL;
    }

    XPCWrappedNativeScope* scope = xpc::GetObjectScope(obj);
    if (!scope->RegisterDOMExpandoObject(obj)) {
      return NULL;
    }

    nsWrapperCache* cache;
    CallQueryInterface(UnwrapDOMObject<nsISupports>(obj, eProxyDOMObject), &cache);
    cache->SetPreservingWrapper(true);

    js::SetProxyExtra(obj, JSPROXYSLOT_EXPANDO, ObjectValue(*expando));
  }
  return expando;
}

bool
DOMProxyHandler::getPropertyDescriptor(JSContext* cx, JSObject* proxy, jsid id, bool set,
                                       JSPropertyDescriptor* desc)
{
  if (!getOwnPropertyDescriptor(cx, proxy, id, set, desc)) {
    return false;
  }
  if (desc->obj) {
    return true;
  }

  JSObject* proto;
  if (!js::GetObjectProto(cx, proxy, &proto)) {
    return false;
  }
  if (!proto) {
    desc->obj = NULL;
    return true;
  }

  return JS_GetPropertyDescriptorById(cx, proto, id, JSRESOLVE_QUALIFIED, desc);
}

bool
DOMProxyHandler::defineProperty(JSContext* cx, JSObject* proxy, jsid id,
                                JSPropertyDescriptor* desc)
{
  if ((desc->attrs & JSPROP_GETTER) && desc->setter == JS_StrictPropertyStub) {
    return JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, NULL,
                                        JSMSG_GETTER_ONLY);
  }

  if (xpc::WrapperFactory::IsXrayWrapper(proxy)) {
    return true;
  }

  JSObject* expando = EnsureExpandoObject(cx, proxy);
  if (!expando) {
    return false;
  }

  return JS_DefinePropertyById(cx, expando, id, desc->value, desc->getter, desc->setter,
                               desc->attrs);
}

bool
DOMProxyHandler::delete_(JSContext* cx, JSObject* proxy, jsid id, bool* bp)
{
  JSBool b = true;

  JSObject* expando;
  if (!xpc::WrapperFactory::IsXrayWrapper(proxy) && (expando = GetExpandoObject(proxy))) {
    Value v;
    if (!JS_DeletePropertyById2(cx, expando, id, &v) || !JS_ValueToBoolean(cx, v, &b)) {
      return false;
    }
  }

  *bp = !!b;
  return true;
}

bool
DOMProxyHandler::enumerate(JSContext* cx, JSObject* proxy, AutoIdVector& props)
{
  JSObject* proto;
  if (!JS_GetPrototype(cx, proxy, &proto)) {
    return false;
  }
  return getOwnPropertyNames(cx, proxy, props) &&
         (!proto || js::GetPropertyNames(cx, proto, 0, &props));
}

bool
DOMProxyHandler::fix(JSContext* cx, JSObject* proxy, Value* vp)
{
  vp->setUndefined();
  return true;
}

bool
DOMProxyHandler::has(JSContext* cx, JSObject* proxy, jsid id, bool* bp)
{
  if (!hasOwn(cx, proxy, id, bp)) {
    return false;
  }

  if (*bp) {
    
    
    return true;
  }

  
  JSObject *proto;
  if (!js::GetObjectProto(cx, proxy, &proto)) {
    return false;
  }
  if (!proto) {
    return true;
  }
  JSBool protoHasProp;
  bool ok = JS_HasPropertyById(cx, proto, id, &protoHasProp);
  if (ok) {
    *bp = protoHasProp;
  }
  return ok;
}


JSString*
DOMProxyHandler::obj_toString(JSContext* cx, const char* className)
{
  size_t nchars = sizeof("[object ]") - 1 + strlen(className);
  jschar* chars = static_cast<jschar*>(JS_malloc(cx, (nchars + 1) * sizeof(jschar)));
  if (!chars) {
    return NULL;
  }

  const char* prefix = "[object ";
  nchars = 0;
  while ((chars[nchars] = (jschar)*prefix) != 0) {
    nchars++, prefix++;
  }
  while ((chars[nchars] = (jschar)*className) != 0) {
    nchars++, className++;
  }
  chars[nchars++] = ']';
  chars[nchars] = 0;

  JSString* str = JS_NewUCString(cx, chars, nchars);
  if (!str) {
    JS_free(cx, chars);
  }
  return str;
}

int32_t
IdToInt32(JSContext* cx, jsid id)
{
  JSAutoRequest ar(cx);

  jsval idval;
  double array_index;
  int32_t i;
  if (!::JS_IdToValue(cx, id, &idval) ||
      !::JS_ValueToNumber(cx, idval, &array_index) ||
      !::JS_DoubleIsInt32(array_index, &i)) {
    return -1;
  }

  return i;
}

} 
} 
