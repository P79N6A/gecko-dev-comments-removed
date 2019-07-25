






































#include "WrapperFactory.h"
#include "ContentWrapper.h"
#include "ChromeWrapper.h"
#include "AccessCheck.h"

namespace xpc {

JSCrossCompartmentWrapper *
WrapperFactory::select(JSContext *cx, JSCompartment *subject, JSCompartment *object)
{
    if(AccessCheck::isPrivileged(object)) {
        return &ChromeWrapper::singleton;
    }
    return &ContentWrapper::singleton;
}

}
