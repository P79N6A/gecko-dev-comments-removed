#include "ChromeObjectWrapper.h"

namespace xpc {











ChromeObjectWrapper ChromeObjectWrapper::singleton;

bool
ChromeObjectWrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                           jsid id, bool set,
                                           js::PropertyDescriptor *desc)
{
    
    
    
    desc->obj = NULL;
    if (!ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                        set, desc)) {
        return false;
    }

    
    JSObject *wrapperProto = JS_GetPrototype(wrapper);
    if (desc->obj || set || !wrapperProto)
        return true;

    
    JS_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return JS_GetPropertyDescriptorById(cx, wrapperProto, id, 0, desc);
}

bool
ChromeObjectWrapper::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    if (!ChromeObjectWrapperBase::has(cx, wrapper, id, bp))
        return false;

    
    JSObject *wrapperProto = JS_GetPrototype(wrapper);
    if (*bp || !wrapperProto)
        return true;

    
    JS_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    JSPropertyDescriptor desc;
    if (!JS_GetPropertyDescriptorById(cx, wrapperProto, id, 0, &desc))
        return false;
    *bp = !!desc.obj;
    return true;
}

bool
ChromeObjectWrapper::get(JSContext *cx, JSObject *wrapper, JSObject *receiver,
                         jsid id, js::Value *vp)
{
    
    if (!ChromeObjectWrapperBase::get(cx, wrapper, receiver, id, vp))
        return false;

    
    JSObject *wrapperProto = JS_GetPrototype(wrapper);
    if (!vp->isUndefined() || !wrapperProto)
        return true;

    
    JS_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return js::GetGeneric(cx, wrapperProto, receiver, id, vp);
}

}
