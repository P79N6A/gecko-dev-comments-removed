




#ifndef mozilla_dom_DOMJSClass_h
#define mozilla_dom_DOMJSClass_h

#include "jsapi.h"
#include "jsfriendapi.h"

#include "mozilla/dom/PrototypeList.h" 



#define DOM_OBJECT_SLOT 0




#define DOM_PROTOTYPE_SLOT (JSCLASS_GLOBAL_SLOT_COUNT + 1)


#define JSCLASS_DOM_GLOBAL JSCLASS_USERBIT1




#define DOM_PROTO_INSTANCE_CLASS_SLOT 0

namespace mozilla {
namespace dom {

typedef bool
(* ResolveProperty)(JSContext* cx, JSObject* wrapper, jsid id, bool set,
                    JSPropertyDescriptor* desc);
typedef bool
(* EnumerateProperties)(JSContext* cx, JSObject* wrapper,
                        JS::AutoIdVector& props);

struct NativePropertyHooks
{
  ResolveProperty mResolveOwnProperty;
  ResolveProperty mResolveProperty;
  EnumerateProperties mEnumerateOwnProperties;
  EnumerateProperties mEnumerateProperties;

  const NativePropertyHooks *mProtoHooks;
};

struct DOMClass
{
  
  
  const prototypes::ID mInterfaceChain[prototypes::id::_ID_Count];

  
  
  
  
  const bool mDOMObjectIsISupports;

  const NativePropertyHooks* mNativeHooks;
};


struct DOMJSClass
{
  
  
  
  JSClass mBase;

  DOMClass mClass;

  
  
  
  
  const int16_t mGetWrapperCacheVTableOffset;

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

inline bool
HasProtoOrIfaceArray(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  
  return !js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).isUndefined();
}

inline JSObject**
GetProtoOrIfaceArray(JSObject* global)
{
  MOZ_ASSERT(js::GetObjectClass(global)->flags & JSCLASS_DOM_GLOBAL);
  return static_cast<JSObject**>(
    js::GetReservedSlot(global, DOM_PROTOTYPE_SLOT).toPrivate());
}

} 
} 

#endif 
