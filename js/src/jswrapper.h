








































#ifndef jswrapper_h___
#define jswrapper_h___

#include "jsapi.h"
#include "jsproxy.h"

JS_BEGIN_EXTERN_C


class JS_FRIEND_API(JSWrapper) : public js::JSProxyHandler {
    uintN mFlags;
  public:
    uintN flags() const { return mFlags; }

    explicit JSWrapper(uintN flags);

    typedef enum { PermitObjectAccess, PermitPropertyAccess, DenyAccess } Permission;

    virtual ~JSWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                       js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                          js::PropertyDescriptor *desc);
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool fix(JSContext *cx, JSObject *wrapper, js::Value *vp);

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
                     js::Value *vp);
    virtual bool keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *argv,
                           js::Value *rval);
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const js::Value *vp, bool *bp);
    virtual JSType typeOf(JSContext *cx, JSObject *proxy);
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper);
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

    virtual void trace(JSTracer *trc, JSObject *wrapper);

    
    enum Action { GET, SET, CALL };
    virtual bool enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp);
    virtual void leave(JSContext *cx, JSObject *wrapper);

    static JSWrapper singleton;

    static JSObject *New(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
                         JSWrapper *handler);

    static inline JSObject *wrappedObject(const JSObject *wrapper) {
        return wrapper->getProxyPrivate().toObjectOrNull();
    }
    static inline JSWrapper *wrapperHandler(const JSObject *wrapper) {
        return static_cast<JSWrapper *>(wrapper->getProxyHandler());
    }

    enum {
        CROSS_COMPARTMENT = 1 << 0,
        LAST_USED_FLAG = CROSS_COMPARTMENT
    };

    static void *getWrapperFamily();
};


class JS_FRIEND_API(JSCrossCompartmentWrapper) : public JSWrapper {
  public:
    JSCrossCompartmentWrapper(uintN flags);

    virtual ~JSCrossCompartmentWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                       js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id, bool set,
                                          js::PropertyDescriptor *desc);
    virtual bool defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                js::PropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);

    
    virtual bool has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, js::Value *vp);
    virtual bool set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
                     js::Value *vp);
    virtual bool keys(JSContext *cx, JSObject *wrapper, js::AutoIdVector &props);
    virtual bool iterate(JSContext *cx, JSObject *wrapper, uintN flags, js::Value *vp);

    
    virtual bool call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp);
    virtual bool construct(JSContext *cx, JSObject *wrapper,
                           uintN argc, js::Value *argv, js::Value *rval);
    virtual bool hasInstance(JSContext *cx, JSObject *wrapper, const js::Value *vp, bool *bp);
    virtual JSString *obj_toString(JSContext *cx, JSObject *wrapper);
    virtual JSString *fun_toString(JSContext *cx, JSObject *wrapper, uintN indent);

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
    Maybe<DummyFrameGuard> frame;
    FrameRegs regs;
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
TransparentObjectWrapper(JSContext *cx, JSObject *obj, JSObject *wrappedProto, JSObject *parent,
                         uintN flags);

}

JS_END_EXTERN_C

#endif
