






#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

#include "WaiveXrayWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

using namespace JS;

namespace xpc {

static bool
WaiveAccessors(JSContext *cx, js::PropertyDescriptor *desc)
{
    if ((desc->attrs & JSPROP_GETTER) && desc->getter) {
        RootedValue v(cx, JS::ObjectValue(*JS_FUNC_TO_DATA_PTR(JSObject *, desc->getter)));
        if (!WrapperFactory::WaiveXrayAndWrap(cx, v.address()))
            return false;
        desc->getter = js::CastAsJSPropertyOp(&v.toObject());
    }

    if ((desc->attrs & JSPROP_SETTER) && desc->setter) {
        RootedValue v(cx, JS::ObjectValue(*JS_FUNC_TO_DATA_PTR(JSObject *, desc->setter)));
        if (!WrapperFactory::WaiveXrayAndWrap(cx, v.address()))
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
WaiveXrayWrapper::getPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                        HandleId id, js::PropertyDescriptor *desc,
                                        unsigned flags)
{
    return CrossCompartmentWrapper::getPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value) && WaiveAccessors(cx, desc);
}

bool
WaiveXrayWrapper::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                           HandleId id, js::PropertyDescriptor *desc,
                                           unsigned flags)
{
    return CrossCompartmentWrapper::getOwnPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value) && WaiveAccessors(cx, desc);
}

bool
WaiveXrayWrapper::get(JSContext *cx, HandleObject wrapper,
                      HandleObject receiver, HandleId id,
                      MutableHandleValue vp)
{
    return CrossCompartmentWrapper::get(cx, wrapper, receiver, id, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp.address());
}

bool
WaiveXrayWrapper::call(JSContext *cx, HandleObject wrapper, const JS::CallArgs &args)
{
    return CrossCompartmentWrapper::call(cx, wrapper, args) &&
           WrapperFactory::WaiveXrayAndWrap(cx, args.rval().address());
}

bool
WaiveXrayWrapper::construct(JSContext *cx, HandleObject wrapper, const JS::CallArgs &args)
{
    return CrossCompartmentWrapper::construct(cx, wrapper, args) &&
           WrapperFactory::WaiveXrayAndWrap(cx, args.rval().address());
}



bool
WaiveXrayWrapper::nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                             JS::NativeImpl impl, JS::CallArgs args)
{
    return CrossCompartmentWrapper::nativeCall(cx, test, impl, args) &&
           WrapperFactory::WaiveXrayAndWrap(cx, args.rval().address());
}

}
