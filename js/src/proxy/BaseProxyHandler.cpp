





#include "js/Proxy.h"
#include "vm/ProxyObject.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"

using namespace js;

bool
BaseProxyHandler::enter(JSContext* cx, HandleObject wrapper, HandleId id, Action act,
                        bool* bp) const
{
    *bp = true;
    return true;
}

bool
BaseProxyHandler::has(JSContext* cx, HandleObject proxy, HandleId id, bool* bp) const
{
    assertEnteredPolicy(cx, proxy, id, GET);
    Rooted<PropertyDescriptor> desc(cx);
    if (!getPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    *bp = !!desc.object();
    return true;
}

bool
BaseProxyHandler::hasOwn(JSContext* cx, HandleObject proxy, HandleId id, bool* bp) const
{
    assertEnteredPolicy(cx, proxy, id, GET);
    Rooted<PropertyDescriptor> desc(cx);
    if (!getOwnPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    *bp = !!desc.object();
    return true;
}

bool
BaseProxyHandler::get(JSContext* cx, HandleObject proxy, HandleObject receiver,
                      HandleId id, MutableHandleValue vp) const
{
    assertEnteredPolicy(cx, proxy, id, GET);

    Rooted<PropertyDescriptor> desc(cx);
    if (!getPropertyDescriptor(cx, proxy, id, &desc))
        return false;
    if (!desc.object()) {
        vp.setUndefined();
        return true;
    }
    desc.assertComplete();
    MOZ_ASSERT(desc.getter() != JS_PropertyStub);
    if (!desc.getter()) {
        vp.set(desc.value());
        return true;
    }
    if (desc.hasGetterObject())
        return InvokeGetter(cx, receiver, ObjectValue(*desc.getterObject()), vp);
    if (!desc.isShared())
        vp.set(desc.value());
    else
        vp.setUndefined();

    
    MOZ_ASSERT(desc.object() != proxy);
    return CallJSGetterOp(cx, desc.getter(), desc.object(), id, vp);
}

bool
BaseProxyHandler::set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v,
                      HandleValue receiver, ObjectOpResult& result) const
{
    assertEnteredPolicy(cx, proxy, id, SET);

    
    
    

    
    Rooted<PropertyDescriptor> ownDesc(cx);
    if (!getOwnPropertyDescriptor(cx, proxy, id, &ownDesc))
        return false;
    ownDesc.assertCompleteIfFound();

    
    
    return SetPropertyIgnoringNamedGetter(cx, proxy, id, v, receiver, ownDesc, result);
}

bool
js::SetPropertyIgnoringNamedGetter(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                                   HandleValue receiver, Handle<PropertyDescriptor> ownDesc_,
                                   ObjectOpResult& result)
{
    Rooted<PropertyDescriptor> ownDesc(cx, ownDesc_);

    
    if (!ownDesc.object()) {
        
        
        RootedObject proto(cx);
        if (!GetPrototype(cx, obj, &proto))
            return false;
        if (proto)
            return SetProperty(cx, proto, id, v, receiver, result);

        
        ownDesc.setDataDescriptor(UndefinedHandleValue, JSPROP_ENUMERATE);
    }

    
    if (ownDesc.isDataDescriptor()) {
        
        if (!ownDesc.writable())
            return result.fail(JSMSG_READ_ONLY);
        if (!receiver.isObject())
            return result.fail(JSMSG_SET_NON_OBJECT_RECEIVER);
        RootedObject receiverObj(cx, &receiver.toObject());

        
        SetterOp setter = ownDesc.setter();
        MOZ_ASSERT(setter != JS_StrictPropertyStub);
        if (setter && setter != JS_StrictPropertyStub) {
            RootedValue valCopy(cx, v);
            return CallJSSetterOp(cx, setter, receiverObj, id, &valCopy, result);
        }

        
        
        bool existingDescriptor;
        if (!HasOwnProperty(cx, receiverObj, id, &existingDescriptor))
            return false;

        
        unsigned attrs =
            existingDescriptor
            ? JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY | JSPROP_IGNORE_PERMANENT
            : JSPROP_ENUMERATE;

        
        
        const Class* clasp = receiverObj->getClass();
        MOZ_ASSERT(clasp->getProperty != JS_PropertyStub);
        MOZ_ASSERT(clasp->setProperty != JS_StrictPropertyStub);
        return DefineProperty(cx, receiverObj, id, v, clasp->getProperty, clasp->setProperty,
                              attrs, result);
    }

    
    MOZ_ASSERT(ownDesc.isAccessorDescriptor());
    RootedObject setter(cx);
    if (ownDesc.hasSetterObject())
        setter = ownDesc.setterObject();
    if (!setter)
        return result.fail(JSMSG_GETTER_ONLY);
    RootedValue setterValue(cx, ObjectValue(*setter));
    if (!InvokeSetter(cx, receiver, setterValue, v))
        return false;
    return result.succeed();
}

bool
BaseProxyHandler::getOwnEnumerablePropertyKeys(JSContext* cx, HandleObject proxy,
                                               AutoIdVector& props) const
{
    assertEnteredPolicy(cx, proxy, JSID_VOID, ENUMERATE);
    MOZ_ASSERT(props.length() == 0);

    if (!ownPropertyKeys(cx, proxy, props))
        return false;

    
    RootedId id(cx);
    size_t i = 0;
    for (size_t j = 0, len = props.length(); j < len; j++) {
        MOZ_ASSERT(i <= j);
        id = props[j];
        if (JSID_IS_SYMBOL(id))
            continue;

        AutoWaivePolicy policy(cx, proxy, id, BaseProxyHandler::GET);
        Rooted<PropertyDescriptor> desc(cx);
        if (!getOwnPropertyDescriptor(cx, proxy, id, &desc))
            return false;
        desc.assertCompleteIfFound();

        if (desc.object() && desc.enumerable())
            props[i++].set(id);
    }

    MOZ_ASSERT(i <= props.length());
    props.resize(i);

    return true;
}

bool
BaseProxyHandler::enumerate(JSContext* cx, HandleObject proxy, MutableHandleObject objp) const
{
    assertEnteredPolicy(cx, proxy, JSID_VOID, ENUMERATE);

    
    
    AutoIdVector props(cx);
    if (!GetPropertyKeys(cx, proxy, 0, &props))
        return false;

    return EnumeratedIdVectorToIterator(cx, proxy, 0, props, objp);
}

bool
BaseProxyHandler::call(JSContext* cx, HandleObject proxy, const CallArgs& args) const
{
    MOZ_CRASH("callable proxies should implement call trap");
}

bool
BaseProxyHandler::construct(JSContext* cx, HandleObject proxy, const CallArgs& args) const
{
    MOZ_CRASH("callable proxies should implement construct trap");
}

const char*
BaseProxyHandler::className(JSContext* cx, HandleObject proxy) const
{
    return proxy->isCallable() ? "Function" : "Object";
}

JSString*
BaseProxyHandler::fun_toString(JSContext* cx, HandleObject proxy, unsigned indent) const
{
    if (proxy->isCallable())
        return JS_NewStringCopyZ(cx, "function () {\n    [native code]\n}");
    RootedValue v(cx, ObjectValue(*proxy));
    ReportIsNotFunction(cx, v);
    return nullptr;
}

bool
BaseProxyHandler::regexp_toShared(JSContext* cx, HandleObject proxy,
                                  RegExpGuard* g) const
{
    MOZ_CRASH("This should have been a wrapped regexp");
}

bool
BaseProxyHandler::boxedValue_unbox(JSContext* cx, HandleObject proxy, MutableHandleValue vp) const
{
    vp.setUndefined();
    return true;
}

bool
BaseProxyHandler::defaultValue(JSContext* cx, HandleObject proxy, JSType hint,
                               MutableHandleValue vp) const
{
    return OrdinaryToPrimitive(cx, proxy, hint, vp);
}

bool
BaseProxyHandler::nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                             CallArgs args) const
{
    ReportIncompatible(cx, args);
    return false;
}

bool
BaseProxyHandler::hasInstance(JSContext* cx, HandleObject proxy, MutableHandleValue v,
                              bool* bp) const
{
    assertEnteredPolicy(cx, proxy, JSID_VOID, GET);
    RootedValue val(cx, ObjectValue(*proxy.get()));
    ReportValueError(cx, JSMSG_BAD_INSTANCEOF_RHS,
                     JSDVG_SEARCH_STACK, val, js::NullPtr());
    return false;
}

bool
BaseProxyHandler::objectClassIs(HandleObject proxy, ESClassValue classValue, JSContext* cx) const
{
    return false;
}

void
BaseProxyHandler::trace(JSTracer* trc, JSObject* proxy) const
{
}

void
BaseProxyHandler::finalize(JSFreeOp* fop, JSObject* proxy) const
{
}

void
BaseProxyHandler::objectMoved(JSObject* proxy, const JSObject* old) const
{
}

JSObject*
BaseProxyHandler::weakmapKeyDelegate(JSObject* proxy) const
{
    return nullptr;
}

bool
BaseProxyHandler::getPrototype(JSContext* cx, HandleObject proxy, MutableHandleObject protop) const
{
    MOZ_CRASH("Must override getPrototype with lazy prototype.");
}

bool
BaseProxyHandler::setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                               ObjectOpResult& result) const
{
    
    
    
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_PROTO_OF,
                         "incompatible Proxy");
    return false;
}

bool
BaseProxyHandler::setImmutablePrototype(JSContext* cx, HandleObject proxy, bool* succeeded) const
{
    *succeeded = false;
    return true;
}

bool
BaseProxyHandler::watch(JSContext* cx, HandleObject proxy, HandleId id, HandleObject callable) const
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_WATCH,
                         proxy->getClass()->name);
    return false;
}

bool
BaseProxyHandler::unwatch(JSContext* cx, HandleObject proxy, HandleId id) const
{
    return true;
}

bool
BaseProxyHandler::getElements(JSContext* cx, HandleObject proxy, uint32_t begin, uint32_t end,
                              ElementAdder* adder) const
{
    assertEnteredPolicy(cx, proxy, JSID_VOID, GET);

    return js::GetElementsWithAdder(cx, proxy, proxy, begin, end, adder);
}

bool
BaseProxyHandler::isCallable(JSObject* obj) const
{
    return false;
}

bool
BaseProxyHandler::isConstructor(JSObject* obj) const
{
    return false;
}
