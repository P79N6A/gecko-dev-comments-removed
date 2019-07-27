




#ifndef mozilla_dom_DOMJSProxyHandler_h
#define mozilla_dom_DOMJSProxyHandler_h

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"

#include "jsapi.h"
#include "js/Proxy.h"
#include "nsString.h"

#define DOM_PROXY_OBJECT_SLOT js::PROXY_PRIVATE_SLOT

namespace mozilla {
namespace dom {

enum {
  


















  JSPROXYSLOT_EXPANDO = 0
};

template<typename T> struct Prefable;

class BaseDOMProxyHandler : public js::BaseProxyHandler
{
public:
  explicit MOZ_CONSTEXPR BaseDOMProxyHandler(const void* aProxyFamily, bool aHasPrototype = false)
    : js::BaseProxyHandler(aProxyFamily, aHasPrototype)
  {}

  
  
  bool getOwnPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> proxy,
                                JS::Handle<jsid> id,
                                JS::MutableHandle<JSPropertyDescriptor> desc) const override;
  virtual bool ownPropertyKeys(JSContext* cx, JS::Handle<JSObject*> proxy,
                               JS::AutoIdVector &props) const override;

  virtual bool enumerate(JSContext *cx, JS::Handle<JSObject*> proxy,
                         JS::MutableHandle<JSObject*> objp) const override;

  
  
  
  
  virtual bool getOwnEnumerablePropertyKeys(JSContext* cx, JS::Handle<JSObject*> proxy,
                                            JS::AutoIdVector &props) const override;

  bool watch(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
             JS::Handle<JSObject*> callable) const override;
  bool unwatch(JSContext* cx, JS::Handle<JSObject*> proxy,
               JS::Handle<jsid> id) const override;

protected:
  
  
  
  
  virtual bool ownPropNames(JSContext* cx, JS::Handle<JSObject*> proxy,
                            unsigned flags,
                            JS::AutoIdVector& props) const = 0;

  
  
  
  
  virtual bool getOwnPropDescriptor(JSContext* cx,
                                    JS::Handle<JSObject*> proxy,
                                    JS::Handle<jsid> id,
                                    bool ignoreNamedProps,
                                    JS::MutableHandle<JSPropertyDescriptor> desc) const = 0;
};

class DOMProxyHandler : public BaseDOMProxyHandler
{
public:
  MOZ_CONSTEXPR DOMProxyHandler()
    : BaseDOMProxyHandler(&family)
  {}

  bool defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                      JS::Handle<JSPropertyDescriptor> desc,
                      JS::ObjectOpResult &result) const override
  {
    bool unused;
    return defineProperty(cx, proxy, id, desc, result, &unused);
  }
  virtual bool defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                              JS::Handle<JSPropertyDescriptor> desc,
                              JS::ObjectOpResult &result, bool *defined) const;
  bool delete_(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
               JS::ObjectOpResult &result) const override;
  bool preventExtensions(JSContext* cx, JS::Handle<JSObject*> proxy,
                         JS::ObjectOpResult& result) const override;
  bool isExtensible(JSContext *cx, JS::Handle<JSObject*> proxy, bool *extensible)
                    const override;
  bool has(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
           bool* bp) const override;
  bool set(JSContext *cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
           JS::Handle<JS::Value> v, JS::Handle<JS::Value> receiver, JS::ObjectOpResult &result)
           const override;

  




  virtual bool setCustom(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                         JS::Handle<JS::Value> v, bool *done) const;

  static JSObject* GetExpandoObject(JSObject* obj);

  
  static JSObject* GetAndClearExpandoObject(JSObject* obj);
  static JSObject* EnsureExpandoObject(JSContext* cx,
                                       JS::Handle<JSObject*> obj);

  static const char family;
};

inline bool IsDOMProxy(JSObject *obj)
{
    const js::Class* clasp = js::GetObjectClass(obj);
    return clasp->isProxy() &&
           js::GetProxyHandler(obj)->family() == &DOMProxyHandler::family;
}

inline const DOMProxyHandler*
GetDOMProxyHandler(JSObject* obj)
{
  MOZ_ASSERT(IsDOMProxy(obj));
  return static_cast<const DOMProxyHandler*>(js::GetProxyHandler(obj));
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
    char16_t s;
    {
      JS::AutoCheckCannotGC nogc;
      if (js::AtomHasLatin1Chars(atom)) {
        s = *js::GetLatin1AtomChars(nogc, atom);
      } else {
        s = *js::GetTwoByteAtomChars(nogc, atom);
      }
    }
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
