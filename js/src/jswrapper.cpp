






#include "jsapi.h"
#include "jscntxt.h"
#include "jscompartment.h"
#include "jsexn.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jswrapper.h"

#ifdef JS_METHODJIT
# include "assembler/jit/ExecutableAllocator.h"
#endif
#include "gc/Marking.h"
#include "methodjit/PolyIC.h"
#include "methodjit/MonoIC.h"

#include "jsobjinlines.h"

#include "builtin/Iterator-inl.h"
#include "vm/RegExpObject-inl.h"

using namespace js;
using namespace js::gc;

namespace js {
int sWrapperFamily;
}

void *
Wrapper::getWrapperFamily()
{
    return &sWrapperFamily;
}

JSObject *
Wrapper::New(JSContext *cx, JSObject *obj, JSObject *proto, JSObject *parent,
             Wrapper *handler)
{
    JS_ASSERT(parent);

#if JS_HAS_XML_SUPPORT
    if (obj->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_WRAP_XML_OBJECT);
        return NULL;
    }
#endif
    return NewProxyObject(cx, handler, ObjectValue(*obj), proto, parent,
                          obj->isCallable() ? obj : NULL, NULL);
}

JSObject *
Wrapper::Renew(JSContext *cx, JSObject *existing, JSObject *obj, Wrapper *handler)
{
#if JS_HAS_XML_SUPPORT
    if (obj->isXML()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_WRAP_XML_OBJECT);
        return NULL;
    }
#endif

    JS_ASSERT(!obj->isCallable());
    return RenewProxyObject(cx, existing, handler, ObjectValue(*obj));
}

Wrapper *
Wrapper::wrapperHandler(RawObject wrapper)
{
    JS_ASSERT(wrapper->isWrapper());
    return static_cast<Wrapper*>(GetProxyHandler(wrapper));
}

JSObject *
Wrapper::wrappedObject(RawObject wrapper)
{
    JS_ASSERT(wrapper->isWrapper());
    return GetProxyTargetObject(wrapper);
}

bool
Wrapper::enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp)
{
    *bp = true;
    return true;
}

JS_FRIEND_API(JSObject *)
js::UnwrapObject(JSObject *wrapped, bool stopAtOuter, unsigned *flagsp)
{
    unsigned flags = 0;
    while (wrapped->isWrapper() &&
           !JS_UNLIKELY(stopAtOuter && wrapped->getClass()->ext.innerObject)) {
        flags |= Wrapper::wrapperHandler(wrapped)->flags();
        wrapped = GetProxyPrivate(wrapped).toObjectOrNull();
    }
    if (flagsp)
        *flagsp = flags;
    return wrapped;
}

JS_FRIEND_API(JSObject *)
js::UnwrapObjectChecked(JSContext *cx, RawObject objArg)
{
    RootedObject obj(cx, objArg);
    while (true) {
        JSObject *wrapper = obj;
        obj = UnwrapOneChecked(cx, obj);
        if (!obj || obj == wrapper)
            return obj;
    }
}

JS_FRIEND_API(JSObject *)
js::UnwrapOneChecked(JSContext *cx, HandleObject obj)
{
    
    if (!obj->isWrapper() ||
        JS_UNLIKELY(!!obj->getClass()->ext.innerObject))
    {
        return obj;
    }

    Wrapper *handler = Wrapper::wrapperHandler(obj);
    bool rvOnFailure;
    if (!handler->enter(cx, obj, JSID_VOID, Wrapper::PUNCTURE, &rvOnFailure))
        return rvOnFailure ? (JSObject*) obj : NULL;
    JSObject *ret = Wrapper::wrappedObject(obj);
    JS_ASSERT(ret);

    return ret;
}

bool
js::IsCrossCompartmentWrapper(RawObject wrapper)
{
    return wrapper->isWrapper() &&
           !!(Wrapper::wrapperHandler(wrapper)->flags() & Wrapper::CROSS_COMPARTMENT);
}

#define CHECKED(op, act)                                                     \
    JS_BEGIN_MACRO                                                           \
        bool status;                                                         \
        if (!enter(cx, wrapper, id, act, &status))                           \
            return status;                                                   \
        return (op);                                                         \
    JS_END_MACRO

#define SET(action) CHECKED(action, SET)
#define GET(action) CHECKED(action, GET)

Wrapper::Wrapper(unsigned flags, bool hasPrototype) : DirectProxyHandler(&sWrapperFamily)
                                                    , mFlags(flags)
{
    setHasPrototype(hasPrototype);
}

Wrapper::~Wrapper()
{
}

bool
Wrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                               jsid id, bool set,
                               PropertyDescriptor *desc)
{
    JS_ASSERT(!hasPrototype()); 
    desc->obj = NULL; 
    CHECKED(DirectProxyHandler::getPropertyDescriptor(cx, wrapper, id, set, desc),
            set ? SET : GET);
}

bool
Wrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                  jsid id, bool set,
                                  PropertyDescriptor *desc)
{
    desc->obj = NULL; 
    CHECKED(DirectProxyHandler::getOwnPropertyDescriptor(cx, wrapper, id, set, desc), GET);
}

bool
Wrapper::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                        PropertyDescriptor *desc)
{
    SET(DirectProxyHandler::defineProperty(cx, wrapper, id, desc));
}

bool
Wrapper::getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                             AutoIdVector &props)
{
    
    jsid id = JSID_VOID;
    GET(DirectProxyHandler::getOwnPropertyNames(cx, wrapper, props));
}

bool
Wrapper::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    *bp = true; 
    SET(DirectProxyHandler::delete_(cx, wrapper, id, bp));
}

bool
Wrapper::enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    JS_ASSERT(!hasPrototype()); 
    
    static jsid id = JSID_VOID;
    GET(DirectProxyHandler::enumerate(cx, wrapper, props));
}












bool
Wrapper::defaultValue(JSContext *cx, JSObject *wrapper_, JSType hint, Value *vp)
{
    RootedObject wrapper(cx, wrapper_);

    bool status;
    if (!enter(cx, wrapper_, JSID_VOID, PUNCTURE, &status)) {
        RootedValue v(cx);
        JS_ClearPendingException(cx);
        if (!DefaultValue(cx, wrapper, hint, &v))
            return false;
        *vp = v;
        return true;
    }
    







    AutoCompartment call(cx, wrappedObject(wrapper));
    return DirectProxyHandler::defaultValue(cx, wrapper_, hint, vp);
}

bool
Wrapper::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JS_ASSERT(!hasPrototype()); 
    *bp = false; 
    GET(DirectProxyHandler::has(cx, wrapper, id, bp));
}

bool
Wrapper::hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    *bp = false; 
    GET(DirectProxyHandler::hasOwn(cx, wrapper, id, bp));
}

bool
Wrapper::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, Value *vp)
{
    vp->setUndefined(); 
    GET(DirectProxyHandler::get(cx, wrapper, receiver, id, vp));
}

bool
Wrapper::set(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id, bool strict,
             Value *vp)
{
    SET(DirectProxyHandler::set(cx, wrapper, receiver, id, strict, vp));
}

bool
Wrapper::keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    
    const jsid id = JSID_VOID;
    GET(DirectProxyHandler::keys(cx, wrapper, props));
}

bool
Wrapper::iterate(JSContext *cx, JSObject *wrapper, unsigned flags, Value *vp)
{
    JS_ASSERT(!hasPrototype()); 
    vp->setUndefined(); 
    const jsid id = JSID_VOID;
    GET(DirectProxyHandler::iterate(cx, wrapper, flags, vp));
}

bool
Wrapper::call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp)
{
    vp->setUndefined(); 
    const jsid id = JSID_VOID;
    CHECKED(DirectProxyHandler::call(cx, wrapper, argc, vp), CALL);
}

bool
Wrapper::construct(JSContext *cx, JSObject *wrapper, unsigned argc, Value *argv, Value *vp)
{
    vp->setUndefined(); 
    const jsid id = JSID_VOID;
    CHECKED(DirectProxyHandler::construct(cx, wrapper, argc, argv, vp), CALL);
}

bool
Wrapper::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args)
{
    const jsid id = JSID_VOID;
    Rooted<JSObject*> wrapper(cx, &args.thisv().toObject());
    CHECKED(DirectProxyHandler::nativeCall(cx, test, impl, args), CALL);
}

bool
Wrapper::hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v, bool *bp)
{
    *bp = false; 
    const jsid id = JSID_VOID;
    GET(DirectProxyHandler::hasInstance(cx, wrapper, v, bp));
}

JSString *
Wrapper::obj_toString(JSContext *cx, JSObject *wrapper)
{
    bool status;
    if (!enter(cx, wrapper, JSID_VOID, GET, &status)) {
        if (status) {
            
            return JS_NewStringCopyZ(cx, "[object Object]");
        }
        return NULL;
    }
    JSString *str = DirectProxyHandler::obj_toString(cx, wrapper);
    return str;
}

JSString *
Wrapper::fun_toString(JSContext *cx, JSObject *wrapper, unsigned indent)
{
    bool status;
    if (!enter(cx, wrapper, JSID_VOID, GET, &status)) {
        if (status) {
            
            if (wrapper->isCallable())
                return JS_NewStringCopyZ(cx, "function () {\n    [native code]\n}");
            ReportIsNotFunction(cx, ObjectValue(*wrapper));
            return NULL;
        }
        return NULL;
    }
    JSString *str = DirectProxyHandler::fun_toString(cx, wrapper, indent);
    return str;
}

Wrapper Wrapper::singleton((unsigned)0);
Wrapper Wrapper::singletonWithPrototype((unsigned)0, true);



extern JSObject *
js::TransparentObjectWrapper(JSContext *cx, JSObject *existing, JSObject *objArg,
                             JSObject *wrappedProtoArg, JSObject *parentArg,
                             unsigned flags)
{
    RootedObject obj(cx, objArg);
    RootedObject wrappedProto(cx, wrappedProtoArg);
    RootedObject parent(cx, parentArg);

    
    JS_ASSERT(!obj->isWrapper() || obj->getClass()->ext.innerObject);
    return Wrapper::New(cx, obj, wrappedProto, parent, &CrossCompartmentWrapper::singleton);
}

ErrorCopier::~ErrorCopier()
{
    JSContext *cx = ac.ref().context();
    if (ac.ref().origin() != cx->compartment && cx->isExceptionPending()) {
        Value exc = cx->getPendingException();
        if (exc.isObject() && exc.toObject().isError() && exc.toObject().getPrivate()) {
            cx->clearPendingException();
            ac.destroy();
            Rooted<JSObject*> errObj(cx, &exc.toObject());
            JSObject *copyobj = js_CopyErrorObject(cx, errObj, scope);
            if (copyobj)
                cx->setPendingException(ObjectValue(*copyobj));
        }
    }
}



CrossCompartmentWrapper::CrossCompartmentWrapper(unsigned flags, bool hasPrototype)
  : Wrapper(CROSS_COMPARTMENT | flags, hasPrototype)
{
}

CrossCompartmentWrapper::~CrossCompartmentWrapper()
{
}

#define PIERCE(cx, wrapper, mode, pre, op, post)                \
    JS_BEGIN_MACRO                                              \
        bool ok;                                                \
        {                                                       \
            AutoCompartment call(cx, wrappedObject(wrapper));   \
            ok = (pre) && (op);                                 \
        }                                                       \
        return ok && (post);                                    \
    JS_END_MACRO

#define NOTHING (true)

bool
CrossCompartmentWrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                               bool set, PropertyDescriptor *desc)
{
    PIERCE(cx, wrapper, set ? SET : GET,
           cx->compartment->wrapId(cx, &id),
           Wrapper::getPropertyDescriptor(cx, wrapper, id, set, desc),
           cx->compartment->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                                  bool set, PropertyDescriptor *desc)
{
    PIERCE(cx, wrapper, set ? SET : GET,
           cx->compartment->wrapId(cx, &id),
           Wrapper::getOwnPropertyDescriptor(cx, wrapper, id, set, desc),
           cx->compartment->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::defineProperty(JSContext *cx, JSObject *wrapper, jsid id, PropertyDescriptor *desc)
{
    AutoPropertyDescriptorRooter desc2(cx, desc);
    PIERCE(cx, wrapper, SET,
           cx->compartment->wrapId(cx, &id) && cx->compartment->wrap(cx, &desc2),
           Wrapper::defineProperty(cx, wrapper, id, &desc2),
           NOTHING);
}

bool
CrossCompartmentWrapper::getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    PIERCE(cx, wrapper, GET,
           NOTHING,
           Wrapper::getOwnPropertyNames(cx, wrapper, props),
           cx->compartment->wrap(cx, props));
}

bool
CrossCompartmentWrapper::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    PIERCE(cx, wrapper, SET,
           cx->compartment->wrapId(cx, &id),
           Wrapper::delete_(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    PIERCE(cx, wrapper, GET,
           NOTHING,
           Wrapper::enumerate(cx, wrapper, props),
           cx->compartment->wrap(cx, props));
}

bool
CrossCompartmentWrapper::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    PIERCE(cx, wrapper, GET,
           cx->compartment->wrapId(cx, &id),
           Wrapper::has(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::hasOwn(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    PIERCE(cx, wrapper, GET,
           cx->compartment->wrapId(cx, &id),
           Wrapper::hasOwn(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::get(JSContext *cx, JSObject *wrapperArg, JSObject *receiverArg,
                             jsid idArg, Value *vp)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedObject receiver(cx, receiverArg);
    RootedId id(cx, idArg);
    PIERCE(cx, wrapper, GET,
           cx->compartment->wrap(cx, receiver.address()) && cx->compartment->wrapId(cx, id.address()),
           Wrapper::get(cx, wrapper, receiver, id, vp),
           cx->compartment->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::set(JSContext *cx, JSObject *wrapper_, JSObject *receiver_, jsid id_,
                             bool strict, Value *vp)
{
    RootedObject wrapper(cx, wrapper_), receiver(cx, receiver_);
    RootedId id(cx, id_);
    RootedValue value(cx, *vp);
    PIERCE(cx, wrapper, SET,
           cx->compartment->wrap(cx, receiver.address()) &&
           cx->compartment->wrapId(cx, id.address()) &&
           cx->compartment->wrap(cx, value.address()),
           Wrapper::set(cx, wrapper, receiver, id, strict, value.address()),
           NOTHING);
}

bool
CrossCompartmentWrapper::keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    PIERCE(cx, wrapper, GET,
           NOTHING,
           Wrapper::keys(cx, wrapper, props),
           cx->compartment->wrap(cx, props));
}





static bool
CanReify(Value *vp)
{
    JSObject *obj;
    return vp->isObject() &&
           (obj = &vp->toObject())->isPropertyIterator() &&
           (obj->asPropertyIterator().getNativeIterator()->flags & JSITER_ENUMERATE);
}

struct AutoCloseIterator
{
    AutoCloseIterator(JSContext *cx, JSObject *obj) : cx(cx), obj(cx, obj) {}

    ~AutoCloseIterator() { if (obj) CloseIterator(cx, obj); }

    void clear() { obj = NULL; }

  private:
    JSContext *cx;
    RootedObject obj;
};

static bool
Reify(JSContext *cx, JSCompartment *origin, Value *vp)
{
    Rooted<PropertyIteratorObject*> iterObj(cx, &vp->toObject().asPropertyIterator());
    NativeIterator *ni = iterObj->getNativeIterator();

    AutoCloseIterator close(cx, iterObj);

    
    RootedObject obj(cx, ni->obj);
    if (!origin->wrap(cx, obj.address()))
        return false;

    




    size_t length = ni->numKeys();
    bool isKeyIter = ni->isKeyIter();
    AutoIdVector keys(cx);
    if (length > 0) {
        if (!keys.reserve(length))
            return false;
        for (size_t i = 0; i < length; ++i) {
            jsid id;
            if (!ValueToId(cx, StringValue(ni->begin()[i]), &id))
                return false;
            keys.infallibleAppend(id);
            if (!origin->wrapId(cx, &keys[i]))
                return false;
        }
    }

    close.clear();
    if (!CloseIterator(cx, iterObj))
        return false;

    RootedValue value(cx, *vp);

    if (isKeyIter) {
        if (!VectorToKeyIterator(cx, obj, ni->flags, keys, &value))
            return false;
    } else {
        if (!VectorToValueIterator(cx, obj, ni->flags, keys, &value))
            return false;
    }

    *vp = value;
    return true;
}

bool
CrossCompartmentWrapper::iterate(JSContext *cx, JSObject *wrapper, unsigned flags, Value *vp)
{
    PIERCE(cx, wrapper, GET,
           NOTHING,
           Wrapper::iterate(cx, wrapper, flags, vp),
           CanReify(vp) ? Reify(cx, cx->compartment, vp) : cx->compartment->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::call(JSContext *cx, JSObject *wrapper_, unsigned argc, Value *vp)
{
    RootedObject wrapper(cx, wrapper_);
    JSObject *wrapped = wrappedObject(wrapper);
    {
        AutoCompartment call(cx, wrapped);

        vp[0] = ObjectValue(*wrapped);
        if (!cx->compartment->wrap(cx, &vp[1]))
            return false;
        Value *argv = JS_ARGV(cx, vp);
        for (size_t n = 0; n < argc; ++n) {
            if (!cx->compartment->wrap(cx, &argv[n]))
                return false;
        }
        if (!Wrapper::call(cx, wrapper, argc, vp))
            return false;
    }
    return cx->compartment->wrap(cx, vp);
}

bool
CrossCompartmentWrapper::construct(JSContext *cx, JSObject *wrapper_, unsigned argc, Value *argv,
                                   Value *rval)
{
    RootedObject wrapper(cx, wrapper_);
    JSObject *wrapped = wrappedObject(wrapper);
    {
        AutoCompartment call(cx, wrapped);

        for (size_t n = 0; n < argc; ++n) {
            if (!cx->compartment->wrap(cx, &argv[n]))
                return false;
        }
        if (!Wrapper::construct(cx, wrapper, argc, argv, rval))
            return false;
    }
    return cx->compartment->wrap(cx, rval);
}

bool
CrossCompartmentWrapper::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                    CallArgs srcArgs)
{
    Rooted<JSObject*> wrapper(cx, &srcArgs.thisv().toObject());
    JS_ASSERT(srcArgs.thisv().isMagic(JS_IS_CONSTRUCTING) ||
              !UnwrapObject(wrapper)->isCrossCompartmentWrapper());

    RootedObject wrapped(cx, wrappedObject(wrapper));
    {
        AutoCompartment call(cx, wrapped);
        InvokeArgsGuard dstArgs;
        if (!cx->stack.pushInvokeArgs(cx, srcArgs.length(), &dstArgs))
            return false;

        Value *src = srcArgs.base();
        Value *srcend = srcArgs.array() + srcArgs.length();
        Value *dst = dstArgs.base();
        for (; src < srcend; ++src, ++dst) {
            *dst = *src;
            if (!cx->compartment->wrap(cx, dst))
                return false;
        }

        if (!CallNonGenericMethod(cx, test, impl, dstArgs))
            return false;

        srcArgs.rval().set(dstArgs.rval());
        dstArgs.pop();
    }
    return cx->compartment->wrap(cx, srcArgs.rval().address());
}

bool
CrossCompartmentWrapper::hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v, bool *bp)
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    if (!cx->compartment->wrap(cx, v.address()))
        return false;
    return Wrapper::hasInstance(cx, wrapper, v, bp);
}

JSString *
CrossCompartmentWrapper::obj_toString(JSContext *cx, JSObject *wrapper)
{
    JSString *str = NULL;
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::obj_toString(cx, wrapper);
        if (!str)
            return NULL;
    }
    if (!cx->compartment->wrap(cx, &str))
        return NULL;
    return str;
}

JSString *
CrossCompartmentWrapper::fun_toString(JSContext *cx, JSObject *wrapper, unsigned indent)
{
    JSString *str = NULL;
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::fun_toString(cx, wrapper, indent);
        if (!str)
            return NULL;
    }
    if (!cx->compartment->wrap(cx, &str))
        return NULL;
    return str;
}

bool
CrossCompartmentWrapper::regexp_toShared(JSContext *cx, JSObject *wrapper, RegExpGuard *g)
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    return Wrapper::regexp_toShared(cx, wrapper, g);
}

bool
CrossCompartmentWrapper::defaultValue(JSContext *cx, JSObject *wrapper, JSType hint, Value *vp)
{
    if (!Wrapper::defaultValue(cx, wrapper, hint, vp))
        return false;
    return cx->compartment->wrap(cx, vp);
}

bool
CrossCompartmentWrapper::iteratorNext(JSContext *cx, JSObject *wrapper, Value *vp)
{
    PIERCE(cx, wrapper, GET,
           NOTHING,
           DirectProxyHandler::iteratorNext(cx, wrapper, vp),
           cx->compartment->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::getPrototypeOf(JSContext *cx, JSObject *proxy, JSObject **protop)
{
    assertSameCompartment(cx, proxy);

    if (!proxy->getTaggedProto().isLazy()) {
        *protop = proxy->getTaggedProto().toObjectOrNull();
        return true;
    }

    RootedObject proto(cx);
    {
        RootedObject wrapped(cx, wrappedObject(proxy));
        AutoCompartment call(cx, wrapped);
        if (!JSObject::getProto(cx, wrapped, &proto))
            return false;
        if (proto)
            proto->setDelegate(cx);
    }

    if (!proxy->compartment()->wrap(cx, proto.address()))
        return false;

    *protop = proto;
    return true;
}

CrossCompartmentWrapper CrossCompartmentWrapper::singleton(0u);



template <class Base>
SecurityWrapper<Base>::SecurityWrapper(unsigned flags)
  : Base(flags)
{}

template <class Base>
bool
SecurityWrapper<Base>::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                  CallArgs args)
{
    



    return Base::nativeCall(cx, test, impl, args);
}

template <class Base>
bool
SecurityWrapper<Base>::objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx)
{
    



    return Base::objectClassIs(obj, classValue, cx);
}

template <class Base>
bool
SecurityWrapper<Base>::regexp_toShared(JSContext *cx, JSObject *obj, RegExpGuard *g)
{
    return Base::regexp_toShared(cx, obj, g);
}


template class js::SecurityWrapper<Wrapper>;
template class js::SecurityWrapper<CrossCompartmentWrapper>;

namespace js {

DeadObjectProxy::DeadObjectProxy()
  : BaseProxyHandler(&sDeadObjectFamily)
{
}

bool
DeadObjectProxy::getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                       jsid id, bool set,
                                       PropertyDescriptor *desc)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                          jsid id, bool set,
                                          PropertyDescriptor *desc)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::defineProperty(JSContext *cx, JSObject *wrapper, jsid id,
                                PropertyDescriptor *desc)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::getOwnPropertyNames(JSContext *cx, JSObject *wrapper,
                                     AutoIdVector &props)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::delete_(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::enumerate(JSContext *cx, JSObject *wrapper,
                           AutoIdVector &props)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::call(JSContext *cx, JSObject *wrapper, unsigned argc, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::construct(JSContext *cx, JSObject *wrapper, unsigned argc,
                           Value *vp, Value *rval)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v,
                             bool *bp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

JSString *
DeadObjectProxy::obj_toString(JSContext *cx, JSObject *wrapper)
{
    return JS_NewStringCopyZ(cx, "[object DeadObject]");
}

JSString *
DeadObjectProxy::fun_toString(JSContext *cx, JSObject *proxy, unsigned indent)
{
    return NULL;
}

bool
DeadObjectProxy::regexp_toShared(JSContext *cx, JSObject *proxy, RegExpGuard *g)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::defaultValue(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::iteratorNext(JSContext *cx, JSObject *proxy, Value *vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::getElementIfPresent(JSContext *cx, JSObject *obj, JSObject *receiver,
                                     uint32_t index, Value *vp, bool *present)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::getPrototypeOf(JSContext *cx, JSObject *proxy, JSObject **protop)
{
    *protop = NULL;
    return true;
}

DeadObjectProxy DeadObjectProxy::singleton;
int DeadObjectProxy::sDeadObjectFamily;

} 

JSObject *
js::NewDeadProxyObject(JSContext *cx, JSObject *parent)
{
    return NewProxyObject(cx, &DeadObjectProxy::singleton, NullValue(),
                          NULL, parent, NULL, NULL);
}

bool
js::IsDeadProxyObject(RawObject obj)
{
    return IsProxy(obj) && GetProxyHandler(obj) == &DeadObjectProxy::singleton;
}

void
js::NukeCrossCompartmentWrapper(JSContext *cx, JSObject *wrapper)
{
    JS_ASSERT(IsCrossCompartmentWrapper(wrapper));

    SetProxyPrivate(wrapper, NullValue());
    SetProxyHandler(wrapper, &DeadObjectProxy::singleton);

    if (IsFunctionProxy(wrapper)) {
        wrapper->setReservedSlot(JSSLOT_PROXY_CALL, NullValue());
        wrapper->setReservedSlot(JSSLOT_PROXY_CONSTRUCT, NullValue());
    }

    wrapper->setReservedSlot(JSSLOT_PROXY_EXTRA + 0, NullValue());
    wrapper->setReservedSlot(JSSLOT_PROXY_EXTRA + 1, NullValue());
}









JS_FRIEND_API(JSBool)
js::NukeCrossCompartmentWrappers(JSContext* cx, 
                                 const CompartmentFilter& sourceFilter,
                                 const CompartmentFilter& targetFilter,
                                 js::NukeReferencesToWindow nukeReferencesToWindow)
{
    CHECK_REQUEST(cx);
    JSRuntime *rt = cx->runtime;

    
    

    for (CompartmentsIter c(rt); !c.done(); c.next()) {
        if (!sourceFilter.match(c))
            continue;

        
        WrapperMap &pmap = c->crossCompartmentWrappers;
        for (WrapperMap::Enum e(pmap); !e.empty(); e.popFront()) {
            
            
            const CrossCompartmentKey &k = e.front().key;
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            JSObject *wobj = &e.front().value.get().toObject();
            JSObject *wrapped = UnwrapObject(wobj);

            if (nukeReferencesToWindow == DontNukeWindowReferences &&
                wrapped->getClass()->ext.innerObject)
                continue;

            if (targetFilter.match(wrapped->compartment())) {
                
                e.removeFront();
                NukeCrossCompartmentWrapper(cx, wobj);
            }
        }
    }

    return JS_TRUE;
}




bool
js::RemapWrapper(JSContext *cx, JSObject *wobj, JSObject *newTarget)
{
    JS_ASSERT(IsCrossCompartmentWrapper(wobj));
    JS_ASSERT(!IsCrossCompartmentWrapper(newTarget));
    JSObject *origTarget = Wrapper::wrappedObject(wobj);
    JS_ASSERT(origTarget);
    Value origv = ObjectValue(*origTarget);
    JSCompartment *wcompartment = wobj->compartment();
    WrapperMap &pmap = wcompartment->crossCompartmentWrappers;

    
    
    
    JS_ASSERT_IF(origTarget != newTarget, !pmap.has(ObjectValue(*newTarget)));

    
    
    JS_ASSERT(&pmap.lookup(origv)->value.toObject() == wobj);
    pmap.remove(origv);

    
    
    NukeCrossCompartmentWrapper(cx, wobj);

    
    
    
    JSObject *tobj = newTarget;
    AutoCompartment ac(cx, wobj);
    if (!wcompartment->wrap(cx, &tobj, wobj))
        return false;

    
    
    
    
    if (tobj != wobj) {
        
        
        
        if (!wobj->swap(cx, tobj))
            return false;
    }

    
    
    JS_ASSERT(Wrapper::wrappedObject(wobj) == newTarget);

    
    
    pmap.put(ObjectValue(*newTarget), ObjectValue(*wobj));
    return true;
}



JS_FRIEND_API(bool)
js::RemapAllWrappersForObject(JSContext *cx, JSObject *oldTarget,
                              JSObject *newTarget)
{
    Value origv = ObjectValue(*oldTarget);

    AutoValueVector toTransplant(cx);
    if (!toTransplant.reserve(cx->runtime->compartments.length()))
        return false;

    for (CompartmentsIter c(cx->runtime); !c.done(); c.next()) {
        WrapperMap &pmap = c->crossCompartmentWrappers;
        if (WrapperMap::Ptr wp = pmap.lookup(origv)) {
            
            toTransplant.infallibleAppend(wp->value);
        }
    }

    for (Value *begin = toTransplant.begin(), *end = toTransplant.end();
         begin != end; ++begin)
    {
        if (!RemapWrapper(cx, &begin->toObject(), newTarget))
            return false;
    }

    return true;
}

JS_FRIEND_API(bool)
js::RecomputeWrappers(JSContext *cx, const CompartmentFilter &sourceFilter,
                      const CompartmentFilter &targetFilter)
{
    AutoValueVector toRecompute(cx);

    for (CompartmentsIter c(cx->runtime); !c.done(); c.next()) {
        
        if (!sourceFilter.match(c))
            continue;

        
        WrapperMap &pmap = c->crossCompartmentWrappers;
        for (WrapperMap::Enum e(pmap); !e.empty(); e.popFront()) {
            
            const CrossCompartmentKey &k = e.front().key;
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            
            Value wrapper = e.front().value.get();
            if (!targetFilter.match(k.wrapped->compartment()))
                continue;

            
            if (!toRecompute.append(wrapper))
                return false;
        }
    }

    
    for (Value *begin = toRecompute.begin(), *end = toRecompute.end(); begin != end; ++begin)
    {
        JSObject *wrapper = &begin->toObject();
        JSObject *wrapped = Wrapper::wrappedObject(wrapper);
        if (!RemapWrapper(cx, wrapper, wrapped))
            return false;
    }

    return true;
}
