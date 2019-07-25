





#ifndef mozilla_dom_BindingUtils_h__
#define mozilla_dom_BindingUtils_h__

#include "mozilla/dom/DOMJSClass.h"
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

template <class T>
inline T*
UnwrapDOMObject(JSObject* obj)
{
  MOZ_ASSERT(IsDOMClass(JS_GetClass(obj)));

  JS::Value val = js::GetReservedSlot(obj, DOM_OBJECT_SLOT);
  
  
  
  
  if (val.isUndefined()) {
    return NULL;
  }
  
  return static_cast<T*>(val.toPrivate());
}





template <prototypes::ID PrototypeID, class T, typename U>
inline nsresult
UnwrapObject(JSContext* cx, JSObject* obj, U& value)
{
  
  JSClass* clasp = js::GetObjectJSClass(obj);
  if (!IsDOMClass(clasp)) {
    
    if (!js::IsWrapper(obj)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    obj = xpc::Unwrap(cx, obj, false);
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
    value = UnwrapDOMObject<T>(obj);
    return NS_OK;
  }

  
  return NS_ERROR_XPC_BAD_CONVERT_JS;
}

inline bool
IsArrayLike(JSContext* cx, JSObject* obj)
{
  MOZ_ASSERT(obj);
  
  
  
  JSAutoEnterCompartment ac;
  if (js::IsWrapper(obj)) {
    obj = xpc::Unwrap(cx, obj, false);
    if (!obj) {
      
      return false;
    }

    if (!ac.enter(cx, obj)) {
      return false;
    }
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






































JSObject*
CreateInterfaceObjects(JSContext* cx, JSObject* global, JSObject* receiver,
                       JSObject* protoProto, JSClass* protoClass,
                       JSClass* constructorClass, JSNative constructor,
                       unsigned ctorNargs, Prefable<JSFunctionSpec>* methods,
                       Prefable<JSPropertySpec>* properties,
                       Prefable<ConstantSpec>* constants,
                       Prefable<JSFunctionSpec>* staticMethods, const char* name);

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
    
    
    JSAutoEnterCompartment ac;
    if (js::IsWrapper(scope)) {
      scope = xpc::Unwrap(cx, scope, false);
      if (!scope || !ac.enter(cx, scope)) {
        return false;
      }
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

  *ok = true;
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
inline nsISupports*
GetParentPointer(T* aObject)
{
  return ToSupports(aObject);
}

inline nsISupports*
GetParentPointer(const ParentObject& aObject)
{
  return ToSupports(aObject.mObject);
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

template<typename T>
static inline JSObject*
WrapNativeParent(JSContext* cx, JSObject* scope, const T& p)
{
  if (!GetParentPointer(p))
    return scope;

  nsWrapperCache* cache = GetWrapperCache(p);
  JSObject* obj;
  if (cache && (obj = cache->GetWrapper())) {
#ifdef DEBUG
    qsObjectHelper helper(GetParentPointer(p), cache);
    JS::Value debugVal;

    bool ok = XPCOMObjectToJsval(cx, scope, helper, NULL, false, &debugVal);
    NS_ASSERTION(ok && JSVAL_TO_OBJECT(debugVal) == obj,
                 "Unexpected object in nsWrapperCache");
#endif
    return obj;
  }

  qsObjectHelper helper(GetParentPointer(p), cache);
  JS::Value v;
  return XPCOMObjectToJsval(cx, scope, helper, NULL, false, &v) ?
         JSVAL_TO_OBJECT(v) :
         NULL;
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
JSBool
ThrowingConstructorWorkers(JSContext* cx, unsigned argc, JS::Value* vp);

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
  PRUint32 mFlags;

  
  
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

    
    
    
    if (behavior != eStringify || !pval) {
      
      
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

} 
} 

#endif 
