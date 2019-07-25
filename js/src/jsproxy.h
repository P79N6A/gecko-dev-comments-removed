








































#ifndef jsproxy_h___
#define jsproxy_h___

#include "jsapi.h"
#include "jscntxt.h"
#include "jsfriendapi.h"

namespace js {


class JS_FRIEND_API(ProxyHandler) {
    void *mFamily;
  public:
    explicit ProxyHandler(void *family);
    virtual ~ProxyHandler();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                       PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, bool set,
                                          PropertyDescriptor *desc) = 0;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoIdVector &props) = 0;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, JSObject *proxy, AutoIdVector &props) = 0;
    virtual bool fix(JSContext *cx, JSObject *proxy, Value *vp) = 0;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
                     Value *vp);
    virtual bool keys(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *proxy, uintN argc, Value *vp);
    virtual bool construct(JSContext *cx, JSObject *proxy, uintN argc, Value *argv, Value *rval);
    virtual bool nativeCall(JSContext *cx, JSObject *proxy, Class *clasp, Native native, CallArgs args);
    virtual bool hasInstance(JSContext *cx, JSObject *proxy, const Value *vp, bool *bp);
    virtual JSType typeOf(JSContext *cx, JSObject *proxy);
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx);
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy);
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy, uintN indent);
    virtual bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);
    virtual void finalize(JSContext *cx, JSObject *proxy);
    virtual void trace(JSTracer *trc, JSObject *proxy);
    virtual bool getElementIfPresent(JSContext *cx, JSObject *obj, JSObject *receiver,
                                     JSUint32 index, Value *vp, bool *present);

    virtual bool isOuterWindow() {
        return false;
    }

    inline void *family() {
        return mFamily;
    }
};


class Proxy {
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
    static bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    static bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool enumerate(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    static bool fix(JSContext *cx, JSObject *proxy, Value *vp);

    
    static bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, Value *vp);
    static bool getElementIfPresent(JSContext *cx, JSObject *proxy, JSObject *receiver,
                                    uint32 index, Value *vp, bool *present);
    static bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, bool strict,
                    Value *vp);
    static bool keys(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    static bool iterate(JSContext *cx, JSObject *proxy, uintN flags, Value *vp);

    
    static bool call(JSContext *cx, JSObject *proxy, uintN argc, Value *vp);
    static bool construct(JSContext *cx, JSObject *proxy, uintN argc, Value *argv, Value *rval);
    static bool nativeCall(JSContext *cx, JSObject *proxy, Class *clasp, Native native, CallArgs args);
    static bool hasInstance(JSContext *cx, JSObject *proxy, const Value *vp, bool *bp);
    static JSType typeOf(JSContext *cx, JSObject *proxy);
    static bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx);
    static JSString *obj_toString(JSContext *cx, JSObject *proxy);
    static JSString *fun_toString(JSContext *cx, JSObject *proxy, uintN indent);
    static bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);
};

inline bool IsObjectProxy(const JSObject *obj)
{
    Class *clasp = GetObjectClass(obj);
    return clasp == &js::ObjectProxyClass || clasp == &js::OuterWindowProxyClass;
}

inline bool IsFunctionProxy(const JSObject *obj)
{
    Class *clasp = GetObjectClass(obj);
    return clasp == &js::FunctionProxyClass;
}

inline bool IsProxy(const JSObject *obj)
{
    return IsObjectProxy(obj) || IsFunctionProxy(obj);
}


const uint32 JSSLOT_PROXY_HANDLER = 0;
const uint32 JSSLOT_PROXY_PRIVATE = 1;
const uint32 JSSLOT_PROXY_EXTRA   = 2;

const uint32 JSSLOT_PROXY_CALL = 4;
const uint32 JSSLOT_PROXY_CONSTRUCT = 5;

inline ProxyHandler *
GetProxyHandler(const JSObject *obj)
{
    JS_ASSERT(IsProxy(obj));
    return (ProxyHandler *) GetReservedSlot(obj, JSSLOT_PROXY_HANDLER).toPrivate();
}

inline const Value &
GetProxyPrivate(const JSObject *obj)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, JSSLOT_PROXY_PRIVATE);
}

inline void
SetProxyPrivate(JSObject *obj, const Value &priv)
{
    JS_ASSERT(IsProxy(obj));
    SetReservedSlot(obj, JSSLOT_PROXY_PRIVATE, priv);
}

inline const Value &
GetProxyExtra(const JSObject *obj, size_t n)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, JSSLOT_PROXY_EXTRA + n);
}

inline void
SetProxyExtra(JSObject *obj, size_t n, const Value &extra)
{
    JS_ASSERT(IsProxy(obj));
    JS_ASSERT(n <= 1);
    SetReservedSlot(obj, JSSLOT_PROXY_EXTRA + n, extra);
}

JS_FRIEND_API(JSObject *)
NewProxyObject(JSContext *cx, ProxyHandler *handler, const Value &priv,
               JSObject *proto, JSObject *parent,
               JSObject *call = NULL, JSObject *construct = NULL);

JS_FRIEND_API(JSBool)
FixProxy(JSContext *cx, JSObject *proxy, JSBool *bp);

} 

JS_BEGIN_EXTERN_C

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JSObject *obj);

JS_END_EXTERN_C

#endif
