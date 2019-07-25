








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "jsapi.h"
#include "jsproxy.h"

JS_BEGIN_EXTERN_C


class JSWrapper : public js::JSProxyHandler {
    uintN mFlags;
  public:
    uintN flags() const { return mFlags; }

    explicit JS_FRIEND_API(JSWrapper(uintN flags));

    typedef enum { PermitObjectAccess, PermitPropertyAccess, DenyAccess } Permission;

    JS_FRIEND_API(virtual ~JSWrapper());

    
    virtual JS_FRIEND_API(bool) getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                      js::PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                         js::PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                               js::PropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                                    js::AutoIdVector &props);
    virtual JS_FRIEND_API(bool) delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual JS_FRIEND_API(bool) fix(JSContext *cx, JSObject *wrapper, js::Value *vp);

    
    virtual JS_FRIEND_API(bool) has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                                    js::Value *vp);
    virtual JS_FRIEND_API(bool) set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                                    js::Value *vp);
    virtual JS_FRIEND_API(bool) enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual JS_FRIEND_API(bool) iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    
    virtual JS_FRIEND_API(bool) call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual JS_FRIEND_API(bool) construct(JSContext *cx, JSObject *wrapper,
                                          uintN argc, js::Value *argv, js::Value *rval);
    virtual JS_FRIEND_API(JSString *) obj_toString(JSContext *cx, JSObject *wrapper);
    virtual JS_FRIEND_API(JSString *) fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

    virtual JS_FRIEND_API(void) trace(JSTracer *trc, JSObject *wrapper);

    
    virtual JS_FRIEND_API(bool) enter(JSContext *cx, JSObject *wrapper, jsid id, bool set);
    virtual JS_FRIEND_API(void) leave(JSContext *cx, JSObject *wrapper);

    static JS_FRIEND_API(JSWrapper) singleton;

    static JS_FRIEND_API(JSObject *) New(JSContext *cx, JSObject *obj,
                                         JSObject *proto, JSObject *parent,
                                         JSWrapper *handler);

    static inline JSObject *wrappedObject(JSObject *wrapper) {
        return wrapper->getProxyPrivate().toObjectOrNull();
    }
};


class JS_FRIEND_API(JSCrossCompartmentWrapper) : public JSWrapper {
  public:
    JSCrossCompartmentWrapper(uintN flags);

    virtual ~JSCrossCompartmentWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                       js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          js::PropertyDescriptor *desc);
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, js::Value *vp);
    virtual bool enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           uintN argc, js::Value *argv, js::Value *rval);
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper);
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

    static bool isCrossCompartmentWrapper(JSObject *obj);

    static JSCrossCompartmentWrapper singleton;
};

namespace js {

class AutoCompartment
{
  public:
    JSContext * const context;
    JSCompartment * const origin;
    JSObject * const target;
    JSCompartment * const destination;
  private:
    LazilyConstructed<DummyFrameGuard> frame;
    JSFrameRegs regs;
    AutoStringRooter input;
    bool entered;

  public:
    AutoCompartment(JSContext *cx, JSObject *target);
    ~AutoCompartment();

    bool enter();
    void leave();

  private:
    
    AutoCompartment(const AutoCompartment &);
    AutoCompartment & operator=(const AutoCompartment &);
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, JSObject *obj, JSObject *wrappedProto, uintN flags);

}

JS_END_EXTERN_C

#endif
