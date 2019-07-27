





#ifndef mozilla_dom_BindingUtils_h__
#define mozilla_dom_BindingUtils_h__

#include "jsfriendapi.h"
#include "jswrapper.h"
#include "js/Conversions.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Alignment.h"
#include "mozilla/Array.h"
#include "mozilla/Assertions.h"
#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/DeferredFinalize.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/CallbackObject.h"
#include "mozilla/dom/DOMJSClass.h"
#include "mozilla/dom/DOMJSProxyHandler.h"
#include "mozilla/dom/Exceptions.h"
#include "mozilla/dom/NonRefcountedDOMObject.h"
#include "mozilla/dom/Nullable.h"
#include "mozilla/dom/RootedDictionary.h"
#include "mozilla/dom/workers/Workers.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"
#include "nsIGlobalObject.h"
#include "nsIXPConnect.h"
#include "nsJSUtils.h"
#include "nsISupportsImpl.h"
#include "qsObjectHelper.h"
#include "xpcpublic.h"
#include "nsIVariant.h"
#include "pldhash.h" 

#include "nsWrapperCacheInlines.h"

class nsIJSID;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {
template<typename DataType> class MozMap;

struct SelfRef
{
  SelfRef() : ptr(nullptr) {}
  explicit SelfRef(nsISupports *p) : ptr(p) {}
  ~SelfRef() { NS_IF_RELEASE(ptr); }

  nsISupports* ptr;
};

nsresult
UnwrapArgImpl(JS::Handle<JSObject*> src, const nsIID& iid, void** ppArg);


template <class Interface>
inline nsresult
UnwrapArg(JS::Handle<JSObject*> src, Interface** ppArg)
{
  return UnwrapArgImpl(src, NS_GET_TEMPLATE_IID(Interface),
                       reinterpret_cast<void**>(ppArg));
}

inline const ErrNum
GetInvalidThisErrorForMethod(bool aSecurityError)
{
  return aSecurityError ? MSG_METHOD_THIS_UNWRAPPING_DENIED :
                          MSG_METHOD_THIS_DOES_NOT_IMPLEMENT_INTERFACE;
}

inline const ErrNum
GetInvalidThisErrorForGetter(bool aSecurityError)
{
  return aSecurityError ? MSG_GETTER_THIS_UNWRAPPING_DENIED :
                          MSG_GETTER_THIS_DOES_NOT_IMPLEMENT_INTERFACE;
}

inline const ErrNum
GetInvalidThisErrorForSetter(bool aSecurityError)
{
  return aSecurityError ? MSG_SETTER_THIS_UNWRAPPING_DENIED :
                          MSG_SETTER_THIS_DOES_NOT_IMPLEMENT_INTERFACE;
}

bool
ThrowInvalidThis(JSContext* aCx, const JS::CallArgs& aArgs,
                 const ErrNum aErrorNumber,
                 const char* aInterfaceName);

bool
ThrowInvalidThis(JSContext* aCx, const JS::CallArgs& aArgs,
                 const ErrNum aErrorNumber,
                 prototypes::ID aProtoId);

inline bool
ThrowMethodFailedWithDetails(JSContext* cx, ErrorResult& rv,
                             const char* ifaceName,
                             const char* memberName,
                             bool reportJSContentExceptions = false)
{
  if (rv.IsUncatchableException()) {
    
    JS_ClearPendingException(cx);
    
    
    return false;
  }
  if (rv.IsErrorWithMessage()) {
    rv.ReportErrorWithMessage(cx);
    return false;
  }
  if (rv.IsJSException()) {
    if (reportJSContentExceptions) {
      rv.ReportJSExceptionFromJSImplementation(cx);
    } else {
      rv.ReportJSException(cx);
    }
    return false;
  }
  if (rv.IsNotEnoughArgsError()) {
    rv.ReportNotEnoughArgsError(cx, ifaceName, memberName);
    return false;
  }
  rv.ReportGenericError(cx);
  return false;
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
IsNonProxyDOMClass(const js::Class* clasp)
{
  return IsDOMClass(clasp) && !clasp->isProxy();
}

inline bool
IsNonProxyDOMClass(const JSClass* clasp)
{
  return IsNonProxyDOMClass(js::Valueify(clasp));
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

static_assert(DOM_OBJECT_SLOT == 0,
              "DOM_OBJECT_SLOT doesn't match the proxy private slot.  "
              "Expect bad things");
template <class T>
inline T*
UnwrapDOMObject(JSObject* obj)
{
  MOZ_ASSERT(IsDOMClass(js::GetObjectClass(obj)),
             "Don't pass non-DOM objects to this function");

  JS::Value val = js::GetReservedOrProxyPrivateSlot(obj, DOM_OBJECT_SLOT);
  return static_cast<T*>(val.toPrivate());
}

template <class T>
inline T*
UnwrapPossiblyNotInitializedDOMObject(JSObject* obj)
{
  
  
  

  MOZ_ASSERT(IsDOMClass(js::GetObjectClass(obj)),
             "Don't pass non-DOM objects to this function");

  JS::Value val = js::GetReservedOrProxyPrivateSlot(obj, DOM_OBJECT_SLOT);
  if (val.isUndefined()) {
    return nullptr;
  }
  return static_cast<T*>(val.toPrivate());
}

inline const DOMJSClass*
GetDOMClass(const js::Class* clasp)
{
  return IsDOMClass(clasp) ? DOMJSClass::FromJSClass(clasp) : nullptr;
}

inline const DOMJSClass*
GetDOMClass(JSObject* obj)
{
  return GetDOMClass(js::GetObjectClass(obj));
}

inline nsISupports*
UnwrapDOMObjectToISupports(JSObject* aObject)
{
  const DOMJSClass* clasp = GetDOMClass(aObject);
  if (!clasp || !clasp->mDOMObjectIsISupports) {
    return nullptr;
  }

  return UnwrapPossiblyNotInitializedDOMObject<nsISupports>(aObject);
}

inline bool
IsDOMObject(JSObject* obj)
{
  return IsDOMClass(js::GetObjectClass(obj));
}

#define UNWRAP_OBJECT(Interface, obj, value)                                 \
  mozilla::dom::UnwrapObject<mozilla::dom::prototypes::id::Interface,        \
    mozilla::dom::Interface##Binding::NativeType>(obj, value)

#define UNWRAP_WORKER_OBJECT(Interface, obj, value)                           \
  UnwrapObject<prototypes::id::Interface##_workers,                           \
    mozilla::dom::Interface##Binding_workers::NativeType>(obj, value)





template <class T, typename U>
MOZ_ALWAYS_INLINE nsresult
UnwrapObject(JSObject* obj, U& value, prototypes::ID protoID,
             uint32_t protoDepth)
{
  
  const DOMJSClass* domClass = GetDOMClass(obj);
  if (!domClass) {
    
    if (!js::IsWrapper(obj)) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }

    obj = js::CheckedUnwrap(obj,  false);
    if (!obj) {
      return NS_ERROR_XPC_SECURITY_MANAGER_VETO;
    }
    MOZ_ASSERT(!js::IsWrapper(obj));
    domClass = GetDOMClass(obj);
    if (!domClass) {
      
      return NS_ERROR_XPC_BAD_CONVERT_JS;
    }
  }

  


  if (domClass->mInterfaceChain[protoDepth] == protoID) {
    value = UnwrapDOMObject<T>(obj);
    return NS_OK;
  }

  
  return NS_ERROR_XPC_BAD_CONVERT_JS;
}

template <prototypes::ID PrototypeID, class T, typename U>
MOZ_ALWAYS_INLINE nsresult
UnwrapObject(JSObject* obj, U& value)
{
  return UnwrapObject<T>(obj, value, PrototypeID,
                         PrototypeTraits<PrototypeID>::Depth);
}

inline bool
IsNotDateOrRegExp(JSContext* cx, JS::Handle<JSObject*> obj)
{
  MOZ_ASSERT(obj);
  return !JS_ObjectIsDate(cx, obj) && !JS_ObjectIsRegExp(cx, obj);
}

MOZ_ALWAYS_INLINE bool
IsObjectValueConvertibleToDictionary(JSContext* cx,
                                     JS::Handle<JS::Value> objVal)
{
  JS::Rooted<JSObject*> obj(cx, &objVal.toObject());
  return IsNotDateOrRegExp(cx, obj);
}

MOZ_ALWAYS_INLINE bool
IsConvertibleToDictionary(JSContext* cx, JS::Handle<JS::Value> val)
{
  return val.isNullOrUndefined() ||
    (val.isObject() && IsObjectValueConvertibleToDictionary(cx, val));
}

MOZ_ALWAYS_INLINE bool
IsConvertibleToCallbackInterface(JSContext* cx, JS::Handle<JSObject*> obj)
{
  return IsNotDateOrRegExp(cx, obj);
}






static_assert((size_t)constructors::id::_ID_Start ==
              (size_t)prototypes::id::_ID_Count &&
              (size_t)namedpropertiesobjects::id::_ID_Start ==
              (size_t)constructors::id::_ID_Count,
              "Overlapping or discontiguous indexes.");
const size_t kProtoAndIfaceCacheCount = namedpropertiesobjects::id::_ID_Count;

class ProtoAndIfaceCache
{
  
  
  
  
  

  class ArrayCache : public Array<JS::Heap<JSObject*>, kProtoAndIfaceCacheCount>
  {
  public:
    JSObject* EntrySlotIfExists(size_t i) {
      return (*this)[i];
    }

    JS::Heap<JSObject*>& EntrySlotOrCreate(size_t i) {
      return (*this)[i];
    }

    JS::Heap<JSObject*>& EntrySlotMustExist(size_t i) {
      return (*this)[i];
    }

    void Trace(JSTracer* aTracer) {
      for (size_t i = 0; i < ArrayLength(*this); ++i) {
        if ((*this)[i]) {
          JS_CallObjectTracer(aTracer, &(*this)[i], "protoAndIfaceCache[i]");
        }
      }
    }

    size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) {
      return aMallocSizeOf(this);
    }
  };

  class PageTableCache
  {
  public:
    PageTableCache() {
      memset(&mPages, 0, sizeof(mPages));
    }

    ~PageTableCache() {
      for (size_t i = 0; i < ArrayLength(mPages); ++i) {
        delete mPages[i];
      }
    }

    JSObject* EntrySlotIfExists(size_t i) {
      MOZ_ASSERT(i < kProtoAndIfaceCacheCount);
      size_t pageIndex = i / kPageSize;
      size_t leafIndex = i % kPageSize;
      Page* p = mPages[pageIndex];
      if (!p) {
        return nullptr;
      }
      return (*p)[leafIndex];
    }

    JS::Heap<JSObject*>& EntrySlotOrCreate(size_t i) {
      MOZ_ASSERT(i < kProtoAndIfaceCacheCount);
      size_t pageIndex = i / kPageSize;
      size_t leafIndex = i % kPageSize;
      Page* p = mPages[pageIndex];
      if (!p) {
        p = new Page;
        mPages[pageIndex] = p;
      }
      return (*p)[leafIndex];
    }

    JS::Heap<JSObject*>& EntrySlotMustExist(size_t i) {
      MOZ_ASSERT(i < kProtoAndIfaceCacheCount);
      size_t pageIndex = i / kPageSize;
      size_t leafIndex = i % kPageSize;
      Page* p = mPages[pageIndex];
      MOZ_ASSERT(p);
      return (*p)[leafIndex];
    }

    void Trace(JSTracer* trc) {
      for (size_t i = 0; i < ArrayLength(mPages); ++i) {
        Page* p = mPages[i];
        if (p) {
          for (size_t j = 0; j < ArrayLength(*p); ++j) {
            if ((*p)[j]) {
              JS_CallObjectTracer(trc, &(*p)[j], "protoAndIfaceCache[i]");
            }
          }
        }
      }
    }

    size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) {
      size_t n = aMallocSizeOf(this);
      for (size_t i = 0; i < ArrayLength(mPages); ++i) {
        n += aMallocSizeOf(mPages[i]);
      }
      return n;
    }

  private:
    static const size_t kPageSize = 16;
    typedef Array<JS::Heap<JSObject*>, kPageSize> Page;
    static const size_t kNPages = kProtoAndIfaceCacheCount / kPageSize +
      size_t(bool(kProtoAndIfaceCacheCount % kPageSize));
    Array<Page*, kNPages> mPages;
  };

public:
  enum Kind {
    WindowLike,
    NonWindowLike
  };

  explicit ProtoAndIfaceCache(Kind aKind) : mKind(aKind) {
    MOZ_COUNT_CTOR(ProtoAndIfaceCache);
    if (aKind == WindowLike) {
      mArrayCache = new ArrayCache();
    } else {
      mPageTableCache = new PageTableCache();
    }
  }

  ~ProtoAndIfaceCache() {
    if (mKind == WindowLike) {
      delete mArrayCache;
    } else {
      delete mPageTableCache;
    }
    MOZ_COUNT_DTOR(ProtoAndIfaceCache);
  }

#define FORWARD_OPERATION(opName, args)              \
  do {                                               \
    if (mKind == WindowLike) {                       \
      return mArrayCache->opName args;               \
    } else {                                         \
      return mPageTableCache->opName args;           \
    }                                                \
  } while(0)

  
  
  JSObject* EntrySlotIfExists(size_t i) {
    FORWARD_OPERATION(EntrySlotIfExists, (i));
  }

  
  
  JS::Heap<JSObject*>& EntrySlotOrCreate(size_t i) {
    FORWARD_OPERATION(EntrySlotOrCreate, (i));
  }

  
  
  
  JS::Heap<JSObject*>& EntrySlotMustExist(size_t i) {
    FORWARD_OPERATION(EntrySlotMustExist, (i));
  }

  void Trace(JSTracer *aTracer) {
    FORWARD_OPERATION(Trace, (aTracer));
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) {
    size_t n = aMallocSizeOf(this);
    n += (mKind == WindowLike
          ? mArrayCache->SizeOfIncludingThis(aMallocSizeOf)
          : mPageTableCache->SizeOfIncludingThis(aMallocSizeOf));
    return n;
  }
#undef FORWARD_OPERATION

private:
  union {
    ArrayCache *mArrayCache;
    PageTableCache *mPageTableCache;
  };
  Kind mKind;
};

inline void
AllocateProtoAndIfaceCache(JSObject* obj, ProtoAndIfaceCache::Kind aKind)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);
  MOZ_ASSERT(js::GetReservedSlot(obj, DOM_PROTOTYPE_SLOT).isUndefined());

  ProtoAndIfaceCache* protoAndIfaceCache = new ProtoAndIfaceCache(aKind);

  js::SetReservedSlot(obj, DOM_PROTOTYPE_SLOT,
                      JS::PrivateValue(protoAndIfaceCache));
}

#ifdef DEBUG
void
VerifyTraceProtoAndIfaceCacheCalled(JS::CallbackTracer *trc, void **thingp,
                                    JSGCTraceKind kind);

struct VerifyTraceProtoAndIfaceCacheCalledTracer : public JS::CallbackTracer
{
    bool ok;

    explicit VerifyTraceProtoAndIfaceCacheCalledTracer(JSRuntime *rt)
      : JS::CallbackTracer(rt, VerifyTraceProtoAndIfaceCacheCalled), ok(false)
    {}
};
#endif

inline void
TraceProtoAndIfaceCache(JSTracer* trc, JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

#ifdef DEBUG
  if (trc->isCallbackTracer() &&
      trc->asCallbackTracer()->hasCallback(
        VerifyTraceProtoAndIfaceCacheCalled)) {
    
    
    static_cast<VerifyTraceProtoAndIfaceCacheCalledTracer*>(trc)->ok = true;
    return;
  }
#endif

  if (!HasProtoAndIfaceCache(obj))
    return;
  ProtoAndIfaceCache* protoAndIfaceCache = GetProtoAndIfaceCache(obj);
  protoAndIfaceCache->Trace(trc);
}

inline void
DestroyProtoAndIfaceCache(JSObject* obj)
{
  MOZ_ASSERT(js::GetObjectClass(obj)->flags & JSCLASS_DOM_GLOBAL);

  ProtoAndIfaceCache* protoAndIfaceCache = GetProtoAndIfaceCache(obj);

  delete protoAndIfaceCache;
}




bool
DefineConstants(JSContext* cx, JS::Handle<JSObject*> obj,
                const ConstantSpec* cs);

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
CreateInterfaceObjects(JSContext* cx, JS::Handle<JSObject*> global,
                       JS::Handle<JSObject*> protoProto,
                       const js::Class* protoClass, JS::Heap<JSObject*>* protoCache,
                       JS::Handle<JSObject*> interfaceProto,
                       const js::Class* constructorClass, const JSNativeHolder* constructor,
                       unsigned ctorNargs, const NamedConstructor* namedConstructors,
                       JS::Heap<JSObject*>* constructorCache,
                       const NativeProperties* regularProperties,
                       const NativeProperties* chromeOnlyProperties,
                       const char* name, bool defineOnGlobal);














bool
DefineProperties(JSContext* cx, JS::Handle<JSObject*> obj,
                 const NativeProperties* properties,
                 const NativeProperties* chromeOnlyProperties);




bool
DefineUnforgeableMethods(JSContext* cx, JS::Handle<JSObject*> obj,
                         const Prefable<const JSFunctionSpec>* props);




bool
DefineUnforgeableAttributes(JSContext* cx, JS::Handle<JSObject*> obj,
                            const Prefable<const JSPropertySpec>* props);

bool
DefineWebIDLBindingUnforgeablePropertiesOnXPCObject(JSContext* cx,
                                                    JS::Handle<JSObject*> obj,
                                                    const NativeProperties* properties);

bool
DefineWebIDLBindingPropertiesOnXPCObject(JSContext* cx,
                                         JS::Handle<JSObject*> obj,
                                         const NativeProperties* properties);

#define HAS_MEMBER_TYPEDEFS                                               \
private:                                                                  \
  typedef char yes[1];                                                    \
  typedef char no[2]

#ifdef _MSC_VER
#define HAS_MEMBER_CHECK(_name)                                           \
  template<typename V> static yes& Check##_name(char (*)[(&V::_name == 0) + 1])
#else
#define HAS_MEMBER_CHECK(_name)                                           \
  template<typename V> static yes& Check##_name(char (*)[sizeof(&V::_name) + 1])
#endif

#define HAS_MEMBER(_memberName, _valueName)                               \
private:                                                                  \
  HAS_MEMBER_CHECK(_memberName);                                          \
  template<typename V> static no& Check##_memberName(...);                \
                                                                          \
public:                                                                   \
  static bool const _valueName =                                          \
    sizeof(Check##_memberName<T>(nullptr)) == sizeof(yes)

template<class T>
struct NativeHasMember
{
  HAS_MEMBER_TYPEDEFS;

  HAS_MEMBER(GetParentObject, GetParentObject);
  HAS_MEMBER(JSBindingFinalized, JSBindingFinalized);
  HAS_MEMBER(WrapObject, WrapObject);
};

template<class T>
struct IsSmartPtr
{
  HAS_MEMBER_TYPEDEFS;

  HAS_MEMBER(get, value);
};

template<class T>
struct IsRefcounted
{
  HAS_MEMBER_TYPEDEFS;

  HAS_MEMBER(AddRef, HasAddref);
  HAS_MEMBER(Release, HasRelease);

public:
  static bool const value = HasAddref && HasRelease;

private:
  
  
  
  static_assert(!IsBaseOf<nsISupports, T>::value || IsRefcounted::value,
                "Classes derived from nsISupports are refcounted!");

};

#undef HAS_MEMBER
#undef HAS_MEMBER_CHECK
#undef HAS_MEMBER_TYPEDEFS

#ifdef DEBUG
template <class T, bool isISupports=IsBaseOf<nsISupports, T>::value>
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

inline bool
TryToOuterize(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  if (js::IsInnerObject(&rval.toObject())) {
    JS::Rooted<JSObject*> obj(cx, &rval.toObject());
    obj = JS_ObjectToOuterObject(cx, obj);
    if (!obj) {
      return false;
    }

    rval.set(JS::ObjectValue(*obj));
  }

  return true;
}



MOZ_ALWAYS_INLINE
bool
MaybeWrapStringValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(rval.isString());
  JSString* str = rval.toString();
  if (JS::GetTenuredGCThingZone(str) != js::GetContextZone(cx)) {
    return JS_WrapValue(cx, rval);
  }
  return true;
}



MOZ_ALWAYS_INLINE
bool
MaybeWrapObjectValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(rval.isObject());

  
  JSObject* obj = &rval.toObject();
  if (js::GetObjectCompartment(obj) != js::GetContextCompartment(cx)) {
    return JS_WrapValue(cx, rval);
  }

  
  
  if (IsDOMObject(obj)) {
    return TryToOuterize(cx, rval);
  }

  
  
  return JS_WrapValue(cx, rval);
}


MOZ_ALWAYS_INLINE
bool
MaybeWrapObjectOrNullValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(rval.isObjectOrNull());
  if (rval.isNull()) {
    return true;
  }
  return MaybeWrapObjectValue(cx, rval);
}


MOZ_ALWAYS_INLINE
bool
MaybeWrapNonDOMObjectValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(rval.isObject());
  MOZ_ASSERT(!GetDOMClass(&rval.toObject()));
  MOZ_ASSERT(!(js::GetObjectClass(&rval.toObject())->flags &
               JSCLASS_PRIVATE_IS_NSISUPPORTS));

  JSObject* obj = &rval.toObject();
  if (js::GetObjectCompartment(obj) == js::GetContextCompartment(cx)) {
    return true;
  }
  return JS_WrapValue(cx, rval);
}


MOZ_ALWAYS_INLINE
bool
MaybeWrapNonDOMObjectOrNullValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(rval.isObjectOrNull());
  if (rval.isNull()) {
    return true;
  }
  return MaybeWrapNonDOMObjectValue(cx, rval);
}




MOZ_ALWAYS_INLINE bool
MaybeWrapValue(JSContext* cx, JS::MutableHandle<JS::Value> rval)
{
  if (rval.isString()) {
    return MaybeWrapStringValue(cx, rval);
  }

  if (!rval.isObject()) {
    return true;
  }

  return MaybeWrapObjectValue(cx, rval);
}

namespace binding_detail {
enum GetOrCreateReflectorWrapBehavior {
  eWrapIntoContextCompartment,
  eDontWrapIntoContextCompartment
};

template <class T>
struct TypeNeedsOuterization
{
  
  
  static const bool value =
    IsBaseOf<nsGlobalWindow, T>::value || IsSame<EventTarget, T>::value;
};

template <class T, GetOrCreateReflectorWrapBehavior wrapBehavior>
MOZ_ALWAYS_INLINE bool
DoGetOrCreateDOMReflector(JSContext* cx, T* value,
                          JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(value);
  JSObject* obj = value->GetWrapperPreserveColor();
  
  bool couldBeDOMBinding = CouldBeDOMBinding(value);
  if (obj) {
    JS::ExposeObjectToActiveJS(obj);
  } else {
    
    if (!couldBeDOMBinding) {
      return false;
    }

    obj = value->WrapObject(cx, JS::NullPtr());
    if (!obj) {
      
      
      
      return false;
    }
  }

#ifdef DEBUG
  const DOMJSClass* clasp = GetDOMClass(obj);
  
  if (clasp) {
    
    
    
    
    
    
    MOZ_ASSERT(clasp, "What happened here?");
    MOZ_ASSERT_IF(clasp->mDOMObjectIsISupports, (IsBaseOf<nsISupports, T>::value));
    MOZ_ASSERT(CheckWrapperCacheCast<T>::Check());
  }
#endif

  rval.set(JS::ObjectValue(*obj));

  bool sameCompartment =
    js::GetObjectCompartment(obj) == js::GetContextCompartment(cx);
  if (sameCompartment && couldBeDOMBinding) {
    return TypeNeedsOuterization<T>::value ? TryToOuterize(cx, rval) : true;
  }

  if (wrapBehavior == eDontWrapIntoContextCompartment) {
    if (TypeNeedsOuterization<T>::value) {
      JSAutoCompartment ac(cx, obj);
      return TryToOuterize(cx, rval);
    }

    return true;
  }

  return JS_WrapValue(cx, rval);
}
} 










template <class T>
MOZ_ALWAYS_INLINE bool
GetOrCreateDOMReflector(JSContext* cx, T* value,
                        JS::MutableHandle<JS::Value> rval)
{
  using namespace binding_detail;
  return DoGetOrCreateDOMReflector<T, eWrapIntoContextCompartment>(cx, value,
                                                                   rval);
}



template <class T>
MOZ_ALWAYS_INLINE bool
GetOrCreateDOMReflectorNoWrap(JSContext* cx, T* value,
                              JS::MutableHandle<JS::Value> rval)
{
  using namespace binding_detail;
  return DoGetOrCreateDOMReflector<T, eDontWrapIntoContextCompartment>(cx,
                                                                       value,
                                                                       rval);
}




template <class T>
inline bool
WrapNewBindingNonWrapperCachedObject(JSContext* cx,
                                     JS::Handle<JSObject*> scopeArg,
                                     T* value,
                                     JS::MutableHandle<JS::Value> rval)
{
  static_assert(IsRefcounted<T>::value, "Don't pass owned classes in here.");
  MOZ_ASSERT(value);
  
  JS::Rooted<JSObject*> obj(cx);
  {
    
    
    Maybe<JSAutoCompartment> ac;
    
    
    
    JS::Rooted<JSObject*> scope(cx, scopeArg);
    if (js::IsWrapper(scope)) {
      scope = js::CheckedUnwrap(scope,  false);
      if (!scope)
        return false;
      ac.emplace(cx, scope);
    }

    MOZ_ASSERT(js::IsObjectInContextCompartment(scope, cx));
    if (!value->WrapObject(cx, JS::NullPtr(), &obj)) {
      return false;
    }
  }

  
  
  rval.set(JS::ObjectValue(*obj));
  return JS_WrapValue(cx, rval);
}





template <class T>
inline bool
WrapNewBindingNonWrapperCachedObject(JSContext* cx,
                                     JS::Handle<JSObject*> scopeArg,
                                     nsAutoPtr<T>& value,
                                     JS::MutableHandle<JS::Value> rval)
{
  static_assert(!IsRefcounted<T>::value, "Only pass owned classes in here.");
  
  
  if (!value) {
    NS_RUNTIMEABORT("Don't try to wrap null objects");
  }
  
  JS::Rooted<JSObject*> obj(cx);
  {
    
    
    Maybe<JSAutoCompartment> ac;
    
    
    
    JS::Rooted<JSObject*> scope(cx, scopeArg);
    if (js::IsWrapper(scope)) {
      scope = js::CheckedUnwrap(scope,  false);
      if (!scope)
        return false;
      ac.emplace(cx, scope);
    }

    MOZ_ASSERT(js::IsObjectInContextCompartment(scope, cx));
    if (!value->WrapObject(cx, JS::NullPtr(), &obj)) {
      return false;
    }

    value.forget();
  }

  
  
  rval.set(JS::ObjectValue(*obj));
  return JS_WrapValue(cx, rval);
}


template <template <typename> class SmartPtr, typename T,
          typename U=typename EnableIf<IsRefcounted<T>::value, T>::Type>
inline bool
WrapNewBindingNonWrapperCachedObject(JSContext* cx, JS::Handle<JSObject*> scope,
                                     const SmartPtr<T>& value,
                                     JS::MutableHandle<JS::Value> rval)
{
  return WrapNewBindingNonWrapperCachedObject(cx, scope, value.get(), rval);
}



bool
NativeInterface2JSObjectAndThrowIfFailed(JSContext* aCx,
                                         JS::Handle<JSObject*> aScope,
                                         JS::MutableHandle<JS::Value> aRetval,
                                         xpcObjectHelper& aHelper,
                                         const nsIID* aIID,
                                         bool aAllowNativeWrapper);





template <class T>
MOZ_ALWAYS_INLINE bool
HandleNewBindingWrappingFailure(JSContext* cx, JS::Handle<JSObject*> scope,
                                T* value, JS::MutableHandle<JS::Value> rval)
{
  if (JS_IsExceptionPending(cx)) {
    return false;
  }

  qsObjectHelper helper(value, GetWrapperCache(value));
  return NativeInterface2JSObjectAndThrowIfFailed(cx, scope, rval,
                                                  helper, nullptr, true);
}




template <class T, bool isSmartPtr=IsSmartPtr<T>::value>
struct HandleNewBindingWrappingFailureHelper
{
  static inline bool Wrap(JSContext* cx, JS::Handle<JSObject*> scope,
                          const T& value, JS::MutableHandle<JS::Value> rval)
  {
    return HandleNewBindingWrappingFailure(cx, scope, value.get(), rval);
  }
};

template <class T>
struct HandleNewBindingWrappingFailureHelper<T, false>
{
  static inline bool Wrap(JSContext* cx, JS::Handle<JSObject*> scope, T& value,
                          JS::MutableHandle<JS::Value> rval)
  {
    return HandleNewBindingWrappingFailure(cx, scope, &value, rval);
  }
};

template<class T>
inline bool
HandleNewBindingWrappingFailure(JSContext* cx, JS::Handle<JSObject*> scope,
                                T& value, JS::MutableHandle<JS::Value> rval)
{
  return HandleNewBindingWrappingFailureHelper<T>::Wrap(cx, scope, value, rval);
}

template<bool Fatal>
inline bool
EnumValueNotFound(JSContext* cx, JSString* str, const char* type,
                  const char* sourceDescription)
{
  return false;
}

template<>
inline bool
EnumValueNotFound<false>(JSContext* cx, JSString* str, const char* type,
                         const char* sourceDescription)
{
  
  return true;
}

template<>
inline bool
EnumValueNotFound<true>(JSContext* cx, JSString* str, const char* type,
                        const char* sourceDescription)
{
  JSAutoByteString deflated(cx, str);
  if (!deflated) {
    return false;
  }
  return ThrowErrorMessage(cx, MSG_INVALID_ENUM_VALUE, sourceDescription,
                           deflated.ptr(), type);
}

template<typename CharT>
inline int
FindEnumStringIndexImpl(const CharT* chars, size_t length, const EnumEntry* values)
{
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
      return i;
    }
  }

  return -1;
}

template<bool InvalidValueFatal>
inline int
FindEnumStringIndex(JSContext* cx, JS::Handle<JS::Value> v, const EnumEntry* values,
                    const char* type, const char* sourceDescription, bool* ok)
{
  
  JSString* str = JS::ToString(cx, v);
  if (!str) {
    *ok = false;
    return 0;
  }

  {
    int index;
    size_t length;
    JS::AutoCheckCannotGC nogc;
    if (js::StringHasLatin1Chars(str)) {
      const JS::Latin1Char* chars = JS_GetLatin1StringCharsAndLength(cx, nogc, str,
                                                                     &length);
      if (!chars) {
        *ok = false;
        return 0;
      }
      index = FindEnumStringIndexImpl(chars, length, values);
    } else {
      const char16_t* chars = JS_GetTwoByteStringCharsAndLength(cx, nogc, str,
                                                                &length);
      if (!chars) {
        *ok = false;
        return 0;
      }
      index = FindEnumStringIndexImpl(chars, length, values);
    }
    if (index >= 0) {
      *ok = true;
      return index;
    }
  }

  *ok = EnumValueNotFound<InvalidValueFatal>(cx, str, type, sourceDescription);
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

template <typename T>
inline bool
GetUseXBLScope(T* aParentObject)
{
  return false;
}

inline bool
GetUseXBLScope(const ParentObject& aParentObject)
{
  return aParentObject.mUseXBLScope;
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

template<class T>
inline void
UpdateWrapper(T* p, nsWrapperCache* cache, JSObject* obj, const JSObject* old)
{
  JS::AutoAssertGCCallback inCallback(obj);
  cache->UpdateWrapper(obj, old);
}

template<class T>
inline void
UpdateWrapper(T* p, void*, JSObject* obj, const JSObject* old)
{
  JS::AutoAssertGCCallback inCallback(obj);
  nsWrapperCache* cache;
  CallQueryInterface(p, &cache);
  UpdateWrapper(p, cache, obj, old);
}









bool
TryPreserveWrapper(JSObject* obj);


bool
InstanceClassHasProtoAtDepth(const js::Class* clasp,
                             uint32_t protoID, uint32_t depth);



bool
XPCOMObjectToJsval(JSContext* cx, JS::Handle<JSObject*> scope,
                   xpcObjectHelper& helper, const nsIID* iid,
                   bool allowNativeWrapper, JS::MutableHandle<JS::Value> rval);


bool
VariantToJsval(JSContext* aCx, nsIVariant* aVariant,
               JS::MutableHandle<JS::Value> aRetval);





template<class T>
inline bool
WrapObject(JSContext* cx, T* p, nsWrapperCache* cache, const nsIID* iid,
           JS::MutableHandle<JS::Value> rval)
{
  if (xpc_FastGetCachedWrapper(cx, cache, rval))
    return true;
  qsObjectHelper helper(p, cache);
  JS::Rooted<JSObject*> scope(cx, JS::CurrentGlobalOrNull(cx));
  return XPCOMObjectToJsval(cx, scope, helper, iid, true, rval);
}



template<>
inline bool
WrapObject<nsIVariant>(JSContext* cx, nsIVariant* p,
                       nsWrapperCache* cache, const nsIID* iid,
                       JS::MutableHandle<JS::Value> rval)
{
  MOZ_ASSERT(iid);
  MOZ_ASSERT(iid->Equals(NS_GET_IID(nsIVariant)));
  return VariantToJsval(cx, p, rval);
}




template<class T>
inline bool
WrapObject(JSContext* cx, T* p, const nsIID* iid,
           JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p, GetWrapperCache(p), iid, rval);
}




template<class T>
inline bool
WrapObject(JSContext* cx, T* p, JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p, nullptr, rval);
}


template<class T>
inline bool
WrapObject(JSContext* cx, const nsCOMPtr<T>& p,
           const nsIID* iid, JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p.get(), iid, rval);
}


template<class T>
inline bool
WrapObject(JSContext* cx, const nsCOMPtr<T>& p,
           JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p, nullptr, rval);
}


template<class T>
inline bool
WrapObject(JSContext* cx, const nsRefPtr<T>& p,
           const nsIID* iid, JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p.get(), iid, rval);
}


template<class T>
inline bool
WrapObject(JSContext* cx, const nsRefPtr<T>& p,
           JS::MutableHandle<JS::Value> rval)
{
  return WrapObject(cx, p, nullptr, rval);
}


template<>
inline bool
WrapObject<JSObject>(JSContext* cx, JSObject* p,
                     JS::MutableHandle<JS::Value> rval)
{
  rval.set(JS::ObjectOrNullValue(p));
  return true;
}

inline bool
WrapObject(JSContext* cx, JSObject& p, JS::MutableHandle<JS::Value> rval)
{
  rval.set(JS::ObjectValue(p));
  return true;
}





template<typename T>
static inline JSObject*
WrapNativeISupportsParent(JSContext* cx, T* p, nsWrapperCache* cache)
{
  qsObjectHelper helper(ToSupports(p), cache);
  JS::Rooted<JSObject*> scope(cx, JS::CurrentGlobalOrNull(cx));
  JS::Rooted<JS::Value> v(cx);
  return XPCOMObjectToJsval(cx, scope, helper, nullptr, false, &v) ?
         v.toObjectOrNull() :
         nullptr;
}



template<typename T, bool isISupports=IsBaseOf<nsISupports, T>::value>
struct WrapNativeParentFallback
{
  static inline JSObject* Wrap(JSContext* cx, T* parent, nsWrapperCache* cache)
  {
    return nullptr;
  }
};



template<typename T >
struct WrapNativeParentFallback<T, true >
{
  static inline JSObject* Wrap(JSContext* cx, T* parent, nsWrapperCache* cache)
  {
    return WrapNativeISupportsParent(cx, parent, cache);
  }
};



template<typename T, bool hasWrapObject=NativeHasMember<T>::WrapObject>
struct WrapNativeParentHelper
{
  static inline JSObject* Wrap(JSContext* cx, T* parent, nsWrapperCache* cache)
  {
    MOZ_ASSERT(cache);

    JSObject* obj;
    if ((obj = cache->GetWrapper())) {
      return obj;
    }

    
    if (!CouldBeDOMBinding(parent)) {
      obj = WrapNativeParentFallback<T>::Wrap(cx, parent, cache);
    } else {
      obj = parent->WrapObject(cx, JS::NullPtr());
    }

    return obj;
  }
};



template<typename T>
struct WrapNativeParentHelper<T, false>
{
  static inline JSObject* Wrap(JSContext* cx, T* parent, nsWrapperCache* cache)
  {
    JSObject* obj;
    if (cache && (obj = cache->GetWrapper())) {
#ifdef DEBUG
      NS_ASSERTION(WrapNativeISupportsParent(cx, parent, cache) == obj,
                   "Unexpected object in nsWrapperCache");
#endif
      return obj;
    }

    return WrapNativeISupportsParent(cx, parent, cache);
  }
};


template<typename T>
static inline JSObject*
WrapNativeParent(JSContext* cx, T* p, nsWrapperCache* cache,
                 bool useXBLScope = false)
{
  if (!p) {
    return JS::CurrentGlobalOrNull(cx);
  }

  JSObject* parent = WrapNativeParentHelper<T>::Wrap(cx, p, cache);
  if (!useXBLScope) {
    return parent;
  }

  
  
  
  if (xpc::IsInContentXBLScope(parent)) {
    return parent;
  }
  JS::Rooted<JSObject*> rootedParent(cx, parent);
  JS::Rooted<JSObject*> xblScope(cx, xpc::GetXBLScope(cx, rootedParent));
  NS_ENSURE_TRUE(xblScope, nullptr);
  JSAutoCompartment ac(cx, xblScope);
  if (NS_WARN_IF(!JS_WrapObject(cx, &rootedParent))) {
    return nullptr;
  }

  return rootedParent;
}



template<typename T>
static inline JSObject*
WrapNativeParent(JSContext* cx, const T& p)
{
  return WrapNativeParent(cx, GetParentPointer(p), GetWrapperCache(p), GetUseXBLScope(p));
}



template<>
inline JSObject*
WrapNativeParent(JSContext* cx, nsIGlobalObject* const& p)
{
  return p ? p->GetGlobalJSObject() : JS::CurrentGlobalOrNull(cx);
}

template<typename T, bool WrapperCached=NativeHasMember<T>::GetParentObject>
struct GetParentObject
{
  static JSObject* Get(JSContext* cx, JS::Handle<JSObject*> obj)
  {
    MOZ_ASSERT(js::IsObjectInContextCompartment(obj, cx));
    T* native = UnwrapDOMObject<T>(obj);
    JSObject* wrappedParent = WrapNativeParent(cx, native->GetParentObject());
    return wrappedParent ? js::GetGlobalForObjectCrossCompartment(wrappedParent) : nullptr;
  }
};

template<typename T>
struct GetParentObject<T, false>
{
  static JSObject* Get(JSContext* cx, JS::Handle<JSObject*> obj)
  {
    MOZ_CRASH();
    return nullptr;
  }
};



template <class T, bool isSmartPtr=IsSmartPtr<T>::value>
struct GetOrCreateDOMReflectorHelper
{
  static inline bool GetOrCreate(JSContext* cx, const T& value,
                                 JS::MutableHandle<JS::Value> rval)
  {
    return GetOrCreateDOMReflector(cx, value.get(), rval);
  }
};

template <class T>
struct GetOrCreateDOMReflectorHelper<T, false>
{
  static inline bool GetOrCreate(JSContext* cx, T& value,
                                 JS::MutableHandle<JS::Value> rval)
  {
    static_assert(IsRefcounted<T>::value, "Don't pass owned classes in here.");
    return GetOrCreateDOMReflector(cx, &value, rval);
  }
};

template<class T>
inline bool
GetOrCreateDOMReflector(JSContext* cx, T& value,
                        JS::MutableHandle<JS::Value> rval)
{
  return GetOrCreateDOMReflectorHelper<T>::GetOrCreate(cx, value, rval);
}




template<class T>
inline bool
GetOrCreateDOMReflector(JSContext* cx, JS::Handle<JSObject*> scope, T& value,
                        JS::MutableHandle<JS::Value> rval)
{
  return GetOrCreateDOMReflector(cx, value, rval);
}



template <class T, bool isSmartPtr=IsSmartPtr<T>::value>
struct GetOrCreateDOMReflectorNoWrapHelper
{
  static inline bool GetOrCreate(JSContext* cx, const T& value,
                                 JS::MutableHandle<JS::Value> rval)
  {
    return GetOrCreateDOMReflectorNoWrap(cx, value.get(), rval);
  }
};

template <class T>
struct GetOrCreateDOMReflectorNoWrapHelper<T, false>
{
  static inline bool GetOrCreate(JSContext* cx, T& value,
                                 JS::MutableHandle<JS::Value> rval)
  {
    return GetOrCreateDOMReflectorNoWrap(cx, &value, rval);
  }
};

template<class T>
inline bool
GetOrCreateDOMReflectorNoWrap(JSContext* cx, T& value,
                              JS::MutableHandle<JS::Value> rval)
{
  return
    GetOrCreateDOMReflectorNoWrapHelper<T>::GetOrCreate(cx, value, rval);
}

template <class T>
inline JSObject*
GetCallbackFromCallbackObject(T* aObj)
{
  return aObj->Callback();
}




template <class T, bool isSmartPtr=IsSmartPtr<T>::value>
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
InitIds(JSContext* cx, const Prefable<Spec>* prefableSpecs, jsid* ids)
{
  MOZ_ASSERT(prefableSpecs);
  MOZ_ASSERT(prefableSpecs->specs);
  do {
    
    
    Spec* spec = prefableSpecs->specs;
    do {
      if (!JS::PropertySpecNameToPermanentId(cx, spec->name, ids)) {
        return false;
      }
    } while (++ids, (++spec)->name);

    
    
    *ids = JSID_VOID;
    ++ids;
  } while ((++prefableSpecs)->specs);

  return true;
}

bool
QueryInterface(JSContext* cx, unsigned argc, JS::Value* vp);

template <class T>
struct
WantsQueryInterface
{
  static_assert(IsBaseOf<nsISupports, T>::value,
                "QueryInterface can't work without an nsISupports.");
  static bool Enabled(JSContext* aCx, JSObject* aGlobal)
  {
    return NS_IsMainThread() && IsChromeOrXBL(aCx, aGlobal);
  }
};

void
GetInterfaceImpl(JSContext* aCx, nsIInterfaceRequestor* aRequestor,
                 nsWrapperCache* aCache, nsIJSID* aIID,
                 JS::MutableHandle<JS::Value> aRetval, ErrorResult& aError);

template<class T>
void
GetInterface(JSContext* aCx, T* aThis, nsIJSID* aIID,
             JS::MutableHandle<JS::Value> aRetval, ErrorResult& aError)
{
  GetInterfaceImpl(aCx, aThis, aThis, aIID, aRetval, aError);
}

bool
UnforgeableValueOf(JSContext* cx, unsigned argc, JS::Value* vp);

bool
ThrowingConstructor(JSContext* cx, unsigned argc, JS::Value* vp);

bool
ThrowConstructorWithoutNew(JSContext* cx, const char* name);

bool
GetPropertyOnPrototype(JSContext* cx, JS::Handle<JSObject*> proxy,
                       JS::Handle<jsid> id, bool* found,
                       JS::MutableHandle<JS::Value> vp);


bool
HasPropertyOnPrototype(JSContext* cx, JS::Handle<JSObject*> proxy,
                       JS::Handle<jsid> id, bool* has);






bool
AppendNamedPropertyIds(JSContext* cx, JS::Handle<JSObject*> proxy,
                       nsTArray<nsString>& names,
                       bool shadowPrototypeProperties, JS::AutoIdVector& props);

namespace binding_detail {




struct FakeString {
  FakeString() :
    mFlags(nsString::F_TERMINATED)
  {
  }

  ~FakeString() {
    if (mFlags & nsString::F_SHARED) {
      nsStringBuffer::FromData(mData)->Release();
    }
  }

  void Rebind(const nsString::char_type* aData, nsString::size_type aLength) {
    MOZ_ASSERT(mFlags == nsString::F_TERMINATED);
    mData = const_cast<nsString::char_type*>(aData);
    mLength = aLength;
  }

  void Truncate() {
    MOZ_ASSERT(mFlags == nsString::F_TERMINATED);
    mData = nsString::char_traits::sEmptyBuffer;
    mLength = 0;
  }

  void SetIsVoid(bool aValue) {
    MOZ_ASSERT(aValue,
               "We don't support SetIsVoid(false) on FakeString!");
    Truncate();
    mFlags |= nsString::F_VOIDED;
  }

  const nsString::char_type* Data() const
  {
    return mData;
  }

  nsString::char_type* BeginWriting()
  {
    return mData;
  }

  nsString::size_type Length() const
  {
    return mLength;
  }

  
  bool SetLength(nsString::size_type aLength, mozilla::fallible_t const&) {
    
    if (aLength < sInlineCapacity) {
      SetData(mInlineStorage);
    } else {
      nsStringBuffer *buf = nsStringBuffer::Alloc((aLength + 1) * sizeof(nsString::char_type)).take();
      if (MOZ_UNLIKELY(!buf)) {
        return false;
      }

      SetData(static_cast<nsString::char_type*>(buf->Data()));
      mFlags = nsString::F_SHARED | nsString::F_TERMINATED;
    }
    mLength = aLength;
    mData[mLength] = char16_t(0);
    return true;
  }

  
  
  const nsAString* ToAStringPtr() const {
    return reinterpret_cast<const nsString*>(this);
  }

operator const nsAString& () const {
    return *reinterpret_cast<const nsString*>(this);
  }

private:
  nsAString* ToAStringPtr() {
    return reinterpret_cast<nsString*>(this);
  }

  nsString::char_type* mData;
  nsString::size_type mLength;
  uint32_t mFlags;

  static const size_t sInlineCapacity = 64;
  nsString::char_type mInlineStorage[sInlineCapacity];

  FakeString(const FakeString& other) = delete;
  void operator=(const FakeString& other) = delete;

  void SetData(nsString::char_type* aData) {
    MOZ_ASSERT(mFlags == nsString::F_TERMINATED);
    mData = const_cast<nsString::char_type*>(aData);
  }

  friend class NonNull<nsAString>;

  
  
  class StringAsserter;
  friend class StringAsserter;

  class StringAsserter : public nsString {
  public:
    static void StaticAsserts() {
      static_assert(offsetof(FakeString, mInlineStorage) ==
                      sizeof(nsString),
                    "FakeString should include all nsString members");
      static_assert(offsetof(FakeString, mData) ==
                      offsetof(StringAsserter, mData),
                    "Offset of mData should match");
      static_assert(offsetof(FakeString, mLength) ==
                      offsetof(StringAsserter, mLength),
                    "Offset of mLength should match");
      static_assert(offsetof(FakeString, mFlags) ==
                      offsetof(StringAsserter, mFlags),
                    "Offset of mFlags should match");
    }
  };
};

} 

enum StringificationBehavior {
  eStringify,
  eEmpty,
  eNull
};

template<typename T>
static inline bool
ConvertJSValueToString(JSContext* cx, JS::Handle<JS::Value> v,
                       StringificationBehavior nullBehavior,
                       StringificationBehavior undefinedBehavior,
                       T& result)
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
        result.SetIsVoid(true);
      }
      return true;
    }

    s = JS::ToString(cx, v);
    if (!s) {
      return false;
    }
  }

  return AssignJSString(cx, result, s);
}

void
NormalizeUSVString(JSContext* aCx, nsAString& aString);

void
NormalizeUSVString(JSContext* aCx, binding_detail::FakeString& aString);

template<typename T>
inline bool
ConvertIdToString(JSContext* cx, JS::HandleId id, T& result, bool& isSymbol)
{
  if (MOZ_LIKELY(JSID_IS_STRING(id))) {
    if (!AssignJSString(cx, result, JSID_TO_STRING(id))) {
      return false;
    }
  } else if (JSID_IS_SYMBOL(id)) {
    isSymbol = true;
    return true;
  } else {
    JS::RootedValue nameVal(cx, js::IdToValue(id));
    if (!ConvertJSValueToString(cx, nameVal, eStringify, eStringify, result)) {
      return false;
    }
  }
  isSymbol = false;
  return true;
}

bool
ConvertJSValueToByteString(JSContext* cx, JS::Handle<JS::Value> v,
                           bool nullable, nsACString& result);

template<typename T>
void DoTraceSequence(JSTracer* trc, FallibleTArray<T>& seq);
template<typename T>
void DoTraceSequence(JSTracer* trc, InfallibleTArray<T>& seq);


namespace binding_detail {

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

} 



template<typename T,
         bool isDictionary=IsBaseOf<DictionaryBase, T>::value,
         bool isTypedArray=IsBaseOf<AllTypedArraysBase, T>::value,
         bool isOwningUnion=IsBaseOf<AllOwningUnionBase, T>::value>
class SequenceTracer
{
  explicit SequenceTracer() = delete; 
};


template<>
class SequenceTracer<JSObject*, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, JSObject** objp, JSObject** end) {
    for (; objp != end; ++objp) {
      JS_CallUnbarrieredObjectTracer(trc, objp, "sequence<object>");
    }
  }
};


template<>
class SequenceTracer<JS::Value, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, JS::Value* valp, JS::Value* end) {
    for (; valp != end; ++valp) {
      JS_CallUnbarrieredValueTracer(trc, valp, "sequence<any>");
    }
  }
};


template<typename T>
class SequenceTracer<Sequence<T>, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, Sequence<T>* seqp, Sequence<T>* end) {
    for (; seqp != end; ++seqp) {
      DoTraceSequence(trc, *seqp);
    }
  }
};


template<typename T>
class SequenceTracer<nsTArray<T>, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, nsTArray<T>* seqp, nsTArray<T>* end) {
    for (; seqp != end; ++seqp) {
      DoTraceSequence(trc, *seqp);
    }
  }
};


template<typename T>
class SequenceTracer<T, true, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, T* dictp, T* end) {
    for (; dictp != end; ++dictp) {
      dictp->TraceDictionary(trc);
    }
  }
};


template<typename T>
class SequenceTracer<T, false, true, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, T* arrayp, T* end) {
    for (; arrayp != end; ++arrayp) {
      arrayp->TraceSelf(trc);
    }
  }
};


template<typename T>
class SequenceTracer<T, false, false, true>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, T* arrayp, T* end) {
    for (; arrayp != end; ++arrayp) {
      arrayp->TraceUnion(trc);
    }
  }
};


template<typename T>
class SequenceTracer<Nullable<T>, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, Nullable<T>* seqp,
                            Nullable<T>* end) {
    for (; seqp != end; ++seqp) {
      if (!seqp->IsNull()) {
        
        
        T& val = seqp->Value();
        T* ptr = &val;
        SequenceTracer<T>::TraceSequence(trc, ptr, ptr+1);
      }
    }
  }
};




template<typename T>
static PLDHashOperator
TraceMozMapValue(T* aValue, void* aClosure)
{
  JSTracer* trc = static_cast<JSTracer*>(aClosure);
  
  SequenceTracer<T>::TraceSequence(trc, aValue, aValue + 1);
  return PL_DHASH_NEXT;
}

template<typename T>
void TraceMozMap(JSTracer* trc, MozMap<T>& map)
{
  map.EnumerateValues(TraceMozMapValue<T>, trc);
}


template<typename T>
class SequenceTracer<MozMap<T>, false, false, false>
{
  explicit SequenceTracer() = delete; 

public:
  static void TraceSequence(JSTracer* trc, MozMap<T>* seqp, MozMap<T>* end) {
    for (; seqp != end; ++seqp) {
      seqp->EnumerateValues(TraceMozMapValue<T>, trc);
    }
  }
};

template<typename T>
void DoTraceSequence(JSTracer* trc, FallibleTArray<T>& seq)
{
  SequenceTracer<T>::TraceSequence(trc, seq.Elements(),
                                   seq.Elements() + seq.Length());
}

template<typename T>
void DoTraceSequence(JSTracer* trc, InfallibleTArray<T>& seq)
{
  SequenceTracer<T>::TraceSequence(trc, seq.Elements(),
                                   seq.Elements() + seq.Length());
}


template<typename T>
class MOZ_STACK_CLASS SequenceRooter : private JS::CustomAutoRooter
{
public:
  SequenceRooter(JSContext *aCx, FallibleTArray<T>* aSequence
                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : JS::CustomAutoRooter(aCx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT),
      mFallibleArray(aSequence),
      mSequenceType(eFallibleArray)
  {
  }

  SequenceRooter(JSContext *aCx, InfallibleTArray<T>* aSequence
                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : JS::CustomAutoRooter(aCx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT),
      mInfallibleArray(aSequence),
      mSequenceType(eInfallibleArray)
  {
  }

  SequenceRooter(JSContext *aCx, Nullable<nsTArray<T> >* aSequence
                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : JS::CustomAutoRooter(aCx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT),
      mNullableArray(aSequence),
      mSequenceType(eNullableArray)
  {
  }

 private:
  enum SequenceType {
    eInfallibleArray,
    eFallibleArray,
    eNullableArray
  };

  virtual void trace(JSTracer *trc) override
  {
    if (mSequenceType == eFallibleArray) {
      DoTraceSequence(trc, *mFallibleArray);
    } else if (mSequenceType == eInfallibleArray) {
      DoTraceSequence(trc, *mInfallibleArray);
    } else {
      MOZ_ASSERT(mSequenceType == eNullableArray);
      if (!mNullableArray->IsNull()) {
        DoTraceSequence(trc, mNullableArray->Value());
      }
    }
  }

  union {
    InfallibleTArray<T>* mInfallibleArray;
    FallibleTArray<T>* mFallibleArray;
    Nullable<nsTArray<T> >* mNullableArray;
  };

  SequenceType mSequenceType;
};


template<typename T>
class MOZ_STACK_CLASS MozMapRooter : private JS::CustomAutoRooter
{
public:
  MozMapRooter(JSContext *aCx, MozMap<T>* aMozMap
               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : JS::CustomAutoRooter(aCx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT),
      mMozMap(aMozMap),
      mMozMapType(eMozMap)
  {
  }

  MozMapRooter(JSContext *aCx, Nullable<MozMap<T>>* aMozMap
                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : JS::CustomAutoRooter(aCx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT),
      mNullableMozMap(aMozMap),
      mMozMapType(eNullableMozMap)
  {
  }

private:
  enum MozMapType {
    eMozMap,
    eNullableMozMap
  };

  virtual void trace(JSTracer *trc) override
  {
    if (mMozMapType == eMozMap) {
      TraceMozMap(trc, *mMozMap);
    } else {
      MOZ_ASSERT(mMozMapType == eNullableMozMap);
      if (!mNullableMozMap->IsNull()) {
        TraceMozMap(trc, mNullableMozMap->Value());
      }
    }
  }

  union {
    MozMap<T>* mMozMap;
    Nullable<MozMap<T>>* mNullableMozMap;
  };

  MozMapType mMozMapType;
};

template<typename T>
class MOZ_STACK_CLASS RootedUnion : public T,
                                    private JS::CustomAutoRooter
{
public:
  explicit RootedUnion(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
    T(),
    JS::CustomAutoRooter(cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  {
  }

  virtual void trace(JSTracer *trc) override
  {
    this->TraceUnion(trc);
  }
};

template<typename T>
class MOZ_STACK_CLASS NullableRootedUnion : public Nullable<T>,
                                            private JS::CustomAutoRooter
{
public:
  explicit NullableRootedUnion(JSContext* cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
    Nullable<T>(),
    JS::CustomAutoRooter(cx MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT)
  {
  }

  virtual void trace(JSTracer *trc) override
  {
    if (!this->IsNull()) {
      this->Value().TraceUnion(trc);
    }
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
         InternJSString(cx, *(vector[vector.length() - 1]).address(), name);
}










bool
XrayResolveOwnProperty(JSContext* cx, JS::Handle<JSObject*> wrapper,
                       JS::Handle<JSObject*> obj,
                       JS::Handle<jsid> id,
                       JS::MutableHandle<JSPropertyDescriptor> desc,
                       bool& cacheOnHolder);












bool
XrayDefineProperty(JSContext* cx, JS::Handle<JSObject*> wrapper,
                   JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                   JS::Handle<JSPropertyDescriptor> desc,
                   JS::ObjectOpResult &result,
                   bool *defined);










bool
XrayOwnPropertyKeys(JSContext* cx, JS::Handle<JSObject*> wrapper,
                    JS::Handle<JSObject*> obj,
                    unsigned flags, JS::AutoIdVector& props);











inline bool
XrayGetNativeProto(JSContext* cx, JS::Handle<JSObject*> obj,
                   JS::MutableHandle<JSObject*> protop)
{
  JS::Rooted<JSObject*> global(cx, js::GetGlobalForObjectCrossCompartment(obj));
  {
    JSAutoCompartment ac(cx, global);
    const DOMJSClass* domClass = GetDOMClass(obj);
    if (domClass) {
      ProtoHandleGetter protoGetter = domClass->mGetProto;
      if (protoGetter) {
        protop.set(protoGetter(cx, global));
      } else {
        protop.set(JS_GetObjectPrototype(cx, global));
      }
    } else {
      const js::Class* clasp = js::GetObjectClass(obj);
      MOZ_ASSERT(IsDOMIfaceAndProtoClass(clasp));
      ProtoGetter protoGetter =
        DOMIfaceAndProtoJSClass::FromJSClass(clasp)->mGetParentProto;
      protop.set(protoGetter(cx, global));
    }
  }

  return JS_WrapObject(cx, protop);
}

extern NativePropertyHooks sEmptyNativePropertyHooks;










enum {
  CONSTRUCTOR_NATIVE_HOLDER_RESERVED_SLOT = 0
};

bool
Constructor(JSContext* cx, unsigned argc, JS::Value* vp);

inline bool
UseDOMXray(JSObject* obj)
{
  const js::Class* clasp = js::GetObjectClass(obj);
  return IsDOMClass(clasp) ||
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
 
template<class T, bool hasCallback=NativeHasMember<T>::JSBindingFinalized>
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


template<typename T>
const T& Constify(T& arg)
{
  return arg;
}


template<typename T>
T& NonNullHelper(T& aArg)
{
  return aArg;
}

template<typename T>
T& NonNullHelper(NonNull<T>& aArg)
{
  return aArg;
}

template<typename T>
const T& NonNullHelper(const NonNull<T>& aArg)
{
  return aArg;
}

template<typename T>
T& NonNullHelper(OwningNonNull<T>& aArg)
{
  return aArg;
}

template<typename T>
const T& NonNullHelper(const OwningNonNull<T>& aArg)
{
  return aArg;
}

inline
void NonNullHelper(NonNull<binding_detail::FakeString>& aArg)
{
  
  
  
  
}

inline
void NonNullHelper(const NonNull<binding_detail::FakeString>& aArg)
{
  
  
  
  
}

inline
void NonNullHelper(binding_detail::FakeString& aArg)
{
  
  
  
  
}

MOZ_ALWAYS_INLINE
const nsAString& NonNullHelper(const binding_detail::FakeString& aArg)
{
  return aArg;
}



nsresult
ReparentWrapper(JSContext* aCx, JS::Handle<JSObject*> aObj);






bool
InterfaceHasInstance(JSContext* cx, JS::Handle<JSObject*> obj,
                     JS::Handle<JSObject*> instance,
                     bool* bp);
bool
InterfaceHasInstance(JSContext* cx, JS::Handle<JSObject*> obj, JS::MutableHandle<JS::Value> vp,
                     bool* bp);
bool
InterfaceHasInstance(JSContext* cx, int prototypeID, int depth,
                     JS::Handle<JSObject*> instance,
                     bool* bp);



bool
ReportLenientThisUnwrappingFailure(JSContext* cx, JSObject* obj);




bool
GetContentGlobalForJSImplementedObject(JSContext* cx, JS::Handle<JSObject*> obj,
                                       nsIGlobalObject** global);

void
ConstructJSImplementation(JSContext* aCx, const char* aContractId,
                          nsIGlobalObject* aGlobal,
                          JS::MutableHandle<JSObject*> aObject,
                          ErrorResult& aRv);

already_AddRefed<nsIGlobalObject>
ConstructJSImplementation(JSContext* aCx, const char* aContractId,
                          const GlobalObject& aGlobal,
                          JS::MutableHandle<JSObject*> aObject,
                          ErrorResult& aRv);







bool NonVoidByteStringToJsval(JSContext *cx, const nsACString &str,
                              JS::MutableHandle<JS::Value> rval);
inline bool ByteStringToJsval(JSContext *cx, const nsACString &str,
                              JS::MutableHandle<JS::Value> rval)
{
    if (str.IsVoid()) {
        rval.setNull();
        return true;
    }
    return NonVoidByteStringToJsval(cx, str, rval);
}

template<class T, bool isISupports=IsBaseOf<nsISupports, T>::value>
struct PreserveWrapperHelper
{
  static void PreserveWrapper(T* aObject)
  {
    aObject->PreserveWrapper(aObject, NS_CYCLE_COLLECTION_PARTICIPANT(T));
  }
};

template<class T>
struct PreserveWrapperHelper<T, true>
{
  static void PreserveWrapper(T* aObject)
  {
    aObject->PreserveWrapper(reinterpret_cast<nsISupports*>(aObject));
  }
};

template<class T>
void PreserveWrapper(T* aObject)
{
  PreserveWrapperHelper<T>::PreserveWrapper(aObject);
}

template<class T, bool isISupports=IsBaseOf<nsISupports, T>::value>
struct CastingAssertions
{
  static bool ToSupportsIsCorrect(T*)
  {
    return true;
  }
  static bool ToSupportsIsOnPrimaryInheritanceChain(T*, nsWrapperCache*)
  {
    return true;
  }
};

template<class T>
struct CastingAssertions<T, true>
{
  static bool ToSupportsIsCorrect(T* aObject)
  {
    return ToSupports(aObject) ==  reinterpret_cast<nsISupports*>(aObject);
  }
  static bool ToSupportsIsOnPrimaryInheritanceChain(T* aObject,
                                                    nsWrapperCache* aCache)
  {
    return reinterpret_cast<void*>(aObject) != aCache;
  }
};

template<class T>
bool
ToSupportsIsCorrect(T* aObject)
{
  return CastingAssertions<T>::ToSupportsIsCorrect(aObject);
}

template<class T>
bool
ToSupportsIsOnPrimaryInheritanceChain(T* aObject, nsWrapperCache* aCache)
{
  return CastingAssertions<T>::ToSupportsIsOnPrimaryInheritanceChain(aObject,
                                                                     aCache);
}











template<class T>
class MOZ_STACK_CLASS BindingJSObjectCreator
{
public:
  explicit BindingJSObjectCreator(JSContext* aCx)
    : mReflector(aCx)
  {
  }

  ~BindingJSObjectCreator()
  {
    if (mReflector) {
      js::SetReservedOrProxyPrivateSlot(mReflector, DOM_OBJECT_SLOT,
                                        JS::UndefinedValue());
    }
  }

  void
  CreateProxyObject(JSContext* aCx, const js::Class* aClass,
                    const DOMProxyHandler* aHandler,
                    JS::Handle<JSObject*> aProto, T* aNative,
                    JS::MutableHandle<JSObject*> aReflector)
  {
    js::ProxyOptions options;
    options.setClass(aClass);
    JS::Rooted<JS::Value> proxyPrivateVal(aCx, JS::PrivateValue(aNative));
    aReflector.set(js::NewProxyObject(aCx, aHandler, proxyPrivateVal, aProto,
                                      options));
    if (aReflector) {
      mNative = aNative;
      mReflector = aReflector;
    }
  }

  void
  CreateObject(JSContext* aCx, const JSClass* aClass,
               JS::Handle<JSObject*> aProto,
               T* aNative, JS::MutableHandle<JSObject*> aReflector)
  {
    aReflector.set(JS_NewObjectWithGivenProto(aCx, aClass, aProto));
    if (aReflector) {
      js::SetReservedSlot(aReflector, DOM_OBJECT_SLOT, JS::PrivateValue(aNative));
      mNative = aNative;
      mReflector = aReflector;
    }
  }

  void
  InitializationSucceeded()
  {
    void* dummy;
    mNative.forget(&dummy);
    mReflector = nullptr;
  }

private:
  struct OwnedNative
  {
    
    
    static_assert(IsBaseOf<NonRefcountedDOMObject, T>::value,
                  "Non-refcounted objects with DOM bindings should inherit "
                  "from NonRefcountedDOMObject.");

    OwnedNative&
    operator=(T* aNative)
    {
      return *this;
    }

    
    
    void
    forget(void**)
    {
    }
  };

  JS::Rooted<JSObject*> mReflector;
  typename Conditional<IsRefcounted<T>::value, nsRefPtr<T>, OwnedNative>::Type mNative;
};

template<class T>
struct DeferredFinalizerImpl
{
  typedef typename Conditional<IsSame<T, nsISupports>::value,
                               nsCOMPtr<T>,
                               typename Conditional<IsRefcounted<T>::value,
                                                    nsRefPtr<T>,
                                                    nsAutoPtr<T>>::Type>::Type SmartPtr;
  typedef nsTArray<SmartPtr> SmartPtrArray;

  static_assert(IsSame<T, nsISupports>::value || !IsBaseOf<nsISupports, T>::value,
                "nsISupports classes should all use the nsISupports instantiation");

  static inline void
  AppendAndTake(nsTArray<nsCOMPtr<nsISupports>>& smartPtrArray, nsISupports* ptr)
  {
    smartPtrArray.AppendElement(dont_AddRef(ptr));
  }
  template<class U>
  static inline void
  AppendAndTake(nsTArray<nsRefPtr<U>>& smartPtrArray, U* ptr)
  {
    smartPtrArray.AppendElement(dont_AddRef(ptr));
  }
  template<class U>
  static inline void
  AppendAndTake(nsTArray<nsAutoPtr<U>>& smartPtrArray, U* ptr)
  {
    smartPtrArray.AppendElement(ptr);
  }

  static void*
  AppendDeferredFinalizePointer(void* aData, void* aObject)
  {
    SmartPtrArray* pointers = static_cast<SmartPtrArray*>(aData);
    if (!pointers) {
      pointers = new SmartPtrArray();
    }
    AppendAndTake(*pointers, static_cast<T*>(aObject));
    return pointers;
  }
  static bool
  DeferredFinalize(uint32_t aSlice, void* aData)
  {
    MOZ_ASSERT(aSlice > 0, "nonsensical/useless call with aSlice == 0");
    SmartPtrArray* pointers = static_cast<SmartPtrArray*>(aData);
    uint32_t oldLen = pointers->Length();
    if (oldLen < aSlice) {
      aSlice = oldLen;
    }
    uint32_t newLen = oldLen - aSlice;
    pointers->RemoveElementsAt(newLen, aSlice);
    if (newLen == 0) {
      delete pointers;
      return true;
    }
    return false;
  }
};

template<class T,
         bool isISupports=IsBaseOf<nsISupports, T>::value>
struct DeferredFinalizer
{
  static void
  AddForDeferredFinalization(T* aObject)
  {
    typedef DeferredFinalizerImpl<T> Impl;
    DeferredFinalize(Impl::AppendDeferredFinalizePointer,
                     Impl::DeferredFinalize, aObject);
  }
};

template<class T>
struct DeferredFinalizer<T, true>
{
  static void
  AddForDeferredFinalization(T* aObject)
  {
    DeferredFinalize(reinterpret_cast<nsISupports*>(aObject));
  }
};

template<class T>
static void
AddForDeferredFinalization(T* aObject)
{
  DeferredFinalizer<T>::AddForDeferredFinalization(aObject);
}




template<class T, bool isISupports=IsBaseOf<nsISupports, T>::value>
class GetCCParticipant
{
  
  template<class U>
  static MOZ_CONSTEXPR nsCycleCollectionParticipant*
  GetHelper(int, typename U::NS_CYCLE_COLLECTION_INNERCLASS* dummy=nullptr)
  {
    return T::NS_CYCLE_COLLECTION_INNERCLASS::GetParticipant();
  }
  
  template<class U>
  static MOZ_CONSTEXPR nsCycleCollectionParticipant*
  GetHelper(double)
  {
    return nullptr;
  }

public:
  static MOZ_CONSTEXPR nsCycleCollectionParticipant*
  Get()
  {
    
    
    
    
    return GetHelper<T>(int());
  }
};

template<class T>
class GetCCParticipant<T, true>
{
public:
  static MOZ_CONSTEXPR nsCycleCollectionParticipant*
  Get()
  {
    return nullptr;
  }
};





bool
IsInPrivilegedApp(JSContext* aCx, JSObject* aObj);





bool
IsInCertifiedApp(JSContext* aCx, JSObject* aObj);

void
FinalizeGlobal(JSFreeOp* aFop, JSObject* aObj);

bool
ResolveGlobal(JSContext* aCx, JS::Handle<JSObject*> aObj,
              JS::Handle<jsid> aId, bool* aResolvedp);

bool
EnumerateGlobal(JSContext* aCx, JS::Handle<JSObject*> aObj);

template <class T>
struct CreateGlobalOptions
{
  static MOZ_CONSTEXPR_VAR ProtoAndIfaceCache::Kind ProtoAndIfaceCacheKind =
    ProtoAndIfaceCache::NonWindowLike;
  
  
  static MOZ_CONSTEXPR_VAR bool ForceInitStandardClassesToFalse = true;
  static void TraceGlobal(JSTracer* aTrc, JSObject* aObj)
  {
    mozilla::dom::TraceProtoAndIfaceCache(aTrc, aObj);
  }
  static bool PostCreateGlobal(JSContext* aCx, JS::Handle<JSObject*> aGlobal)
  {
    MOZ_ALWAYS_TRUE(TryPreserveWrapper(aGlobal));

    return true;
  }
};

template <>
struct CreateGlobalOptions<nsGlobalWindow>
{
  static MOZ_CONSTEXPR_VAR ProtoAndIfaceCache::Kind ProtoAndIfaceCacheKind =
    ProtoAndIfaceCache::WindowLike;
  static MOZ_CONSTEXPR_VAR bool ForceInitStandardClassesToFalse = false;
  static void TraceGlobal(JSTracer* aTrc, JSObject* aObj);
  static bool PostCreateGlobal(JSContext* aCx, JS::Handle<JSObject*> aGlobal);
};

nsresult
RegisterDOMNames();



template <class T, ProtoHandleGetter GetProto>
JS::Handle<JSObject*>
CreateGlobal(JSContext* aCx, T* aNative, nsWrapperCache* aCache,
             const JSClass* aClass, JS::CompartmentOptions& aOptions,
             JSPrincipals* aPrincipal, bool aInitStandardClasses,
             JS::MutableHandle<JSObject*> aGlobal)
{
  aOptions.setTrace(CreateGlobalOptions<T>::TraceGlobal);

  aGlobal.set(JS_NewGlobalObject(aCx, aClass, aPrincipal,
                                 JS::DontFireOnNewGlobalHook, aOptions));
  if (!aGlobal) {
    NS_WARNING("Failed to create global");
    return JS::NullPtr();
  }

  JSAutoCompartment ac(aCx, aGlobal);

  {
    js::SetReservedSlot(aGlobal, DOM_OBJECT_SLOT, PRIVATE_TO_JSVAL(aNative));
    NS_ADDREF(aNative);

    aCache->SetWrapper(aGlobal);

    dom::AllocateProtoAndIfaceCache(aGlobal,
                                    CreateGlobalOptions<T>::ProtoAndIfaceCacheKind);

    if (!CreateGlobalOptions<T>::PostCreateGlobal(aCx, aGlobal)) {
      return JS::NullPtr();
    }
  }

  if (aInitStandardClasses &&
      !CreateGlobalOptions<T>::ForceInitStandardClassesToFalse &&
      !JS_InitStandardClasses(aCx, aGlobal)) {
    NS_WARNING("Failed to init standard classes");
    return JS::NullPtr();
  }

  JS::Handle<JSObject*> proto = GetProto(aCx, aGlobal);
  if (!proto || !JS_SplicePrototype(aCx, aGlobal, proto)) {
    NS_WARNING("Failed to set proto");
    return JS::NullPtr();
  }

  return proto;
}





class InternedStringId
{
  jsid id;

 public:
  InternedStringId() : id(JSID_VOID) {}

  bool init(JSContext *cx, const char *string) {
    JSString* str = JS_InternString(cx, string);
    if (!str)
      return false;
    id = INTERNED_STRING_TO_JSID(cx, str);
    return true;
  }

  operator const jsid& () {
    return id;
  }

  operator JS::Handle<jsid> () {
    
    return JS::Handle<jsid>::fromMarkedLocation(&id);
  }
};

bool
GenericBindingGetter(JSContext* cx, unsigned argc, JS::Value* vp);

bool
GenericBindingSetter(JSContext* cx, unsigned argc, JS::Value* vp);

bool
GenericBindingMethod(JSContext* cx, unsigned argc, JS::Value* vp);

bool
GenericPromiseReturningBindingMethod(JSContext* cx, unsigned argc, JS::Value* vp);

bool
StaticMethodPromiseWrapper(JSContext* cx, unsigned argc, JS::Value* vp);









bool
ConvertExceptionToPromise(JSContext* cx,
                          JSObject* promiseScope,
                          JS::MutableHandle<JS::Value> rval);




inline bool
GlobalPropertiesAreOwn()
{
  return true;
}

#ifdef DEBUG
void
AssertReturnTypeMatchesJitinfo(const JSJitInfo* aJitinfo,
                               JS::Handle<JS::Value> aValue);
#endif



bool
CheckPermissions(JSContext* aCx, JSObject* aObj, const char* const aPermissions[]);







bool
EnforceNotInPrerendering(JSContext* aCx, JSObject* aObj);




void
HandlePrerenderingViolation(nsPIDOMWindow* aWindow);

bool
CallerSubsumes(JSObject* aObject);

MOZ_ALWAYS_INLINE bool
CallerSubsumes(JS::Handle<JS::Value> aValue)
{
  if (!aValue.isObject()) {
    return true;
  }
  return CallerSubsumes(&aValue.toObject());
}

template<class T>
inline bool
WrappedJSToDictionary(JSContext* aCx, nsISupports* aObject, T& aDictionary)
{
  nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj = do_QueryInterface(aObject);
  if (!wrappedObj) {
    return false;
  }

  JS::Rooted<JSObject*> obj(aCx, wrappedObj->GetJSObject());
  if (!obj) {
    return false;
  }

  JSAutoCompartment ac(aCx, obj);
  JS::Rooted<JS::Value> v(aCx, JS::ObjectValue(*obj));
  return aDictionary.Init(aCx, v);
}

template<class T>
inline bool
WrappedJSToDictionary(nsISupports* aObject, T& aDictionary)
{
  nsCOMPtr<nsIXPConnectWrappedJS> wrappedObj = do_QueryInterface(aObject);
  NS_ENSURE_TRUE(wrappedObj, false);
  JS::Rooted<JSObject*> obj(CycleCollectedJSRuntime::Get()->Runtime(),
                            wrappedObj->GetJSObject());
  NS_ENSURE_TRUE(obj, false);

  nsIGlobalObject* global = xpc::NativeGlobal(obj);
  NS_ENSURE_TRUE(global, false);

  
  
  AutoEntryScript aes(global, "WebIDL dictionary creation");
  aes.TakeOwnershipOfErrorReporting();

  JS::Rooted<JS::Value> v(aes.cx(), JS::ObjectValue(*obj));
  return aDictionary.Init(aes.cx(), v);
}


template<class T, class S>
inline nsRefPtr<T>
StrongOrRawPtr(already_AddRefed<S>&& aPtr)
{
  return aPtr.template downcast<T>();
}

template<class T,
         class ReturnType=typename Conditional<IsRefcounted<T>::value, T*,
                                               nsAutoPtr<T>>::Type>
inline ReturnType
StrongOrRawPtr(T* aPtr)
{
  return ReturnType(aPtr);
}

template<class T, template<typename> class SmartPtr, class S>
inline void
StrongOrRawPtr(SmartPtr<S>&& aPtr) = delete;

template<class T>
struct StrongPtrForMember
{
  typedef typename Conditional<IsRefcounted<T>::value,
                               nsRefPtr<T>, nsAutoPtr<T>>::Type Type;
};

inline
JSObject*
GetErrorPrototype(JSContext* aCx, JS::Handle<JSObject*> aForObj)
{
  return JS_GetErrorPrototype(aCx);
}




bool SystemGlobalResolve(JSContext* cx, JS::Handle<JSObject*> obj,
                         JS::Handle<jsid> id, bool* resolvedp);




bool SystemGlobalEnumerate(JSContext* cx, JS::Handle<JSObject*> obj);


} 
} 

#endif 
