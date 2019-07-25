








































#ifndef jsproxy_h___
#define jsproxy_h___

#include "jsapi.h"
#include "jsobj.h"

namespace js {


class JSProxyHandler {
  public:
    virtual ~JSProxyHandler();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                          PropertyDescriptor *desc) = 0;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, JSIdArray **idap) = 0;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, JSObject *proxy, JSIdArray **idap) = 0;
    virtual bool fix(JSContext *cx, JSObject *proxy, Value *vp) = 0;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    virtual bool enumerateOwn(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);

    
    virtual void finalize(JSContext *cx, JSObject *proxy);
    virtual void trace(JSTracer *trc, JSObject *proxy);
    virtual const void *family() = 0;
};


class JSProxy {
  public:
    
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                      PropertyDescriptor *desc);
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, Value *vp);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                         PropertyDescriptor *desc);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, Value *vp);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, PropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, const Value &v);
    static bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    static bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool enumerate(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    static bool fix(JSContext *cx, JSObject *proxy, Value *vp);

    
    static bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    static bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    static bool enumerateOwn(JSContext *cx, JSObject *proxy, JSIdArray **idap);
    static bool iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);
};


const uint32 JSSLOT_PROXY_HANDLER = JSSLOT_PRIVATE + 0;

const uint32 JSSLOT_PROXY_CLASS = JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_PROXY_PRIVATE = JSSLOT_PRIVATE + 2;

const uint32 JSSLOT_PROXY_CALL = JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_PROXY_CONSTRUCT = JSSLOT_PRIVATE + 2;

extern JS_FRIEND_API(js::Class) ObjectProxyClass;
extern JS_FRIEND_API(js::Class) FunctionProxyClass;
extern js::Class CallableObjectClass;

}

inline bool
JSObject::isObjectProxy() const
{
    return getClass() == &js::ObjectProxyClass;
}

inline bool
JSObject::isFunctionProxy() const
{
    return getClass() == &js::FunctionProxyClass;
}

inline bool
JSObject::isProxy() const
{
    return isObjectProxy() || isFunctionProxy();
}

inline const js::Value &
JSObject::getProxyHandler() const
{
    JS_ASSERT(isProxy());
    const js::Value &handler = fslots[js::JSSLOT_PROXY_HANDLER];
    JS_ASSERT(handler.isObject() || handler.isInt32());
    return handler;
}

inline const js::Value &
JSObject::getProxyPrivate() const
{
    JS_ASSERT(isObjectProxy());
    return fslots[js::JSSLOT_PROXY_PRIVATE];
}

inline void
JSObject::setProxyPrivate(const js::Value &priv)
{
    JS_ASSERT(isObjectProxy());
    fslots[js::JSSLOT_PROXY_PRIVATE] = priv;
}

namespace js {

JS_FRIEND_API(JSObject *)
NewObjectProxy(JSContext *cx, const Value &handler, JSObject *proto, JSObject *parent,
               JSString *className);

JS_FRIEND_API(JSObject *)
NewFunctionProxy(JSContext *cx, const Value &handler, JSObject *proto, JSObject *parent,
                 JSObject *call, JSObject *construct);

JS_FRIEND_API(JSBool)
GetProxyObjectClass(JSContext *cx, JSObject *proxy, const char **namep);

JS_FRIEND_API(JSBool)
FixProxy(JSContext *cx, JSObject *proxy, JSBool *bp);

}

JS_BEGIN_EXTERN_C

extern js::Class js_ProxyClass;

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif
