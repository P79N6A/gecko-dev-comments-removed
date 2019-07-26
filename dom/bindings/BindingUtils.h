





#ifndef mozilla_dom_BindingUtils_h__
#define mozilla_dom_BindingUtils_h__

#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/DOMJSProxyHandler.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/workers/Workers.h"
#include "mozilla/ErrorResult.h"

#include "jsfriendapi.h"
#include "jswrapper.h"

#include "nsIXPConnect.h"
#include "qsObjectHelper.h"
#include "xpcpublic.h"
#include "nsTraceRefcnt.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/Likely.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/CallbackObject.h"

namespace mozilla {
namespace dom {

bool
ThrowErrorMessage(JSContext* aCx, const ErrNum aErrorNumber, ...);

template<bool mainThread>
inline bool
Throw(JSContext* cx, nsresult rv)
{
  using mozilla::dom::workers::exceptions::ThrowDOMExceptionForNSResult;

  
  if (mainThread) {
    xpc::Throw(cx, rv);
  } else {
    if (!JS_IsExceptionPending(cx)) {
      ThrowDOMExceptionForNSResult(cx, rv);
    }
  }
  return false;
}

template<bool mainThread>
inline bool
ThrowMethodFailedWithDetails(JSContext* cx, ErrorResult& rv,
                             const char* ,
                             const char* )
{
  if (rv.IsTypeError()) {
    rv.ReportTypeError(cx);
    return false;
  }
  if (rv.IsJSException()) {
    rv.ReportJSException(cx);
    return false;
  }
  return Throw<mainThread>(cx, rv.ErrorCode());
}


inline bool
IsDOMClass(const JSClass* clasp)
{
  return clasp->flags & JSCLASS_IS_DOMJSCLASS;
}

inline bool
IsDOMClass(const js::Class* clasp)
{
  return IsDOMClass(Jsvalify(clasp));
}



inline bool
IsDOMIfaceAndProtoClass(const JSClass* clasp)
{
  return clasp->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS;
}

inline bool
IsDOMIfaceAndProtoClass(const js::Class* clasp)
{
  return IsDOMIfaceAndProtoClass(Jsvalify(clasp));
}

MOZ_STATIC_ASSERT(DOM_OBJECT_SLOT == js::JSSLOT_PROXY_PRIVATE,
                  "JSSLOT_PROXY_PRIVATE doesn't match DOM_OBJECT_SLOT.  "
                  "Expect bad things");
template <class T>
inline T*
UnwrapDOMObject(JSObject* obj)
{
  MOZ_ASSERT(IsDOMClass(js::GetObjectClass(obj)) || IsDOMProxy(obj),
             "Don't pass non-DOM objects to this function");

  JS::Value val = js::GetReservedSlot(obj, DOM_OBJECT_SLOT);
  return static_cast<T*>(val.toPrivate());
}

inline const DOMClass*
GetDOMClass(JSObject* obj)
{
  js::Class* clasp = js::GetObjectClass(obj);
  if (IsDOMClass(clasp)) {
    return &DOMJSClass::FromJSClass(clasp)->mClass;
  }

  if (js::IsObjectProxyClass(clasp) || js::IsFunctionProxyClass(clasp)) {
    js::BaseProxyHandler* handler = js::GetProxyHandler(obj);
    if (handler->family() == ProxyFamily()) {
      return &static_cast<DOMProxyHandler*>(handler)->mClass;
    }
  }

  return nullptr;
}

inline bool
UnwrapDOMObjectToISupports(JSObject* obj, nsISupports*& result)
{
  const DOMClass* clasp = GetDOMClass(obj);
  if (!clasp || !clasp->mDOMObjectIsISupports) {
    return false;
  }
 
  result = UnwrapDOMObject<nsISupports>(obj);
  return true;
}

inline bool
IsDOMObject(JSObject* obj)
{
  js::Class* clasp = js::GetObjectClass(obj);
  return IsDOMClass(clasp) || IsDOMProxy(obj, clasp);
}





template <prototypes::ID PrototypeID, class T, typename U>
MOZ_ALWAYS_INLINE nsresult
UnwrapObject(JSContext* cx, JSObject* obj, U& value)
{
  
  const DOMClass* domClass = GetDOMClass(obj);
  if (!domClass) {
    
    if (!js::IsWrapper(obj)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    obj = js::UnwrapObjectChecked(obj,  false);
    if (!obj) {
      return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }
    MOZ_ASSERT(!js::IsWrapper(obj));
    domClass = GetDOMClass(obj);
    if (!domClass) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }
  }

  


  if (domClass->mInterfaceChain[PrototypeTraits<PrototypeID>::Depth] ==
      PrototypeID) {
    value = UnwrapDOMObject<T>(obj);
    return NS_OK;
  }

  
  return NS_ERROR_XPC_BAD_CONVERT_JS;
}

inline bool
IsNotDateOrRegExp(JSContext* cx, JSObject* obj)
{
  MOZ_ASSERT(obj);
  
  
  
  Maybe<JSAutoCompartment> ac;
  if (js::IsWrapper(obj)) {
    obj = js::UnwrapObjectChecked(obj,  false);
    if (!obj) {
      
      
      
      
      
      
      
      
      
      
      return true;
    }

    ac.construct(cx, obj);
  }

  
  return !JS_ObjectIsDate(cx, obj) && !JS_ObjectIsRegExp(cx, obj);
}

MOZ_ALWAYS_INLINE bool
IsArrayLike(JSContext* cx, JSObject* obj)
{
  return IsNotDateOrRegExp(cx, obj);
}

MOZ_ALWAYS_INLINE bool
IsConvertibleToDictionary(JSContext* cx, JSObject* obj)
{
  return IsNotDateOrRegExp(cx, obj);
}

MOZ_ALWAYS_INLINE bool
IsConvertibleToDictionary(JSContext* cx, JS::Value val)
{
  return val.isNullOrUndefined() ||
    (val.isObject() && IsConvertibleToDictionary(cx, &val.toObject()));
}

MOZ_ALWAYS_INLINE bool
IsConvertibleToCallbackInterface(JSContext* cx, JSObject* obj)
{
  return IsNotDateOrRegExp(cx, obj);
}


template <class T, typename U>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, U& value)
{
  return UnwrapObject<static_cast<prototypes::ID>(
           PrototypeIDMap<T>::PrototypeID), T>(cx, obj, value);
}




MOZ_STATIC_ASSERT((size_t)constructors::id::_ID_Start ==
                  (size_t)prototypes::id::_ID_Count,
                  "Overlapping or discontiguous indexes.");
const size_t kProtoAndIfaceCacheCount = constructors::id::_ID_Count;

inline void
AllocateProtoAndIfaceCache(JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);
  MOZ_ASSERT(js::GetReservedSlot(obj, DOM_PROTOTYPE_SLOT).isUndefined());

  
  JSObject** protoAndIfaceArray = new JSObject*[kProtoAndIfaceCacheCount]();

  js::SetReservedSlot(obj, DOM_PROTOTYPE_SLOT,
                      JS::PrivateValue(protoAndIfaceArray));
}

inline void
TraceProtoAndIfaceCache(JSTracer* trc, JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

  if (!HasProtoAndIfaceArray(obj))
    return;
  JSObject** protoAndIfaceArray = GetProtoAndIfaceArray(obj);
  for (size_t i = 0; i < kProtoAndIfaceCacheCount; ++i) {
    JSObject* proto = protoAndIfaceArray[i];
    if (proto) {
      JS_CallObjectTracer(trc, proto, "protoAndIfaceArray[i]");
    }
  }
}

inline void
DestroyProtoAndIfaceCache(JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

  JSObject** protoAndIfaceArray = GetProtoAndIfaceArray(obj);

  delete [] protoAndIfaceArray;
}




bool
DefineConstants(JSContext* cx, JSObject* obj, ConstantSpec* cs);

struct JSNativeHolder
{
  JSNative mNative;
  const NativePropertyHooks* mPropertyHooks;
};

struct NamedConstructor
{
  const char* mName;
  const JSNativeHolder mHolder;
  unsigned mNargs;
};







































void
CreateInterfaceObjects(JSContext* cx, JSObject* global, JSObject* protoProto,
                       JSClass* protoClass, JSObject** protoCache,
                       JSClass* constructorClass, const JSNativeHolder* constructor,
                       unsigned ctorNargs, const NamedConstructor* namedConstructors,
                       JSObject** constructorCache, const DOMClass* domClass,
                       const NativeProperties* regularProperties,
                       const NativeProperties* chromeOnlyProperties,
                       const char* name);




bool
DefineUnforgeableAttributes(JSContext* cx, JSObject* obj,
                            Prefable<JSPropertySpec>* props);

bool
DefineWebIDLBindingPropertiesOnXPCProto(JSContext* cx, JSObject* proto, const NativeProperties* properties);

#ifdef _MSC_VER
#define HAS_MEMBER_CHECK(_name)                                           \
  template<typename V> static yes& Check(char (*)[(&V::_name == 0) + 1])
#else
#define HAS_MEMBER_CHECK(_name)                                           \
  template<typename V> static yes& Check(char (*)[sizeof(&V::_name) + 1])
#endif

#define HAS_MEMBER(_name)                                                 \
template<typename T>                                                      \
class Has##_name##Member {                                                \
  typedef char yes[1];                                                    \
  typedef char no[2];                                                     \
  HAS_MEMBER_CHECK(_name);                                                \
  template<typename V> static no& Check(...);                             \
                                                                          \
public:                                                                   \
  static bool const Value = sizeof(Check<T>(nullptr)) == sizeof(yes);     \
};

HAS_MEMBER(AddRef)
HAS_MEMBER(Release)
HAS_MEMBER(QueryInterface)

template<typename T>
struct IsRefCounted
{
  static bool const Value = HasAddRefMember<T>::Value &&
                            HasReleaseMember<T>::Value;
};

template<typename T>
struct IsISupports
{
  static bool const Value = IsRefCounted<T>::Value &&
                            HasQueryInterfaceMember<T>::Value;
};

HAS_MEMBER(WrapObject)



template<typename T>
struct HasWrapObject
{
private:
  typedef char yes[1];
  typedef char no[2];
  typedef JSObject* (nsWrapperCache::*WrapObject)(JSContext*, JSObject*);
  template<typename U, U> struct SFINAE;
  template <typename V> static no& Check(SFINAE<WrapObject, &V::WrapObject>*);
  template <typename V> static yes& Check(...);

public:
  static bool const Value = HasWrapObjectMember<T>::Value &&
                            sizeof(Check<T>(nullptr)) == sizeof(yes);
};

#ifdef DEBUG
template <class T, bool isISupports=IsISupports<T>::Value>
struct
CheckWrapperCacheCast
{
  static bool Check()
  {
    return reinterpret_cast<uintptr_t>(
      static_cast<nsWrapperCache*>(
        reinterpret_cast<T*>(1))) == 1;
  }
};
template <class T>
struct
CheckWrapperCacheCast<T, true>
{
  static bool Check()
  {
    return true;
  }
};
#endif

MOZ_ALWAYS_INLINE bool
CouldBeDOMBinding(void*)
{
  return true;
}

MOZ_ALWAYS_INLINE bool
CouldBeDOMBinding(nsWrapperCache* aCache)
{
  return aCache->IsDOMBinding();
}





inline const JS::Value&
GetSystemOnlyWrapperSlot(JSObject* obj)
{
  MOZ_ASSERT(IsDOMClass(js::GetObjectJSClass(obj)) &&
             !(js::GetObjectJSClass(obj)->flags & JSCLASS_DOM_GLOBAL));
  return js::GetReservedSlot(obj, DOM_OBJECT_SLOT_SOW);
}
inline void
SetSystemOnlyWrapperSlot(JSObject* obj, const JS::Value& v)
{
  MOZ_ASSERT(IsDOMClass(js::GetObjectJSClass(obj)) &&
             !(js::GetObjectJSClass(obj)->flags & JSCLASS_DOM_GLOBAL));
  js::SetReservedSlot(obj, DOM_OBJECT_SLOT_SOW, v);
}

inline bool
GetSameCompartmentWrapperForDOMBinding(JSObject*& obj)
{
  js::Class* clasp = js::GetObjectClass(obj);
  if (dom::IsDOMClass(clasp)) {
    if (!(clasp->flags & JSCLASS_DOM_GLOBAL)) {
      JS::Value v = GetSystemOnlyWrapperSlot(obj);
      if (v.isObject()) {
        obj = &v.toObject();
      }
    }
    return true;
  }
  return IsDOMProxy(obj, clasp);
}

inline void
SetSystemOnlyWrapper(JSObject* obj, nsWrapperCache* cache, JSObject& wrapper)
{
  SetSystemOnlyWrapperSlot(obj, JS::ObjectValue(wrapper));
  cache->SetHasSystemOnlyWrapper();
}




MOZ_ALWAYS_INLINE bool
MaybeWrapValue(JSContext* cx, JS::Value* vp)
{
  if (vp->isString()) {
    JSString* str = vp->toString();
    if (JS::GetGCThingZone(str) != js::GetContextZone(cx)) {
      return JS_WrapValue(cx, vp);
    }
    return true;
  }

  if (vp->isObject()) {
    JSObject* obj = &vp->toObject();
    if (js::GetObjectCompartment(obj) != js::GetContextCompartment(cx)) {
      return JS_WrapValue(cx, vp);
    }

    
    
    if (GetSameCompartmentWrapperForDOMBinding(obj)) {
      
      *vp = JS::ObjectValue(*obj);
      return true;
    }

    if (!IS_SLIM_WRAPPER(obj)) {
      
      return JS_WrapValue(cx, vp);
    }

    
  }

  return true;
}

static inline void
WrapNewBindingForSameCompartment(JSContext* cx, JSObject* obj, void* value,
                                 JS::Value* vp)
{
  *vp = JS::ObjectValue(*obj);
}

static inline void
WrapNewBindingForSameCompartment(JSContext* cx, JSObject* obj,
                                 nsWrapperCache* value, JS::Value* vp)
{
  if (value->HasSystemOnlyWrapper()) {
    *vp = GetSystemOnlyWrapperSlot(obj);
    MOZ_ASSERT(vp->isObject());
  } else {
    *vp = JS::ObjectValue(*obj);
  }
}






template <class T>
MOZ_ALWAYS_INLINE bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, T* value, JS::Value* vp)
{
  JSObject* obj = value->GetWrapperPreserveColor();
  bool couldBeDOMBinding = CouldBeDOMBinding(value);
  if (obj) {
    xpc_UnmarkNonNullGrayObject(obj);
  } else {
    
    if (!couldBeDOMBinding) {
      return false;
    }

    obj = value->WrapObject(cx, scope);
    if (!obj) {
      
      
      
      return false;
    }
  }

#ifdef DEBUG
  const DOMClass* clasp = GetDOMClass(obj);
  
  
  if (clasp) {
    
    
    
    
    
    
    MOZ_ASSERT(clasp, "What happened here?");
    MOZ_ASSERT_IF(clasp->mDOMObjectIsISupports, IsISupports<T>::Value);
    MOZ_ASSERT(CheckWrapperCacheCast<T>::Check());
  }

  
  
  
  
  
  MOZ_ASSERT(js::IsObjectInContextCompartment(scope, cx));
#endif

  bool sameCompartment =
    js::GetObjectCompartment(obj) == js::GetContextCompartment(cx);
  if (sameCompartment && couldBeDOMBinding) {
    WrapNewBindingForSameCompartment(cx, obj, value, vp);
    return true;
  }

  *vp = JS::ObjectValue(*obj);
  return (sameCompartment && IS_SLIM_WRAPPER(obj)) || JS_WrapValue(cx, vp);
}




template <class T>
inline bool
WrapNewBindingNonWrapperCachedObject(JSContext* cx, JSObject* scope, T* value,
                                     JS::Value* vp)
{
  
  JSObject* obj;
  {
    
    
    Maybe<JSAutoCompartment> ac;
    if (js::IsWrapper(scope)) {
      scope = js::UnwrapObjectChecked(scope,  false);
      if (!scope)
        return false;
      ac.construct(cx, scope);
    }

    obj = value->WrapObject(cx, scope);
  }

  
  
  *vp = JS::ObjectValue(*obj);
  return JS_WrapValue(cx, vp);
}


template <template <typename> class SmartPtr, typename T>
inline bool
WrapNewBindingNonWrapperCachedObject(JSContext* cx, JSObject* scope,
                                     const SmartPtr<T>& value, JS::Value* vp)
{
  return WrapNewBindingNonWrapperCachedObject(cx, scope, value.get(), vp);
}



bool
NativeInterface2JSObjectAndThrowIfFailed(JSContext* aCx,
                                         JSObject* aScope,
                                         JS::Value* aRetval,
                                         xpcObjectHelper& aHelper,
                                         const nsIID* aIID,
                                         bool aAllowNativeWrapper);





template <class T>
MOZ_ALWAYS_INLINE bool
HandleNewBindingWrappingFailure(JSContext* cx, JSObject* scope, T* value,
                                JS::Value* vp)
{
  if (JS_IsExceptionPending(cx)) {
    return false;
  }

  qsObjectHelper helper(value, GetWrapperCache(value));
  return NativeInterface2JSObjectAndThrowIfFailed(cx, scope, vp, helper,
                                                  nullptr, true);
}



HAS_MEMBER(get)

template <class T, bool isSmartPtr=HasgetMember<T>::Value>
struct HandleNewBindingWrappingFailureHelper
{
  static inline bool Wrap(JSContext* cx, JSObject* scope, const T& value,
                          JS::Value* vp)
  {
    return HandleNewBindingWrappingFailure(cx, scope, value.get(), vp);
  }
};

template <class T>
struct HandleNewBindingWrappingFailureHelper<T, false>
{
  static inline bool Wrap(JSContext* cx, JSObject* scope, T& value,
                          JS::Value* vp)
  {
    return HandleNewBindingWrappingFailure(cx, scope, &value, vp);
  }
};

template<class T>
inline bool
HandleNewBindingWrappingFailure(JSContext* cx, JSObject* scope, T& value,
                                JS::Value* vp)
{
  return HandleNewBindingWrappingFailureHelper<T>::Wrap(cx, scope, value, vp);
}

template<bool Fatal>
inline bool
EnumValueNotFound(JSContext* cx, const jschar* chars, size_t length,
                  const char* type)
{
  return false;
}

template<>
inline bool
EnumValueNotFound<false>(JSContext* cx, const jschar* chars, size_t length,
                         const char* type)
{
  
  return true;
}

template<>
inline bool
EnumValueNotFound<true>(JSContext* cx, const jschar* chars, size_t length,
                        const char* type)
{
  NS_LossyConvertUTF16toASCII deflated(static_cast<const PRUnichar*>(chars),
                                       length);
  return ThrowErrorMessage(cx, MSG_INVALID_ENUM_VALUE, deflated.get(), type);
}


template<bool InvalidValueFatal>
inline int
FindEnumStringIndex(JSContext* cx, JS::Value v, const EnumEntry* values,
                    const char* type, bool* ok)
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

  *ok = EnumValueNotFound<InvalidValueFatal>(cx, chars, length, type);
  return -1;
}

inline nsWrapperCache*
GetWrapperCache(const ParentObject& aParentObject)
{
  return aParentObject.mWrapperCache;
}

template<class T>
inline T*
GetParentPointer(T* aObject)
{
  return aObject;
}

inline nsISupports*
GetParentPointer(const ParentObject& aObject)
{
  return aObject.mObject;
}

template<class T>
inline void
ClearWrapper(T* p, nsWrapperCache* cache)
{
  cache->ClearWrapper();
}

template<class T>
inline void
ClearWrapper(T* p, void*)
{
  nsWrapperCache* cache;
  CallQueryInterface(p, &cache);
  ClearWrapper(p, cache);
}









bool
TryPreserveWrapper(JSObject* obj);



JSBool
InstanceClassHasProtoAtDepth(JSHandleObject protoObject, uint32_t protoID,
                             uint32_t depth);



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
WrapObject(JSContext* cx, JSObject* scope, const nsCOMPtr<T> &p, const nsIID* iid,
           JS::Value* vp)
{
  return WrapObject(cx, scope, p.get(), iid, vp);
}


template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, const nsCOMPtr<T> &p, JS::Value* vp)
{
  return WrapObject(cx, scope, p, NULL, vp);
}


template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, const nsRefPtr<T> &p, const nsIID* iid,
           JS::Value* vp)
{
  return WrapObject(cx, scope, p.get(), iid, vp);
}


template<class T>
inline bool
WrapObject(JSContext* cx, JSObject* scope, const nsRefPtr<T> &p, JS::Value* vp)
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

inline bool
WrapObject(JSContext* cx, JSObject* scope, JSObject& p, JS::Value* vp)
{
  vp->setObject(p);
  return true;
}

bool
WrapCallbackInterface(JSContext *cx, JSObject *scope, nsISupports* callback,
                      JS::Value* vp);

static inline bool
WrapCallbackInterface(JSContext *cx, JSObject *scope, nsISupports& callback,
                      JS::Value* vp)
{
  return WrapCallbackInterface(cx, scope, &callback, vp);
}


template <template <typename> class SmartPtr, class T>
inline bool
WrapCallbackInterface(JSContext* cx, JSObject* scope, const SmartPtr<T>& value,
                      JS::Value* vp)
{
  return WrapCallbackInterface(cx, scope, value.get(), vp);
}





template<typename T>
static inline JSObject*
WrapNativeISupportsParent(JSContext* cx, JSObject* scope, T* p,
                          nsWrapperCache* cache)
{
  qsObjectHelper helper(ToSupports(p), cache);
  JS::Value v;
  return XPCOMObjectToJsval(cx, scope, helper, nullptr, false, &v) ?
         JSVAL_TO_OBJECT(v) :
         nullptr;
}



template<typename T, bool isISupports=IsISupports<T>::Value >
struct WrapNativeParentFallback
{
  static inline JSObject* Wrap(JSContext* cx, JSObject* scope, T* parent,
                               nsWrapperCache* cache)
  {
    return nullptr;
  }
};



template<typename T >
struct WrapNativeParentFallback<T, true >
{
  static inline JSObject* Wrap(JSContext* cx, JSObject* scope, T* parent,
                               nsWrapperCache* cache)
  {
    return WrapNativeISupportsParent(cx, scope, parent, cache);
  }
};



template<typename T, bool hasWrapObject=HasWrapObject<T>::Value >
struct WrapNativeParentHelper
{
  static inline JSObject* Wrap(JSContext* cx, JSObject* scope, T* parent,
                               nsWrapperCache* cache)
  {
    MOZ_ASSERT(cache);

    JSObject* obj;
    if ((obj = cache->GetWrapper())) {
      return obj;
    }

    
    if (!CouldBeDOMBinding(parent)) {
      obj = WrapNativeParentFallback<T>::Wrap(cx, scope, parent, cache);
    } else {
      obj = parent->WrapObject(cx, scope);
    }

    return obj;
  }
};



template<typename T>
struct WrapNativeParentHelper<T, false >
{
  static inline JSObject* Wrap(JSContext* cx, JSObject* scope, T* parent,
                               nsWrapperCache* cache)
  {
    JSObject* obj;
    if (cache && (obj = cache->GetWrapper())) {
#ifdef DEBUG
      NS_ASSERTION(WrapNativeISupportsParent(cx, scope, parent, cache) == obj,
                   "Unexpected object in nsWrapperCache");
#endif
      return obj;
    }

    return WrapNativeISupportsParent(cx, scope, parent, cache);
  }
};


template<typename T>
static inline JSObject*
WrapNativeParent(JSContext* cx, JSObject* scope, T* p, nsWrapperCache* cache)
{
  if (!p) {
    return scope;
  }

  return WrapNativeParentHelper<T>::Wrap(cx, scope, p, cache);
}



template<typename T>
static inline JSObject*
WrapNativeParent(JSContext* cx, JSObject* scope, const T& p)
{
  return WrapNativeParent(cx, scope, GetParentPointer(p), GetWrapperCache(p));
}

HAS_MEMBER(GetParentObject)

template<typename T, bool WrapperCached=HasGetParentObjectMember<T>::Value>
struct GetParentObject
{
  static JSObject* Get(JSContext* cx, JSObject* obj)
  {
    T* native = UnwrapDOMObject<T>(obj);
    return WrapNativeParent(cx, obj, native->GetParentObject());
  }
};

template<typename T>
struct GetParentObject<T, false>
{
  static JSObject* Get(JSContext* cx, JSObject* obj)
  {
    MOZ_CRASH();
    return nullptr;
  }
};

MOZ_ALWAYS_INLINE
JSObject* GetJSObjectFromCallback(CallbackObject* callback)
{
  return callback->Callback();
}

MOZ_ALWAYS_INLINE
JSObject* GetJSObjectFromCallback(void* noncallback)
{
  return nullptr;
}

template<typename T>
static inline JSObject*
WrapCallThisObject(JSContext* cx, JSObject* scope, const T& p)
{
  
  
  
  JSObject* obj = GetJSObjectFromCallback(p);
  if (!obj) {
    
    
    obj = WrapNativeParent(cx, scope, p);
    if (!obj) {
      return nullptr;
    }
  }

  
  if (!JS_WrapObject(cx, &obj)) {
    return nullptr;
  }

  return obj;
}



template <class T, bool isSmartPtr=HasgetMember<T>::Value>
struct WrapNewBindingObjectHelper
{
  static inline bool Wrap(JSContext* cx, JSObject* scope, const T& value,
                          JS::Value* vp)
  {
    return WrapNewBindingObject(cx, scope, value.get(), vp);
  }
};

template <class T>
struct WrapNewBindingObjectHelper<T, false>
{
  static inline bool Wrap(JSContext* cx, JSObject* scope, T& value,
                          JS::Value* vp)
  {
    return WrapNewBindingObject(cx, scope, &value, vp);
  }
};

template<class T>
inline bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, T& value,
                     JS::Value* vp)
{
  return WrapNewBindingObjectHelper<T>::Wrap(cx, scope, value, vp);
}

template <class T>
inline JSObject*
GetCallbackFromCallbackObject(T* aObj)
{
  return aObj->Callback();
}




template <class T, bool isSmartPtr=HasgetMember<T>::Value>
struct GetCallbackFromCallbackObjectHelper
{
  static inline JSObject* Get(const T& aObj)
  {
    return GetCallbackFromCallbackObject(aObj.get());
  }
};

template <class T>
struct GetCallbackFromCallbackObjectHelper<T, false>
{
  static inline JSObject* Get(T& aObj)
  {
    return GetCallbackFromCallbackObject(&aObj);
  }
};

template<class T>
inline JSObject*
GetCallbackFromCallbackObject(T& aObj)
{
  return GetCallbackFromCallbackObjectHelper<T>::Get(aObj);
}

static inline bool
InternJSString(JSContext* cx, jsid& id, const char* chars)
{
  if (JSString *str = ::JS_InternString(cx, chars)) {
    id = INTERNED_STRING_TO_JSID(cx, str);
    return true;
  }
  return false;
}


template <typename Spec>
static bool
InitIds(JSContext* cx, Prefable<Spec>* prefableSpecs, jsid* ids)
{
  MOZ_ASSERT(prefableSpecs);
  MOZ_ASSERT(prefableSpecs->specs);
  do {
    
    
    Spec* spec = prefableSpecs->specs;
    do {
      if (!InternJSString(cx, *ids, spec->name)) {
        return false;
      }
    } while (++ids, (++spec)->name);

    
    
    *ids = JSID_VOID;
    ++ids;
  } while ((++prefableSpecs)->specs);

  return true;
}

JSBool
QueryInterface(JSContext* cx, unsigned argc, JS::Value* vp);
JSBool
ThrowingConstructor(JSContext* cx, unsigned argc, JS::Value* vp);

bool
GetPropertyOnPrototype(JSContext* cx, JSObject* proxy, jsid id, bool* found,
                       JS::Value* vp);

bool
HasPropertyOnPrototype(JSContext* cx, JSObject* proxy, DOMProxyHandler* handler,
                       jsid id);

template<class T>
class NonNull
{
public:
  NonNull()
#ifdef DEBUG
    : inited(false)
#endif
  {}

  operator T&() {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return *ptr;
  }

  operator const T&() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return *ptr;
  }

  void operator=(T* t) {
    ptr = t;
    MOZ_ASSERT(ptr);
#ifdef DEBUG
    inited = true;
#endif
  }

  template<typename U>
  void operator=(U* t) {
    ptr = t->ToAStringPtr();
    MOZ_ASSERT(ptr);
#ifdef DEBUG
    inited = true;
#endif
  }

  T** Slot() {
#ifdef DEBUG
    inited = true;
#endif
    return &ptr;
  }

  T* Ptr() {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "NonNull<T> was set to null");
    return ptr;
  }

  
  T* get() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr);
    return ptr;
  }

protected:
  T* ptr;
#ifdef DEBUG
  bool inited;
#endif
};

template<class T>
class OwningNonNull
{
public:
  OwningNonNull()
#ifdef DEBUG
    : inited(false)
#endif
  {}

  operator T&() {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr, "OwningNonNull<T> was set to null");
    return *ptr;
  }

  void operator=(T* t) {
    init(t);
  }

  void operator=(const already_AddRefed<T>& t) {
    init(t);
  }

  already_AddRefed<T> forget() {
#ifdef DEBUG
    inited = false;
#endif
    return ptr.forget();
  }

  
  T* get() const {
    MOZ_ASSERT(inited);
    MOZ_ASSERT(ptr);
    return ptr;
  }

protected:
  template<typename U>
  void init(U t) {
    ptr = t;
    MOZ_ASSERT(ptr);
#ifdef DEBUG
    inited = true;
#endif
  }

  nsRefPtr<T> ptr;
#ifdef DEBUG
  bool inited;
#endif
};

class NonNullLazyRootedObject : public Maybe<JS::RootedObject>
{
public:
  operator JSObject&() const {
    MOZ_ASSERT(!empty() && ref(), "Can not alias null.");
    return *ref();
  }
};

class LazyRootedObject : public Maybe<JS::RootedObject>
{
public:
  operator JSObject*() const {
    return empty() ? (JSObject*) nullptr : ref();
  }
};



struct FakeDependentString {
  FakeDependentString() :
    mFlags(nsDependentString::F_TERMINATED)
  {
  }

  void SetData(const nsDependentString::char_type* aData,
               nsDependentString::size_type aLength) {
    MOZ_ASSERT(mFlags == nsDependentString::F_TERMINATED);
    mData = aData;
    mLength = aLength;
  }

  void Truncate() {
    mData = nsDependentString::char_traits::sEmptyBuffer;
    mLength = 0;
  }

  void SetNull() {
    Truncate();
    mFlags |= nsDependentString::F_VOIDED;
  }

  
  
  const nsAString* ToAStringPtr() const {
    return reinterpret_cast<const nsDependentString*>(this);
  }

  nsAString* ToAStringPtr() {
    return reinterpret_cast<nsDependentString*>(this);
  }

  operator const nsAString& () const {
    return *reinterpret_cast<const nsDependentString*>(this);
  }

private:
  const nsDependentString::char_type* mData;
  nsDependentString::size_type mLength;
  uint32_t mFlags;

  
  
  class DependentStringAsserter;
  friend class DependentStringAsserter;

  class DepedentStringAsserter : public nsDependentString {
  public:
    static void StaticAsserts() {
      MOZ_STATIC_ASSERT(sizeof(FakeDependentString) == sizeof(nsDependentString),
                        "Must have right object size");
      MOZ_STATIC_ASSERT(offsetof(FakeDependentString, mData) ==
                          offsetof(DepedentStringAsserter, mData),
                        "Offset of mData should match");
      MOZ_STATIC_ASSERT(offsetof(FakeDependentString, mLength) ==
                          offsetof(DepedentStringAsserter, mLength),
                        "Offset of mLength should match");
      MOZ_STATIC_ASSERT(offsetof(FakeDependentString, mFlags) ==
                          offsetof(DepedentStringAsserter, mFlags),
                        "Offset of mFlags should match");
    }
  };
};

enum StringificationBehavior {
  eStringify,
  eEmpty,
  eNull
};


static inline bool
ConvertJSValueToString(JSContext* cx, const JS::Value& v, JS::Value* pval,
                       StringificationBehavior nullBehavior,
                       StringificationBehavior undefinedBehavior,
                       FakeDependentString& result)
{
  JSString *s;
  if (v.isString()) {
    s = v.toString();
  } else {
    StringificationBehavior behavior;
    if (v.isNull()) {
      behavior = nullBehavior;
    } else if (v.isUndefined()) {
      behavior = undefinedBehavior;
    } else {
      behavior = eStringify;
    }

    if (behavior != eStringify) {
      if (behavior == eEmpty) {
        result.Truncate();
      } else {
        result.SetNull();
      }
      return true;
    }

    s = JS_ValueToString(cx, v);
    if (!s) {
      return false;
    }
    pval->setString(s);  
  }

  size_t len;
  const jschar *chars = JS_GetStringCharsZAndLength(cx, s, &len);
  if (!chars) {
    return false;
  }

  result.SetData(chars, len);
  return true;
}



template<class T>
class UnionMember {
    AlignedStorage2<T> storage;

public:
    T& SetValue() {
      new (storage.addr()) T();
      return *storage.addr();
    }
    const T& Value() const {
      return *storage.addr();
    }
    void Destroy() {
      storage.addr()->~T();
    }
};


template<typename T>
class AutoSequence : public AutoFallibleTArray<T, 16>
{
public:
  AutoSequence() : AutoFallibleTArray<T, 16>()
  {}

  
  operator const Sequence<T>&() const {
    return *reinterpret_cast<const Sequence<T>*>(this);
  }
};

inline bool
IdEquals(jsid id, const char* string)
{
  return JSID_IS_STRING(id) &&
         JS_FlatStringEqualsAscii(JSID_TO_FLAT_STRING(id), string);
}

inline bool
AddStringToIDVector(JSContext* cx, JS::AutoIdVector& vector, const char* name)
{
  return vector.growBy(1) &&
         InternJSString(cx, vector[vector.length() - 1], name);
}










bool
XrayResolveOwnProperty(JSContext* cx, JSObject* wrapper, JSObject* obj,
                       jsid id, JSPropertyDescriptor* desc, unsigned flags);








bool
XrayResolveNativeProperty(JSContext* cx, JSObject* wrapper, JSObject* obj,
                          jsid id, JSPropertyDescriptor* desc);










bool
XrayEnumerateProperties(JSContext* cx, JSObject* wrapper, JSObject* obj,
                        unsigned flags, JS::AutoIdVector& props);

extern NativePropertyHooks sWorkerNativePropertyHooks;











enum {
  CONSTRUCTOR_NATIVE_HOLDER_RESERVED_SLOT = 0,
  CONSTRUCTOR_XRAY_EXPANDO_SLOT
};

JSBool
Constructor(JSContext* cx, unsigned argc, JS::Value* vp);

inline bool
UseDOMXray(JSObject* obj)
{
  const js::Class* clasp = js::GetObjectClass(obj);
  return IsDOMClass(clasp) ||
         IsDOMProxy(obj, clasp) ||
         JS_IsNativeFunction(obj, Constructor) ||
         IsDOMIfaceAndProtoClass(clasp);
}

#ifdef DEBUG
inline bool
HasConstructor(JSObject* obj)
{
  return JS_IsNativeFunction(obj, Constructor) ||
         js::GetObjectClass(obj)->construct;
}
 #endif
 

template<class T>
inline void
Take(nsRefPtr<T>& smartPtr, T* ptr)
{
  smartPtr = dont_AddRef(ptr);
}


template<class T>
inline void
Take(nsAutoPtr<T>& smartPtr, T* ptr)
{
  smartPtr = ptr;
}

inline void
MustInheritFromNonRefcountedDOMObject(NonRefcountedDOMObject*)
{
}



JSObject* GetXrayExpandoChain(JSObject *obj);
void SetXrayExpandoChain(JSObject *obj, JSObject *chain);















bool
NativeToString(JSContext* cx, JSObject* wrapper, JSObject* obj, const char* pre,
               const char* post, JS::Value* v);

HAS_MEMBER(JSBindingFinalized)

template<class T, bool hasCallback=HasJSBindingFinalizedMember<T>::Value>
struct JSBindingFinalized
{
  static void Finalized(T* self)
  {
  }
};

template<class T>
struct JSBindingFinalized<T, true>
{
  static void Finalized(T* self)
  {
    self->JSBindingFinalized();
  }
};

nsresult
ReparentWrapper(JSContext* aCx, JSObject* aObj);






JSBool
InterfaceHasInstance(JSContext* cx, JSHandleObject obj, JSObject* instance,
                     JSBool* bp);
JSBool
InterfaceHasInstance(JSContext* cx, JSHandleObject obj, JSMutableHandleValue vp,
                     JSBool* bp);

} 
} 

#endif 
