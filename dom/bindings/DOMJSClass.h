




#ifndef mozilla_dom_DOMJSClass_h
#define mozilla_dom_DOMJSClass_h

#include "jsfriendapi.h"
#include "mozilla/Assertions.h"

#include "mozilla/dom/PrototypeList.h" 

#include "mozilla/dom/JSSlots.h"

class nsCycleCollectionParticipant;


#define DOM_PROTOTYPE_SLOT JSCLASS_GLOBAL_SLOT_COUNT


#define DOM_GLOBAL_SLOTS 1


#define JSCLASS_DOM_GLOBAL JSCLASS_USERBIT1
#define JSCLASS_IS_DOMIFACEANDPROTOJSCLASS JSCLASS_USERBIT2

namespace mozilla {
namespace dom {

typedef bool
(* ResolveOwnProperty)(JSContext* cx, JS::Handle<JSObject*> wrapper,
                       JS::Handle<JSObject*> obj, JS::Handle<jsid> id,
                       JS::MutableHandle<JSPropertyDescriptor> desc);

typedef bool
(* EnumerateOwnProperties)(JSContext* cx, JS::Handle<JSObject*> wrapper,
                           JS::Handle<JSObject*> obj,
                           JS::AutoIdVector& props);

bool
CheckPermissions(JSContext* aCx, JSObject* aObj, const char* const aPermissions[]);

struct ConstantSpec
{
  const char* name;
  JS::Value value;
};

typedef bool (*PropertyEnabled)(JSContext* cx, JSObject* global);

template<typename T>
struct Prefable {
  inline bool isEnabled(JSContext* cx, JSObject* obj) const {
    if (!enabled) {
      return false;
    }
    if (!enabledFunc && !availableFunc && !checkPermissions) {
      return true;
    }
    
    JS::Rooted<JSObject*> rootedObj(cx, obj);
    if (enabledFunc &&
        !enabledFunc(cx, js::GetGlobalForObjectCrossCompartment(rootedObj))) {
      return false;
    }
    if (availableFunc &&
        !availableFunc(cx, js::GetGlobalForObjectCrossCompartment(rootedObj))) {
      return false;
    }
    if (checkPermissions &&
        !CheckPermissions(cx, js::GetGlobalForObjectCrossCompartment(rootedObj),
                          checkPermissions)) {
      return false;
    }
    return true;
  }

  
  bool enabled;
  
  
  
  PropertyEnabled enabledFunc;
  
  
  
  
  PropertyEnabled availableFunc;
  const char* const* checkPermissions;
  
  
  
  const T* specs;
};

struct NativeProperties
{
  const Prefable<const JSFunctionSpec>* staticMethods;
  jsid* staticMethodIds;
  const JSFunctionSpec* staticMethodsSpecs;
  const Prefable<const JSPropertySpec>* staticAttributes;
  jsid* staticAttributeIds;
  const JSPropertySpec* staticAttributeSpecs;
  const Prefable<const JSFunctionSpec>* methods;
  jsid* methodIds;
  const JSFunctionSpec* methodsSpecs;
  const Prefable<const JSPropertySpec>* attributes;
  jsid* attributeIds;
  const JSPropertySpec* attributeSpecs;
  const Prefable<const JSPropertySpec>* unforgeableAttributes;
  jsid* unforgeableAttributeIds;
  const JSPropertySpec* unforgeableAttributeSpecs;
  const Prefable<const ConstantSpec>* constants;
  jsid* constantIds;
  const ConstantSpec* constantSpecs;
};

struct NativePropertiesHolder
{
  const NativeProperties* regular;
  const NativeProperties* chromeOnly;
};




struct NativePropertyHooks
{
  
  
  ResolveOwnProperty mResolveOwnProperty;
  
  
  EnumerateOwnProperties mEnumerateOwnProperties;

  
  NativePropertiesHolder mNativeProperties;

  
  
  
  prototypes::ID mPrototypeID;

  
  
  
  constructors::ID mConstructorID;

  
  const NativePropertyHooks* mProtoHooks;
};

enum DOMObjectType {
  eInstance,
  eInterface,
  eInterfacePrototype
};

typedef JSObject* (*ParentGetter)(JSContext* aCx, JS::Handle<JSObject*> aObj);






typedef JS::Handle<JSObject*> (*ProtoGetter)(JSContext* aCx,
                                             JS::Handle<JSObject*> aGlobal);

struct DOMClass
{
  
  
  const prototypes::ID mInterfaceChain[MAX_PROTOTYPE_CHAIN_LENGTH];

  
  
  
  
  const bool mDOMObjectIsISupports;

  const NativePropertyHooks* mNativeHooks;

  ParentGetter mGetParent;
  ProtoGetter mGetProto;

  
  
  
  nsCycleCollectionParticipant* mParticipant;
};


struct DOMJSClass
{
  
  
  
  const js::Class mBase;

  const DOMClass mClass;

  static const DOMJSClass* FromJSClass(const JSClass* base) {
    MOZ_ASSERT(base->flags & JSCLASS_IS_DOMJSCLASS);
    return reinterpret_cast<const DOMJSClass*>(base);
  }

  static const DOMJSClass* FromJSClass(const js::Class* base) {
    return FromJSClass(Jsvalify(base));
  }

  const JSClass* ToJSClass() const { return Jsvalify(&mBase); }
};


struct DOMIfaceAndProtoJSClass
{
  
  
  
  
  const JSClass mBase;

  
  DOMObjectType mType;

  const NativePropertyHooks* mNativeHooks;

  
  
  const char* mToString;

  const prototypes::ID mPrototypeID;
  const uint32_t mDepth;

  static const DOMIfaceAndProtoJSClass* FromJSClass(const JSClass* base) {
    MOZ_ASSERT(base->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS);
    return reinterpret_cast<const DOMIfaceAndProtoJSClass*>(base);
  }
  static const DOMIfaceAndProtoJSClass* FromJSClass(const js::Class* base) {
    return FromJSClass(Jsvalify(base));
  }

  const JSClass* ToJSClass() const { return &mBase; }
};

class ProtoAndIfaceCache;

inline bool
HasProtoAndIfaceCache(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  
  return !js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).isUndefined();
}

inline ProtoAndIfaceCache*
GetProtoAndIfaceCache(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  return static_cast<ProtoAndIfaceCache*>(
    js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).toPrivate());
}

} 
} 

#endif 
