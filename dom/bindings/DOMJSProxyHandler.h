




#ifndef mozilla_dom_DOMJSProxyHandler_h
#define mozilla_dom_DOMJSProxyHandler_h

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"

#include "jsapi.h"
#include "jsproxy.h"
#include "nsString.h"

#define DOM_PROXY_OBJECT_SLOT js::PROXY_PRIVATE_SLOT

namespace mozilla {
namespace dom {

enum {
  JSPROXYSLOT_EXPANDO = 0
};

template<typename T> struct Prefable;


extern const char HandlerFamily;
inline const void* ProxyFamily() { return &HandlerFamily; }

inline bool IsDOMProxy(JSObject *obj)
{
    const js::Class* clasp = js::GetObjectClass(obj);
    return clasp->isProxy() &&
           js::GetProxyHandler(obj)->family() == ProxyFamily();
}

class BaseDOMProxyHandler : public js::BaseProxyHandler
{
public:
  BaseDOMProxyHandler(const void* aProxyFamily, bool aHasPrototype = false)
    : js::BaseProxyHandler(aProxyFamily, aHasPrototype)
  {}

  
  
  bool enumerate(JSContext* cx, JS::Handle<JSObject*> proxy,
                 JS::AutoIdVector& props) const MOZ_OVERRIDE;
  bool getPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> proxy,
                             JS::Handle<jsid> id,
                             JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE;

  bool watch(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
             JS::Handle<JSObject*> callable) const MOZ_OVERRIDE;
  bool unwatch(JSContext* cx, JS::Handle<JSObject*> proxy,
               JS::Handle<jsid> id) const MOZ_OVERRIDE;
  virtual bool getOwnPropertyNames(JSContext* cx, JS::Handle<JSObject*> proxy,
                                   JS::AutoIdVector &props) const MOZ_OVERRIDE;
  
  
  
  
  virtual bool keys(JSContext* cx, JS::Handle<JSObject*> proxy,
                    JS::AutoIdVector &props) const MOZ_OVERRIDE;

protected:
  
  
  
  virtual bool ownPropNames(JSContext* cx, JS::Handle<JSObject*> proxy,
                            unsigned flags,
                            JS::AutoIdVector& props) const = 0;
};

class DOMProxyHandler : public BaseDOMProxyHandler
{
public:
  DOMProxyHandler()
    : BaseDOMProxyHandler(ProxyFamily())
  {
  }

  bool preventExtensions(JSContext *cx, JS::Handle<JSObject*> proxy) const MOZ_OVERRIDE;
  bool defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                      JS::MutableHandle<JSPropertyDescriptor> desc) const MOZ_OVERRIDE
  {
    bool unused;
    return defineProperty(cx, proxy, id, desc, &unused);
  }
  virtual bool defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                              JS::MutableHandle<JSPropertyDescriptor> desc, bool* defined)
                              const;
  bool set(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<JSObject*> receiver,
           JS::Handle<jsid> id, bool strict, JS::MutableHandle<JS::Value> vp)
           const MOZ_OVERRIDE;
  bool delete_(JSContext* cx, JS::Handle<JSObject*> proxy,
               JS::Handle<jsid> id, bool* bp) const MOZ_OVERRIDE;
  bool has(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
           bool* bp) const MOZ_OVERRIDE;
  bool isExtensible(JSContext *cx, JS::Handle<JSObject*> proxy, bool *extensible)
                    const MOZ_OVERRIDE;

  




  virtual bool setCustom(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                         JS::MutableHandle<JS::Value> vp, bool *done) const;

  static JSObject* GetExpandoObject(JSObject* obj)
  {
    MOZ_ASSERT(IsDOMProxy(obj), "expected a DOM proxy object");
    JS::Value v = js::GetProxyExtra(obj, JSPROXYSLOT_EXPANDO);
    if (v.isObject()) {
      return &v.toObject();
    }

    if (v.isUndefined()) {
      return nullptr;
    }

    js::ExpandoAndGeneration* expandoAndGeneration =
      static_cast<js::ExpandoAndGeneration*>(v.toPrivate());
    v = expandoAndGeneration->expando;
    return v.isUndefined() ? nullptr : &v.toObject();
  }
  
  static JSObject* GetAndClearExpandoObject(JSObject* obj);
  static JSObject* EnsureExpandoObject(JSContext* cx,
                                       JS::Handle<JSObject*> obj);
};

inline DOMProxyHandler*
GetDOMProxyHandler(JSObject* obj)
{
  MOZ_ASSERT(IsDOMProxy(obj));
  return static_cast<DOMProxyHandler*>(js::GetProxyHandler(obj));
}

extern jsid s_length_id;

int32_t IdToInt32(JSContext* cx, JS::Handle<jsid> id);



inline int32_t
GetArrayIndexFromId(JSContext* cx, JS::Handle<jsid> id)
{
  if (MOZ_LIKELY(JSID_IS_INT(id))) {
    return JSID_TO_INT(id);
  }
  if (MOZ_LIKELY(id == s_length_id)) {
    return -1;
  }
  if (MOZ_LIKELY(JSID_IS_ATOM(id))) {
    JSAtom* atom = JSID_TO_ATOM(id);
    jschar s = *js::GetAtomChars(atom);
    if (MOZ_LIKELY((unsigned)s >= 'a' && (unsigned)s <= 'z'))
      return -1;

    uint32_t i;
    JSLinearString* str = js::AtomToLinearString(JSID_TO_ATOM(id));
    return js::StringIsArrayIndex(str, &i) ? i : -1;
  }
  return IdToInt32(cx, id);
}

inline bool
IsArrayIndex(int32_t index)
{
  return index >= 0;
}

inline void
FillPropertyDescriptor(JS::MutableHandle<JSPropertyDescriptor> desc,
                       JSObject* obj, bool readonly, bool enumerable = true)
{
  desc.object().set(obj);
  desc.setAttributes((readonly ? JSPROP_READONLY : 0) |
                     (enumerable ? JSPROP_ENUMERATE : 0));
  desc.setGetter(nullptr);
  desc.setSetter(nullptr);
}

inline void
FillPropertyDescriptor(JS::MutableHandle<JSPropertyDescriptor> desc,
                       JSObject* obj, JS::Value v,
                       bool readonly, bool enumerable = true)
{
  desc.value().set(v);
  FillPropertyDescriptor(desc, obj, readonly, enumerable);
}

inline void
FillPropertyDescriptor(JS::MutableHandle<JSPropertyDescriptor> desc,
                       JSObject* obj, unsigned attributes, JS::Value v)
{
  desc.object().set(obj);
  desc.value().set(v);
  desc.setAttributes(attributes);
  desc.setGetter(nullptr);
  desc.setSetter(nullptr);
}

} 
} 

#endif 
