






#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

#include "WaiveXrayWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

namespace xpc {

static bool
WaiveAccessors(JSContext *cx, js::PropertyDescriptor *desc)
{
    if ((desc->attrs & JSPROP_GETTER) && desc->getter) {
        JS::Value v = JS::ObjectValue(*JS_FUNC_TO_DATA_PTR(JSObject *, desc->getter));
        if (!WrapperFactory::WaiveXrayAndWrap(cx, &v))
            return false;
        desc->getter = js::CastAsJSPropertyOp(&v.toObject());
    }

    if ((desc->attrs & JSPROP_SETTER) && desc->setter) {
        JS::Value v = JS::ObjectValue(*JS_FUNC_TO_DATA_PTR(JSObject *, desc->setter));
        if (!WrapperFactory::WaiveXrayAndWrap(cx, &v))
            return false;
        desc->setter = js::CastAsJSStrictPropertyOp(&v.toObject());
    }
    return true;
}

WaiveXrayWrapper::WaiveXrayWrapper(unsigned flags) : js::CrossCompartmentWrapper(flags)
{
}

WaiveXrayWrapper::~WaiveXrayWrapper()
{
}

bool
WaiveXrayWrapper::getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*>wrapper,
                                        JS::Handle<jsid> id, js::PropertyDescriptor *desc,
                                        unsigned flags)
{
    return CrossCompartmentWrapper::getPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value) && WaiveAccessors(cx, desc);
}

bool
WaiveXrayWrapper::getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                           JS::Handle<jsid> id, js::PropertyDescriptor *desc,
                                           unsigned flags)
{
    return CrossCompartmentWrapper::getOwnPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value) && WaiveAccessors(cx, desc);
}

bool
WaiveXrayWrapper::get(JSContext *cx, JS::Handle<JSObject*> wrapper,
                      JS::Handle<JSObject*> receiver, JS::Handle<jsid> id,
                      JS::MutableHandle<JS::Value> vp)
{
    return CrossCompartmentWrapper::get(cx, wrapper, receiver, id, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp.address());
}

bool
WaiveXrayWrapper::call(JSContext *cx, JS::Handle<JSObject*> wrapper, unsigned argc,
                      js::Value *vp)
{
    return CrossCompartmentWrapper::call(cx, wrapper, argc, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
WaiveXrayWrapper::construct(JSContext *cx, JS::Handle<JSObject*> wrapper,
                              unsigned argc, js::Value *argv, JS::MutableHandle<JS::Value> rval)
{
    return CrossCompartmentWrapper::construct(cx, wrapper, argc, argv, rval) &&
           WrapperFactory::WaiveXrayAndWrap(cx, rval.address());
}



bool
WaiveXrayWrapper::nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                             JS::NativeImpl impl, JS::CallArgs args)
{
    return CrossCompartmentWrapper::nativeCall(cx, test, impl, args) &&
           WrapperFactory::WaiveXrayAndWrap(cx, args.rval().address());
}

}
