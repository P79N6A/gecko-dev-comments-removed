






































#include "FilteringWrapper.h"
#include "AccessCheck.h"
#include "CrossOriginWrapper.h"
#include "XrayWrapper.h"

#include "XPCWrapper.h"

using namespace js;

namespace xpc {

template <typename Base, typename Policy>
FilteringWrapper<Base, Policy>::FilteringWrapper(uintN flags) : Base(flags)
{
}

template <typename Base, typename Policy>
FilteringWrapper<Base, Policy>::~FilteringWrapper()
{
}

typedef JSWrapper::Permission Permission;

static const Permission PermitObjectAccess = JSWrapper::PermitObjectAccess;
static const Permission PermitPropertyAccess = JSWrapper::PermitPropertyAccess;
static const Permission DenyAccess = JSWrapper::DenyAccess;

template <typename Policy>
static bool
Filter(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    size_t w = 0;
    for (size_t n = 0; n < props.length(); ++n) {
        jsid id = props[n];
        Permission perm;
        if (perm != PermitObjectAccess && !Policy::check(cx, wrapper, id, false, perm))
            return false; 
        if (perm != DenyAccess) {
            props[w++] = id;
        }
    }
    props.resize(w);
    return true;
}

template <typename Policy>
static bool
CheckAndReport(JSContext *cx, JSObject *wrapper, jsid id, bool set, Permission &perm)
{
    if (!Policy::check(cx, wrapper, id, set, perm)) {
        return false;
    }
    if (perm == DenyAccess) {
        AccessCheck::deny(cx, id);
        return false;
    }
    return true;
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return Base::getOwnPropertyNames(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerate(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return Base::enumerate(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerateOwn(JSContext *cx, JSObject *wrapper, AutoValueVector &props)
{
    return Base::enumerateOwn(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::iterate(JSContext *cx, JSObject *wrapper, uintN flags, jsval *vp)
{
    
    
    
    
    return JSProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enter(JSContext *cx, JSObject *wrapper, jsid id, bool set)
{
    Permission perm;
    return CheckAndReport<Policy>(cx, wrapper, JSVAL_VOID, set, perm) &&
           Base::enter(cx, wrapper, id, set);
}

#define SOW FilteringWrapper<JSCrossCompartmentWrapper, OnlyIfSubjectIsSystem>
#define COW FilteringWrapper<JSCrossCompartmentWrapper, ExposedPropertiesOnly>
#define XOW FilteringWrapper<XrayWrapper<CrossOriginWrapper>, CrossOriginAccessiblePropertiesOnly>

template<> SOW SOW::singleton(0);
template<> COW COW::singleton(0);
template<> XOW XOW::singleton(0);

template class SOW;
template class COW;
template class XOW;

}
