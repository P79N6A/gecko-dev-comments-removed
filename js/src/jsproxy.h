








































#ifndef jsproxy_h___
#define jsproxy_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsobj.h"

namespace js {


class JS_FRIEND_API(JSProxyHandler) {
    void *mFamily;
  public:
    explicit JSProxyHandler(void *family);
    virtual ~JSProxyHandler();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                       PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                          PropertyDescriptor *desc) = 0;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoIdVector &props) = 0;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, JSObject *proxy, js::AutoIdVector &props) = 0;
    virtual bool fix(JSContext *cx, JSObject *proxy, Value *vp) = 0;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
                     js::Value *vp);
    virtual bool keys(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, js::Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *proxy, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *proxy,
                                          uintN argc, js::Value *argv, js::Value *rval);
    virtual bool hasInstance(JSContext *cx, JSObject *proxy, const js::Value *vp, bool *bp);
    virtual JSType typeOf(JSContext *cx, JSObject *proxy);
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy);
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy, uintN indent);
    virtual bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, js::Value *vp);
    virtual void finalize(JSContext *cx, JSObject *proxy);
    virtual void trace(JSTracer *trc, JSObject *proxy);

    virtual bool isOuterWindow() {
        return false;
    }

    inline void *family() {
        return mFamily;
    }
};


class JSProxy {
  public:
    
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                      PropertyDescriptor *desc);
    static bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set, Value *vp);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                         PropertyDescriptor *desc);
    static bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                         Value *vp);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, PropertyDescriptor *desc);
    static bool defineProperty(JSContext *cx, JSObject *proxy, jsid id, const Value &v);
    static bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    static bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool enumerate(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    static bool fix(JSContext *cx, JSObject *proxy, Value *vp);

    
    static bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    static bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
                    Value *vp);
    static bool keys(JSContext *cx, JSObject *proxy, js::AutoIdVector &props);
    static bool iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);

    
    static bool call(JSContext *cx, JSObject *proxy, uintN argc, js::Value *vp);
    static bool construct(JSContext *cx, JSObject *proxy, uintN argc, js::Value *argv, js::Value *rval);
    static bool hasInstance(JSContext *cx, JSObject *proxy, const js::Value *vp, bool *bp);
    static JSType typeOf(JSContext *cx, JSObject *proxy);
    static JSString *obj_toString(JSContext *cx, JSObject *proxy);
    static JSString *fun_toString(JSContext *cx, JSObject *proxy, uintN indent);
    static bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, js::Value *vp);
};


const uint32 JSSLOT_PROXY_HANDLER = 0;
const uint32 JSSLOT_PROXY_PRIVATE = 1;
const uint32 JSSLOT_PROXY_EXTRA   = 2;

const uint32 JSSLOT_PROXY_CALL = 3;
const uint32 JSSLOT_PROXY_CONSTRUCT = 4;

extern JS_FRIEND_API(js::Class) ObjectProxyClass;
extern JS_FRIEND_API(js::Class) FunctionProxyClass;
extern JS_FRIEND_API(js::Class) OuterWindowProxyClass;
extern js::Class CallableObjectClass;

}

inline bool
JSObject::isObjectProxy() const
{
    return getClass() == &js::ObjectProxyClass ||
           getClass() == &js::OuterWindowProxyClass;
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

inline js::JSProxyHandler *
JSObject::getProxyHandler() const
{
    JS_ASSERT(isProxy());
    return (js::JSProxyHandler *) getSlot(js::JSSLOT_PROXY_HANDLER).toPrivate();
}

inline const js::Value &
JSObject::getProxyPrivate() const
{
    JS_ASSERT(isProxy());
    return getSlot(js::JSSLOT_PROXY_PRIVATE);
}

inline void
JSObject::setProxyPrivate(const js::Value &priv)
{
    JS_ASSERT(isProxy());
    setSlot(js::JSSLOT_PROXY_PRIVATE, priv);
}

inline const js::Value &
JSObject::getProxyExtra() const
{
    JS_ASSERT(isProxy());
    return getSlot(js::JSSLOT_PROXY_EXTRA);
}

inline void
JSObject::setProxyExtra(const js::Value &extra)
{
    JS_ASSERT(isProxy());
    setSlot(js::JSSLOT_PROXY_EXTRA, extra);
}

namespace js {

JS_FRIEND_API(JSObject *)
NewProxyObject(JSContext *cx, JSProxyHandler *handler, const js::Value &priv,
               JSObject *proto, JSObject *parent,
               JSObject *call = NULL, JSObject *construct = NULL);

JS_FRIEND_API(JSBool)
FixProxy(JSContext *cx, JSObject *proxy, JSBool *bp);

}

JS_BEGIN_EXTERN_C

extern js::Class js_ProxyClass;

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif
