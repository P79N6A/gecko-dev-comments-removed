






#ifndef jsproxy_h___
#define jsproxy_h___

#include "jsapi.h"
#include "jsfriendapi.h"

namespace js {

class JS_FRIEND_API(Wrapper);


































class JS_FRIEND_API(BaseProxyHandler) {
    void *mFamily;
    bool mHasPrototype;
  protected:
    
    void setHasPrototype(bool hasPrototype) { mHasPrototype = hasPrototype; }

  public:
    explicit BaseProxyHandler(void *family);
    virtual ~BaseProxyHandler();

    bool hasPrototype() {
        return mHasPrototype;
    }

    inline void *family() {
        return mFamily;
    }

    virtual bool isOuterWindow() {
        return false;
    }

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       bool set, PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy,
                                          jsid id, bool set,
                                          PropertyDescriptor *desc) = 0;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                PropertyDescriptor *desc) = 0;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy,
                                     AutoIdVector &props) = 0;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp) = 0;
    virtual bool enumerate(JSContext *cx, JSObject *proxy,
                           AutoIdVector &props) = 0;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, Value *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, bool strict, Value *vp);
    virtual bool keys(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, unsigned flags,
                         Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *proxy, unsigned argc, Value *vp);
    virtual bool construct(JSContext *cx, JSObject *proxy, unsigned argc, Value *argv, Value *rval);
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);
    virtual bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    virtual JSType typeOf(JSContext *cx, JSObject *proxy);
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx);
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy);
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy, unsigned indent);
    virtual bool regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g);
    virtual bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);
    virtual void finalize(JSFreeOp *fop, JSObject *proxy);
    virtual bool getElementIfPresent(JSContext *cx, JSObject *obj, JSObject *receiver,
                                     uint32_t index, Value *vp, bool *present);
    virtual bool getPrototypeOf(JSContext *cx, JSObject *proxy, JSObject **protop);

    
    virtual JSObject *weakmapKeyDelegate(JSObject *proxy);
};







class JS_PUBLIC_API(DirectProxyHandler) : public BaseProxyHandler {
public:
    explicit DirectProxyHandler(void *family);

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       bool set,
                                       PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy,
                                          jsid id, bool set,
                                          PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                PropertyDescriptor *desc) MOZ_OVERRIDE;
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy,
                                     AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id,
                         bool *bp) MOZ_OVERRIDE;
    virtual bool enumerate(JSContext *cx, JSObject *proxy,
                           AutoIdVector &props) MOZ_OVERRIDE;

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id,
                     bool *bp) MOZ_OVERRIDE;
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id,
                        bool *bp) MOZ_OVERRIDE;
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, Value *vp) MOZ_OVERRIDE;
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver,
                     jsid id, bool strict, Value *vp) MOZ_OVERRIDE;
    virtual bool keys(JSContext *cx, JSObject *proxy,
                      AutoIdVector &props) MOZ_OVERRIDE;
    virtual bool iterate(JSContext *cx, JSObject *proxy, unsigned flags,
                         Value *vp) MOZ_OVERRIDE;

    
    virtual bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                            CallArgs args) MOZ_OVERRIDE;
    virtual bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v,
                             bool *bp) MOZ_OVERRIDE;
    virtual JSType typeOf(JSContext *cx, JSObject *proxy) MOZ_OVERRIDE;
    virtual bool objectClassIs(JSObject *obj, ESClassValue classValue,
                               JSContext *cx) MOZ_OVERRIDE;
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy) MOZ_OVERRIDE;
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy,
                                   unsigned indent) MOZ_OVERRIDE;
    virtual bool regexp_toShared(JSContext *cx, JSObject *proxy,
                                 RegExpGuard *g) MOZ_OVERRIDE;
    virtual bool defaultValue(JSContext *cx, JSObject *obj, JSType hint,
                              Value *vp) MOZ_OVERRIDE;
    virtual JSObject *weakmapKeyDelegate(JSObject *proxy);
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

    
    static bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    static bool get(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id, MutableHandleValue vp);
    static bool getElementIfPresent(JSContext *cx, HandleObject proxy, HandleObject receiver,
                                    uint32_t index, MutableHandleValue vp, bool *present);
    static bool set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id, bool strict,
                    MutableHandleValue vp);
    static bool keys(JSContext *cx, JSObject *proxy, AutoIdVector &props);
    static bool iterate(JSContext *cx, HandleObject proxy, unsigned flags, MutableHandleValue vp);

    
    static bool call(JSContext *cx, JSObject *proxy, unsigned argc, Value *vp);
    static bool construct(JSContext *cx, JSObject *proxy, unsigned argc, Value *argv, Value *rval);
    static bool nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args);
    static bool hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp);
    static JSType typeOf(JSContext *cx, JSObject *proxy);
    static bool objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx);
    static JSString *obj_toString(JSContext *cx, JSObject *proxy);
    static JSString *fun_toString(JSContext *cx, JSObject *proxy, unsigned indent);
    static bool regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g);
    static bool defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp);
    static bool getPrototypeOf(JSContext *cx, JSObject *proxy, JSObject **protop);

    static JSObject * const LazyProto;
};

inline bool IsObjectProxyClass(const Class *clasp)
{
    return clasp == &js::ObjectProxyClass || clasp == &js::OuterWindowProxyClass;
}

inline bool IsFunctionProxyClass(const Class *clasp)
{
    return clasp == &js::FunctionProxyClass;
}

inline bool IsObjectProxy(RawObject obj)
{
    return IsObjectProxyClass(GetObjectClass(obj));
}

inline bool IsFunctionProxy(RawObject obj)
{
    return IsFunctionProxyClass(GetObjectClass(obj));
}

inline bool IsProxy(RawObject obj)
{
    Class *clasp = GetObjectClass(obj);
    return IsObjectProxyClass(clasp) || IsFunctionProxyClass(clasp);
}








const uint32_t JSSLOT_PROXY_PRIVATE = 0;
const uint32_t JSSLOT_PROXY_HANDLER = 1;
const uint32_t JSSLOT_PROXY_EXTRA   = 2;

const uint32_t JSSLOT_PROXY_CALL = 4;
const uint32_t JSSLOT_PROXY_CONSTRUCT = 5;

inline BaseProxyHandler *
GetProxyHandler(RawObject obj)
{
    JS_ASSERT(IsProxy(obj));
    return (BaseProxyHandler *) GetReservedSlot(obj, JSSLOT_PROXY_HANDLER).toPrivate();
}

inline const Value &
GetProxyPrivate(RawObject obj)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, JSSLOT_PROXY_PRIVATE);
}

inline JSObject *
GetProxyTargetObject(RawObject obj)
{
    JS_ASSERT(IsProxy(obj));
    return GetProxyPrivate(obj).toObjectOrNull();
}

inline const Value &
GetProxyCall(RawObject obj)
{
    JS_ASSERT(IsFunctionProxy(obj));
    return GetReservedSlot(obj, JSSLOT_PROXY_CALL);
}

inline const Value &
GetProxyExtra(RawObject obj, size_t n)
{
    JS_ASSERT(IsProxy(obj));
    return GetReservedSlot(obj, JSSLOT_PROXY_EXTRA + n);
}

inline void
SetProxyHandler(RawObject obj, BaseProxyHandler *handler)
{
    JS_ASSERT(IsProxy(obj));
    SetReservedSlot(obj, JSSLOT_PROXY_HANDLER, PrivateValue(handler));
}

inline void
SetProxyPrivate(RawObject obj, const Value &value)
{
    JS_ASSERT(IsProxy(obj));
    SetReservedSlot(obj, JSSLOT_PROXY_PRIVATE, value);
}

inline void
SetProxyExtra(RawObject obj, size_t n, const Value &extra)
{
    JS_ASSERT(IsProxy(obj));
    JS_ASSERT(n <= 1);
    SetReservedSlot(obj, JSSLOT_PROXY_EXTRA + n, extra);
}

JS_FRIEND_API(JSObject *)
NewProxyObject(JSContext *cx, BaseProxyHandler *handler, const Value &priv,
               JSObject *proto, JSObject *parent,
               JSObject *call = NULL, JSObject *construct = NULL);

JSObject *
RenewProxyObject(JSContext *cx, JSObject *obj, BaseProxyHandler *handler, Value priv);

} 

extern JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, JSHandleObject obj);

#endif
