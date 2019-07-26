






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

int js::sWrapperFamily;

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

    AutoMarkInDeadZone amd(cx->zone());

    return NewProxyObject(cx, handler, ObjectValue(*obj), proto, parent,
                          obj->isCallable() ? obj : NULL, NULL);
}

JSObject *
Wrapper::Renew(JSContext *cx, JSObject *existing, JSObject *obj, Wrapper *handler)
{
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
js::UnwrapObjectChecked(RawObject obj, bool stopAtOuter)
{
    while (true) {
        JSObject *wrapper = obj;
        obj = UnwrapOneChecked(obj, stopAtOuter);
        if (!obj || obj == wrapper)
            return obj;
    }
}

JS_FRIEND_API(JSObject *)
js::UnwrapOneChecked(RawObject obj, bool stopAtOuter)
{
    if (!obj->isWrapper() ||
        JS_UNLIKELY(!!obj->getClass()->ext.innerObject && stopAtOuter))
    {
        return obj;
    }

    Wrapper *handler = Wrapper::wrapperHandler(obj);
    return handler->isSafeToUnwrap() ? Wrapper::wrappedObject(obj) : NULL;
}

bool
js::IsCrossCompartmentWrapper(RawObject wrapper)
{
    return wrapper->isWrapper() &&
           !!(Wrapper::wrapperHandler(wrapper)->flags() & Wrapper::CROSS_COMPARTMENT);
}

Wrapper::Wrapper(unsigned flags, bool hasPrototype) : DirectProxyHandler(&sWrapperFamily)
                                                    , mFlags(flags)
                                                    , mSafeToUnwrap(true)
{
    setHasPrototype(hasPrototype);
}

Wrapper::~Wrapper()
{
}











bool
Wrapper::defaultValue(JSContext *cx, JSObject *wrapperArg, JSType hint, Value *vp)
{
    RootedObject wrapper(cx, wrapperArg);

    if (!wrapperHandler(wrapper)->isSafeToUnwrap()) {
        RootedValue v(cx);
        if (!DefaultValue(cx, wrapper, hint, &v))
            return false;
        *vp = v;
        return true;
    }
    







    AutoCompartment call(cx, wrappedObject(wrapper));
    return DirectProxyHandler::defaultValue(cx, wrapper, hint, vp);
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
        RootedValue exc(cx, cx->getPendingException());
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

bool CrossCompartmentWrapper::finalizeInBackground(HandleValue priv)
{
    if (!priv.isObject())
        return true;

    



    return IsBackgroundFinalized(priv.toObject().getAllocKind());
}

#define PIERCE(cx, wrapper, pre, op, post)                      \
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
CrossCompartmentWrapper::getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                               PropertyDescriptor *desc, unsigned flags)
{
    RootedId idCopy(cx, id);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, idCopy.address()),
           Wrapper::getPropertyDescriptor(cx, wrapper, idCopy, desc, flags),
           cx->compartment->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                                  HandleId id, PropertyDescriptor *desc,
                                                  unsigned flags)
{
    RootedId idCopy(cx, id);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, idCopy.address()),
           Wrapper::getOwnPropertyDescriptor(cx, wrapper, idCopy, desc, flags),
           cx->compartment->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                        PropertyDescriptor *desc)
{
    RootedId idCopy(cx, id);
    AutoPropertyDescriptorRooter desc2(cx, desc);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, idCopy.address()) && cx->compartment->wrap(cx, &desc2),
           Wrapper::defineProperty(cx, wrapper, idCopy, &desc2),
           NOTHING);
}

bool
CrossCompartmentWrapper::getOwnPropertyNames(JSContext *cx, JSObject *wrapperArg,
                                             AutoIdVector &props)
{
    RootedObject wrapper(cx, wrapperArg);
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getOwnPropertyNames(cx, wrapper, props),
           cx->compartment->wrap(cx, props));
}

bool
CrossCompartmentWrapper::delete_(JSContext *cx, JSObject *wrapperArg, jsid id, bool *bp)
{
    RootedObject wrapper(cx, wrapperArg);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, &id),
           Wrapper::delete_(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::enumerate(JSContext *cx, JSObject *wrapperArg, AutoIdVector &props)
{
    RootedObject wrapper(cx, wrapperArg);
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::enumerate(cx, wrapper, props),
           cx->compartment->wrap(cx, props));
}

bool
CrossCompartmentWrapper::has(JSContext *cx, JSObject *wrapperArg, jsid id, bool *bp)
{
    RootedObject wrapper(cx, wrapperArg);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, &id),
           Wrapper::has(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::hasOwn(JSContext *cx, JSObject *wrapperArg, jsid id, bool *bp)
{
    RootedObject wrapper(cx, wrapperArg);
    PIERCE(cx, wrapper,
           cx->compartment->wrapId(cx, &id),
           Wrapper::hasOwn(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::get(JSContext *cx, JSObject *wrapperArg, JSObject *receiverArg,
                             jsid idArg, Value *vpArg)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedObject receiver(cx, receiverArg);
    RootedId id(cx, idArg);
    RootedValue vp(cx, *vpArg);

    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!cx->compartment->wrap(cx, receiver.address()) ||
            !cx->compartment->wrapId(cx, id.address()))
        {
            return false;
        }

        if (!Wrapper::get(cx, wrapper, receiver, id, vp.address()))
            return false;
    }

    bool ok = cx->compartment->wrap(cx, &vp);
    *vpArg = vp.get();
    return ok;
}

bool
CrossCompartmentWrapper::set(JSContext *cx, JSObject *wrapper_, JSObject *receiver_, jsid id_,
                             bool strict, Value *valueArg)
{
    RootedObject wrapper(cx, wrapper_), receiver(cx, receiver_);
    RootedId id(cx, id_);
    RootedValue value(cx, *valueArg);
    PIERCE(cx, wrapper,
           cx->compartment->wrap(cx, receiver.address()) &&
           cx->compartment->wrapId(cx, id.address()) &&
           cx->compartment->wrap(cx, &value),
           Wrapper::set(cx, wrapper, receiver, id, strict, value.address()),
           NOTHING);
}

bool
CrossCompartmentWrapper::keys(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    PIERCE(cx, wrapper,
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
Reify(JSContext *cx, JSCompartment *origin, MutableHandleValue vp)
{
    Rooted<PropertyIteratorObject*> iterObj(cx, &vp.toObject().asPropertyIterator());
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
            RootedId id(cx);
            if (!ValueToId<CanGC>(cx, StringValue(ni->begin()[i]), &id))
                return false;
            keys.infallibleAppend(id);
            if (!origin->wrapId(cx, &keys[i]))
                return false;
        }
    }

    close.clear();
    if (!CloseIterator(cx, iterObj))
        return false;

    if (isKeyIter) {
        if (!VectorToKeyIterator(cx, obj, ni->flags, keys, vp))
            return false;
    } else {
        if (!VectorToValueIterator(cx, obj, ni->flags, keys, vp))
            return false;
    }
    return true;
}

bool
CrossCompartmentWrapper::iterate(JSContext *cx, JSObject *wrapperArg, unsigned flags, Value *vpArg)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedValue vp(cx, *vpArg);

    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!Wrapper::iterate(cx, wrapper, flags, vp.address()))
            return false;
    }

    bool ok = CanReify(vp.address())
              ? Reify(cx, cx->compartment, &vp)
              : cx->compartment->wrap(cx, &vp);
    *vpArg = vp.get();
    return ok;
}

bool
CrossCompartmentWrapper::call(JSContext *cx, JSObject *wrapperArg, unsigned argc, Value *vp)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedObject wrapped(cx, wrappedObject(wrapper));

    CallArgs args = CallArgsFromVp(argc, vp);
    {
        AutoCompartment call(cx, wrapped);

        args.setCallee(ObjectValue(*wrapped));
        if (!cx->compartment->wrap(cx, args.mutableThisv()))
            return false;

        for (size_t n = 0; n < args.length(); ++n) {
            if (!cx->compartment->wrap(cx, args.handleAt(n)))
                return false;
        }

        if (!Wrapper::call(cx, wrapper, argc, vp))
            return false;
    }

    return cx->compartment->wrap(cx, args.rval());
}

bool
CrossCompartmentWrapper::construct(JSContext *cx, JSObject *wrapperArg, unsigned argc, Value *argv,
                                   Value *rvalArg)
{
    RootedObject wrapper(cx, wrapperArg);
    JSObject *wrapped = wrappedObject(wrapper);
    {
        AutoCompartment call(cx, wrapped);

        for (size_t n = 0; n < argc; ++n) {
            RootedValue arg(cx, argv[n]);
            if (!cx->compartment->wrap(cx, &arg))
                return false;
            argv[n] = arg;
        }
        if (!Wrapper::construct(cx, wrapper, argc, argv, rvalArg))
            return false;
    }
    RootedValue rval(cx, *rvalArg);
    bool ok = cx->compartment->wrap(cx, &rval);
    *rvalArg = rval;
    return ok;
}

bool
CrossCompartmentWrapper::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                    CallArgs srcArgs)
{
    RootedObject wrapper(cx, &srcArgs.thisv().toObject());
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

        RootedValue source(cx);
        for (; src < srcend; ++src, ++dst) {
            source = *src;
            if (!cx->compartment->wrap(cx, &source))
                return false;
            *dst = source.get();

            
            
            
            
            if ((src == srcArgs.base() + 1) && dst->isObject()) {
                JSObject *thisObj = &dst->toObject();
                if (thisObj->isWrapper() &&
                    !Wrapper::wrapperHandler(thisObj)->isSafeToUnwrap())
                {
                    JS_ASSERT(!IsCrossCompartmentWrapper(thisObj));
                    *dst = ObjectValue(*Wrapper::wrappedObject(thisObj));
                }
            }
        }

        if (!CallNonGenericMethod(cx, test, impl, dstArgs))
            return false;

        srcArgs.rval().set(dstArgs.rval());
        dstArgs.pop();
    }
    return cx->compartment->wrap(cx, srcArgs.rval());
}

bool
CrossCompartmentWrapper::hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v, bool *bp)
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    if (!cx->compartment->wrap(cx, v))
        return false;
    return Wrapper::hasInstance(cx, wrapper, v, bp);
}

JSString *
CrossCompartmentWrapper::obj_toString(JSContext *cx, JSObject *wrapperArg)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedString str(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::obj_toString(cx, wrapper);
        if (!str)
            return NULL;
    }
    if (!cx->compartment->wrap(cx, str.address()))
        return NULL;
    return str;
}

JSString *
CrossCompartmentWrapper::fun_toString(JSContext *cx, JSObject *wrapperArg, unsigned indent)
{
    RootedObject wrapper(cx, wrapperArg);
    RootedString str(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::fun_toString(cx, wrapper, indent);
        if (!str)
            return NULL;
    }
    if (!cx->compartment->wrap(cx, str.address()))
        return NULL;
    return str;
}

bool
CrossCompartmentWrapper::regexp_toShared(JSContext *cx, JSObject *wrapperArg, RegExpGuard *g)
{
    RootedObject wrapper(cx, wrapperArg);
    AutoCompartment call(cx, wrappedObject(wrapper));
    return Wrapper::regexp_toShared(cx, wrapper, g);
}

bool
CrossCompartmentWrapper::defaultValue(JSContext *cx, JSObject *wrapper, JSType hint, Value *vpArg)
{
    if (!Wrapper::defaultValue(cx, wrapper, hint, vpArg))
        return false;

    RootedValue vp(cx, *vpArg);
    bool ok = cx->compartment->wrap(cx, &vp);
    *vpArg = vp;
    return ok;
}

bool
CrossCompartmentWrapper::getPrototypeOf(JSContext *cx, JSObject *wrapperArg, JSObject **protop)
{
    RootedObject wrapper(cx, wrapperArg);
    assertSameCompartment(cx, wrapper);

    if (!wrapper->getTaggedProto().isLazy()) {
        *protop = wrapper->getTaggedProto().toObjectOrNull();
        return true;
    }

    RootedObject proto(cx);
    {
        RootedObject wrapped(cx, wrappedObject(wrapper));
        AutoCompartment call(cx, wrapped);
        if (!JSObject::getProto(cx, wrapped, &proto))
            return false;
        if (proto)
            proto->setDelegate(cx);
    }

    if (!wrapper->compartment()->wrap(cx, proto.address()))
        return false;

    *protop = proto;
    return true;
}

CrossCompartmentWrapper CrossCompartmentWrapper::singleton(0u);



template <class Base>
SecurityWrapper<Base>::SecurityWrapper(unsigned flags)
  : Base(flags)
{
    Base::setSafeToUnwrap(false);
    BaseProxyHandler::setHasPolicy(true);
}

template <class Base>
bool
SecurityWrapper<Base>::enter(JSContext *cx, JSObject *wrapper, jsid id,
                             Wrapper::Action act, bool *bp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNWRAP_DENIED);
    *bp = false;
    return false;
}

 template <class Base>
 bool
SecurityWrapper<Base>::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                  CallArgs args)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::objectClassIs(JSObject *obj, ESClassValue classValue, JSContext *cx)
{
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::regexp_toShared(JSContext *cx, JSObject *obj, RegExpGuard *g)
{
    return Base::regexp_toShared(cx, obj, g);
}


template class js::SecurityWrapper<Wrapper>;
template class js::SecurityWrapper<CrossCompartmentWrapper>;

DeadObjectProxy::DeadObjectProxy()
  : BaseProxyHandler(&sDeadObjectFamily)
{
}

bool
DeadObjectProxy::getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                       PropertyDescriptor *desc, unsigned flags)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                          PropertyDescriptor *desc, unsigned flags)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_DEAD_OBJECT);
    return false;
}

bool
DeadObjectProxy::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
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

static void
NukeSlot(JSObject *wrapper, uint32_t slot, Value v)
{
    Value old = wrapper->getSlot(slot);
    if (old.isMarkable()) {
        Cell *cell = static_cast<Cell *>(old.toGCThing());
        AutoMarkInDeadZone amd(cell->zone());
        wrapper->setReservedSlot(slot, v);
    } else {
        wrapper->setReservedSlot(slot, v);
    }
}

void
js::NukeCrossCompartmentWrapper(JSContext *cx, JSObject *wrapper)
{
    JS_ASSERT(IsCrossCompartmentWrapper(wrapper));

    NotifyGCNukeWrapper(wrapper);

    NukeSlot(wrapper, JSSLOT_PROXY_PRIVATE, NullValue());
    SetProxyHandler(wrapper, &DeadObjectProxy::singleton);

    if (IsFunctionProxy(wrapper)) {
        NukeSlot(wrapper, JSSLOT_PROXY_CALL, NullValue());
        NukeSlot(wrapper, JSSLOT_PROXY_CONSTRUCT, NullValue());
    }

    NukeSlot(wrapper, JSSLOT_PROXY_EXTRA + 0, NullValue());
    NukeSlot(wrapper, JSSLOT_PROXY_EXTRA + 1, NullValue());

    JS_ASSERT(IsDeadProxyObject(wrapper));
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

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            
            const CrossCompartmentKey &k = e.front().key;
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            AutoWrapperRooter wobj(cx, WrapperValue(e));
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
js::RemapWrapper(JSContext *cx, JSObject *wobjArg, JSObject *newTargetArg)
{
    RootedObject wobj(cx, wobjArg);
    RootedObject newTarget(cx, newTargetArg);
    JS_ASSERT(IsCrossCompartmentWrapper(wobj));
    JS_ASSERT(!IsCrossCompartmentWrapper(newTarget));
    JSObject *origTarget = Wrapper::wrappedObject(wobj);
    JS_ASSERT(origTarget);
    Value origv = ObjectValue(*origTarget);
    JSCompartment *wcompartment = wobj->compartment();

    
    
    
    JS_ASSERT_IF(origTarget != newTarget,
                 !wcompartment->lookupWrapper(ObjectValue(*newTarget)));

    
    
    WrapperMap::Ptr p = wcompartment->lookupWrapper(origv);
    JS_ASSERT(&p->value.unsafeGet()->toObject() == wobj);
    wcompartment->removeWrapper(p);

    
    
    NukeCrossCompartmentWrapper(cx, wobj);

    
    
    
    RootedObject tobj(cx, newTarget);
    AutoCompartment ac(cx, wobj);
    if (!wcompartment->wrap(cx, tobj.address(), wobj))
        MOZ_CRASH();

    
    
    
    
    if (tobj != wobj) {
        
        
        
        if (!JSObject::swap(cx, wobj, tobj))
            MOZ_CRASH();
    }

    
    
    JS_ASSERT(Wrapper::wrappedObject(wobj) == newTarget);

    
    
    wcompartment->putWrapper(ObjectValue(*newTarget), ObjectValue(*wobj));
    return true;
}



JS_FRIEND_API(bool)
js::RemapAllWrappersForObject(JSContext *cx, JSObject *oldTargetArg,
                              JSObject *newTargetArg)
{
    RootedValue origv(cx, ObjectValue(*oldTargetArg));
    RootedObject newTarget(cx, newTargetArg);

    AutoWrapperVector toTransplant(cx);
    if (!toTransplant.reserve(cx->runtime->numCompartments))
        return false;

    for (CompartmentsIter c(cx->runtime); !c.done(); c.next()) {
        if (WrapperMap::Ptr wp = c->lookupWrapper(origv)) {
            
            toTransplant.infallibleAppend(WrapperValue(wp));
        }
    }

    for (WrapperValue *begin = toTransplant.begin(), *end = toTransplant.end();
         begin != end; ++begin)
    {
        if (!RemapWrapper(cx, &begin->toObject(), newTarget))
            MOZ_CRASH();
    }

    return true;
}

JS_FRIEND_API(bool)
js::RecomputeWrappers(JSContext *cx, const CompartmentFilter &sourceFilter,
                      const CompartmentFilter &targetFilter)
{
    AutoMaybeTouchDeadZones agc(cx);

    AutoWrapperVector toRecompute(cx);

    for (CompartmentsIter c(cx->runtime); !c.done(); c.next()) {
        
        if (!sourceFilter.match(c))
            continue;

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            const CrossCompartmentKey &k = e.front().key;
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            
            if (!targetFilter.match(static_cast<JSObject *>(k.wrapped)->compartment()))
                continue;

            
            if (!toRecompute.append(WrapperValue(e)))
                return false;
        }
    }

    
    for (WrapperValue *begin = toRecompute.begin(), *end = toRecompute.end(); begin != end; ++begin)
    {
        JSObject *wrapper = &begin->toObject();
        JSObject *wrapped = Wrapper::wrappedObject(wrapper);
        if (!RemapWrapper(cx, wrapper, wrapped))
            MOZ_CRASH();
    }

    return true;
}
