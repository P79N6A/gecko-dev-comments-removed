





#include "jswrapper.h"

#include "jscntxt.h"
#include "jscompartment.h"
#include "jsexn.h"

#include "vm/ErrorObject.h"
#include "vm/WrapperObject.h"

#include "jsobjinlines.h"

using namespace js;








bool
Wrapper::defaultValue(JSContext *cx, HandleObject proxy, JSType hint, MutableHandleValue vp) const
{
    vp.set(ObjectValue(*proxy->as<ProxyObject>().target()));
    if (hint == JSTYPE_VOID)
        return ToPrimitive(cx, vp);
    return ToPrimitive(cx, hint, vp);
}

JSObject *
Wrapper::New(JSContext *cx, JSObject *obj, JSObject *parent, const Wrapper *handler,
             const WrapperOptions *options)
{
    JS_ASSERT(parent);

    RootedValue priv(cx, ObjectValue(*obj));
    mozilla::Maybe<WrapperOptions> opts;
    if (!options) {
        opts.emplace();
        opts->selectDefaultClass(obj->isCallable());
        options = opts.ptr();
    }
    return NewProxyObject(cx, handler, priv, options->proto(), parent, *options);
}

JSObject *
Wrapper::Renew(JSContext *cx, JSObject *existing, JSObject *obj, const Wrapper *handler)
{
    JS_ASSERT(!obj->isCallable());
    existing->as<ProxyObject>().renew(cx, handler, ObjectValue(*obj));
    return existing;
}

const Wrapper *
Wrapper::wrapperHandler(JSObject *wrapper)
{
    JS_ASSERT(wrapper->is<WrapperObject>());
    return static_cast<const Wrapper*>(wrapper->as<ProxyObject>().handler());
}

JSObject *
Wrapper::wrappedObject(JSObject *wrapper)
{
    JS_ASSERT(wrapper->is<WrapperObject>());
    return wrapper->as<ProxyObject>().target();
}

JS_FRIEND_API(JSObject *)
js::UncheckedUnwrap(JSObject *wrapped, bool stopAtOuter, unsigned *flagsp)
{
    unsigned flags = 0;
    while (true) {
        if (!wrapped->is<WrapperObject>() ||
            MOZ_UNLIKELY(stopAtOuter && wrapped->getClass()->ext.innerObject))
        {
            break;
        }
        flags |= Wrapper::wrapperHandler(wrapped)->flags();
        wrapped = wrapped->as<ProxyObject>().private_().toObjectOrNull();

        
        
        if (wrapped)
            wrapped = MaybeForwarded(wrapped);
    }
    if (flagsp)
        *flagsp = flags;
    return wrapped;
}

JS_FRIEND_API(JSObject *)
js::CheckedUnwrap(JSObject *obj, bool stopAtOuter)
{
    while (true) {
        JSObject *wrapper = obj;
        obj = UnwrapOneChecked(obj, stopAtOuter);
        if (!obj || obj == wrapper)
            return obj;
    }
}

JS_FRIEND_API(JSObject *)
js::UnwrapOneChecked(JSObject *obj, bool stopAtOuter)
{
    if (!obj->is<WrapperObject>() ||
        MOZ_UNLIKELY(!!obj->getClass()->ext.innerObject && stopAtOuter))
    {
        return obj;
    }

    const Wrapper *handler = Wrapper::wrapperHandler(obj);
    return handler->hasSecurityPolicy() ? nullptr : Wrapper::wrappedObject(obj);
}

const char Wrapper::family = 0;
const Wrapper Wrapper::singleton((unsigned)0);
const Wrapper Wrapper::singletonWithPrototype((unsigned)0, true);
JSObject *Wrapper::defaultProto = TaggedProto::LazyProto;



extern JSObject *
js::TransparentObjectWrapper(JSContext *cx, HandleObject existing, HandleObject obj,
                             HandleObject parent)
{
    
    JS_ASSERT(!obj->is<WrapperObject>() || obj->getClass()->ext.innerObject);
    return Wrapper::New(cx, obj, parent, &CrossCompartmentWrapper::singleton);
}

ErrorCopier::~ErrorCopier()
{
    JSContext *cx = ac->context()->asJSContext();
    if (ac->origin() != cx->compartment() && cx->isExceptionPending()) {
        RootedValue exc(cx);
        if (cx->getPendingException(&exc) && exc.isObject() && exc.toObject().is<ErrorObject>()) {
            cx->clearPendingException();
            ac.reset();
            Rooted<ErrorObject*> errObj(cx, &exc.toObject().as<ErrorObject>());
            JSObject *copyobj = js_CopyErrorObject(cx, errObj);
            if (copyobj)
                cx->setPendingException(ObjectValue(*copyobj));
        }
    }
}

bool Wrapper::finalizeInBackground(Value priv) const
{
    if (!priv.isObject())
        return true;

    






    if (IsInsideNursery(&priv.toObject()))
        return true;
    return IsBackgroundFinalized(priv.toObject().tenuredGetAllocKind());
}



template <class Base>
bool
SecurityWrapper<Base>::isExtensible(JSContext *cx, HandleObject wrapper, bool *extensible) const
{
    
    
    
    *extensible = true;
    return true;
}

template <class Base>
bool
SecurityWrapper<Base>::preventExtensions(JSContext *cx, HandleObject wrapper) const
{
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::enter(JSContext *cx, HandleObject wrapper, HandleId id,
                             Wrapper::Action act, bool *bp) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    *bp = false;
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                  CallArgs args) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::setPrototypeOf(JSContext *cx, HandleObject wrapper,
                                      HandleObject proto, bool *bp) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}




template <class Base>
bool
SecurityWrapper<Base>::defaultValue(JSContext *cx, HandleObject wrapper,
                                    JSType hint, MutableHandleValue vp) const
{
    return DefaultValue(cx, wrapper, hint, vp);
}

template <class Base>
bool
SecurityWrapper<Base>::objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx) const
{
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::regexp_toShared(JSContext *cx, HandleObject obj, RegExpGuard *g) const
{
    return Base::regexp_toShared(cx, obj, g);
}

template <class Base>
bool
SecurityWrapper<Base>::boxedValue_unbox(JSContext *cx, HandleObject obj, MutableHandleValue vp) const
{
    vp.setUndefined();
    return true;
}

template <class Base>
bool
SecurityWrapper<Base>::defineProperty(JSContext *cx, HandleObject wrapper,
                                      HandleId id, MutableHandle<PropertyDescriptor> desc) const
{
    if (desc.getter() || desc.setter()) {
        JSString *str = IdToString(cx, id);
        AutoStableStringChars chars(cx);
        const char16_t *prop = nullptr;
        if (str->ensureFlat(cx) && chars.initTwoByte(cx, str))
            prop = chars.twoByteChars();
        JS_ReportErrorNumberUC(cx, js_GetErrorMessage, nullptr,
                               JSMSG_ACCESSOR_DEF_DENIED, prop);
        return false;
    }

    return Base::defineProperty(cx, wrapper, id, desc);
}

template <class Base>
bool
SecurityWrapper<Base>::watch(JSContext *cx, HandleObject proxy,
                             HandleId id, HandleObject callable) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::unwatch(JSContext *cx, HandleObject proxy,
                               HandleId id) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}


template class js::SecurityWrapper<Wrapper>;
template class js::SecurityWrapper<CrossCompartmentWrapper>;

