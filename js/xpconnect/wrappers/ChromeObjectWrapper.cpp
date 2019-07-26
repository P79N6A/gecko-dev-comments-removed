#include "ChromeObjectWrapper.h"

namespace xpc {











ChromeObjectWrapper ChromeObjectWrapper::singleton;

using js::assertEnteredPolicy;

static bool
AllowedByBase(JSContext *cx, JSObject *wrapper, jsid id, js::Wrapper::Action act)
{
    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    bool bp;
    ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    return handler->ChromeObjectWrapperBase::enter(cx, wrapper, id, act, &bp);
}

static bool
PropIsFromStandardPrototype(JSContext *cx, JSPropertyDescriptor *desc)
{
    MOZ_ASSERT(desc->obj);
    JSObject *unwrapped = js::UnwrapObject(desc->obj);
    JSAutoCompartment ac(cx, unwrapped);
    return JS_IdentifyClassPrototype(cx, unwrapped) != JSProto_Null;
}





static bool
PropIsFromStandardPrototype(JSContext *cx, JSObject *wrapperArg, jsid idArg)
{
    JS::Rooted<JSObject *> wrapper(cx, wrapperArg);
    JS::Rooted<jsid> id(cx, idArg);

    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    JSPropertyDescriptor desc;
    ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    if (!handler->ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                                 &desc, 0) ||
        !desc.obj)
    {
        return false;
    }
    return PropIsFromStandardPrototype(cx, &desc);
}

bool
ChromeObjectWrapper::getPropertyDescriptor(JSContext *cx,
                                           JS::Handle<JSObject *> wrapper,
                                           JS::Handle<jsid> id,
                                           js::PropertyDescriptor *desc,
                                           unsigned flags)
{
    assertEnteredPolicy(cx, wrapper, id);
    
    desc->obj = NULL;
    if (AllowedByBase(cx, wrapper, id, Wrapper::GET) &&
        !ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
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
    assertEnteredPolicy(cx, wrapper, id);
    
    if (AllowedByBase(cx, wrapper, id, js::Wrapper::GET) &&
        !ChromeObjectWrapperBase::has(cx, wrapper, id, bp))
    {
        return false;
    }

    
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
    assertEnteredPolicy(cx, wrapper, id);
    vp->setUndefined();
    JSPropertyDescriptor desc;
    
    
    
    if (AllowedByBase(cx, wrapper, id, js::Wrapper::GET) &&
        !PropIsFromStandardPrototype(cx, wrapper, id))
    {
        
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




bool
ChromeObjectWrapper::objectClassIs(JSObject *obj, js::ESClassValue classValue,
                                   JSContext *cx)
{
  return CrossCompartmentWrapper::objectClassIs(obj, classValue, cx);
}





bool
ChromeObjectWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id,
                           js::Wrapper::Action act, bool *bp)
{
    if (AllowedByBase(cx, wrapper, id, act))
        return true;
    
    
    *bp = (act == Wrapper::GET);
    if (!*bp || id == JSID_VOID)
        return false;

    
    
    JS::RootedObject rootedWrapper(cx, wrapper);
    JS::RootedId rootedId(cx, id);
    js::AutoWaivePolicy policy(cx, rootedWrapper, rootedId);
    return PropIsFromStandardPrototype(cx, wrapper, id);
}

}
