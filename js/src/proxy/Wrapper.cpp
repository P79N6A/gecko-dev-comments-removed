





#include "jscntxt.h"
#include "jscompartment.h"
#include "jsexn.h"
#include "jswrapper.h"

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
             const WrapperOptions &options)
{
    JS_ASSERT(parent);

    RootedValue priv(cx, ObjectValue(*obj));
    return NewProxyObject(cx, handler, priv, options.proto(), parent, options);
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

bool
Wrapper::isConstructor(JSObject *obj) const
{
    
    
    
    return isCallable(obj);
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
    return IsBackgroundFinalized(priv.toObject().asTenured()->getAllocKind());
}
