





#include <string.h>

#include "jsapi.h"
#include "jscntxt.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsproxy.h"
#include "jswrapper.h"

#include "gc/Marking.h"
#include "proxy/DeadObjectProxy.h"
#include "proxy/ScriptedDirectProxyHandler.h"
#include "proxy/ScriptedIndirectProxyHandler.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/ObjectImpl-inl.h"

using namespace js;
using namespace js::gc;

void
js::AutoEnterPolicy::reportErrorIfExceptionIsNotPending(JSContext *cx, jsid id)
{
    if (JS_IsExceptionPending(cx))
        return;

    if (JSID_IS_VOID(id)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_OBJECT_ACCESS_DENIED);
    } else {
        JSString *str = IdToString(cx, id);
        AutoStableStringChars chars(cx);
        const char16_t *prop = nullptr;
        if (str->ensureFlat(cx) && chars.initTwoByte(cx, str))
            prop = chars.twoByteChars();

        JS_ReportErrorNumberUC(cx, js_GetErrorMessage, nullptr, JSMSG_PROPERTY_ACCESS_DENIED,
                               prop);
    }
}

#ifdef DEBUG
void
js::AutoEnterPolicy::recordEnter(JSContext *cx, HandleObject proxy, HandleId id, Action act)
{
    if (allowed()) {
        context = cx;
        enteredProxy.emplace(proxy);
        enteredId.emplace(id);
        enteredAction = act;
        prev = cx->runtime()->enteredPolicy;
        cx->runtime()->enteredPolicy = this;
    }
}

void
js::AutoEnterPolicy::recordLeave()
{
    if (enteredProxy) {
        JS_ASSERT(context->runtime()->enteredPolicy == this);
        context->runtime()->enteredPolicy = prev;
    }
}

JS_FRIEND_API(void)
js::assertEnteredPolicy(JSContext *cx, JSObject *proxy, jsid id,
                        BaseProxyHandler::Action act)
{
    MOZ_ASSERT(proxy->is<ProxyObject>());
    MOZ_ASSERT(cx->runtime()->enteredPolicy);
    MOZ_ASSERT(cx->runtime()->enteredPolicy->enteredProxy->get() == proxy);
    MOZ_ASSERT(cx->runtime()->enteredPolicy->enteredId->get() == id);
    MOZ_ASSERT(cx->runtime()->enteredPolicy->enteredAction & act);
}
#endif

#define INVOKE_ON_PROTOTYPE(cx, handler, proxy, protoCall)                   \
    JS_BEGIN_MACRO                                                           \
        RootedObject proto(cx);                                              \
        if (!JSObject::getProto(cx, proxy, &proto))                          \
            return false;                                                    \
        if (!proto)                                                          \
            return true;                                                     \
        assertSameCompartment(cx, proxy, proto);                             \
        return protoCall;                                                    \
    JS_END_MACRO                                                             \

bool
Proxy::getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                             MutableHandle<PropertyDescriptor> desc)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    desc.object().set(nullptr); 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::GET_PROPERTY_DESCRIPTOR, true);
    if (!policy.allowed())
        return policy.returnValue();
    if (!handler->hasPrototype())
        return handler->getPropertyDescriptor(cx, proxy, id, desc);
    if (!handler->getOwnPropertyDescriptor(cx, proxy, id, desc))
        return false;
    if (desc.object())
        return true;
    INVOKE_ON_PROTOTYPE(cx, handler, proxy, JS_GetPropertyDescriptorById(cx, proto, id, desc));
}

bool
Proxy::getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    Rooted<PropertyDescriptor> desc(cx);
    if (!Proxy::getPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    return NewPropertyDescriptorObject(cx, desc, vp);
}

bool
Proxy::getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                MutableHandle<PropertyDescriptor> desc)
{
    JS_CHECK_RECURSION(cx, return false);

    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    desc.object().set(nullptr); 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::GET_PROPERTY_DESCRIPTOR, true);
    if (!policy.allowed())
        return policy.returnValue();
    return handler->getOwnPropertyDescriptor(cx, proxy, id, desc);
}

bool
Proxy::getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);

    Rooted<PropertyDescriptor> desc(cx);
    if (!Proxy::getOwnPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    return NewPropertyDescriptorObject(cx, desc, vp);
}

bool
Proxy::defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                      MutableHandle<PropertyDescriptor> desc)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::SET, true);
    if (!policy.allowed())
        return policy.returnValue();
    return proxy->as<ProxyObject>().handler()->defineProperty(cx, proxy, id, desc);
}

bool
Proxy::getOwnPropertyNames(JSContext *cx, HandleObject proxy, AutoIdVector &props)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE, BaseProxyHandler::ENUMERATE, true);
    if (!policy.allowed())
        return policy.returnValue();
    return proxy->as<ProxyObject>().handler()->getOwnPropertyNames(cx, proxy, props);
}

bool
Proxy::delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    *bp = true; 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::SET, true);
    if (!policy.allowed())
        return policy.returnValue();
    return proxy->as<ProxyObject>().handler()->delete_(cx, proxy, id, bp);
}

JS_FRIEND_API(bool)
js::AppendUnique(JSContext *cx, AutoIdVector &base, AutoIdVector &others)
{
    AutoIdVector uniqueOthers(cx);
    if (!uniqueOthers.reserve(others.length()))
        return false;
    for (size_t i = 0; i < others.length(); ++i) {
        bool unique = true;
        for (size_t j = 0; j < base.length(); ++j) {
            if (others[i].get() == base[j]) {
                unique = false;
                break;
            }
        }
        if (unique)
            uniqueOthers.append(others[i]);
    }
    return base.appendAll(uniqueOthers);
}

bool
Proxy::enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE, BaseProxyHandler::ENUMERATE, true);
    if (!policy.allowed())
        return policy.returnValue();
    if (!handler->hasPrototype())
        return proxy->as<ProxyObject>().handler()->enumerate(cx, proxy, props);
    if (!handler->keys(cx, proxy, props))
        return false;
    AutoIdVector protoProps(cx);
    INVOKE_ON_PROTOTYPE(cx, handler, proxy,
                        GetPropertyNames(cx, proto, 0, &protoProps) &&
                        AppendUnique(cx, props, protoProps));
}

bool
Proxy::has(JSContext *cx, HandleObject proxy, HandleId id, bool *bp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    *bp = false; 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::GET, true);
    if (!policy.allowed())
        return policy.returnValue();
    if (!handler->hasPrototype())
        return handler->has(cx, proxy, id, bp);
    if (!handler->hasOwn(cx, proxy, id, bp))
        return false;
    if (*bp)
        return true;
    bool Bp;
    INVOKE_ON_PROTOTYPE(cx, handler, proxy,
                        JS_HasPropertyById(cx, proto, id, &Bp) &&
                        ((*bp = Bp) || true));
}

bool
Proxy::hasOwn(JSContext *cx, HandleObject proxy, HandleId id, bool *bp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    *bp = false; 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::GET, true);
    if (!policy.allowed())
        return policy.returnValue();
    return handler->hasOwn(cx, proxy, id, bp);
}

bool
Proxy::get(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
           MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    vp.setUndefined(); 
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::GET, true);
    if (!policy.allowed())
        return policy.returnValue();
    bool own;
    if (!handler->hasPrototype()) {
        own = true;
    } else {
        if (!handler->hasOwn(cx, proxy, id, &own))
            return false;
    }
    if (own)
        return handler->get(cx, proxy, receiver, id, vp);
    INVOKE_ON_PROTOTYPE(cx, handler, proxy, JSObject::getGeneric(cx, proto, receiver, id, vp));
}

bool
Proxy::callProp(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id,
                MutableHandleValue vp)
{
    
    
    if (!Proxy::get(cx, proxy, receiver, id, vp))
        return false;

#if JS_HAS_NO_SUCH_METHOD
    if (MOZ_UNLIKELY(vp.isPrimitive())) {
        if (!OnUnknownMethod(cx, proxy, IdToValue(id), vp))
            return false;
    }
#endif

    return true;
}

bool
Proxy::set(JSContext *cx, HandleObject proxy, HandleObject receiver, HandleId id, bool strict,
           MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, id, BaseProxyHandler::SET, true);
    if (!policy.allowed())
        return policy.returnValue();

    
    
    if (!handler->hasPrototype())
        return handler->set(cx, proxy, receiver, id, strict, vp);

    
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!Proxy::getPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    if (desc.object() && desc.setter() && desc.setter() != JS_StrictPropertyStub)
        return CallSetter(cx, receiver, id, desc.setter(), desc.attributes(), strict, vp);

    
    
    
    
    
    
    Rooted<PropertyDescriptor> newDesc(cx);
    if (desc.object() == proxy)
        newDesc.setAttributes(desc.attributes());
    else
        newDesc.setAttributes(JSPROP_ENUMERATE);
    newDesc.value().set(vp);
    return handler->defineProperty(cx, receiver, id, &newDesc);
}

bool
Proxy::keys(JSContext *cx, HandleObject proxy, AutoIdVector &props)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE, BaseProxyHandler::ENUMERATE, true);
    if (!policy.allowed())
        return policy.returnValue();
    return handler->keys(cx, proxy, props);
}

bool
Proxy::iterate(JSContext *cx, HandleObject proxy, unsigned flags, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    vp.setUndefined(); 
    if (!handler->hasPrototype()) {
        AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE,
                               BaseProxyHandler::ENUMERATE, true);
        
        
        if (!policy.allowed()) {
            AutoIdVector props(cx);
            return policy.returnValue() &&
                   EnumeratedIdVectorToIterator(cx, proxy, flags, props, vp);
        }
        return handler->iterate(cx, proxy, flags, vp);
    }
    AutoIdVector props(cx);
    
    if ((flags & JSITER_OWNONLY)
        ? !Proxy::keys(cx, proxy, props)
        : !Proxy::enumerate(cx, proxy, props)) {
        return false;
    }
    return EnumeratedIdVectorToIterator(cx, proxy, flags, props, vp);
}

bool
Proxy::isExtensible(JSContext *cx, HandleObject proxy, bool *extensible)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->isExtensible(cx, proxy, extensible);
}

bool
Proxy::preventExtensions(JSContext *cx, HandleObject proxy)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    return handler->preventExtensions(cx, proxy);
}

bool
Proxy::call(JSContext *cx, HandleObject proxy, const CallArgs &args)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();

    
    
    
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE,
                           BaseProxyHandler::CALL, true);
    if (!policy.allowed()) {
        args.rval().setUndefined();
        return policy.returnValue();
    }

    return handler->call(cx, proxy, args);
}

bool
Proxy::construct(JSContext *cx, HandleObject proxy, const CallArgs &args)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();

    
    
    
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE,
                           BaseProxyHandler::CALL, true);
    if (!policy.allowed()) {
        args.rval().setUndefined();
        return policy.returnValue();
    }

    return handler->construct(cx, proxy, args);
}

bool
Proxy::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl, CallArgs args)
{
    JS_CHECK_RECURSION(cx, return false);
    RootedObject proxy(cx, &args.thisv().toObject());
    
    
    
    return proxy->as<ProxyObject>().handler()->nativeCall(cx, test, impl, args);
}

bool
Proxy::hasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    *bp = false; 
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE, BaseProxyHandler::GET, true);
    if (!policy.allowed())
        return policy.returnValue();
    return proxy->as<ProxyObject>().handler()->hasInstance(cx, proxy, v, bp);
}

bool
Proxy::objectClassIs(HandleObject proxy, ESClassValue classValue, JSContext *cx)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->objectClassIs(proxy, classValue, cx);
}

const char *
Proxy::className(JSContext *cx, HandleObject proxy)
{
    
    
    int stackDummy;
    if (!JS_CHECK_STACK_SIZE(GetNativeStackLimit(cx), &stackDummy))
        return "too much recursion";

    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE,
                           BaseProxyHandler::GET,  false);
    
    if (!policy.allowed()) {
        return handler->BaseProxyHandler::className(cx, proxy);
    }
    return handler->className(cx, proxy);
}

JSString *
Proxy::fun_toString(JSContext *cx, HandleObject proxy, unsigned indent)
{
    JS_CHECK_RECURSION(cx, return nullptr);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE,
                           BaseProxyHandler::GET,  false);
    
    if (!policy.allowed())
        return handler->BaseProxyHandler::fun_toString(cx, proxy, indent);
    return handler->fun_toString(cx, proxy, indent);
}

bool
Proxy::regexp_toShared(JSContext *cx, HandleObject proxy, RegExpGuard *g)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->regexp_toShared(cx, proxy, g);
}

bool
Proxy::boxedValue_unbox(JSContext *cx, HandleObject proxy, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->boxedValue_unbox(cx, proxy, vp);
}

bool
Proxy::defaultValue(JSContext *cx, HandleObject proxy, JSType hint, MutableHandleValue vp)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->defaultValue(cx, proxy, hint, vp);
}

JSObject * const TaggedProto::LazyProto = reinterpret_cast<JSObject *>(0x1);

bool
Proxy::getPrototypeOf(JSContext *cx, HandleObject proxy, MutableHandleObject proto)
{
    JS_ASSERT(proxy->getTaggedProto().isLazy());
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->getPrototypeOf(cx, proxy, proto);
}

bool
Proxy::setPrototypeOf(JSContext *cx, HandleObject proxy, HandleObject proto, bool *bp)
{
    JS_ASSERT(proxy->getTaggedProto().isLazy());
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->setPrototypeOf(cx, proxy, proto, bp);
}

 bool
Proxy::watch(JSContext *cx, JS::HandleObject proxy, JS::HandleId id, JS::HandleObject callable)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->watch(cx, proxy, id, callable);
}

 bool
Proxy::unwatch(JSContext *cx, JS::HandleObject proxy, JS::HandleId id)
{
    JS_CHECK_RECURSION(cx, return false);
    return proxy->as<ProxyObject>().handler()->unwatch(cx, proxy, id);
}

 bool
Proxy::slice(JSContext *cx, HandleObject proxy, uint32_t begin, uint32_t end,
             HandleObject result)
{
    JS_CHECK_RECURSION(cx, return false);
    const BaseProxyHandler *handler = proxy->as<ProxyObject>().handler();
    AutoEnterPolicy policy(cx, handler, proxy, JSID_VOIDHANDLE, BaseProxyHandler::GET,
                            true);
    if (!policy.allowed()) {
        if (policy.returnValue()) {
            JS_ASSERT(!cx->isExceptionPending());
            return js::SliceSlowly(cx, proxy, proxy, begin, end, result);
        }
        return false;
    }
    return handler->slice(cx, proxy, begin, end, result);
}

JSObject *
js::proxy_innerObject(JSObject *obj)
{
    return obj->as<ProxyObject>().private_().toObjectOrNull();
}

bool
js::proxy_LookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                        MutableHandleObject objp, MutableHandleShape propp)
{
    bool found;
    if (!Proxy::has(cx, obj, id, &found))
        return false;

    if (found) {
        MarkNonNativePropertyFound(propp);
        objp.set(obj);
    } else {
        objp.set(nullptr);
        propp.set(nullptr);
    }
    return true;
}

bool
js::proxy_LookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                         MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return proxy_LookupGeneric(cx, obj, id, objp, propp);
}

bool
js::proxy_LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                        MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return proxy_LookupGeneric(cx, obj, id, objp, propp);
}

bool
js::proxy_DefineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                        PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<PropertyDescriptor> desc(cx);
    desc.object().set(obj);
    desc.value().set(value);
    desc.setAttributes(attrs);
    desc.setGetter(getter);
    desc.setSetter(setter);
    return Proxy::defineProperty(cx, obj, id, &desc);
}

bool
js::proxy_DefineProperty(JSContext *cx, HandleObject obj, HandlePropertyName name, HandleValue value,
                         PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return proxy_DefineGeneric(cx, obj, id, value, getter, setter, attrs);
}

bool
js::proxy_DefineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue value,
                        PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return proxy_DefineGeneric(cx, obj, id, value, getter, setter, attrs);
}

bool
js::proxy_GetGeneric(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id,
                     MutableHandleValue vp)
{
    return Proxy::get(cx, obj, receiver, id, vp);
}

bool
js::proxy_GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandlePropertyName name,
                      MutableHandleValue vp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return proxy_GetGeneric(cx, obj, receiver, id, vp);
}

bool
js::proxy_GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                     MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return proxy_GetGeneric(cx, obj, receiver, id, vp);
}

bool
js::proxy_SetGeneric(JSContext *cx, HandleObject obj, HandleId id,
                     MutableHandleValue vp, bool strict)
{
    return Proxy::set(cx, obj, obj, id, strict, vp);
}

bool
js::proxy_SetProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                      MutableHandleValue vp, bool strict)
{
    Rooted<jsid> id(cx, NameToId(name));
    return proxy_SetGeneric(cx, obj, id, vp, strict);
}

bool
js::proxy_SetElement(JSContext *cx, HandleObject obj, uint32_t index,
                     MutableHandleValue vp, bool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return proxy_SetGeneric(cx, obj, id, vp, strict);
}

bool
js::proxy_GetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    Rooted<PropertyDescriptor> desc(cx);
    if (!Proxy::getOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;
    *attrsp = desc.attributes();
    return true;
}

bool
js::proxy_SetGenericAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!Proxy::getOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;
    desc.setAttributes(*attrsp);
    return Proxy::defineProperty(cx, obj, id, &desc);
}

bool
js::proxy_DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    bool deleted;
    if (!Proxy::delete_(cx, obj, id, &deleted))
        return false;
    *succeeded = deleted;
    return js_SuppressDeletedProperty(cx, obj, id);
}

void
js::proxy_Trace(JSTracer *trc, JSObject *obj)
{
    JS_ASSERT(obj->is<ProxyObject>());
    ProxyObject::trace(trc, obj);
}

 void
ProxyObject::trace(JSTracer *trc, JSObject *obj)
{
    ProxyObject *proxy = &obj->as<ProxyObject>();

#ifdef DEBUG
    if (trc->runtime()->gc.isStrictProxyCheckingEnabled() && proxy->is<WrapperObject>()) {
        JSObject *referent = MaybeForwarded(&proxy->private_().toObject());
        if (referent->compartment() != proxy->compartment()) {
            



            Value key = ObjectValue(*referent);
            WrapperMap::Ptr p = proxy->compartment()->lookupWrapper(key);
            JS_ASSERT(p);
            JS_ASSERT(*p->value().unsafeGet() == ObjectValue(*proxy));
        }
    }
#endif

    
    
    MarkCrossCompartmentSlot(trc, obj, proxy->slotOfPrivate(), "private");
    MarkSlot(trc, proxy->slotOfExtra(0), "extra0");

    



    if (!proxy->is<CrossCompartmentWrapperObject>())
        MarkSlot(trc, proxy->slotOfExtra(1), "extra1");

    



    unsigned numSlots = JSCLASS_RESERVED_SLOTS(proxy->getClass());
    for (unsigned i = PROXY_MINIMUM_SLOTS; i < numSlots; i++)
        MarkSlot(trc, proxy->slotOfClassSpecific(i), "class-specific");
}

JSObject *
js::proxy_WeakmapKeyDelegate(JSObject *obj)
{
    JS_ASSERT(obj->is<ProxyObject>());
    return obj->as<ProxyObject>().handler()->weakmapKeyDelegate(obj);
}

bool
js::proxy_Convert(JSContext *cx, HandleObject proxy, JSType hint, MutableHandleValue vp)
{
    JS_ASSERT(proxy->is<ProxyObject>());
    return Proxy::defaultValue(cx, proxy, hint, vp);
}

void
js::proxy_Finalize(FreeOp *fop, JSObject *obj)
{
    JS_ASSERT(obj->is<ProxyObject>());
    obj->as<ProxyObject>().handler()->finalize(fop, obj);
}

void
js::proxy_ObjectMoved(JSObject *obj, const JSObject *old)
{
    JS_ASSERT(obj->is<ProxyObject>());
    obj->as<ProxyObject>().handler()->objectMoved(obj, old);
}

bool
js::proxy_HasInstance(JSContext *cx, HandleObject proxy, MutableHandleValue v, bool *bp)
{
    bool b;
    if (!Proxy::hasInstance(cx, proxy, v, &b))
        return false;
    *bp = !!b;
    return true;
}

bool
js::proxy_Call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject proxy(cx, &args.callee());
    JS_ASSERT(proxy->is<ProxyObject>());
    return Proxy::call(cx, proxy, args);
}

bool
js::proxy_Construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedObject proxy(cx, &args.callee());
    JS_ASSERT(proxy->is<ProxyObject>());
    return Proxy::construct(cx, proxy, args);
}

bool
js::proxy_Watch(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable)
{
    return Proxy::watch(cx, obj, id, callable);
}

bool
js::proxy_Unwatch(JSContext *cx, HandleObject obj, HandleId id)
{
    return Proxy::unwatch(cx, obj, id);
}

bool
js::proxy_Slice(JSContext *cx, HandleObject proxy, uint32_t begin, uint32_t end,
                HandleObject result)
{
    return Proxy::slice(cx, proxy, begin, end, result);
}

const Class js::ProxyObject::class_ =
    PROXY_CLASS_DEF("Proxy",
                    0,
                    JSCLASS_HAS_CACHED_PROTO(JSProto_Proxy));

const Class* const js::ProxyClassPtr = &js::ProxyObject::class_;

JS_FRIEND_API(JSObject *)
js::NewProxyObject(JSContext *cx, const BaseProxyHandler *handler, HandleValue priv, JSObject *proto_,
                   JSObject *parent_, const ProxyOptions &options)
{
    return ProxyObject::New(cx, handler, priv, TaggedProto(proto_), parent_,
                            options);
}

void
ProxyObject::renew(JSContext *cx, const BaseProxyHandler *handler, Value priv)
{
    JS_ASSERT_IF(IsCrossCompartmentWrapper(this), IsDeadProxyObject(this));
    JS_ASSERT(getParent() == cx->global());
    JS_ASSERT(getClass() == &ProxyObject::class_);
    JS_ASSERT(!isCallable());
    JS_ASSERT(!getClass()->ext.innerObject);
    JS_ASSERT(getTaggedProto().isLazy());

    setHandler(handler);
    setCrossCompartmentSlot(PRIVATE_SLOT, priv);
    setSlot(EXTRA_SLOT + 0, UndefinedValue());
    setSlot(EXTRA_SLOT + 1, UndefinedValue());
}

JS_FRIEND_API(JSObject *)
js_InitProxyClass(JSContext *cx, HandleObject obj)
{
    static const JSFunctionSpec static_methods[] = {
        JS_FN("create",         proxy_create,          2, 0),
        JS_FN("createFunction", proxy_createFunction,  3, 0),
        JS_FN("revocable",      proxy_revocable,       2, 0),
        JS_FS_END
    };

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, proxy, cx->names().Proxy, 2);
    if (!ctor)
        return nullptr;

    if (!JS_DefineFunctions(cx, ctor, static_methods))
        return nullptr;
    if (!JS_DefineProperty(cx, obj, "Proxy", ctor, 0,
                           JS_PropertyStub, JS_StrictPropertyStub)) {
        return nullptr;
    }

    global->setConstructor(JSProto_Proxy, ObjectValue(*ctor));
    return ctor;
}
