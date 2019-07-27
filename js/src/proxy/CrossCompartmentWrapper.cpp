





#include "jswrapper.h"

#include "jsiter.h"

#include "proxy/DeadObjectProxy.h"
#include "vm/WrapperObject.h"

#include "jscompartmentinlines.h"
#include "jsobjinlines.h"

using namespace js;

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
CrossCompartmentWrapper::isExtensible(JSContext *cx, HandleObject wrapper, bool *extensible) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::isExtensible(cx, wrapper, extensible),
           NOTHING);
}

bool
CrossCompartmentWrapper::preventExtensions(JSContext *cx, HandleObject wrapper) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::preventExtensions(cx, wrapper),
           NOTHING);
}

bool
CrossCompartmentWrapper::getPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                               MutableHandle<PropertyDescriptor> desc) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getPropertyDescriptor(cx, wrapper, id, desc),
           cx->compartment()->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id,
                                                  MutableHandle<PropertyDescriptor> desc) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getOwnPropertyDescriptor(cx, wrapper, id, desc),
           cx->compartment()->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::defineProperty(JSContext *cx, HandleObject wrapper, HandleId id,
                                        MutableHandle<PropertyDescriptor> desc) const
{
    Rooted<PropertyDescriptor> desc2(cx, desc);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &desc2),
           Wrapper::defineProperty(cx, wrapper, id, &desc2),
           NOTHING);
}

bool
CrossCompartmentWrapper::getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                             AutoIdVector &props) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getOwnPropertyNames(cx, wrapper, props),
           NOTHING);
}

bool
CrossCompartmentWrapper::delete_(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::delete_(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::enumerate(JSContext *cx, HandleObject wrapper, AutoIdVector &props) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::enumerate(cx, wrapper, props),
           NOTHING);
}

bool
CrossCompartmentWrapper::has(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::has(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::hasOwn(JSContext *cx, HandleObject wrapper, HandleId id, bool *bp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::hasOwn(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::get(JSContext *cx, HandleObject wrapper, HandleObject receiver,
                             HandleId id, MutableHandleValue vp) const
{
    RootedObject receiverCopy(cx, receiver);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!cx->compartment()->wrap(cx, &receiverCopy))
            return false;

        if (!Wrapper::get(cx, wrapper, receiverCopy, id, vp))
            return false;
    }
    return cx->compartment()->wrap(cx, vp);
}

bool
CrossCompartmentWrapper::set(JSContext *cx, HandleObject wrapper, HandleObject receiver,
                             HandleId id, bool strict, MutableHandleValue vp) const
{
    RootedObject receiverCopy(cx, receiver);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &receiverCopy) &&
           cx->compartment()->wrap(cx, vp),
           Wrapper::set(cx, wrapper, receiverCopy, id, strict, vp),
           NOTHING);
}

bool
CrossCompartmentWrapper::keys(JSContext *cx, HandleObject wrapper, AutoIdVector &props) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::keys(cx, wrapper, props),
           NOTHING);
}





static bool
CanReify(HandleValue vp)
{
    JSObject *obj;
    return vp.isObject() &&
           (obj = &vp.toObject())->is<PropertyIteratorObject>() &&
           (obj->as<PropertyIteratorObject>().getNativeIterator()->flags & JSITER_ENUMERATE);
}

struct AutoCloseIterator
{
    AutoCloseIterator(JSContext *cx, JSObject *obj) : cx(cx), obj(cx, obj) {}

    ~AutoCloseIterator() { if (obj) CloseIterator(cx, obj); }

    void clear() { obj = nullptr; }

  private:
    JSContext *cx;
    RootedObject obj;
};

static bool
Reify(JSContext *cx, JSCompartment *origin, MutableHandleValue vp)
{
    Rooted<PropertyIteratorObject*> iterObj(cx, &vp.toObject().as<PropertyIteratorObject>());
    NativeIterator *ni = iterObj->getNativeIterator();

    AutoCloseIterator close(cx, iterObj);

    
    RootedObject obj(cx, ni->obj);
    if (!origin->wrap(cx, &obj))
        return false;

    




    size_t length = ni->numKeys();
    bool isKeyIter = ni->isKeyIter();
    AutoIdVector keys(cx);
    if (length > 0) {
        if (!keys.reserve(length))
            return false;
        for (size_t i = 0; i < length; ++i) {
            RootedId id(cx);
            RootedValue v(cx, StringValue(ni->begin()[i]));
            if (!ValueToId<CanGC>(cx, v, &id))
                return false;
            keys.infallibleAppend(id);
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
CrossCompartmentWrapper::iterate(JSContext *cx, HandleObject wrapper, unsigned flags,
                                 MutableHandleValue vp) const
{
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!Wrapper::iterate(cx, wrapper, flags, vp))
            return false;
    }

    if (CanReify(vp))
        return Reify(cx, cx->compartment(), vp);
    return cx->compartment()->wrap(cx, vp);
}

bool
CrossCompartmentWrapper::call(JSContext *cx, HandleObject wrapper, const CallArgs &args) const
{
    RootedObject wrapped(cx, wrappedObject(wrapper));

    {
        AutoCompartment call(cx, wrapped);

        args.setCallee(ObjectValue(*wrapped));
        if (!cx->compartment()->wrap(cx, args.mutableThisv()))
            return false;

        for (size_t n = 0; n < args.length(); ++n) {
            if (!cx->compartment()->wrap(cx, args[n]))
                return false;
        }

        if (!Wrapper::call(cx, wrapper, args))
            return false;
    }

    return cx->compartment()->wrap(cx, args.rval());
}

bool
CrossCompartmentWrapper::construct(JSContext *cx, HandleObject wrapper, const CallArgs &args) const
{
    RootedObject wrapped(cx, wrappedObject(wrapper));
    {
        AutoCompartment call(cx, wrapped);

        for (size_t n = 0; n < args.length(); ++n) {
            if (!cx->compartment()->wrap(cx, args[n]))
                return false;
        }
        if (!Wrapper::construct(cx, wrapper, args))
            return false;
    }
    return cx->compartment()->wrap(cx, args.rval());
}

bool
CrossCompartmentWrapper::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                    CallArgs srcArgs) const
{
    RootedObject wrapper(cx, &srcArgs.thisv().toObject());
    JS_ASSERT(srcArgs.thisv().isMagic(JS_IS_CONSTRUCTING) ||
              !UncheckedUnwrap(wrapper)->is<CrossCompartmentWrapperObject>());

    RootedObject wrapped(cx, wrappedObject(wrapper));
    {
        AutoCompartment call(cx, wrapped);
        InvokeArgs dstArgs(cx);
        if (!dstArgs.init(srcArgs.length()))
            return false;

        Value *src = srcArgs.base();
        Value *srcend = srcArgs.array() + srcArgs.length();
        Value *dst = dstArgs.base();

        RootedValue source(cx);
        for (; src < srcend; ++src, ++dst) {
            source = *src;
            if (!cx->compartment()->wrap(cx, &source))
                return false;
            *dst = source.get();

            
            
            
            
            if ((src == srcArgs.base() + 1) && dst->isObject()) {
                RootedObject thisObj(cx, &dst->toObject());
                if (thisObj->is<WrapperObject>() &&
                    Wrapper::wrapperHandler(thisObj)->hasSecurityPolicy())
                {
                    JS_ASSERT(!thisObj->is<CrossCompartmentWrapperObject>());
                    *dst = ObjectValue(*Wrapper::wrappedObject(thisObj));
                }
            }
        }

        if (!CallNonGenericMethod(cx, test, impl, dstArgs))
            return false;

        srcArgs.rval().set(dstArgs.rval());
    }
    return cx->compartment()->wrap(cx, srcArgs.rval());
}

bool
CrossCompartmentWrapper::hasInstance(JSContext *cx, HandleObject wrapper, MutableHandleValue v,
                                     bool *bp) const
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    if (!cx->compartment()->wrap(cx, v))
        return false;
    return Wrapper::hasInstance(cx, wrapper, v, bp);
}

const char *
CrossCompartmentWrapper::className(JSContext *cx, HandleObject wrapper) const
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    return Wrapper::className(cx, wrapper);
}

JSString *
CrossCompartmentWrapper::fun_toString(JSContext *cx, HandleObject wrapper, unsigned indent) const
{
    RootedString str(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::fun_toString(cx, wrapper, indent);
        if (!str)
            return nullptr;
    }
    if (!cx->compartment()->wrap(cx, str.address()))
        return nullptr;
    return str;
}

bool
CrossCompartmentWrapper::regexp_toShared(JSContext *cx, HandleObject wrapper, RegExpGuard *g) const
{
    RegExpGuard wrapperGuard(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!Wrapper::regexp_toShared(cx, wrapper, &wrapperGuard))
            return false;
    }

    
    RegExpShared *re = wrapperGuard.re();
    return cx->compartment()->regExps.get(cx, re->getSource(), re->getFlags(), g);
}

bool
CrossCompartmentWrapper::boxedValue_unbox(JSContext *cx, HandleObject wrapper, MutableHandleValue vp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::boxedValue_unbox(cx, wrapper, vp),
           cx->compartment()->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::defaultValue(JSContext *cx, HandleObject wrapper, JSType hint,
                                      MutableHandleValue vp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::defaultValue(cx, wrapper, hint, vp),
           cx->compartment()->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::getPrototypeOf(JSContext *cx, HandleObject wrapper,
                                        MutableHandleObject protop) const
{
    {
        RootedObject wrapped(cx, wrappedObject(wrapper));
        AutoCompartment call(cx, wrapped);
        if (!JSObject::getProto(cx, wrapped, protop))
            return false;
        if (protop)
            protop->setDelegate(cx);
    }

    return cx->compartment()->wrap(cx, protop);
}

bool
CrossCompartmentWrapper::setPrototypeOf(JSContext *cx, HandleObject wrapper,
                                        HandleObject proto, bool *bp) const
{
    RootedObject protoCopy(cx, proto);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &protoCopy),
           Wrapper::setPrototypeOf(cx, wrapper, protoCopy, bp),
           NOTHING);
}

const CrossCompartmentWrapper CrossCompartmentWrapper::singleton(0u);

bool
js::IsCrossCompartmentWrapper(JSObject *obj)
{
    return IsWrapper(obj) &&
           !!(Wrapper::wrapperHandler(obj)->flags() & Wrapper::CROSS_COMPARTMENT);
}

void
js::NukeCrossCompartmentWrapper(JSContext *cx, JSObject *wrapper)
{
    JS_ASSERT(wrapper->is<CrossCompartmentWrapperObject>());

    NotifyGCNukeWrapper(wrapper);

    wrapper->as<ProxyObject>().nuke(&DeadObjectProxy::singleton);

    JS_ASSERT(IsDeadProxyObject(wrapper));
}









JS_FRIEND_API(bool)
js::NukeCrossCompartmentWrappers(JSContext* cx,
                                 const CompartmentFilter& sourceFilter,
                                 const CompartmentFilter& targetFilter,
                                 js::NukeReferencesToWindow nukeReferencesToWindow)
{
    CHECK_REQUEST(cx);
    JSRuntime *rt = cx->runtime();

    
    

    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (!sourceFilter.match(c))
            continue;

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            
            const CrossCompartmentKey &k = e.front().key();
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            AutoWrapperRooter wobj(cx, WrapperValue(e));
            JSObject *wrapped = UncheckedUnwrap(wobj);

            if (nukeReferencesToWindow == DontNukeWindowReferences &&
                wrapped->getClass()->ext.innerObject)
                continue;

            if (targetFilter.match(wrapped->compartment())) {
                
                e.removeFront();
                NukeCrossCompartmentWrapper(cx, wobj);
            }
        }
    }

    return true;
}




bool
js::RemapWrapper(JSContext *cx, JSObject *wobjArg, JSObject *newTargetArg)
{
    RootedObject wobj(cx, wobjArg);
    RootedObject newTarget(cx, newTargetArg);
    JS_ASSERT(wobj->is<CrossCompartmentWrapperObject>());
    JS_ASSERT(!newTarget->is<CrossCompartmentWrapperObject>());
    JSObject *origTarget = Wrapper::wrappedObject(wobj);
    JS_ASSERT(origTarget);
    Value origv = ObjectValue(*origTarget);
    JSCompartment *wcompartment = wobj->compartment();

    AutoDisableProxyCheck adpc(cx->runtime());

    
    
    
    JS_ASSERT_IF(origTarget != newTarget,
                 !wcompartment->lookupWrapper(ObjectValue(*newTarget)));

    
    
    WrapperMap::Ptr p = wcompartment->lookupWrapper(origv);
    JS_ASSERT(&p->value().unsafeGet()->toObject() == wobj);
    wcompartment->removeWrapper(p);

    
    
    NukeCrossCompartmentWrapper(cx, wobj);

    
    
    
    RootedObject tobj(cx, newTarget);
    AutoCompartment ac(cx, wobj);
    if (!wcompartment->wrap(cx, &tobj, wobj))
        MOZ_CRASH();

    
    
    
    
    if (tobj != wobj) {
        
        
        
        if (!JSObject::swap(cx, wobj, tobj))
            MOZ_CRASH();
    }

    
    
    JS_ASSERT(Wrapper::wrappedObject(wobj) == newTarget);

    
    
    JS_ASSERT(wobj->is<WrapperObject>());
    wcompartment->putWrapper(cx, CrossCompartmentKey(newTarget), ObjectValue(*wobj));
    return true;
}



JS_FRIEND_API(bool)
js::RemapAllWrappersForObject(JSContext *cx, JSObject *oldTargetArg,
                              JSObject *newTargetArg)
{
    RootedValue origv(cx, ObjectValue(*oldTargetArg));
    RootedObject newTarget(cx, newTargetArg);

    AutoWrapperVector toTransplant(cx);
    if (!toTransplant.reserve(cx->runtime()->numCompartments))
        return false;

    for (CompartmentsIter c(cx->runtime(), SkipAtoms); !c.done(); c.next()) {
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
    AutoWrapperVector toRecompute(cx);

    for (CompartmentsIter c(cx->runtime(), SkipAtoms); !c.done(); c.next()) {
        
        if (!sourceFilter.match(c))
            continue;

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            const CrossCompartmentKey &k = e.front().key();
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
