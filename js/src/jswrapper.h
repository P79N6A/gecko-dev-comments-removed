








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "jsapi.h"
#include "jsproxy.h"


class JSWrapper : public js::JSProxyHandler {
    uintN mFlags;
  public:
    uintN flags() const { return mFlags; }

    explicit JS_FRIEND_API(JSWrapper(uintN flags));

    typedef enum { PermitObjectAccess, PermitPropertyAccess, DenyAccess } Permission;

    JS_FRIEND_API(virtual ~JSWrapper());

    
    virtual JS_FRIEND_API(bool) getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                      JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                         JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                               JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                                    js::AutoValueVector &props);
    virtual JS_FRIEND_API(bool) delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) enumerate(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual JS_FRIEND_API(bool) fix(JSContext *cx, JSObject *wrapper, jsval *vp);

    
    virtual JS_FRIEND_API(bool) has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                                    jsval *vp);
    virtual JS_FRIEND_API(bool) set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                                    jsval *vp);
    virtual JS_FRIEND_API(bool) enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual JS_FRIEND_API(bool) iterate(JSContext *cx, JSObject *wrapper, uintN flags, jsval *vp);

    
    virtual JS_FRIEND_API(bool) call(JSContext *cx, JSObject *wrapper, uintN argc, jsval *vp);
    virtual JS_FRIEND_API(bool) construct(JSContext *cx, JSObject *wrapper,
                                          uintN argc, jsval *argv, jsval *rval);
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
        return JSVAL_TO_OBJECT(wrapper->getProxyPrivate());
    }
};


class JSCrossCompartmentWrapper : public JSWrapper {
  public:
    JS_FRIEND_API(JSCrossCompartmentWrapper(uintN flags));

    virtual JS_FRIEND_API(~JSCrossCompartmentWrapper());

    
    virtual JS_FRIEND_API(bool) getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                     JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                         JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                               JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual JS_FRIEND_API(bool) delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) enumerate(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);

    
    virtual JS_FRIEND_API(bool) has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, jsval *vp);
    virtual JS_FRIEND_API(bool) set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, jsval *vp);
    virtual JS_FRIEND_API(bool) enumerateOwn(JSContext *cx, JSObject *wrapper, js::AutoValueVector &props);
    virtual JS_FRIEND_API(bool) iterate(JSContext *cx, JSObject *wrapper, uintN flags, jsval *vp);

    
    virtual JS_FRIEND_API(bool) call(JSContext *cx, JSObject *wrapper, uintN argc, jsval *vp);
    virtual JS_FRIEND_API(bool) construct(JSContext *cx, JSObject *wrapper,
                                          uintN argc, jsval *argv, jsval *rval);
    virtual JS_FRIEND_API(JSString *) obj_toString(JSContext *cx, JSObject *wrapper);
    virtual JS_FRIEND_API(JSString *) fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

    static JS_FRIEND_API(bool) isCrossCompartmentWrapper(JSObject *obj);

    static JS_FRIEND_API(JSCrossCompartmentWrapper) singleton;

    
    static const jsid id = JSVAL_VOID;
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
    LazilyConstructed<ExecuteFrameGuard> frame;
    JSFrameRegs regs;
    JSRegExpStatics statics;
    AutoValueRooter input;

  public:
    AutoCompartment(JSContext *cx, JSObject *target);
    ~AutoCompartment();

    bool entered() const { return context->compartment == destination; }
    bool enter();
    void leave();

    jsval *getvp() {
        JS_ASSERT(entered());
        return frame.ref().getvp();
    }

  private:
    
    AutoCompartment(const AutoCompartment &);
    AutoCompartment & operator=(const AutoCompartment &);
};

extern JSObject *
TransparentObjectWrapper(JSContext *cx, JSObject *obj, JSObject *wrappedProto, uintN flags);

}

#endif
