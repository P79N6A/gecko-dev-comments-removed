




#ifndef mozilla_dom_DOMJSProxyHandler_h
#define mozilla_dom_DOMJSProxyHandler_h

#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "jsproxy.h"
#include "xpcpublic.h"
#include "nsStringGlue.h"

#define DOM_PROXY_OBJECT_SLOT js::JSSLOT_PROXY_PRIVATE

namespace mozilla {
namespace dom {

enum {
  JSPROXYSLOT_EXPANDO = 0,
  JSPROXYSLOT_XRAY_EXPANDO
};

template<typename T> struct Prefable;

class DOMProxyHandler : public js::BaseProxyHandler
{
public:
  DOMProxyHandler(const DOMClass& aClass)
    : js::BaseProxyHandler(ProxyFamily()),
      mClass(aClass)
  {
  }

  bool preventExtensions(JSContext *cx, JS::Handle<JSObject*> proxy) MOZ_OVERRIDE;
  bool getPropertyDescriptor(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                            JSPropertyDescriptor* desc, unsigned flags) MOZ_OVERRIDE;
  bool defineProperty(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id,
                      JSPropertyDescriptor* desc) MOZ_OVERRIDE;
  bool delete_(JSContext* cx, JS::Handle<JSObject*> proxy,
               JS::Handle<jsid> id, bool* bp) MOZ_OVERRIDE;
  bool enumerate(JSContext* cx, JS::Handle<JSObject*> proxy, JS::AutoIdVector& props) MOZ_OVERRIDE;
  bool has(JSContext* cx, JS::Handle<JSObject*> proxy, JS::Handle<jsid> id, bool* bp) MOZ_OVERRIDE;
  bool isExtensible(JSObject *proxy) MOZ_OVERRIDE;

  static JSObject* GetExpandoObject(JSObject* obj)
  {
    MOZ_ASSERT(IsDOMProxy(obj), "expected a DOM proxy object");
    JS::Value v = js::GetProxyExtra(obj, JSPROXYSLOT_EXPANDO);
    return v.isUndefined() ? NULL : v.toObjectOrNull();
  }
  static JSObject* GetAndClearExpandoObject(JSObject* obj);
  static JSObject* EnsureExpandoObject(JSContext* cx,
                                       JS::Handle<JSObject*> obj);

  const DOMClass& mClass;

protected:
  
  
  bool AppendNamedPropertyIds(JSContext* cx, JS::Handle<JSObject*> proxy,
                              nsTArray<nsString>& names,
                              JS::AutoIdVector& props);
};

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
FillPropertyDescriptor(JSPropertyDescriptor* desc, JSObject* obj, bool readonly)
{
  desc->obj = obj;
  desc->attrs = (readonly ? JSPROP_READONLY : 0) | JSPROP_ENUMERATE;
  desc->getter = NULL;
  desc->setter = NULL;
  desc->shortid = 0;
}

inline void
FillPropertyDescriptor(JSPropertyDescriptor* desc, JSObject* obj, JS::Value v, bool readonly)
{
  desc->value = v;
  FillPropertyDescriptor(desc, obj, readonly);
}

JSObject*
EnsureExpandoObject(JSContext* cx, JS::Handle<JSObject*> obj);

} 
} 

#endif 
