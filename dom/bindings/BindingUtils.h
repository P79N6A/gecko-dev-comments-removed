





#ifndef mozilla_dom_BindingUtils_h__
#define mozilla_dom_BindingUtils_h__

#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/DOMJSProxyHandler.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/workers/Workers.h"
#include "mozilla/ErrorResult.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jswrapper.h"

#include "nsIXPConnect.h"
#include "qsObjectHelper.h"
#include "xpcpublic.h"
#include "nsTraceRefcnt.h"
#include "nsWrapperCacheInlines.h"
#include "mozilla/Likely.h"



class nsGlobalWindow;

namespace mozilla {
namespace dom {

enum ErrNum {
#define MSG_DEF(_name, _argc, _str) \
  _name,
#include "mozilla/dom/Errors.msg"
#undef MSG_DEF
  Err_Limit
};

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
ThrowMethodFailedWithDetails(JSContext* cx, const ErrorResult& rv,
                             const char* ,
                             const char* )
{
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




enum DOMObjectSlot {
  eNonDOMObject = -1,
  eRegularDOMObject = DOM_OBJECT_SLOT,
  eProxyDOMObject = DOM_PROXY_OBJECT_SLOT
};

template <class T>
inline T*
UnwrapDOMObject(JSObject* obj, DOMObjectSlot slot)
{
  MOZ_ASSERT(slot != eNonDOMObject,
             "Don't pass non-DOM objects to this function");

#ifdef DEBUG
  if (IsDOMClass(js::GetObjectClass(obj))) {
    MOZ_ASSERT(slot == eRegularDOMObject);
  } else {
    MOZ_ASSERT(IsDOMProxy(obj));
    MOZ_ASSERT(slot == eProxyDOMObject);
  }
#endif

  JS::Value val = js::GetReservedSlot(obj, slot);
  
  
  
  
  if (val.isUndefined()) {
    return NULL;
  }
  
  return static_cast<T*>(val.toPrivate());
}


inline const DOMClass*
GetDOMClass(JSObject* obj)
{
  js::Class* clasp = js::GetObjectClass(obj);
  if (IsDOMClass(clasp)) {
    return &DOMJSClass::FromJSClass(clasp)->mClass;
  }

  MOZ_ASSERT(IsDOMProxy(obj));
  js::BaseProxyHandler* handler = js::GetProxyHandler(obj);
  return &static_cast<DOMProxyHandler*>(handler)->mClass;
}

inline DOMObjectSlot
GetDOMClass(JSObject* obj, const DOMClass*& result)
{
  js::Class* clasp = js::GetObjectClass(obj);
  if (IsDOMClass(clasp)) {
    result = &DOMJSClass::FromJSClass(clasp)->mClass;
    return eRegularDOMObject;
  }

  if (js::IsObjectProxyClass(clasp) || js::IsFunctionProxyClass(clasp)) {
    js::BaseProxyHandler* handler = js::GetProxyHandler(obj);
    if (handler->family() == ProxyFamily()) {
      result = &static_cast<DOMProxyHandler*>(handler)->mClass;
      return eProxyDOMObject;
    }
  }

  return eNonDOMObject;
}

inline bool
UnwrapDOMObjectToISupports(JSObject* obj, nsISupports*& result)
{
  const DOMClass* clasp;
  DOMObjectSlot slot = GetDOMClass(obj, clasp);
  if (slot == eNonDOMObject || !clasp->mDOMObjectIsISupports) {
    return false;
  }
 
  result = UnwrapDOMObject<nsISupports>(obj, slot);
  return true;
}

inline bool
IsDOMObject(JSObject* obj)
{
  js::Class* clasp = js::GetObjectClass(obj);
  return IsDOMClass(clasp) ||
         ((js::IsObjectProxyClass(clasp) || js::IsFunctionProxyClass(clasp)) &&
          js::GetProxyHandler(obj)->family() == ProxyFamily());
}





template <prototypes::ID PrototypeID, class T, typename U>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, U& value)
{
  
  const DOMClass* domClass;
  DOMObjectSlot slot = GetDOMClass(obj, domClass);
  if (slot == eNonDOMObject) {
    
    if (!js::IsWrapper(obj)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    obj = xpc::Unwrap(cx, obj, false);
    if (!obj) {
      return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }
    MOZ_ASSERT(!js::IsWrapper(obj));
    slot = GetDOMClass(obj, domClass);
    if (slot == eNonDOMObject) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }
  }

  


  if (domClass->mInterfaceChain[PrototypeTraits<PrototypeID>::Depth] ==
      PrototypeID) {
    value = UnwrapDOMObject<T>(obj, slot);
    return NS_OK;
  }

  
  return NS_ERROR_XPC_BAD_CONVERT_JS;
}

inline bool
IsArrayLike(JSContext* cx, JSObject* obj)
{
  MOZ_ASSERT(obj);
  
  
  
  Maybe<JSAutoCompartment> ac;
  if (js::IsWrapper(obj)) {
    obj = xpc::Unwrap(cx, obj, false);
    if (!obj) {
      
      return false;
    }

    ac.construct(cx, obj);
  }

  
  
  return JS_IsArrayObject(cx, obj) || JS_IsTypedArrayObject(obj, cx);
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
    obj = xpc::Unwrap(cx, obj, false);
    if (!obj) {
      
      return false;
    }
    clasp = js::GetObjectJSClass(obj);
  }
  return IS_WRAPPER_CLASS(js::Valueify(clasp)) || IsDOMClass(clasp) ||
    JS_IsArrayBufferObject(obj, cx);
}


template <class T, typename U>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, U& value)
{
  return UnwrapObject<static_cast<prototypes::ID>(
           PrototypeIDMap<T>::PrototypeID), T>(cx, obj, value);
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

  if (!HasProtoOrIfaceArray(obj))
    return;
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

template<typename T>
struct Prefable {
  
  bool enabled;
  
  
  
  T* specs;
};

struct NativeProperties
{
  Prefable<JSFunctionSpec>* staticMethods;
  jsid* staticMethodIds;
  JSFunctionSpec* staticMethodsSpecs;
  Prefable<JSFunctionSpec>* methods;
  jsid* methodIds;
  JSFunctionSpec* methodsSpecs;
  Prefable<JSPropertySpec>* attributes;
  jsid* attributeIds;
  JSPropertySpec* attributeSpecs;
  Prefable<JSPropertySpec>* unforgeableAttributes;
  jsid* unforgeableAttributeIds;
  JSPropertySpec* unforgeableAttributeSpecs;
  Prefable<ConstantSpec>* constants;
  jsid* constantIds;
  ConstantSpec* constantSpecs;
};







































JSObject*
CreateInterfaceObjects(JSContext* cx, JSObject* global, JSObject* receiver,
                       JSObject* protoProto, JSClass* protoClass,
                       JSClass* constructorClass, JSNative constructor,
                       unsigned ctorNargs, const DOMClass* domClass,
                       const NativeProperties* properties,
                       const NativeProperties* chromeProperties,
                       const char* name);




bool
DefineUnforgeableAttributes(JSContext* cx, JSObject* obj,
                            Prefable<JSPropertySpec>* props);

inline bool
MaybeWrapValue(JSContext* cx, JSObject* obj, JS::Value* vp)
{
  if (vp->isObject() &&
      js::GetObjectCompartment(&vp->toObject()) != js::GetObjectCompartment(obj)) {
    return JS_WrapValue(cx, vp);
  }

  return true;
}

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


template <template <typename> class SmartPtr, class T>
inline bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, const SmartPtr<T>& value,
                     JS::Value* vp)
{
  return WrapNewBindingObject(cx, scope, value.get(), vp);
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
      scope = xpc::Unwrap(cx, scope, false);
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


template <template <typename> class SmartPtr, class T>
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

struct ParentObject {
  template<class T>
  ParentObject(T* aObject) :
    mObject(aObject),
    mWrapperCache(GetWrapperCache(aObject))
  {}

  template<class T, template<typename> class SmartPtr>
  ParentObject(const SmartPtr<T>& aObject) :
    mObject(aObject.get()),
    mWrapperCache(GetWrapperCache(aObject.get()))
  {}

  ParentObject(nsISupports* aObject, nsWrapperCache* aCache) :
    mObject(aObject),
    mWrapperCache(aCache)
  {}

  nsISupports* const mObject;
  nsWrapperCache* const mWrapperCache;
};

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

bool
WrapCallbackInterface(JSContext *cx, JSObject *scope, nsISupports* callback,
                      JS::Value* vp);

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
  typedef JSObject* (nsWrapperCache::*WrapObject)(JSContext*, JSObject*, bool*);
  template<typename U, U> struct SFINAE;
  template <typename V> static no& Check(SFINAE<WrapObject, &V::WrapObject>*);
  template <typename V> static yes& Check(...);

public:
  static bool const Value = HasWrapObjectMember<T>::Value &&
                            sizeof(Check<T>(nullptr)) == sizeof(yes);
};

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
    MOZ_NOT_REACHED("Don't know how to deal with triedToWrap == false for "
                    "non-nsISupports classes");
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

    bool triedToWrap;
    obj = parent->WrapObject(cx, scope, &triedToWrap);
    if (!triedToWrap) {
      obj = WrapNativeParentFallback<T>::Wrap(cx, scope, parent, cache);
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


template <class T>
inline bool
WrapNewBindingObject(JSContext* cx, JSObject* scope, OwningNonNull<T>& value,
                     JS::Value* vp)
{
  return WrapNewBindingObject(cx, scope, &static_cast<T&>(value), vp);
}



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


template<typename T>
class Optional {
public:
  Optional() {}

  bool WasPassed() const {
    return !mImpl.empty();
  }

  void Construct() {
    mImpl.construct();
  }

  template <class T1, class T2>
  void Construct(const T1 &t1, const T2 &t2) {
    mImpl.construct(t1, t2);
  }

  const T& Value() const {
    return mImpl.ref();
  }

  T& Value() {
    return mImpl.ref();
  }

private:
  
  Optional(const Optional& other) MOZ_DELETE;
  const Optional &operator=(const Optional &other) MOZ_DELETE;
  
  Maybe<T> mImpl;
};


template<>
class Optional<nsAString> {
public:
  Optional() : mPassed(false) {}

  bool WasPassed() const {
    return mPassed;
  }

  void operator=(const nsAString* str) {
    MOZ_ASSERT(str);
    mStr = str;
    mPassed = true;
  }

  void operator=(const FakeDependentString* str) {
    MOZ_ASSERT(str);
    mStr = str->ToAStringPtr();
    mPassed = true;
  }

  const nsAString& Value() const {
    MOZ_ASSERT(WasPassed());
    return *mStr;
  }

private:
  
  Optional(const Optional& other) MOZ_DELETE;
  const Optional &operator=(const Optional &other) MOZ_DELETE;
  
  bool mPassed;
  const nsAString* mStr;
};





template<typename T>
class Sequence : public AutoFallibleTArray<T, 16>
{
public:
  Sequence() : AutoFallibleTArray<T, 16>() {}
};



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


bool
XrayResolveProperty(JSContext* cx, JSObject* wrapper, jsid id,
                    JSPropertyDescriptor* desc,
                    const NativeProperties* nativeProperties,
                    const NativeProperties* chromeOnlyNativeProperties);

bool
XrayEnumerateProperties(JSObject* wrapper,
                        JS::AutoIdVector& props,
                        const NativeProperties* nativeProperties,
                        const NativeProperties* chromeOnlyNativeProperties);


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

struct MainThreadDictionaryBase
{
protected:
  JSContext* ParseJSON(const nsAString& aJSON,
                       mozilla::Maybe<JSAutoRequest>& aAr,
                       mozilla::Maybe<JSAutoCompartment>& aAc,
                       JS::Value& aVal);
};

} 
} 

#endif 
