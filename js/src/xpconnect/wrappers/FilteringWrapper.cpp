






































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
Filter(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    size_t w = 0;
    for (size_t n = 0; n < props.length(); ++n) {
        jsid id = props[n];
        Permission perm;
        if (!Policy::check(cx, wrapper, id, JSWrapper::GET, perm))
            return false; 
        if (perm != DenyAccess)
            props[w++] = id;
    }
    props.resize(w);
    return true;
}

template <typename Policy>
static bool
CheckAndReport(JSContext *cx, JSObject *wrapper, jsid id, JSWrapper::Action act, Permission &perm)
{
    if (!Policy::check(cx, wrapper, id, act, perm)) {
        return false;
    }
    if (perm == DenyAccess) {
        
        
        
        
        JSAutoEnterCompartment ac;
        if (!ac.enter(cx, wrapper))
            return false;

        AccessCheck::deny(cx, id);
        return false;
    }
    return true;
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyNames(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    return Base::getOwnPropertyNames(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerate(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    return Base::enumerate(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerateOwn(JSContext *cx, JSObject *wrapper, AutoIdVector &props)
{
    return Base::enumerateOwn(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::iterate(JSContext *cx, JSObject *wrapper, uintN flags, Value *vp)
{
    
    
    
    
    return JSProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enter(JSContext *cx, JSObject *wrapper, jsid id,
                                      JSWrapper::Action act)
{
    Permission perm;
    return CheckAndReport<Policy>(cx, wrapper, id, act, perm) &&
           Base::enter(cx, wrapper, id, act);
}

#define SOW FilteringWrapper<JSCrossCompartmentWrapper, OnlyIfSubjectIsSystem>
#define COW FilteringWrapper<JSCrossCompartmentWrapper, ExposedPropertiesOnly>
#define XOW FilteringWrapper<XrayWrapper<JSCrossCompartmentWrapper, CrossCompartmentXray>, \
                             CrossOriginAccessiblePropertiesOnly>
#define NNXOW FilteringWrapper<JSCrossCompartmentWrapper, CrossOriginAccessiblePropertiesOnly>

template<> SOW SOW::singleton(0);
template<> COW COW::singleton(0);
template<> XOW XOW::singleton(0);
template<> NNXOW NNXOW::singleton(0);

template class SOW;
template class COW;
template class XOW;
template class NNXOW;

}
