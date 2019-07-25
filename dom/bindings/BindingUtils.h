





#ifndef mozilla_dom_BindingUtils_h__
#define mozilla_dom_BindingUtils_h__

#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/workers/Workers.h"

#include "jsapi.h"
#include "jsfriendapi.h"

#include "XPCQuickStubs.h"
#include "XPCWrapper.h"
#include "nsTraceRefcnt.h"
#include "nsWrapperCacheInlines.h"



class nsGlobalWindow;

namespace mozilla {
namespace dom {

template<bool mainThread>
inline bool
Throw(JSContext* cx, nsresult rv)
{
  using mozilla::dom::workers::exceptions::ThrowDOMExceptionForNSResult;

  
  if (mainThread) {
    XPCThrower::Throw(rv, cx);
  } else {
    if (!JS_IsExceptionPending(cx)) {
      ThrowDOMExceptionForNSResult(cx, rv);
    }
  }
  return false;
}

template<bool mainThread>
inline bool
ThrowMethodFailedWithDetails(JSContext* cx, nsresult rv,
                             const char* ,
                             const char* )
{
  return Throw<mainThread>(cx, rv);
}

inline bool
IsDOMClass(const JSClass* clasp)
{
  return clasp->flags & JSCLASS_IS_DOMJSCLASS;
}

template <class T>
inline T*
UnwrapDOMObject(JSObject* obj, const JSClass* clasp)
{
  MOZ_ASSERT(IsDOMClass(clasp));
  MOZ_ASSERT(JS_GetClass(obj) == clasp);

  size_t slot = DOMJSClass::FromJSClass(clasp)->mNativeSlot;
  MOZ_ASSERT((slot == DOM_OBJECT_SLOT &&
              !(clasp->flags & JSCLASS_DOM_GLOBAL)) ||
             (slot == DOM_GLOBAL_OBJECT_SLOT &&
              (clasp->flags & JSCLASS_DOM_GLOBAL)));

  JS::Value val = js::GetReservedSlot(obj, slot);
  
  
  
  
  if (val.isUndefined()) {
    return NULL;
  }
  
  return static_cast<T*>(val.toPrivate());
}

template <class T>
inline T*
UnwrapDOMObject(JSObject* obj, const js::Class* clasp)
{
  return UnwrapDOMObject<T>(obj, Jsvalify(clasp));
}




template <prototypes::ID PrototypeID, class T>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, T** value)
{
  
  JSClass* clasp = js::GetObjectJSClass(obj);
  if (!IsDOMClass(clasp)) {
    
    if (!js::IsWrapper(obj)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    obj = XPCWrapper::Unwrap(cx, obj, false);
    if (!obj) {
      return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }
    MOZ_ASSERT(!js::IsWrapper(obj));
    clasp = js::GetObjectJSClass(obj);
    if (!IsDOMClass(clasp)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }
  }

  MOZ_ASSERT(IsDOMClass(clasp));

  


  DOMJSClass* domClass = DOMJSClass::FromJSClass(clasp);
  if (domClass->mInterfaceChain[PrototypeTraits<PrototypeID>::Depth] ==
      PrototypeID) {
    *value = UnwrapDOMObject<T>(obj, clasp);
    return NS_OK;
  }

  
  return NS_ERROR_XPC_BAD_CONVERT_JS;
}

inline bool
IsArrayLike(JSContext* cx, JSObject* obj)
{
  MOZ_ASSERT(obj);
  
  if (js::IsWrapper(obj)) {
    obj = XPCWrapper::Unwrap(cx, obj, false);
    if (!obj) {
      
      return false;
    }
  }

  
  
  return JS_IsArrayObject(cx, obj);
}

inline bool
IsPlatformObject(JSContext* cx, JSObject* obj)
{
  
  
  
  
  
  MOZ_ASSERT(obj);
  
  JSClass* clasp = js::GetObjectJSClass(obj);
  if (IsDOMClass(clasp)) {
    return true;
  }
  
  if (js::IsWrapper(obj)) {
    obj = XPCWrapper::Unwrap(cx, obj, false);
    if (!obj) {
      
      return false;
    }
    clasp = js::GetObjectJSClass(obj);
  }
  return IS_WRAPPER_CLASS(js::Valueify(clasp)) || IsDOMClass(clasp) ||
    JS_IsArrayBufferObject(obj, cx);
}

template <class T>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, T* *value)
{
  return UnwrapObject<static_cast<prototypes::ID>(
           PrototypeIDMap<T>::PrototypeID)>(cx, obj, value);
}

const size_t kProtoOrIfaceCacheCount =
  prototypes::id::_ID_Count + constructors::id::_ID_Count;

inline void
AllocateProtoOrIfaceCache(JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);
  MOZ_ASSERT(js::GetReservedSlot(obj, DOM_PROTOTYPE_SLOT).isUndefined());

  
  JSObject** protoOrIfaceArray = new JSObject*[kProtoOrIfaceCacheCount]();

  js::SetReservedSlot(obj, DOM_PROTOTYPE_SLOT,
                      JS::PrivateValue(protoOrIfaceArray));
}

inline void
TraceProtoOrIfaceCache(JSTracer* trc, JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

  JSObject** protoOrIfaceArray = GetProtoOrIfaceArray(obj);
  for (size_t i = 0; i < kProtoOrIfaceCacheCount; ++i) {
    JSObject* proto = protoOrIfaceArray[i];
    if (proto) {
      JS_CALL_OBJECT_TRACER(trc, proto, "protoOrIfaceArray[i]");
    }
  }
}

inline void
DestroyProtoOrIfaceCache(JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

  JSObject** protoOrIfaceArray = GetProtoOrIfaceArray(obj);

  delete [] protoOrIfaceArray;
}

struct ConstantSpec
{
  const char* name;
  JS::Value value;
};




bool
DefineConstants(JSContext* cx, JSObject* obj, ConstantSpec* cs);






































JSObject*
CreateInterfaceObjects(JSContext* cx, JSObject* global, JSObject* receiver,
                       JSObject* protoProto, JSClass* protoClass,
                       JSClass* constructorClass, JSNative constructor,
                       unsigned ctorNargs, JSFunctionSpec* methods,
                       JSPropertySpec* properties, ConstantSpec* constants,
                       JSFunctionSpec* staticMethods, const char* name);

template <class T>
inline bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, T* value, JS::Value* vp)
{
  JSObject* obj = value->GetWrapper();
  if (obj && js::GetObjectCompartment(obj) == js::GetObjectCompartment(scope)) {
    *vp = JS::ObjectValue(*obj);
    return true;
  }

  if (!obj) {
    bool triedToWrap;
    obj = value->WrapObject(cx, scope, &triedToWrap);
    if (!obj) {
      
      
      
      
      return false;
    }
  }

  
  
  
  
  
  MOZ_ASSERT(js::IsObjectInContextCompartment(scope, cx));
  *vp = JS::ObjectValue(*obj);
  return JS_WrapValue(cx, vp);
}


template <template <class> class SmartPtr, class T>
inline bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, const SmartPtr<T>& value,
                     JS::Value* vp)
{
  return WrapNewBindingObject(cx, scope, value.get(), vp);
}





bool
DoHandleNewBindingWrappingFailure(JSContext* cx, JSObject* scope,
                                  nsISupports* value, JS::Value* vp);





template <class T>
bool
HandleNewBindingWrappingFailure(JSContext* cx, JSObject* scope, T* value,
                                JS::Value* vp)
{
  nsCOMPtr<nsISupports> val;
  CallQueryInterface(value, getter_AddRefs(val));
  return DoHandleNewBindingWrappingFailure(cx, scope, val, vp);
}


template <template <class> class SmartPtr, class T>
MOZ_ALWAYS_INLINE bool
HandleNewBindingWrappingFailure(JSContext* cx, JSObject* scope,
                                const SmartPtr<T>& value, JS::Value* vp)
{
  return HandleNewBindingWrappingFailure(cx, scope, value.get(), vp);
}

struct EnumEntry {
  const char* value;
  size_t length;
};

inline int
FindEnumStringIndex(JSContext* cx, JS::Value v, const EnumEntry* values, bool* ok)
{
  
  JSString* str = JS_ValueToString(cx, v);
  if (!str) {
    *ok = false;
    return 0;
  }
  JS::Anchor<JSString*> anchor(str);
  size_t length;
  const jschar* chars = JS_GetStringCharsAndLength(cx, str, &length);
  if (!chars) {
    *ok = false;
    return 0;
  }
  int i = 0;
  for (const EnumEntry* value = values; value->value; ++value, ++i) {
    if (length != value->length) {
      continue;
    }

    bool equal = true;
    const char* val = value->value;
    for (size_t j = 0; j != length; ++j) {
      if (unsigned(val[j]) != unsigned(chars[j])) {
        equal = false;
        break;
      }
    }

    if (equal) {
      *ok = true;
      return i;
    }
  }

  
  *ok = Throw<false>(cx, NS_ERROR_XPC_BAD_CONVERT_JS);
  return 0;
}

inline nsWrapperCache*
GetWrapperCache(nsWrapperCache* cache)
{
  return cache;
}

inline nsWrapperCache*
GetWrapperCache(nsGlobalWindow* not_allowed);

inline nsWrapperCache*
GetWrapperCache(void* p)
{
  return NULL;
}



bool
XPCOMObjectToJsval(JSContext* cx, JSObject* scope, xpcObjectHelper &helper,
                   const nsIID* iid, bool allowNativeWrapper, JS::Value* rval);

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, T* p, nsWrapperCache* cache,
           const nsIID* iid, JS::Value* vp)
{
  if (xpc_FastGetCachedWrapper(cache, scope, vp))
    return true;
  qsObjectHelper helper(p, cache);
  return XPCOMObjectToJsval(cx, scope, helper, iid, true, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, T* p, const nsIID* iid,
           JS::Value* vp)
{
  return WrapObject(cx, scope, p, GetWrapperCache(p), iid, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, T* p, JS::Value* vp)
{
  return WrapObject(cx, scope, p, NULL, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, nsCOMPtr<T> &p, const nsIID* iid,
           JS::Value* vp)
{
  return WrapObject(cx, scope, p.get(), iid, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, nsCOMPtr<T> &p, JS::Value* vp)
{
  return WrapObject(cx, scope, p, NULL, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, nsRefPtr<T> &p, const nsIID* iid,
           JS::Value* vp)
{
  return WrapObject(cx, scope, p.get(), iid, vp);
}

template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, nsRefPtr<T> &p, JS::Value* vp)
{
  return WrapObject(cx, scope, p, NULL, vp);
}

template<>
inline bool
WrapObject<JSObject>(JSContext* cx, JSObject* scope, JSObject* p, JS::Value* vp)
{
  vp->setObjectOrNull(p);
  return true;
}

template<class T>
static inline JSObject*
WrapNativeParent(JSContext* cx, JSObject* scope, T* p)
{
  if (!p)
    return scope;

  nsWrapperCache* cache = GetWrapperCache(p);
  JSObject* obj;
  if (cache && (obj = cache->GetWrapper())) {
#ifdef DEBUG
    qsObjectHelper helper(p, cache);
    JS::Value debugVal;

    bool ok = XPCOMObjectToJsval(cx, scope, helper, NULL, false, &debugVal);
    NS_ASSERTION(ok && JSVAL_TO_OBJECT(debugVal) == obj,
                 "Unexpected object in nsWrapperCache");
#endif
    return obj;
  }

  qsObjectHelper helper(p, cache);
  JS::Value v;
  return XPCOMObjectToJsval(cx, scope, helper, NULL, false, &v) ?
         JSVAL_TO_OBJECT(v) :
         NULL;
}


template <typename Spec>
static bool
InitIds(JSContext* cx, Spec* specs, jsid* ids)
{
  Spec* spec = specs;
  do {
    JSString *str = ::JS_InternString(cx, spec->name);
    if (!str) {
      return false;
    }

    *ids = INTERNED_STRING_TO_JSID(cx, str);
  } while (++ids, (++spec)->name);

  return true;
}

JSBool
QueryInterface(JSContext* cx, unsigned argc, JS::Value* vp);
JSBool
ThrowingConstructor(JSContext* cx, unsigned argc, JS::Value* vp);
JSBool
ThrowingConstructorWorkers(JSContext* cx, unsigned argc, JS::Value* vp);

} 
} 

#endif 
