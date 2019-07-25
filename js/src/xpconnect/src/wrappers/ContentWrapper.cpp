






































#include "ContentWrapper.h"
#include "AccessCheck.h"

namespace xpc {

ContentWrapper ContentWrapper::singleton;

ContentWrapper::ContentWrapper() : JSCrossCompartmentWrapper()
{
}

ContentWrapper::~ContentWrapper()
{
}

bool
ContentWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id, Mode mode)
{
    return AccessCheck::enter(cx, wrapper, wrappedObject(wrapper), id, mode);
}

void
ContentWrapper::leave(JSContext *cx, JSObject *wrapper)
{
    return AccessCheck::leave(cx, wrapper, wrappedObject(wrapper));
}

}
