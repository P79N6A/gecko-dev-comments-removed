





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
CheckAnyPermissions(JSContext* aCx, JSObject* aObj, const char* const aPermissions[]);

bool
CheckAllPermissions(JSContext* aCx, JSObject* aObj, const char* const aPermissions[]);

struct ConstantSpec
{
  const char* name;
  JS::Value value;
};

typedef bool (*PropertyEnabled)(JSContext* cx, JSObject* global);

template<typename T>
struct Prefable {
  inline bool isEnabled(JSContext* cx, JS::Handle<JSObject*> obj) const {
    if (!enabled) {
      return false;
    }
    if (!enabledFunc && !availableFunc && !checkAnyPermissions && !checkAllPermissions) {
      return true;
    }
    if (enabledFunc &&
        !enabledFunc(cx, js::GetGlobalForObjectCrossCompartment(obj))) {
      return false;
    }
    if (availableFunc &&
        !availableFunc(cx, js::GetGlobalForObjectCrossCompartment(obj))) {
      return false;
    }
    if (checkAnyPermissions &&
        !CheckAnyPermissions(cx, js::GetGlobalForObjectCrossCompartment(obj),
                             checkAnyPermissions)) {
      return false;
    }
    if (checkAllPermissions &&
        !CheckAllPermissions(cx, js::GetGlobalForObjectCrossCompartment(obj),
                             checkAllPermissions)) {
      return false;
    }
    return true;
  }

  
  bool enabled;
  
  
  
  PropertyEnabled enabledFunc;
  
  
  
  
  PropertyEnabled availableFunc;
  const char* const* checkAnyPermissions;
  const char* const* checkAllPermissions;
  
  
  
  const T* specs;
};

struct NativeProperties
{
  const Prefable<const JSFunctionSpec>* staticMethods;
  jsid* staticMethodIds;
  const JSFunctionSpec* staticMethodSpecs;

  const Prefable<const JSPropertySpec>* staticAttributes;
  jsid* staticAttributeIds;
  const JSPropertySpec* staticAttributeSpecs;

  const Prefable<const JSFunctionSpec>* methods;
  jsid* methodIds;
  const JSFunctionSpec* methodSpecs;

  const Prefable<const JSPropertySpec>* attributes;
  jsid* attributeIds;
  const JSPropertySpec* attributeSpecs;

  const Prefable<const JSFunctionSpec>* unforgeableMethods;
  jsid* unforgeableMethodIds;
  const JSFunctionSpec* unforgeableMethodSpecs;

  const Prefable<const JSPropertySpec>* unforgeableAttributes;
  jsid* unforgeableAttributeIds;
  const JSPropertySpec* unforgeableAttributeSpecs;

  const Prefable<const ConstantSpec>* constants;
  jsid* constantIds;
  const ConstantSpec* constantSpecs;

  
  int32_t iteratorAliasMethodIndex;
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
  eGlobalInstance,
  eInterface,
  eInterfacePrototype,
  eGlobalInterfacePrototype,
  eNamedPropertiesObject
};

inline
bool
IsInstance(DOMObjectType type)
{
  return type == eInstance || type == eGlobalInstance;
}

inline
bool
IsInterfacePrototype(DOMObjectType type)
{
  return type == eInterfacePrototype || type == eGlobalInterfacePrototype;
}

typedef JSObject* (*ParentGetter)(JSContext* aCx, JS::Handle<JSObject*> aObj);

typedef JSObject* (*ProtoGetter)(JSContext* aCx,
                                 JS::Handle<JSObject*> aGlobal);






typedef JS::Handle<JSObject*> (*ProtoHandleGetter)(JSContext* aCx,
                                                   JS::Handle<JSObject*> aGlobal);


struct DOMJSClass
{
  
  
  
  const js::Class mBase;

  
  
  const prototypes::ID mInterfaceChain[MAX_PROTOTYPE_CHAIN_LENGTH];

  
  
  
  
  const bool mDOMObjectIsISupports;

  const NativePropertyHooks* mNativeHooks;

  ParentGetter mGetParent;
  ProtoHandleGetter mGetProto;

  
  
  
  nsCycleCollectionParticipant* mParticipant;

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
  
  
  
  
  const js::Class mBase;

  
  
  DOMObjectType mType;

  const NativePropertyHooks* mNativeHooks;

  
  
  const char* mToString;

  const prototypes::ID mPrototypeID;
  const uint32_t mDepth;

  ProtoGetter mGetParentProto;

  static const DOMIfaceAndProtoJSClass* FromJSClass(const JSClass* base) {
    MOZ_ASSERT(base->flags & JSCLASS_IS_DOMIFACEANDPROTOJSCLASS);
    return reinterpret_cast<const DOMIfaceAndProtoJSClass*>(base);
  }
  static const DOMIfaceAndProtoJSClass* FromJSClass(const js::Class* base) {
    return FromJSClass(Jsvalify(base));
  }

  const JSClass* ToJSClass() const { return Jsvalify(&mBase); }
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
