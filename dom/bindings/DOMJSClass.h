




#ifndef mozilla_dom_DOMJSClass_h
#define mozilla_dom_DOMJSClass_h

#include "jsapi.h"
#include "jsfriendapi.h"
#include "mozilla/Assertions.h"

#include "mozilla/dom/PrototypeList.h" 

class nsCycleCollectionParticipant;



#define DOM_OBJECT_SLOT 0




#define DOM_XRAY_EXPANDO_SLOT 1





#define DOM_OBJECT_SLOT_SOW 2


#define DOM_PROTOTYPE_SLOT JSCLASS_GLOBAL_SLOT_COUNT


#define JSCLASS_DOM_GLOBAL JSCLASS_USERBIT1
#define JSCLASS_IS_DOMIFACEANDPROTOJSCLASS JSCLASS_USERBIT2




#define DOM_PROTO_INSTANCE_CLASS_SLOT 0



#define DOM_INTERFACE_SLOTS_BASE (DOM_XRAY_EXPANDO_SLOT + 1)

MOZ_STATIC_ASSERT(DOM_PROTO_INSTANCE_CLASS_SLOT != DOM_XRAY_EXPANDO_SLOT,
                  "Interface prototype object use both of these, so they must "
                  "not be the same slot.");

namespace mozilla {
namespace dom {

typedef bool
(* ResolveOwnProperty)(JSContext* cx, JSObject* wrapper, JSObject* obj, jsid id,
                       JSPropertyDescriptor* desc, unsigned flags);

typedef bool
(* EnumerateOwnProperties)(JSContext* cx, JSObject* wrapper, JSObject* obj,
                           JS::AutoIdVector& props);

struct ConstantSpec
{
  const char* name;
  JS::Value value;
};

typedef bool (*PropertyEnabled)(JSContext* cx, JSObject* global);

template<typename T>
struct Prefable {
  inline bool isEnabled(JSContext* cx, JSObject* obj) {
    return enabled &&
      (!enabledFunc ||
       enabledFunc(cx, js::GetGlobalForObjectCrossCompartment(obj)));
  }

  
  bool enabled;
  
  
  
  PropertyEnabled enabledFunc;
  
  
  
  T* specs;
};

struct NativeProperties
{
  Prefable<JSFunctionSpec>* staticMethods;
  jsid* staticMethodIds;
  JSFunctionSpec* staticMethodsSpecs;
  Prefable<JSPropertySpec>* staticAttributes;
  jsid* staticAttributeIds;
  JSPropertySpec* staticAttributeSpecs;
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

typedef JSObject* (*ParentGetter)(JSContext* aCx, JSObject* aObj);
typedef JSObject* (*ProtoGetter)(JSContext* aCx, JSObject* aGlobal);

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
  
  
  
  JSClass mBase;

  DOMClass mClass;

  static DOMJSClass* FromJSClass(JSClass* base) {
    MOZ_ASSERT(base->flags & JSCLASS_IS_DOMJSCLASS);
    return reinterpret_cast<DOMJSClass*>(base);
  }
  static const DOMJSClass* FromJSClass(const JSClass* base) {
    MOZ_ASSERT(base->flags & JSCLASS_IS_DOMJSCLASS);
    return reinterpret_cast<const DOMJSClass*>(base);
  }

  static DOMJSClass* FromJSClass(js::Class* base) {
    return FromJSClass(Jsvalify(base));
  }
  static const DOMJSClass* FromJSClass(const js::Class* base) {
    return FromJSClass(Jsvalify(base));
  }

  JSClass* ToJSClass() { return &mBase; }
};


struct DOMIfaceAndProtoJSClass
{
  
  
  
  
  JSClass mBase;

  
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

  JSClass* ToJSClass() { return &mBase; }
};

inline bool
HasProtoAndIfaceArray(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  
  return !js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).isUndefined();
}

inline JSObject**
GetProtoAndIfaceArray(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  return static_cast<JSObject**>(
    js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).toPrivate());
}

} 
} 

#endif 
