





#include "ChromeObjectWrapper.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "xpcprivate.h"
#include "jsapi.h"
#include "jswrapper.h"
#include "nsXULAppAPI.h"

using namespace JS;

namespace xpc {

const ChromeObjectWrapper ChromeObjectWrapper::singleton;

bool
ChromeObjectWrapper::defineProperty(JSContext *cx, HandleObject wrapper,
                                    HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc,
                                    JS::ObjectOpResult &result) const
{
    if (!AccessCheck::checkPassToPrivilegedCode(cx, wrapper, desc.value()))
        return false;
    return ChromeObjectWrapperBase::defineProperty(cx, wrapper, id, desc, result);
}

bool
ChromeObjectWrapper::set(JSContext *cx, HandleObject wrapper,
                         HandleObject receiver, HandleId id,
                         MutableHandleValue vp, ObjectOpResult &result) const
{
    if (!AccessCheck::checkPassToPrivilegedCode(cx, wrapper, vp))
        return false;
    return ChromeObjectWrapperBase::set(cx, wrapper, receiver, id, vp, result);
}

}
