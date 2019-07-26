






#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

#include "WaiveXrayWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

namespace xpc {

WaiveXrayWrapper::WaiveXrayWrapper(unsigned flags) : js::CrossCompartmentWrapper(flags)
{
}

WaiveXrayWrapper::~WaiveXrayWrapper()
{
}

bool
WaiveXrayWrapper::getPropertyDescriptor(JSContext *cx, JS::Handle<JSObject *>wrapper,
                                        JS::Handle<jsid> id, js::PropertyDescriptor *desc,
                                        unsigned flags)
{
    return CrossCompartmentWrapper::getPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value);
}

bool
WaiveXrayWrapper::getOwnPropertyDescriptor(JSContext *cx, JS::Handle<JSObject *> wrapper,
                                           JS::Handle<jsid> id, js::PropertyDescriptor *desc,
                                           unsigned flags)
{
    return CrossCompartmentWrapper::getOwnPropertyDescriptor(cx, wrapper, id, desc, flags) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value);
}

bool
WaiveXrayWrapper::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                        js::Value *vp)
{
    return CrossCompartmentWrapper::get(cx, wrapper, receiver, id, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
WaiveXrayWrapper::call(JSContext *cx, JSObject *wrapper, unsigned argc, js::Value *vp)
{
    return CrossCompartmentWrapper::call(cx, wrapper, argc, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
WaiveXrayWrapper::construct(JSContext *cx, JSObject *wrapper,
                              unsigned argc, js::Value *argv, js::Value *rval)
{
    return CrossCompartmentWrapper::construct(cx, wrapper, argc, argv, rval) &&
           WrapperFactory::WaiveXrayAndWrap(cx, rval);
}

}
