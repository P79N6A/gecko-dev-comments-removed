








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "jsapi.h"
#include "jsproxy.h"

namespace js {


class JSWrapper : public JSProxyHandler {
  protected:
    JS_FRIEND_API(JSWrapper());

  public:
    JS_FRIEND_API(virtual ~JSWrapper());

    
    virtual JS_FRIEND_API(bool) getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                                      JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                                         JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                               JSPropertyDescriptor *desc);
    virtual JS_FRIEND_API(bool) getOwnPropertyNames(JSContext *cx, JSObject *proxy,
                                                    AutoValueVector &props);
    virtual JS_FRIEND_API(bool) delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) enumerate(JSContext *cx, JSObject *proxy, AutoValueVector &props);
    virtual JS_FRIEND_API(bool) fix(JSContext *cx, JSObject *proxy, jsval *vp);

    
    virtual JS_FRIEND_API(bool) has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual JS_FRIEND_API(bool) get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id,
                                    jsval *vp);
    virtual JS_FRIEND_API(bool) set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id,
                                    jsval *vp);
    virtual JS_FRIEND_API(bool) enumerateOwn(JSContext *cx, JSObject *proxy, AutoValueVector &props);
    virtual JS_FRIEND_API(bool) iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp);

    
    virtual JS_FRIEND_API(void) trace(JSTracer *trc, JSObject *proxy);

    static JS_FRIEND_API(JSWrapper) singleton;

    static JS_FRIEND_API(JSObject *) New(JSContext *cx, JSObject *obj,
                                         JSObject *proto, JSObject *parent,
                                         JSProxyHandler *handler);

    static inline JSObject *wrappedObject(JSObject *proxy) {
        return JSVAL_TO_OBJECT(proxy->getProxyPrivate());
    }
};

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

}

#endif
