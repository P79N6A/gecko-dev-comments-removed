








































#ifndef jsproxy_h___
#define jsproxy_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

namespace js {


class JSProxyHandler {
  public:
    virtual ~JSProxyHandler();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       JSPropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                          JSPropertyDescriptor *desc) = 0;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                JSPropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoValueVector &props) = 0;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, JSObject *proxy, js::AutoValueVector &props) = 0;
    virtual bool fix(JSContext *cx, JSObject *proxy, jsval *vp) = 0;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    virtual bool enumerateOwn(JSContext *cx, JSObject *proxy, js::AutoValueVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp);

    
    virtual void finalize(JSContext *cx, JSObject *proxy);
    virtual void trace(JSTracer *trc, JSObject *proxy);
    virtual const void *family() = 0;
};


class JSProxy {
  public:
    
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                      JSPropertyDescriptor *desc);
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, jsval *vp);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                         JSPropertyDescriptor *desc);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, jsval *vp);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, JSPropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, jsval v);
    static bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoValueVector &props);
    static bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool enumerate(JSContext *cx, JSObject *proxy, js::AutoValueVector &props);
    static bool fix(JSContext *cx, JSObject *proxy, jsval *vp);

    
    static bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    static bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    static bool enumerateOwn(JSContext *cx, JSObject *proxy, js::AutoValueVector &props);
    static bool iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp);
};


const uint32 JSSLOT_PROXY_HANDLER = JSSLOT_PRIVATE + 0;

const uint32 JSSLOT_PROXY_CLASS = JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_PROXY_PRIVATE = JSSLOT_PRIVATE + 2;

const uint32 JSSLOT_PROXY_CALL = JSSLOT_PRIVATE + 1;
const uint32 JSSLOT_PROXY_CONSTRUCT = JSSLOT_PRIVATE + 2;

extern JS_FRIEND_API(JSClass) ObjectProxyClass;
extern JS_FRIEND_API(JSClass) FunctionProxyClass;
extern JSClass CallableObjectClass;

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

inline jsval
JSObject::getProxyHandler() const
{
    JS_ASSERT(isProxy());
    jsval handler = fslots[js::JSSLOT_PROXY_HANDLER];
    JS_ASSERT(JSVAL_IS_OBJECT(handler) || JSVAL_IS_INT(handler));
    return handler;
}

inline jsval
JSObject::getProxyPrivate() const
{
    JS_ASSERT(isObjectProxy());
    return fslots[js::JSSLOT_PROXY_PRIVATE];
}

inline void
JSObject::setProxyPrivate(jsval priv)
{
    JS_ASSERT(isObjectProxy());
    fslots[js::JSSLOT_PROXY_PRIVATE] = priv;
}

namespace js {

JS_FRIEND_API(JSObject *)
NewObjectProxy(JSContext *cx, jsval handler, JSObject *proto, JSObject *parent,
               JSString *className);

JS_FRIEND_API(JSObject *)
NewFunctionProxy(JSContext *cx, jsval handler, JSObject *proto, JSObject *parent,
                 JSObject *call, JSObject *construct);

JS_FRIEND_API(JSBool)
GetProxyObjectClass(JSContext *cx, JSObject *proxy, const char **namep);

JS_FRIEND_API(JSBool)
FixProxy(JSContext *cx, JSObject *proxy, JSBool *bp);

}

JS_BEGIN_EXTERN_C

extern JSClass js_ProxyClass;

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif
