





#include "jsiter.h"
#include "jswrapper.h"

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
CrossCompartmentWrapper::getPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                               MutableHandle<PropertyDescriptor> desc) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getPropertyDescriptor(cx, wrapper, id, desc),
           cx->compartment()->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::getOwnPropertyDescriptor(JSContext* cx, HandleObject wrapper, HandleId id,
                                                  MutableHandle<PropertyDescriptor> desc) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getOwnPropertyDescriptor(cx, wrapper, id, desc),
           cx->compartment()->wrap(cx, desc));
}

bool
CrossCompartmentWrapper::defineProperty(JSContext* cx, HandleObject wrapper, HandleId id,
                                        Handle<PropertyDescriptor> desc,
                                        ObjectOpResult& result) const
{
    Rooted<PropertyDescriptor> desc2(cx, desc);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &desc2),
           Wrapper::defineProperty(cx, wrapper, id, desc2, result),
           NOTHING);
}

bool
CrossCompartmentWrapper::ownPropertyKeys(JSContext* cx, HandleObject wrapper,
                                         AutoIdVector& props) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::ownPropertyKeys(cx, wrapper, props),
           NOTHING);
}

bool
CrossCompartmentWrapper::delete_(JSContext* cx, HandleObject wrapper, HandleId id,
                                 ObjectOpResult& result) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::delete_(cx, wrapper, id, result),
           NOTHING);
}

bool
CrossCompartmentWrapper::getPrototype(JSContext* cx, HandleObject wrapper,
                                      MutableHandleObject protop) const
{
    {
        RootedObject wrapped(cx, wrappedObject(wrapper));
        AutoCompartment call(cx, wrapped);
        if (!GetPrototype(cx, wrapped, protop))
            return false;
        if (protop)
            protop->setDelegate(cx);
    }

    return cx->compartment()->wrap(cx, protop);
}

bool
CrossCompartmentWrapper::setPrototype(JSContext* cx, HandleObject wrapper,
                                      HandleObject proto, ObjectOpResult& result) const
{
    RootedObject protoCopy(cx, proto);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &protoCopy),
           Wrapper::setPrototype(cx, wrapper, protoCopy, result),
           NOTHING);
}

bool
CrossCompartmentWrapper::setImmutablePrototype(JSContext* cx, HandleObject wrapper, bool* succeeded) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::setImmutablePrototype(cx, wrapper, succeeded),
           NOTHING);
}

bool
CrossCompartmentWrapper::preventExtensions(JSContext* cx, HandleObject wrapper,
                                           ObjectOpResult& result) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::preventExtensions(cx, wrapper, result),
           NOTHING);
}

bool
CrossCompartmentWrapper::isExtensible(JSContext* cx, HandleObject wrapper, bool* extensible) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::isExtensible(cx, wrapper, extensible),
           NOTHING);
}

bool
CrossCompartmentWrapper::has(JSContext* cx, HandleObject wrapper, HandleId id, bool* bp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::has(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::hasOwn(JSContext* cx, HandleObject wrapper, HandleId id, bool* bp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::hasOwn(cx, wrapper, id, bp),
           NOTHING);
}

bool
CrossCompartmentWrapper::get(JSContext* cx, HandleObject wrapper, HandleObject receiver,
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
CrossCompartmentWrapper::set(JSContext* cx, HandleObject wrapper, HandleId id, HandleValue v,
                             HandleValue receiver, ObjectOpResult& result) const
{
    RootedValue valCopy(cx, v);
    RootedValue receiverCopy(cx, receiver);
    PIERCE(cx, wrapper,
           cx->compartment()->wrap(cx, &valCopy) &&
           cx->compartment()->wrap(cx, &receiverCopy),
           Wrapper::set(cx, wrapper, id, valCopy, receiverCopy, result),
           NOTHING);
}

bool
CrossCompartmentWrapper::getOwnEnumerablePropertyKeys(JSContext* cx, HandleObject wrapper,
                                                      AutoIdVector& props) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::getOwnEnumerablePropertyKeys(cx, wrapper, props),
           NOTHING);
}





static bool
CanReify(HandleObject obj)
{
    return obj->is<PropertyIteratorObject>() &&
           (obj->as<PropertyIteratorObject>().getNativeIterator()->flags & JSITER_ENUMERATE);
}

struct AutoCloseIterator
{
    AutoCloseIterator(JSContext* cx, JSObject* obj) : cx(cx), obj(cx, obj) {}

    ~AutoCloseIterator() { if (obj) CloseIterator(cx, obj); }

    void clear() { obj = nullptr; }

  private:
    JSContext* cx;
    RootedObject obj;
};

static bool
Reify(JSContext* cx, JSCompartment* origin, MutableHandleObject objp)
{
    Rooted<PropertyIteratorObject*> iterObj(cx, &objp->as<PropertyIteratorObject>());
    NativeIterator* ni = iterObj->getNativeIterator();

    AutoCloseIterator close(cx, iterObj);

    
    RootedObject obj(cx, ni->obj);
    if (!origin->wrap(cx, &obj))
        return false;

    




    size_t length = ni->numKeys();
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

    return EnumeratedIdVectorToIterator(cx, obj, ni->flags, keys, objp);
}

bool
CrossCompartmentWrapper::enumerate(JSContext* cx, HandleObject wrapper,
                                   MutableHandleObject objp) const
{
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!Wrapper::enumerate(cx, wrapper, objp))
            return false;
    }

    if (CanReify(objp))
        return Reify(cx, cx->compartment(), objp);
    return cx->compartment()->wrap(cx, objp);
}

bool
CrossCompartmentWrapper::call(JSContext* cx, HandleObject wrapper, const CallArgs& args) const
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
CrossCompartmentWrapper::construct(JSContext* cx, HandleObject wrapper, const CallArgs& args) const
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
CrossCompartmentWrapper::nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                                    CallArgs srcArgs) const
{
    RootedObject wrapper(cx, &srcArgs.thisv().toObject());
    MOZ_ASSERT(srcArgs.thisv().isMagic(JS_IS_CONSTRUCTING) ||
               !UncheckedUnwrap(wrapper)->is<CrossCompartmentWrapperObject>());

    RootedObject wrapped(cx, wrappedObject(wrapper));
    {
        AutoCompartment call(cx, wrapped);
        InvokeArgs dstArgs(cx);
        if (!dstArgs.init(srcArgs.length()))
            return false;

        Value* src = srcArgs.base();
        Value* srcend = srcArgs.array() + srcArgs.length();
        Value* dst = dstArgs.base();

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
                    MOZ_ASSERT(!thisObj->is<CrossCompartmentWrapperObject>());
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
CrossCompartmentWrapper::hasInstance(JSContext* cx, HandleObject wrapper, MutableHandleValue v,
                                     bool* bp) const
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    if (!cx->compartment()->wrap(cx, v))
        return false;
    return Wrapper::hasInstance(cx, wrapper, v, bp);
}

const char*
CrossCompartmentWrapper::className(JSContext* cx, HandleObject wrapper) const
{
    AutoCompartment call(cx, wrappedObject(wrapper));
    return Wrapper::className(cx, wrapper);
}

JSString*
CrossCompartmentWrapper::fun_toString(JSContext* cx, HandleObject wrapper, unsigned indent) const
{
    RootedString str(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        str = Wrapper::fun_toString(cx, wrapper, indent);
        if (!str)
            return nullptr;
    }
    if (!cx->compartment()->wrap(cx, &str))
        return nullptr;
    return str;
}

bool
CrossCompartmentWrapper::regexp_toShared(JSContext* cx, HandleObject wrapper, RegExpGuard* g) const
{
    RegExpGuard wrapperGuard(cx);
    {
        AutoCompartment call(cx, wrappedObject(wrapper));
        if (!Wrapper::regexp_toShared(cx, wrapper, &wrapperGuard))
            return false;
    }

    
    RegExpShared* re = wrapperGuard.re();
    return cx->compartment()->regExps.get(cx, re->getSource(), re->getFlags(), g);
}

bool
CrossCompartmentWrapper::boxedValue_unbox(JSContext* cx, HandleObject wrapper, MutableHandleValue vp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::boxedValue_unbox(cx, wrapper, vp),
           cx->compartment()->wrap(cx, vp));
}

bool
CrossCompartmentWrapper::defaultValue(JSContext* cx, HandleObject wrapper, JSType hint,
                                      MutableHandleValue vp) const
{
    PIERCE(cx, wrapper,
           NOTHING,
           Wrapper::defaultValue(cx, wrapper, hint, vp),
           cx->compartment()->wrap(cx, vp));
}

const CrossCompartmentWrapper CrossCompartmentWrapper::singleton(0u);

bool
js::IsCrossCompartmentWrapper(JSObject* obj)
{
    return IsWrapper(obj) &&
           !!(Wrapper::wrapperHandler(obj)->flags() & Wrapper::CROSS_COMPARTMENT);
}

void
js::NukeCrossCompartmentWrapper(JSContext* cx, JSObject* wrapper)
{
    MOZ_ASSERT(wrapper->is<CrossCompartmentWrapperObject>());

    NotifyGCNukeWrapper(wrapper);

    wrapper->as<ProxyObject>().nuke(&DeadObjectProxy::singleton);

    MOZ_ASSERT(IsDeadProxyObject(wrapper));
}









JS_FRIEND_API(bool)
js::NukeCrossCompartmentWrappers(JSContext* cx,
                                 const CompartmentFilter& sourceFilter,
                                 const CompartmentFilter& targetFilter,
                                 js::NukeReferencesToWindow nukeReferencesToWindow)
{
    CHECK_REQUEST(cx);
    JSRuntime* rt = cx->runtime();

    
    

    for (CompartmentsIter c(rt, SkipAtoms); !c.done(); c.next()) {
        if (!sourceFilter.match(c))
            continue;

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            
            const CrossCompartmentKey& k = e.front().key();
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            AutoWrapperRooter wobj(cx, WrapperValue(e));
            JSObject* wrapped = UncheckedUnwrap(wobj);

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
js::RemapWrapper(JSContext* cx, JSObject* wobjArg, JSObject* newTargetArg)
{
    RootedObject wobj(cx, wobjArg);
    RootedObject newTarget(cx, newTargetArg);
    MOZ_ASSERT(wobj->is<CrossCompartmentWrapperObject>());
    MOZ_ASSERT(!newTarget->is<CrossCompartmentWrapperObject>());
    JSObject* origTarget = Wrapper::wrappedObject(wobj);
    MOZ_ASSERT(origTarget);
    Value origv = ObjectValue(*origTarget);
    JSCompartment* wcompartment = wobj->compartment();

    AutoDisableProxyCheck adpc(cx->runtime());

    
    
    
    MOZ_ASSERT_IF(origTarget != newTarget,
                  !wcompartment->lookupWrapper(ObjectValue(*newTarget)));

    
    
    WrapperMap::Ptr p = wcompartment->lookupWrapper(origv);
    MOZ_ASSERT(&p->value().unsafeGet()->toObject() == wobj);
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

    
    
    MOZ_ASSERT(Wrapper::wrappedObject(wobj) == newTarget);

    
    
    MOZ_ASSERT(wobj->is<WrapperObject>());
    wcompartment->putWrapper(cx, CrossCompartmentKey(newTarget), ObjectValue(*wobj));
    return true;
}



JS_FRIEND_API(bool)
js::RemapAllWrappersForObject(JSContext* cx, JSObject* oldTargetArg,
                              JSObject* newTargetArg)
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

    for (const WrapperValue& v : toTransplant) {
        if (!RemapWrapper(cx, &v.toObject(), newTarget))
            MOZ_CRASH();
    }

    return true;
}

JS_FRIEND_API(bool)
js::RecomputeWrappers(JSContext* cx, const CompartmentFilter& sourceFilter,
                      const CompartmentFilter& targetFilter)
{
    AutoWrapperVector toRecompute(cx);

    for (CompartmentsIter c(cx->runtime(), SkipAtoms); !c.done(); c.next()) {
        
        if (!sourceFilter.match(c))
            continue;

        
        for (JSCompartment::WrapperEnum e(c); !e.empty(); e.popFront()) {
            
            const CrossCompartmentKey& k = e.front().key();
            if (k.kind != CrossCompartmentKey::ObjectWrapper)
                continue;

            
            if (!targetFilter.match(static_cast<JSObject*>(k.wrapped)->compartment()))
                continue;

            
            if (!toRecompute.append(WrapperValue(e)))
                return false;
        }
    }

    
    for (const WrapperValue& v : toRecompute) {
        JSObject* wrapper = &v.toObject();
        JSObject* wrapped = Wrapper::wrappedObject(wrapper);
        if (!RemapWrapper(cx, wrapper, wrapped))
            MOZ_CRASH();
    }

    return true;
}
