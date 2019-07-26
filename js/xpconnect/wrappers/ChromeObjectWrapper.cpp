#include "ChromeObjectWrapper.h"

namespace xpc {











ChromeObjectWrapper ChromeObjectWrapper::singleton;

static bool
PropIsFromStandardPrototype(JSContext *cx, JSPropertyDescriptor *desc)
{
    MOZ_ASSERT(desc->obj);
    JSObject *unwrapped = js::UnwrapObject(desc->obj);
    JSAutoCompartment ac(cx, unwrapped);
    return JS_IdentifyClassPrototype(cx, unwrapped) != JSProto_Null;
}

bool
ChromeObjectWrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper,
                                           jsid id, js::PropertyDescriptor *desc,
                                           unsigned flags)
{
    
    
    
    desc->obj = NULL;
    if (!ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                        desc, flags)) {
        return false;
    }

    
    
    
    if (desc->obj && PropIsFromStandardPrototype(cx, desc))
        desc->obj = NULL;

    
    JSObject *wrapperProto;
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
      return false;
    if (desc->obj || !wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return JS_GetPropertyDescriptorById(cx, wrapperProto, id, 0, desc);
}

bool
ChromeObjectWrapper::has(JSContext *cx, JSObject *wrapper, jsid id, bool *bp)
{
    
    if (!ChromeObjectWrapperBase::has(cx, wrapper, id, bp))
        return false;

    
    JSObject *wrapperProto;
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
        return false;
    if (*bp || !wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
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
    
    
    
    JSPropertyDescriptor desc;
    if (!ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id, &desc,
                                                        0)) {
        return false;
    }

    
    
    vp->setUndefined();
    if (desc.obj && !PropIsFromStandardPrototype(cx, &desc)) {
        
        if (!ChromeObjectWrapperBase::get(cx, wrapper, receiver, id, vp))
            return false;
        
        if (!vp->isUndefined())
            return true;
    }

    
    JSObject *wrapperProto;
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
        return false;
    if (!wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return js::GetGeneric(cx, wrapperProto, receiver, id, vp);
}

}
