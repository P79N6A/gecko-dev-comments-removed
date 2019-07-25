








































#include "jsapi.h"
#include "jscntxt.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jswrapper.h"

#include "jsobjinlines.h"

using namespace js;

JSWrapper::JSWrapper()
{
}

JSWrapper::~JSWrapper()
{
}

bool
JSWrapper::getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                 JSPropertyDescriptor *desc)
{
    JSObject *wobj = wrappedObject(proxy);
    return JS_GetPropertyDescriptorById(cx, wobj, id, JSRESOLVE_QUALIFIED, desc);
}

bool
JSWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                    JSPropertyDescriptor *desc)
{
    return JS_GetPropertyDescriptorById(cx, wrappedObject(proxy), id, JSRESOLVE_QUALIFIED, desc);
}

bool
JSWrapper::defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                          JSPropertyDescriptor *desc)
{
    return JS_DefinePropertyById(cx, wrappedObject(proxy), id, desc->value,
                                 desc->getter, desc->setter, desc->attrs);
}

bool
JSWrapper::getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    return GetPropertyNames(cx, wrappedObject(proxy), JSITER_OWNONLY | JSITER_HIDDEN, props);
}

bool
JSWrapper::delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    AutoValueRooter tvr(cx);
    if (!JS_DeletePropertyById2(cx, wrappedObject(proxy), id, tvr.addr()))
        return false;
    *bp = js_ValueToBoolean(tvr.value());
    return true;
}

bool
JSWrapper::enumerate(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    return GetPropertyNames(cx, wrappedObject(proxy), 0, props);
}

bool
JSWrapper::fix(JSContext *cx, JSObject *proxy, jsval *vp)
{
    *vp = JSVAL_VOID;
    return true;
}

bool
JSWrapper::has(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    JSBool found;
    if (!JS_HasPropertyById(cx, wrappedObject(proxy), id, &found))
        return false;
    *bp = !!found;
    return true;
}

bool
JSWrapper::hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    JSPropertyDescriptor desc;
    JSObject *wobj = wrappedObject(proxy);
    if (!JS_GetPropertyDescriptorById(cx, wobj, id, JSRESOLVE_QUALIFIED, &desc))
        return false;
    *bp = (desc.obj == wobj);
    return true;
}

bool
JSWrapper::get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp)
{
    return JS_GetPropertyById(cx, wrappedObject(proxy), id, vp);
}

bool
JSWrapper::set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp)
{
    return JS_SetPropertyById(cx, wrappedObject(proxy), id, vp);
}

bool
JSWrapper::enumerateOwn(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    return GetPropertyNames(cx, wrappedObject(proxy), JSITER_OWNONLY, props);
}

bool
JSWrapper::iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp)
{
    return GetIterator(cx, wrappedObject(proxy), flags, vp);
}

void
JSWrapper::trace(JSTracer *trc, JSObject *proxy)
{
    JS_CALL_OBJECT_TRACER(trc, wrappedObject(proxy), "wrappedObject");
}

JSWrapper JSWrapper::singleton;

JSObject *
JSWrapper::New(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
               JSProxyHandler *handler)
{
    return NewProxyObject(cx, handler, OBJECT_TO_JSVAL(obj), proto, parent,
                          obj->isCallable() ? obj : NULL, NULL);
}


class JSCrossCompartmentWrapper : public JSWrapper {
  protected:
    JSCrossCompartmentWrapper();

  public:
    virtual ~JSCrossCompartmentWrapper();

    
    virtual bool getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                       JSPropertyDescriptor *desc);
    virtual bool getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id,
                                          JSPropertyDescriptor *desc);
    virtual bool defineProperty(JSContext *cx, JSObject *proxy, jsid id,
                                JSPropertyDescriptor *desc);
    virtual bool getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoValueVector &props);
    virtual bool delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool enumerate(JSContext *cx, JSObject *proxy, AutoValueVector &props);

    
    virtual bool has(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp);
    virtual bool get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    virtual bool set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp);
    virtual bool enumerateOwn(JSContext *cx, JSObject *proxy, AutoValueVector &props);
    virtual bool iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp);

    
    virtual bool call(JSContext *cx, JSObject *proxy, uintN argc, jsval *vp);
    virtual bool construct(JSContext *cx, JSObject *proxy, JSObject *receiver,
                           uintN argc, jsval *argv, jsval *rval);
    virtual JSString *obj_toString(JSContext *cx, JSObject *proxy);
    virtual JSString *fun_toString(JSContext *cx, JSObject *proxy, uintN indent);

    static JSCrossCompartmentWrapper singleton;
};

bool
JSObject::isCrossCompartmentWrapper() const
{
    return isProxy() && getProxyHandler() == &JSCrossCompartmentWrapper::singleton;
}

JSObject *
JSObject::unwrap()
{
    JSObject *wrapped = this;
    while (wrapped->isCrossCompartmentWrapper())
        wrapped = JSVAL_TO_OBJECT(wrapped->getProxyPrivate());
    return wrapped;
}



JSCompartment::JSCompartment(JSRuntime *rt) : rt(rt), marked(false)
{
}

JSCompartment::~JSCompartment()
{
}

bool
JSCompartment::init()
{
    return crossCompartmentWrappers.init();
}

bool
JSCompartment::wrap(JSContext *cx, jsval *vp)
{
    JS_ASSERT(cx->compartment == this);

    JS_CHECK_RECURSION(cx, return false);

    
    if (JSVAL_IS_NULL(*vp) || !JSVAL_IS_GCTHING(*vp))
        return true;

    
    if (JSVAL_IS_STRING(*vp) && JSString::isStatic(JSVAL_TO_STRING(*vp)))
        return true;

    
    if (JSVAL_IS_DOUBLE(*vp))
        return js_NewNumberInRootedValue(cx, *JSVAL_TO_DOUBLE(*vp), vp);

    
    if (!JSVAL_IS_PRIMITIVE(*vp)) {
        JSObject *obj =  JSVAL_TO_OBJECT(*vp)->unwrap();
        *vp = OBJECT_TO_JSVAL(obj);
        
        if (obj->getCompartment(cx) == this)
            return true;
    }

    
    if (WrapperMap::Ptr p = crossCompartmentWrappers.lookup(*vp)) {
        *vp = p->value;
        return true;
    }

    if (JSVAL_IS_STRING(*vp)) {
        JSString *str = JSVAL_TO_STRING(*vp);
        JSString *wrapped = js_NewStringCopyN(cx, str->chars(), str->length());
        if (!wrapped)
            return false;
        return crossCompartmentWrappers.put(*vp, *vp = STRING_TO_JSVAL(wrapped));
    }

    JSObject *obj = JSVAL_TO_OBJECT(*vp);

    









    AutoObjectRooter proto(cx, obj->getProto());
    if (!wrap(cx, proto.addr()))
        return false;
    JSObject *wrapper = JSWrapper::New(cx, obj, proto.object(), NULL,
                                       &JSCrossCompartmentWrapper::singleton);
    if (!wrapper)
        return false;
    *vp = OBJECT_TO_JSVAL(wrapper);
    if (!crossCompartmentWrappers.put(wrapper->getProxyPrivate(), *vp))
        return false;

    






    JSObject *global = cx->fp ? cx->fp->scopeChain->getGlobal() : cx->globalObject;
    wrapper->setParent(global);
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSString **strp)
{
    AutoValueRooter tvr(cx, *strp);
    if (!wrap(cx, tvr.addr()))
        return false;
    *strp = JSVAL_TO_STRING(tvr.value());
    return true;
}

bool
JSCompartment::wrap(JSContext *cx, JSObject **objp)
{
    if (!*objp)
        return true;
    AutoValueRooter tvr(cx, *objp);
    if (!wrap(cx, tvr.addr()))
        return false;
    *objp = JSVAL_TO_OBJECT(tvr.value());
    return true;
}

bool
JSCompartment::wrapId(JSContext *cx, jsid *idp) {
    if (JSID_IS_INT(*idp))
        return true;
    AutoValueRooter tvr(cx, ID_TO_VALUE(*idp));
    if (!wrap(cx, tvr.addr()))
        return false;
    return JS_ValueToId(cx, tvr.value(), idp);
}

bool
JSCompartment::wrap(JSContext *cx, JSPropertyDescriptor *desc) {
    return wrap(cx, &desc->obj) &&
           (!(desc->attrs & JSPROP_GETTER) || wrap(cx, (jsval *) &desc->getter)) &&
           (!(desc->attrs & JSPROP_SETTER) || wrap(cx, (jsval *) &desc->setter)) &&
           wrap(cx, &desc->value);
}

bool
JSCompartment::wrap(JSContext *cx, AutoValueVector &props) {
    jsid *vector = props.begin();
    jsint length = props.length();
    for (size_t n = 0; n < size_t(length); ++n) {
        if (!wrap(cx, &vector[n]))
            return false;
    }
    return true;
}

bool
JSCompartment::wrapException(JSContext *cx) {
    JS_ASSERT(cx->compartment == this);

    if (cx->throwing) {
        AutoValueRooter tvr(cx, cx->exception);
        cx->throwing = false;
        cx->exception = JSVAL_NULL;
        if (wrap(cx, tvr.addr())) {
            cx->throwing = true;
            cx->exception = tvr.value();
        }
        return false;
    }
    return true;
}

void
JSCompartment::sweep(JSContext *cx)
{
    
    for (WrapperMap::Enum e(crossCompartmentWrappers); !e.empty(); e.popFront()) {
        if (js_IsAboutToBeFinalized(JSVAL_TO_GCTHING(e.front().value)))
            e.removeFront();
    }
}

static bool
SetupFakeFrame(JSContext *cx, ExecuteFrameGuard &frame, JSFrameRegs &regs, JSObject *obj)
{
    const uintN vplen = 2;
    const uintN nfixed = 0;
    if (!cx->stack().getExecuteFrame(cx, js_GetTopStackFrame(cx), vplen, nfixed, frame))
        return false;

    jsval *vp = frame.getvp();
    vp[0] = JSVAL_VOID;
    vp[1] = JSVAL_VOID;

    JSStackFrame *fp = frame.getFrame();
    PodZero(fp);  
    fp->argv = vp + 2;
    fp->scopeChain = obj->getGlobal();

    regs.pc = NULL;
    regs.sp = fp->slots();

    cx->stack().pushExecuteFrame(cx, frame, regs, NULL);
    return true;
}

AutoCompartment::AutoCompartment(JSContext *cx, JSObject *target)
    : context(cx),
      origin(cx->compartment),
      target(target),
      destination(target->getCompartment(cx))
{
    JS_ASSERT(origin != destination);  
}

AutoCompartment::~AutoCompartment()
{
    if (entered())
        leave();
}

bool
AutoCompartment::enter()
{
    JS_ASSERT(!entered());
    context->compartment = destination;
    frame.construct();
    bool ok = SetupFakeFrame(context, frame.ref(), regs, target);
    if (!ok) {
        frame.destroy();
        context->compartment = origin;
    }
    return ok;
}

void
AutoCompartment::leave()
{
    JS_ASSERT(entered());
    frame.destroy();
    context->compartment = origin;
    origin->wrapException(context);
}

#define PIERCE(cx, pre, op, post)                                        \
    JS_BEGIN_MACRO                                                       \
        AutoCompartment call(cx, wrappedObject(proxy));                  \
        if (!call.enter() || !(pre) || !(op))                            \
            return false;                                                \
        call.leave();                                                    \
        return (post);                                                   \
    JS_END_MACRO

#define NOTHING (true)



JSCrossCompartmentWrapper::JSCrossCompartmentWrapper()
{
}

JSCrossCompartmentWrapper::~JSCrossCompartmentWrapper()
{
}

bool
JSCrossCompartmentWrapper::getPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, JSPropertyDescriptor *desc)
{
    PIERCE(cx,
           call.destination->wrapId(cx, &id),
           JSWrapper::getPropertyDescriptor(cx, proxy, id, desc),
           call.origin->wrap(cx, desc));
}

bool
JSCrossCompartmentWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *proxy, jsid id, JSPropertyDescriptor *desc)
{
    PIERCE(cx,
           call.destination->wrapId(cx, &id),
           JSWrapper::getOwnPropertyDescriptor(cx, proxy, id, desc),
           call.origin->wrap(cx, desc));
}

bool
JSCrossCompartmentWrapper::defineProperty(JSContext *cx, JSObject *proxy, jsid id, JSPropertyDescriptor *desc)
{
    AutoDescriptor desc2(cx, desc);
    PIERCE(cx,
           call.destination->wrapId(cx, &id) && call.destination->wrap(cx, &desc2),
           JSWrapper::getOwnPropertyDescriptor(cx, proxy, id, &desc2),
           NOTHING);
}

bool
JSCrossCompartmentWrapper::getOwnPropertyNames(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    PIERCE(cx,
           NOTHING,
           JSWrapper::getOwnPropertyNames(cx, proxy, props),
           call.origin->wrap(cx, props));
}

bool
JSCrossCompartmentWrapper::delete_(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    PIERCE(cx,
           call.destination->wrapId(cx, &id),
           JSWrapper::delete_(cx, proxy, id, bp),
           NOTHING);
}

bool
JSCrossCompartmentWrapper::enumerate(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    PIERCE(cx,
           NOTHING,
           JSWrapper::enumerate(cx, proxy, props),
           call.origin->wrap(cx, props));
}

bool
JSCrossCompartmentWrapper::has(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    PIERCE(cx,
           call.destination->wrapId(cx, &id),
           JSWrapper::has(cx, proxy, id, bp),
           NOTHING);
}

bool
JSCrossCompartmentWrapper::hasOwn(JSContext *cx, JSObject *proxy, jsid id, bool *bp)
{
    PIERCE(cx,
           call.destination->wrapId(cx, &id),
           JSWrapper::hasOwn(cx, proxy, id, bp),
           NOTHING);
}

bool
JSCrossCompartmentWrapper::get(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp)
{
    PIERCE(cx,
           call.destination->wrap(cx, &receiver) && call.destination->wrapId(cx, &id),
           JSWrapper::get(cx, proxy, receiver, id, vp),
           call.origin->wrap(cx, vp));
}

bool
JSCrossCompartmentWrapper::set(JSContext *cx, JSObject *proxy, JSObject *receiver, jsid id, jsval *vp)
{
    AutoValueRooter tvr(cx, *vp);
    PIERCE(cx,
           call.destination->wrap(cx, &receiver) && call.destination->wrapId(cx, &id) && call.destination->wrap(cx, tvr.addr()),
           JSWrapper::set(cx, proxy, receiver, id, tvr.addr()),
           NOTHING);
}

bool
JSCrossCompartmentWrapper::enumerateOwn(JSContext *cx, JSObject *proxy, AutoValueVector &props)
{
    PIERCE(cx,
           NOTHING,
           JSWrapper::enumerateOwn(cx, proxy, props),
           call.origin->wrap(cx, props));
}

bool
JSCrossCompartmentWrapper::iterate(JSContext *cx, JSObject *proxy, uintN flags, jsval *vp)
{
    PIERCE(cx,
           NOTHING,
           JSWrapper::iterate(cx, proxy, flags, vp),
           call.origin->wrap(cx, vp));
}

bool
JSCrossCompartmentWrapper::call(JSContext *cx, JSObject *proxy, uintN argc, jsval *vp)
{
    AutoCompartment call(cx, wrappedObject(proxy));
    if (!call.enter())
        return false;

    vp[0] = OBJECT_TO_JSVAL(call.target);
    if (!call.destination->wrap(cx, &vp[1]))
        return false;
    jsval *argv = JS_ARGV(cx, vp);
    for (size_t n = 0; n < argc; ++n) {
        if (!call.destination->wrap(cx, &argv[n]))
            return false;
    }
    jsval *fakevp = call.getvp();
    fakevp[0] = vp[0];
    fakevp[1] = vp[1];
    if (!JSWrapper::call(cx, proxy, argc, vp))
        return false;

    call.leave();
    return call.origin->wrap(cx, vp);
}

bool
JSCrossCompartmentWrapper::construct(JSContext *cx, JSObject *proxy, JSObject *receiver,
                                     uintN argc, jsval *argv, jsval *rval)
{
    AutoCompartment call(cx, wrappedObject(proxy));
    if (!call.enter())
        return false;

    for (size_t n = 0; n < argc; ++n) {
        if (!call.destination->wrap(cx, &argv[n]))
            return false;
    }
    jsval *vp = call.getvp();
    vp[0] = OBJECT_TO_JSVAL(call.target);
    if (!call.destination->wrap(cx, &receiver) ||
        !JSWrapper::construct(cx, proxy, receiver, argc, argv, rval)) {
        return false;
    }

    call.leave();
    return call.origin->wrap(cx, rval) &&
           call.origin->wrapException(cx);
}

JSString *
JSCrossCompartmentWrapper::obj_toString(JSContext *cx, JSObject *proxy)
{
    AutoCompartment call(cx, wrappedObject(proxy));
    if (!call.enter())
        return NULL;

    JSString *str = JSWrapper::obj_toString(cx, proxy);
    if (!str)
        return NULL;
    AutoValueRooter tvr(cx, str);

    call.leave();
    if (!call.origin->wrap(cx, &str))
        return NULL;
    return str;
}

JSString *
JSCrossCompartmentWrapper::fun_toString(JSContext *cx, JSObject *proxy, uintN indent)
{
    AutoCompartment call(cx, wrappedObject(proxy));
    if (!call.enter())
        return NULL;

    JSString *str = JSWrapper::fun_toString(cx, proxy, indent);
    if (!str)
        return NULL;
    AutoValueRooter tvr(cx, str);

    call.leave();
    if (!call.origin->wrap(cx, &str))
        return NULL;
    return str;
}

JSCrossCompartmentWrapper JSCrossCompartmentWrapper::singleton;
